#
# Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

LINKED_OBJ := libnvodm_avp.o

C_FILES += nvodm_avp.c

LCINCS += -I../../../../core/include

LCDEFS += -DWIN_INTERFACE_CUSTOM
LCDEFS += -UDEBUG
LCDEFS += -U_DEBUG
LCDEFS += -DNDEBUG
LCDEFS += -DNV_DEBUG=0
LCDEFS += -DNVODM_BOARD_IS_FPGA=0
LCDEFS += -D__thumb
LCDEFS += -mthumb-interwork
LCDEFS += -mthumb

OBJ += $(C_FILES:%.c=%.o)
OBJ += prebuilt/nvodm_services_avp.o

BUILD_TOOLS_DIR ?=
BUILD_FLAVOR ?= release

GCC_VER ?= 4.3.3
EXE_SUFFIX :=
CROSS_COMPILE := $(BUILD_TOOLS_DIR)/bin/arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AS := $(CROSS_COMPILE)gcc
CPP :=
CPPFLAGS :=
ASFLAGS := -x assembler-with-cpp
LD_EXE := $(CROSS_COMPILE)gcc
LD_TARGET_PREFIX := -o
LD_WHOLE_ARCHIVE := -Wl,--no-whole-archive
LD_NO_WHOLE_ARCHIVE := -Wl,--no-whole-archive

# CPU_OPTS += -mfloat-abi=softfp
# CPU_OPTS += -march=armv4
CPU_OPTS += -mthumb-interwork
ifeq ($(BUILD_FLAVOR),release)
  CPU_OPTS += -O2
  CPU_OPTS += -fno-unwind-tables
  CPU_OPTS += -finline-functions
  CPU_OPTS += -fomit-frame-pointer
  CPU_OPTS += -finline-limit=300
else
  CPU_OPTS += -g2 -ggdb
endif
SCATTER_FLAG := -Wl,-T$(SCATTER_FILE)

CCINC += -I$(BUILD_TOOLS_DIR)/arm-none-eabi/include
CCINC += -I$(BUILD_TOOLS_DIR)/arm-none-eabi/lib/gcc/arm-none-eabi/$(GCC_VER)/include
CCINC += -I$(BUILD_TOOLS_DIR)/arm-none-eabi/include/c++/$(GCC_VER)
CCINC += -I$(BUILD_TOOLS_DIR)/arm-none-eabi/include/c++/$(GCC_VER)/arm-none-eabi

LIBC_DIRS := -L$(BUILD_TOOLS_DIR)/arm-none-eabi/lib
LIBC_LIBS := -lc -lm

CCDEFS := $(LCDEFS)

CCINC += $(LCINCS)

ifeq ($(BUILD_FLAVOR),debug)
  CCDEFS += -DDEBUG -D_DEBUG -UNDEBUG
  CCDEFS += -DNV_DEBUG=1
else
  CCDEFS += -UDEBUG -U_DEBUG -DNDEBUG
  CCDEFS += -DNV_DEBUG=0
endif

CCOPT := $(CPU_OPTS) $(CCDEFS) $(CCINC)



default: $(LINKED_OBJ)

$(LINKED_OBJ): $(OBJ)
	$(LD) -r -o $@ $^
	$(CROSS_COMPILE)nm --extern-only $@ | awk '{print $$3}' | grep -v 'NvOdmAvp.*CpuInit' > symbols
	$(CROSS_COMPILE)objcopy --localize-symbols=symbols $@

sweep:
	rm -fr $(OBJ)
	rm -fr $(TARGET)$(EXE_SUFFIX) $(TARGET).bin

%.o : %.c 
	$(CC) $(CCOPT) $(LCINCS) -c $< -o $@

%.o : %.arm
	$(AS) $(ASFLAGS) $(CCOPT) $(LCINCS) -c $< -o $@

%.o : %.S
	$(AS) $(ASFLAGS) $(CCOPT) $(LCINCS) -c $< -o $@
