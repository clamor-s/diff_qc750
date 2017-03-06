include $(NVIDIA_BASE)
include $(BUILD_PACKAGE)

# BUILD_PACKAGE doesn't consider additional dependencies
$(LOCAL_BUILT_MODULE): $(LOCAL_ADDITIONAL_DEPENDENCIES)
