PRODUCT_COPY_FILES += \
    device/keenhi/addon/media/bootanimation_qc750.zip:./system/media/bootanimation.zip 

PRODUCT_PACKAGES += \
		wplay \
		NVIDIATegraZone \
		CheckOtaUpdate \
		SystemUpdate \
		pm_service \
		pm_service2 \
		libdmard06_cal_jni \
		GSensorCal \
		ProductTest \
		libproduction_cal_jni \
		CoreService \
		libgsensor_cal_jni \
		libradflash \
		Qc750Setup \
		Qc750SetupUi \
		camera.tegra \
		App2Sd \
		libotgswitch

include device/keenhi/addon/qc750apps/app/qc750-3rd-app.mk
include device/keenhi/addon/google/products/gms_simple.mk
