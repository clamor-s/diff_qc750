# ! This Makefile does the Wrong Thing because the generated code has too many
# warnings and the build system doesn't have bison/flex libraries in the
# include path.

HOST_BUILD := 1

TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


LCINCS += $(NVLIB_COMMON_INCLUDES)

EXECUTABLES := $(OUTDIR)/nvidl$(EXE_SUFFIX)

C_FILES :=
C_FILES += nvidl.tab.c
C_FILES += lex.yy.c
C_FILES += nvidl.c


#ifeq ($(TARGET_OS),linux)
#LEX := /home/gnu/flex-2.5.4/bin/flex
#YACC := /home/gnu/bison-2.0/bin/bison
#endif
#ifeq ($(TARGET_OS),winxp)
#LEX := $(BUILD_TOOLS_DIR)/gnu/bin/flex.exe
#YACC := $(BUILD_TOOLS_DIR)/gnu/bin/bison.exe
#endif

# generated lex and yacc code has too many warnings
WARN :=

#
# To re-run lex & yacc type
#   make doyacc
#
# the lex and yac commands are as follows:
#
#   $(YACC) -v -d nvidl.y
#   $(LEX) nvidl.l
#


OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(EXECUTABLES) note

$(EXECUTABLES): $(OBJS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules

.PHONY: note
note:
ifeq ($(MAKELEVEL),0)
	@echo ""
	@echo "Using checked in lex & yacc output.  To run lex & yacc type"
	@echo "     make doyacc"
	@echo ""
endif

ENABLE_YACC := 0
ifeq ($(ENABLE_YACC),0)
.PHONY: doyacc
doyacc:
	-p4 edit nvidl.tab.c nvidl.tab.h lex.yy.c
	$(RM)   nvidl.tab.c nvidl.tab.h lex.yy.c
	$(MAKE) nvidl.tab.c nvidl.tab.h lex.yy.c ENABLE_YACC=1
nvidl.tab.c nvidl.tab.h lex.yy.c:
	$(MAKE) ENABLE_YACC=1 $@
else
nvidl.tab.c: nvidl.y Makefile
	-p4 edit $@ ${@:.c=.h}
	$(RM) $@ ${@:.c=.h}
	$(YACC) -o nvidl.tab.c -v -d nvidl.y

nvidl.tab.h: nvidl.tab.c

lex.yy.c: nvidl.l Makefile nvidl.tab.h
	-p4 edit $@
	$(RM) $@
	$(LEX) nvidl.l
endif
