LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvcrypto
LOCAL_NVIDIA_NO_COVERAGE := true
LOCAL_SRC_FILES += nvcrypto_padding.c
LOCAL_SRC_FILES += nvcrypto_cipher.c
LOCAL_SRC_FILES += nvcrypto_cipher_aes.c
LOCAL_SRC_FILES += nvcrypto_hash.c
LOCAL_SRC_FILES += nvcrypto_hash_cmac.c
LOCAL_SRC_FILES += nvcrypto_random.c
LOCAL_SRC_FILES += nvcrypto_random_ansix931.c

LOCAL_CFLAGS += -Wno-error=enum-compare

LOCAL_NVIDIA_NO_EXTRA_WARNINGS := 1
include $(NVIDIA_STATIC_LIBRARY)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvswcrypto
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/utils/aes_ref

LOCAL_SRC_FILES += nvcrypto_padding.c
LOCAL_SRC_FILES += nvcrypto_cipher.c
LOCAL_SRC_FILES += nvcrypto_hash.c
LOCAL_SRC_FILES += nvcrypto_hash_cmac.c
LOCAL_SRC_FILES += nvcrypto_random.c
LOCAL_SRC_FILES += nvcrypto_cipher_sw_aes.c

include $(NVIDIA_HOST_STATIC_LIBRARY)
