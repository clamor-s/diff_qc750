#
# Coverage instrumentation support
#

# If it's a debug build
ifeq ($(TARGET_BUILD_TYPE),debug)
# If coverage instrumentation is enabled through the environment variable
ifneq ($(NVIDIA_COVERAGE_ENABLED),)
# If instrumentation isn't disabled for the module
ifneq ($(LOCAL_NVIDIA_NO_COVERAGE),true)

# Instrument the code
LOCAL_CFLAGS += -fprofile-arcs -ftest-coverage
# Disable debugging code
LOCAL_CFLAGS += -UNV_DEBUG -DNV_DEBUG=0 -UDEBUG -U_DEBUG -DNDEBUG

# If coverage output isn't disabled
ifneq ($(LOCAL_NVIDIA_NULL_COVERAGE),true)
LOCAL_LDFLAGS += -lgcc -lgcov
# If it's a shared library module
ifeq ($(LOCAL_MODULE_CLASS),SHARED_LIBRARIES)
# Include an instance of atexit(3) referencing __dso_handle
LOCAL_WHOLE_STATIC_LIBRARIES += libatexit
LOCAL_LDFLAGS += -Wl,--exclude-libs=libatexit
endif 	# LOCAL_MODULE_CLASS == SHARED_LIBRARIES
else	# !LOCAL_NVIDIA_NULL_COVERAGE
# Link to NULL-output libgcov
LOCAL_STATIC_LIBRARIES += libgcov_null
LOCAL_LDFLAGS += -Wl,--exclude-libs=libgcov_null
endif	# LOCAL_NVIDIA_NULL_COVERAGE

endif	# !LOCAL_NVIDIA_NO_COVERAGE
endif	# NVIDIA_COVERAGE_ENABLED
endif	# TARGET_BUILD_TYPE == debug
