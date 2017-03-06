/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "AndroidAnimation"
#define LOG_NDEBUG 1

#include "config.h"
#include "AndroidAnimation.h"

#if USE(ACCELERATED_COMPOSITING)

#include "AndroidLog.h"
#include "Animation.h"
#include "GraphicsLayerAndroid.h"
#include "Timer.h"
#include "TimingFunction.h"
#include "TranslateTransformOperation.h"
#include "UnitBezier.h"

namespace WebCore {

static int gUniqueId;

static long gDebugAndroidAnimationInstances;

long AndroidAnimation::instancesCount()
{
    return gDebugAndroidAnimationInstances;
}

AndroidAnimation::AndroidAnimation(AnimatedPropertyID type,
                                   const Animation* animation,
                                   KeyframeValueList* operations,
                                   double beginTime)
    : m_beginTime(beginTime)
    , m_duration(animation->duration())
    , m_fillsBackwards(animation->fillsBackwards())
    , m_fillsForwards(animation->fillsForwards())
    , m_iterationCount(animation->iterationCount())
    , m_direction(animation->direction())
    , m_timingFunction(animation->timingFunction())
    , m_type(type)
    , m_operations(operations)
    , m_uniqueId(++gUniqueId)
{
    ASSERT(m_timingFunction);
    ASSERT(m_duration >= 0);
    if (!m_duration)
        m_duration = std::numeric_limits<double>::epsilon();

    gDebugAndroidAnimationInstances++;
}

AndroidAnimation::~AndroidAnimation()
{
    gDebugAndroidAnimationInstances--;
}

void AndroidAnimation::suggestBeginTime(double time)
{
    if (m_beginTime <= std::numeric_limits<double>::epsilon()) // overflow or not yet set
        m_beginTime = time;
}

double AndroidAnimation::elapsedTime(double time)
{
    return std::max(time - m_beginTime, 0.);
}

double AndroidAnimation::checkIterationsAndProgress(double time)
{
    if (!m_iterationCount)
        return 0;

    double progress = 0;
    if (hasStarted(time))
        progress = elapsedTime(time);

    double fractionalTime = progress / m_duration;
    int iterationsRun = static_cast<int>(fractionalTime);
    fractionalTime -= iterationsRun;

    if (m_iterationCount != WebCore::Animation::IterationCountInfinite) {
        if (iterationsRun >= m_iterationCount) {
            iterationsRun = m_iterationCount - 1;
            fractionalTime = 1;
        }
    }

    if ((m_direction == Animation::AnimationDirectionAlternate) && (iterationsRun & 1))
        fractionalTime = 1 - fractionalTime;

    return fractionalTime;
}

double AndroidAnimation::applyTimingFunction(float from, float to, double progress,
                                             const TimingFunction* tf)
{
    double fractionalTime = progress;
    double offset = from;
    double scale = 1.0 / (to - from);

    if (scale != 1 || offset)
        fractionalTime = (fractionalTime - offset) * scale;

    const TimingFunction* timingFunction = tf;

    if (!timingFunction)
        timingFunction = m_timingFunction.get();

    if (timingFunction && timingFunction->isCubicBezierTimingFunction()) {
        const CubicBezierTimingFunction* bezierFunction = static_cast<const CubicBezierTimingFunction*>(timingFunction);
        UnitBezier bezier(bezierFunction->x1(),
                          bezierFunction->y1(),
                          bezierFunction->x2(),
                          bezierFunction->y2());
        ASSERT(m_duration > 0);
        fractionalTime = bezier.solve(fractionalTime, 1.0f / (200.0f * m_duration));
    } else if (timingFunction && timingFunction->isStepsTimingFunction()) {
        const StepsTimingFunction* stepFunction = static_cast<const StepsTimingFunction*>(timingFunction);
        if (stepFunction->stepAtStart()) {
            fractionalTime = (floor(stepFunction->numberOfSteps() * fractionalTime) + 1) / stepFunction->numberOfSteps();
            if (fractionalTime > 1.0)
                fractionalTime = 1.0;
        } else {
            fractionalTime = floor(stepFunction->numberOfSteps() * fractionalTime) / stepFunction->numberOfSteps();
        }
    }
    return fractionalTime;
}

bool AndroidAnimation::evaluate(LayerAndroid* layer, double time)
{
    if (!m_operations->size())
        return false;

    ASSERT(m_beginTime > std::numeric_limits<double>::epsilon());
    if (!hasStarted(time) && !m_fillsBackwards)
        return true; // Request draw until animation starts.

    // If the last keyframe has been already applied, re-apply it only if the animation has
    // fill:forwards.  Strictly, we might end up here and not have painted the exact last
    // keyframe. This should be taken care of by the style recalculation that results after
    // finishing the animation. That should reset any fill:none or fill:backwards elements to their
    // initial positions and cause a layer sync.
    if (hasEnded(time) && !m_fillsForwards)
        return false;

    float progress = checkIterationsAndProgress(time);

    applyForProgress(layer, progress);

    return !hasEnded(time);
}

PassRefPtr<AndroidOpacityAnimation> AndroidOpacityAnimation::create(
                                                const Animation* animation,
                                                KeyframeValueList* operations,
                                                double beginTime)
{
    return adoptRef(new AndroidOpacityAnimation(animation, operations,
                                                beginTime));
}

AndroidOpacityAnimation::AndroidOpacityAnimation(const Animation* animation,
                                                 KeyframeValueList* operations,
                                                 double beginTime)
    : AndroidAnimation(AnimatedPropertyOpacity, animation, operations, beginTime)
{
}

void AndroidAnimation::pickValues(double progress, int* start, int* end)
{
    float distance = -1;
    unsigned int foundAt = 0;
    for (unsigned int i = 0; i < m_operations->size(); i++) {
        const AnimationValue* value = m_operations->at(i);
        float key = value->keyTime();
        float d = progress - key;
        if (distance == -1 || (d >= 0 && d < distance && i + 1 < m_operations->size())) {
            distance = d;
            foundAt = i;
        }
    }

    *start = foundAt;

    if (foundAt + 1 < m_operations->size())
        *end = foundAt + 1;
    else
        *end = foundAt;
}

void AndroidOpacityAnimation::applyForProgress(LayerAndroid* layer, float progress)
{
    // First, we need to get the from and to values
    int from, to;
    pickValues(progress, &from, &to);
    FloatAnimationValue* fromValue = (FloatAnimationValue*) m_operations->at(from);
    FloatAnimationValue* toValue = (FloatAnimationValue*) m_operations->at(to);

    ALOGV("[layer %d] opacity fromValue %x, key %.2f, toValue %x, key %.2f for progress %.2f",
          layer->uniqueId(),
          fromValue, fromValue->keyTime(),
          toValue, toValue->keyTime(), progress);

    // We now have the correct two values to work with, let's compute the
    // progress value

    const TimingFunction* timingFunction = fromValue->timingFunction();
    progress = applyTimingFunction(fromValue->keyTime(), toValue->keyTime(),
                                   progress, timingFunction);


    float value = fromValue->value() + ((toValue->value() - fromValue->value()) * progress);

    layer->setOpacity(value);
}

PassRefPtr<AndroidTransformAnimation> AndroidTransformAnimation::create(
                                                     const Animation* animation,
                                                     KeyframeValueList* operations,
                                                     double beginTime)
{
    return adoptRef(new AndroidTransformAnimation(animation, operations, beginTime));
}

AndroidTransformAnimation::AndroidTransformAnimation(const Animation* animation,
                                                     KeyframeValueList* operations,
                                                     double beginTime)
    : AndroidAnimation(AnimatedPropertyWebkitTransform, animation, operations, beginTime)
{
}

void AndroidTransformAnimation::applyForProgress(LayerAndroid* layer, float progress)
{
    // First, we need to get the from and to values
    int from, to;
    pickValues(progress, &from, &to);

    TransformAnimationValue* fromValue = (TransformAnimationValue*) m_operations->at(from);
    TransformAnimationValue* toValue = (TransformAnimationValue*) m_operations->at(to);

    ALOGV("[layer %d] fromValue %x, key %.2f, toValue %x, key %.2f for progress %.2f",
          layer->uniqueId(),
          fromValue, fromValue->keyTime(),
          toValue, toValue->keyTime(), progress);

    // We now have the correct two values to work with, let's compute the
    // progress value

    const TimingFunction* timingFunction = fromValue->timingFunction();
    float p = applyTimingFunction(fromValue->keyTime(), toValue->keyTime(),
                                  progress, timingFunction);
    ALOGV("progress %.2f => %.2f from: %.2f to: %.2f", progress, p, fromValue->keyTime(),
          toValue->keyTime());
    progress = p;

    // With both values and the progress, we also need to check out that
    // the operations are compatible (i.e. we are animating the same number
    // of values; if not we do a matrix blend)

    TransformationMatrix transformMatrix;
    bool valid = true;
    unsigned int fromSize = fromValue->value()->size();
    if (fromSize) {
        if (toValue->value()->size() != fromSize)
            valid = false;
        else {
            for (unsigned int j = 0; j < fromSize && valid; j++) {
                if (!fromValue->value()->operations()[j]->isSameType(
                    *toValue->value()->operations()[j]))
                    valid = false;
            }
        }
    }

    IntSize size(layer->getSize().width(), layer->getSize().height());
    if (valid) {
        for (size_t i = 0; i < toValue->value()->size(); ++i)
            toValue->value()->operations()[i]->blend(fromValue->value()->at(i),
                                                     progress)->apply(transformMatrix, size);
    } else {
        TransformationMatrix source;

        fromValue->value()->apply(size, source);
        toValue->value()->apply(size, transformMatrix);

        transformMatrix.blend(source, progress);
    }

    // Set the final transform on the layer
    layer->setTransform(transformMatrix);
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
