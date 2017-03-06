#define LOG_TAG "IFrameLayerAndroid"
#define LOG_NDEBUG 1

#include "config.h"
#include "IFrameLayerAndroid.h"

#if USE(ACCELERATED_COMPOSITING)

#include "AndroidLog.h"
#include "DumpLayer.h"

namespace WebCore {

void IFrameLayerAndroid::updatePosition(SkRect viewport,
                                        IFrameLayerAndroid* parentIframeLayer,
                                        SkRect* viewportForChildren,
                                        IFrameLayerAndroid** parentIframeLayerForChildren)
{
    LayerAndroid::updatePosition(viewport, parentIframeLayer, viewportForChildren, parentIframeLayerForChildren);

    // As we are an iframe, accumulate the offset from the parent with
    // the current position, and change the parent pointer.

    // If this is the top level, take the current position
    SkPoint parentOffset;
    parentOffset.set(0,0);
    if (parentIframeLayer)
        parentOffset = parentIframeLayer->getPosition();

    SkPoint offset = parentOffset + getPosition();

    *viewportForChildren = SkRect::MakeXYWH(offset.fX, offset.fY, getWidth(), getHeight());
    *parentIframeLayerForChildren = this;
}

void IFrameLayerAndroid::dumpLayer(LayerDumper* dumper) const
{
    LayerAndroid::dumpLayer(dumper);
    dumper->writeIntPoint("m_iframeOffset", m_iframeOffset);
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
