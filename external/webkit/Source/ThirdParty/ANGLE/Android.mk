LOCAL_PATH:= $(call my-dir)/src

include $(CLEAR_VARS)

LOCAL_MODULE:= libANGLE

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	compiler/BuiltInFunctionEmulator.cpp \
	compiler/CodeGenGLSL.cpp \
	compiler/Compiler.cpp \
	compiler/DetectRecursion.cpp \
	compiler/ForLoopUnroll.cpp \
	compiler/InfoSink.cpp \
	compiler/Initialize.cpp \
	compiler/InitializeDll.cpp \
	compiler/IntermTraverse.cpp \
	compiler/Intermediate.cpp \
	compiler/MapLongVariableNames.cpp \
	compiler/OutputESSL.cpp \
	compiler/OutputGLSL.cpp \
	compiler/OutputGLSLBase.cpp \
	compiler/ParseHelper.cpp \
	compiler/PoolAlloc.cpp \
	compiler/RemoveTree.cpp \
	compiler/ShaderLang.cpp \
	compiler/SymbolTable.cpp \
	compiler/TranslatorESSL.cpp \
	compiler/TranslatorGLSL.cpp \
	compiler/ValidateLimitations.cpp \
	compiler/VariableInfo.cpp \
	compiler/VersionGLSL.cpp \
	compiler/glslang_lex.cpp \
	compiler/glslang_tab.cpp \
	compiler/intermOut.cpp \
	compiler/ossource_posix.cpp \
	compiler/parseConst.cpp \
	compiler/preprocessor/atom.c \
	compiler/preprocessor/cpp.c \
	compiler/preprocessor/cppstruct.c \
	compiler/preprocessor/memory.c \
	compiler/preprocessor/scanner.c \
	compiler/preprocessor/symbols.c \
	compiler/preprocessor/tokens.c \
	compiler/util.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../include \

include external/stlport/libstlport.mk

LOCAL_SYSTEM_SHARED_LIBRARIES := libstlport

ifeq ($(ENABLE_ASSERTIONS),true)
LOCAL_CFLAGS += -DDEBUG=1 -UNDEBUG
endif

include $(BUILD_STATIC_LIBRARY)
