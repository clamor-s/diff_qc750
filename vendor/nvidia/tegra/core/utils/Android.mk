LOCAL_PATH := $(call my-dir)
include $(addprefix $(LOCAL_PATH)/, $(addsuffix /Android.mk, \
	aes_ref \
	hybrid \
	md5 \
	nvappmain \
	nvapputil \
	nvblob \
	nvdumper \
	nvfxmath \
	nvidl \
	nvintr \
	nvos \
	nvosutils \
	nvos/aos/avp \
	nvreftrack \
	nvusbhost/libnvusbhost \
	nvtestmain \
	nvtestresults \
	tegrastats \
	aes_keysched_lock \
))

