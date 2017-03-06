# DO NOT add conditionals to this makefile of the form
#
#    ifeq ($(TARGET_TEGRA_VERSION),<latest SOC>)
#        <stuff for latest SOC>
#    endif
#
# Such conditionals break forward compatibility with future SOCs.
# If you must add conditionals to this makefile, use the form
#
#    ifneq ($(filter <list of older SOCs>,$(TARGET_TEGRA_VERSION)),)
#       <stuff for old SOCs>
#    else
#       <stuff for new SOCs>
#    endif

ifneq ($(filter ap20 t20, $(TARGET_TEGRA_VERSION)),)
    LOCAL_OEMCRYPTO_LEVEL := 1
else
    LOCAL_OEMCRYPTO_LEVEL := 3
endif
nv_modules := \
    _collections \
    _fileio \
    _functools \
    _multibytecodec \
    _nvcamera \
    _random \
    _socket \
    _struct \
    _weakref \
    alcameradatataptest \
    aleglstream \
    alplaybacktest \
    alsnapshottest \
    alvcplayer \
    alvcrecorder \
    alvideorecordtest \
    alvepreview \
    array \
    audio_policy.$(TARGET_BOARD_PLATFORM) \
    audio.primary.$(TARGET_BOARD_PLATFORM) \
    binascii \
    bmp2h \
    bootloader \
    btmacwriter\
    camera.$(TARGET_BOARD_PLATFORM) \
    cmath \
    com.google.widevine.software.drm \
    com.google.widevine.software.drm.xml \
    com.nvidia.display \
    com.nvidia.nvstereoutils \
    com.nvidia.nvwfd \
    datetime \
    dfs_cfg \
    dfs_log \
    dfs_monitor \
    dfs_stress \
    downloader \
    fcntl \
    get_fs_stat \
    gps.$(TARGET_BOARD_PLATFORM) \
    gralloc.$(TARGET_BOARD_PLATFORM) \
    hdcp_test \
    hwcomposer.$(TARGET_BOARD_PLATFORM) \
    libaacdec \
    libaudioavp \
    libaudioservice \
    libaudioutils \
    libcplconnectclient \
    libexpat \
    libakmd \
    libardrv_dynamic \
    libtinyalsa \
    libcg \
    libcgdrv \
    libDEVMGR \
    libdrmwvmcommon \
    libEGL_perfhud \
    libEGL_tegra \
    libEGL_tegra_impl \
    libGLESv1_CM_perfhud \
    libGLESv1_CM_tegra \
    libGLESv1_CM_tegra_impl \
    libGLESv2_perfhud \
    libGLESv2_tegra \
    libGLESv2_tegra_impl \
    libh264enc \
    libhybrid \
    libinvensense_hal \
    libjni_nvremote \
    libjni_nvremoteprotopkg \
    libmd5 \
    libmllite \
    libmlplatform \
    libmpeg2vdec_vld \
    libmpeg4enc \
    libmpl \
    libmplmpu \
    libnv_parser \
    libnv3gpwriter \
    libnv3p \
    libnv3pserver \
    libnvaacplusenc \
    libnvaboot \
    libnvaes_ref \
    libnvamrnbcommon \
    libnvamrnbdec \
    libnvamrnbenc \
    libnvamrwbdec \
    libnvamrwbenc \
    libnvappmain \
    libnvappmain_aos \
    libnvapputil \
    libnvasfparserhal \
    libnvaudio_memutils \
    libnvaudio_power \
    libnvaudio_ratecontrol \
    libnvaudioutils \
    libnvaviparserhal \
    libnvavp \
    libnvbasewriter \
    libnvbct \
    libnvboothost \
    libnvbootupdate \
    libnvbsacdec \
    libnvcamera \
    libnvcamerahdr \
    libnvcamerautil \
    libnvcap \
    libnvcapclk \
    libnvcap_video \
    libnvcpl \
    libnvcpud \
    libnvcrypto \
    libnvddk_2d \
    libnvddk_2d_ap15 \
    libnvddk_2d_ar3d \
    libnvddk_2d_combined \
    libnvddk_2d_fastc \
    libnvddk_2d_swref \
    libnvddk_2d_v2 \
    libnvddk_aes \
    libnvddk_audiodap \
    libnvddk_disp \
    libnvddk_fuse \
    libnvddk_fuse_read \
    libnvddk_fuse_read_host \
    libnvddk_i2s \
    libnvddk_misc \
    libnvddk_spdif \
    libnvdigitalzoom \
    libnvdioconverter \
    libnvdispatch_helper \
    libnvdrawpath \
    libnvdtvsrc \
    libnvfs \
    libnvfsmgr \
    libnvfxmath \
    libnvglsi \
    libnvh264dec \
    libnvhdmi3dplay_jni \
    libnvidia_display_jni \
    libnvilbccommon \
    libnvilbcdec \
    libnvilbcenc \
    libnvimageio \
    libnvintr \
    libnvjpegprogressivedec \
    libnvmm \
    libnvmm_ac3audio \
    libnvmm_asfparser \
    libnvmm_audio \
    libnvmm_aviparser \
    libnvmm_camera \
    libnvmm_contentpipe \
    libnvmm_image \
    libnvmm_manager \
    libnvmm_msaudio \
    libnvmm_parser \
    libnvmm_service \
    libnvmm_utils \
    libnvmm_vc1_video \
    libnvmm_video \
    libnvmm_writer \
    libnvmmcommon \
    libnvmmlite \
    libnvmmlite_audio \
    libnvmmlite_image \
    libnvmmlite_msaudio \
    libnvmmlite_utils \
    libnvmmlite_video \
    libnvmmtransport \
    libnvmpeg2vdec_recon \
    libnvmpeg4dec \
    libnvodm_audiocodec \
    libnvodm_disp_devfb \
    libnvodm_dtvtuner \
    libnvodm_hdmi \
    libnvodm_imager \
    libnvodm_misc \
    libnvodm_query \
    libnvodm_services \
    libnvoggdec \
    libnvomx \
    libnvomxadaptor \
    libnvomxilclient \
    libnvos \
    libnvos_aos \
    libnvos_aos_libgcc_avp \
    libnvparser \
    libnvpartmgr \
    libnvreftrack \
    libnvremoteevtmgr \
    libnvremotell \
    libnvremoteprotocol \
    libnvrm \
    libnvrm_channel_impl \
    libnvrm_graphics \
    libnvrm_impl \
    libnvrm_limits \
    libnvrm_secure \
    libnvrsa \
    libnvseaes_keysched_lock_avp \
    libnvsm \
    libnvstitching \
    libnvstormgr \
    libnvsuperjpegdec \
    libnvsystemuiext_jni \
    libnvsystem_utils \
    libnvtestio \
    libnvtestmain \
    libnvtestresults \
    libnvtestrun \
    libnvtsdemux \
    libnvtvmr \
    libnvusbhost \
    libnvvideodec \
    libnvvideoenc \
    libnvwavdec \
    libnvwavenc \
    libnvwinsys \
    libnvwma \
    libnvwmalsl \
    libnvwsi \
    libnvwsi_core \
    libopengles1_detect \
    libopengles2_detect \
    libopenmaxil_detect \
    libopenvg_detect \
    libpython2.6 \
    librs_jni \
    libsensors.base \
    libsensors.isl29018 \
    libsensors.isl29028 \
    libsensors.mpl \
    libstagefrighthw \
    librm31080 \
    ts.default \
    librm_ts_service \
    rm_ts_server \
    synaptics_direct_touch_daemon \
    libwfd_common \
    libwfd_sink \
    libwfd_source \
    libwvdrm_L$(LOCAL_OEMCRYPTO_LEVEL) \
    libwvmcommon \
    libwvocs_L$(LOCAL_OEMCRYPTO_LEVEL) \
    libWVStreamControlAPI_L$(LOCAL_OEMCRYPTO_LEVEL) \
    math \
    microboot \
    MockNVCP \
    nfc.$(TARGET_BOARD_PLATFORM) \
    nvavp_os_0ff00000.bin \
    nvavp_os_eff00000.bin \
    nvavp_smmu.bin \
    nvavp_vid_ucode_alt.bin \
    nvavp_vid_ucode.bin \
    nvavp_aud_ucode.bin \
    nvblob \
    nvcap_test \
    NvCPLSvc \
    nvcpud \
    nvdumppublickey \
    nvflash \
    nv_hciattach \
    nvhost \
    nvidia_display \
    nvidl \
    nvsbktool \
    nvsignblob \
    nvtest \
    operator \
    parser \
    python \
    sensors.tegra \
    shaderfix \
    select \
    strop \
    tegrastats \
    test-libwvm \
    test-wvdrmplugin \
    test-wvplayer_L$(LOCAL_OEMCRYPTO_LEVEL) \
    time \
    tokenize \
    ttytestapp \
    unicodedata \
    xaplay \
    libdrmdecrypt \
    tinyplay \
    tinycap \
    tinymix
    
    #DidimCalibration \
    #WidevineSamplePlayer \
    #NvwfdTest \
    #NvwfdProtocolsPack \
    #NvwfdTest \
    #NvwfdServiceTest \
    

ifeq ($(BOARD_BUILD_BOOTLOADER),true)
nv_modules += \
    bootloader
endif

ifeq ($(SECURE_OS_BUILD),y)
nv_modules += \
    libsmapi \
    libtf_crypto_sst \
    tf_daemon
endif

nv_modules += avp_load

ifneq ($(TARGET_TEGRA_VERSION),t30)
nv_modules += nvrm_avp.bin
nv_modules += nvrm_avp_0ff00000.bin
nv_modules += nvrm_avp_eff00000.bin
nv_modules += nvrm_avp_8e000000.bin
nv_modules += nvrm_avp_9e000000.bin
nv_modules += nvrm_avp_be000000.bin

nv_modules += \
    nvmm_aacdec.axf \
    nvmm_adtsdec.axf \
    nvmm_h264dec2x.axf \
    nvmm_h264dec.axf \
    nvmm_jpegdec.axf \
    nvmm_jpegenc.axf \
    nvmm_manager.axf \
    nvmm_mp3dec.axf \
    nvmm_mpeg2dec.axf \
    nvmm_mpeg4dec.axf \
    nvmm_service.axf \
    nvmm_vc1dec_2x.axf \
    nvmm_vc1dec.axf \
    nvmm_wavdec.axf \
    nvmm_wmadec.axf \
    nvmm_wmaprodec.axf

nv_modules += load_test0.axf
nv_modules += load_test1.axf
nv_modules += load_test2.axf
nv_modules += load_test3.axf
nv_modules += load_test4.axf
nv_modules += arb_test.axf
nv_modules += transport_stress.axf
nv_modules += memory_stress.axf

ifneq ($(TARGET_TEGRA_VERSION),ap20)
nv_modules += \
    avp_smmu.axf \
    avp_smmu_stress.axf
endif
endif

ifeq ($(TARGET_TEGRA_VERSION),ap20)
nv_modules += \
    libnvddk_se
endif

include $(CLEAR_VARS)

LOCAL_MODULE := nvidia_tegra_proprietary_src_modules
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(nv_modules)
LOCAL_REQUIRED_MODULES += $(ALL_NVIDIA_TESTS)
include $(BUILD_PHONY_PACKAGE)
