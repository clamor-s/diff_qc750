LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := v8cctest
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional

LOCAL_CPP_EXTENSION := .cc

LOCAL_STATIC_LIBRARIES := libv8
LOCAL_SHARED_LIBRARIES := libcutils

intermediates := $(call local-intermediates-dir)

LOCAL_SRC_FILES:= \
	test/cctest/cctest.cc \
	test/cctest/gay-fixed.cc \
	test/cctest/gay-precision.cc \
	test/cctest/gay-shortest.cc \
	test/cctest/test-accessors.cc \
	test/cctest/test-alloc.cc \
	test/cctest/test-api.cc \
	test/cctest/test-ast.cc \
	test/cctest/test-bignum.cc \
	test/cctest/test-bignum-dtoa.cc \
	test/cctest/test-circular-queue.cc \
	test/cctest/test-compiler.cc \
	test/cctest/test-conversions.cc \
	test/cctest/test-cpu-profiler.cc \
	test/cctest/test-dataflow.cc \
	test/cctest/test-date.cc \
	test/cctest/test-debug.cc \
	test/cctest/test-decls.cc \
	test/cctest/test-deoptimization.cc \
	test/cctest/test-dictionary.cc \
	test/cctest/test-diy-fp.cc \
	test/cctest/test-double.cc \
	test/cctest/test-dtoa.cc \
	test/cctest/test-fast-dtoa.cc \
	test/cctest/test-fixed-dtoa.cc \
	test/cctest/test-flags.cc \
	test/cctest/test-func-name-inference.cc \
	test/cctest/test-hashing.cc \
	test/cctest/test-hashmap.cc \
	test/cctest/test-heap.cc \
	test/cctest/test-heap-profiler.cc \
	test/cctest/test-list.cc \
	test/cctest/test-liveedit.cc \
	test/cctest/test-lock.cc \
	test/cctest/test-lockers.cc \
	test/cctest/test-log.cc \
	test/cctest/test-mark-compact.cc \
	test/cctest/test-parsing.cc \
	test/cctest/test-platform-tls.cc \
	test/cctest/test-profile-generator.cc \
	test/cctest/test-random.cc \
	test/cctest/test-regexp.cc \
	test/cctest/test-reloc-info.cc \
	test/cctest/test-serialize.cc \
	test/cctest/test-sockets.cc \
	test/cctest/test-spaces.cc \
	test/cctest/test-strings.cc \
	test/cctest/test-strtod.cc \
	test/cctest/test-thread-termination.cc \
	test/cctest/test-threads.cc \
	test/cctest/test-unbound-queue.cc \
	test/cctest/test-utils.cc \
	test/cctest/test-version.cc \
	test/cctest/test-platform-linux.cc \
	test/cctest/test-weakmaps.cc

ifeq ($(TARGET_ARCH),arm)
LOCAL_SRC_FILES += \
	test/cctest/test-assembler-arm.cc \
    test/cctest/test-disasm-arm.cc
else
#FIXME
endif

V8_LOCAL_JS_TEST_LIBRARY_FILES := \
	tools/splaytree.js \
	tools/codemap.js \
	tools/csvparser.js \
	tools/consarray.js \
	tools/profile.js \
	tools/profile_view.js \
	tools/logreader.js \
    test/cctest/log-eq-of-logging-and-traversal.js

LOCAL_JS_TEST_LIBRARY_FILES := $(addprefix $(LOCAL_PATH)/, $(V8_LOCAL_JS_TEST_LIBRARY_FILES))

include $(LOCAL_PATH)/Android.cflags.mk

# Copy js2c.py to intermediates directory and invoke there to avoid generating
# jsmin.pyc in the source directory
JS2C_PY := $(intermediates)/js2c.py $(intermediates)/jsmin.py
$(JS2C_PY): $(intermediates)/%.py : $(LOCAL_PATH)/tools/%.py | $(ACP)
	@echo "Copying $@"
	$(copy-file-to-target)

# Generate resources.cc
GEN3 := $(intermediates)/resources.cc
$(GEN3): SCRIPT := $(intermediates)/js2c.py
$(GEN3): $(LOCAL_JS_TEST_LIBRARY_FILES) $(JS2C_PY)
	@echo "Generating libraries.cc"
	@mkdir -p $(dir $@)
	python $(SCRIPT) $(GEN3) TEST off $(LOCAL_JS_TEST_LIBRARY_FILES)
LOCAL_GENERATED_SOURCES := $(intermediates)/resources.cc


LOCAL_C_INCLUDES += $(LOCAL_PATH)/src

include $(BUILD_EXECUTABLE)
