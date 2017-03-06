#
# Nvidia specific targets
#

.PHONY: dev nv-blob

dev: droidcore target-files-package
ifneq ($(NO_ROOT_DEVICE),)
	device/nvidia/common/generate_nvtest_ramdisk.sh $(TARGET_PRODUCT) $(TARGET_BUILD_TYPE)
endif

# generate blob for bootloaders
nv-blob: \
      $(HOST_OUT_EXECUTABLES)/nvblob \
      $(HOST_OUT_EXECUTABLES)/nvsignblob \
      $(TOP)/device/nvidia/common/security/signkey.pk8 \
      $(PRODUCT_OUT)/bootloader.bin \
      $(PRODUCT_OUT)/microboot.bin
	$(hide) python $(filter %nvblob,$^) \
		$(filter %bootloader.bin,$^) EBT 1 \
		$(filter %microboot.bin,$^) NVC 1


