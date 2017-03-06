LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

pkglist := nvrm nvrm_graphics
pkglist := $(addprefix $(LOCAL_PATH)/,$(addsuffix .idl,$(pkglist)))
headers := $(filter-out $(pkglist),$(wildcard $(LOCAL_PATH)/*.idl))

$(foreach header,$(headers), \
  $(eval _dst := $(TARGET_OUT_HEADERS)/$(patsubst %.idl,%.h,$(notdir $(header)))) \
  $(eval $(call nvidl-rule,-h,$(header),$(_dst))) \
  $(eval $(_dst): $(pkglist)) \
  $(eval all_copied_headers: $(_dst)))
_dst :=

