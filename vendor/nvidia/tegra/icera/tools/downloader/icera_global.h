/*************************************************************************************************
 *
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 ************************************************************************************************/

/**
 * @file icera_global.h Fundamental definitions.
 */

/**
 * @mainpage
 * libcommon.a is the place where all common functions, types, and global variables are
 * defined for general use by the Icera stack, LL1 and scheduler functions.
 */

/**
 * @defgroup dmem_scratch Scratch stack
 */


#ifndef ICERA_GLOBAL_H
/**
 * Macro definition used to avoid recursive inclusion.
 */
#define ICERA_GLOBAL_H

/*************************************************************************************************
 * Project header files
 ************************************************************************************************/

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/

#ifndef __dxp__
#include <assert.h>         /* Needed for standard assertion macros. */
#endif
#if defined (_MSC_VER)
/** Support for non-ANSI C use of bool in the code. */
# if !defined (__cplusplus)
typedef unsigned char bool;
#  define false 0
#  define true  1
# endif
#else
#include <stdbool.h>        /* Needed for bool type definition. */
#endif

#include <stddef.h>

#if defined(__dxp__)
#include <icera_sdk.h>

/* this macro will be updated to with various chip revisions ICE8060_A1 etc */
#if defined (ICE8060_A0)
  #define TARGET_DXP8060
#elif defined (ICE9040_A0)
  #define TARGET_DXP9040
#else
  #error "Unsupported ICE"
#endif
/* Checking of the toolchain compatibility */
#if defined (TARGET_DXP8060)
    #if !(ICERA_SDK_EV_AT_LEAST(4,4,'c'))
        #error "DXP toolchain too old to compile this software. Please upgrade your SDK."
        /* or comment the #error line if you really have to                       */
    #endif
#elif defined (TARGET_DXP9040)
    #if !(ICERA_SDK_EV_AT_LEAST(4,5,'a'))
        #error "DXP toolchain too old to compile this software. Please upgrade your SDK."
        /* or comment the #error line if you really have to                       */
    #endif
#endif

#ifndef OVERRIDE_DXP_TOOLCHAIN_VERSION_CHECK
#if defined (TARGET_DXP8060)
    #if ICERA_SDK_EV_AT_LEAST(4,4,'f')
        #error "DXP toolchain too recent. This software has not yet been validated with it."
    #endif
#elif defined (TARGET_DXP9040)
    #if ICERA_SDK_EV_AT_LEAST(4,5,'b')
        #error "DXP toolchain too recent. This software has not yet been validated with it."
    #endif
#endif

#endif
#endif


/*************************************************************************************************
 * Macros
 ************************************************************************************************/

/**
 * Branch annotations to improve code locality.
 * Requires -freorder-blocks to be defined in GCC_OPTS.
 *
 */
#if defined(__dxp__)
#if !defined(likely)
#if defined(__dxp__)
#if (ICERA_SDK_EV_AT_LEAST(4,1,'a'))
    /* GNAT pending with __builtin_expect on EV4 */
#define likely(x)   (x)
#else
#define likely(x)   __builtin_expect(!!(x),1)
#endif /* ICERA_SDK_EV_AT_LEAST */
#else /* __dxp__ */
  #define likely(x) (x)
#endif /* __dxp__ */
#endif /* !defined(likely) */

#if !defined(unlikely)
#if defined(__dxp__)
#if (ICERA_SDK_EV_AT_LEAST(4,1,'a'))
    /* GNAT pending with __builtin_expect on EV4 */
#define unlikely(x)   (x)
#else
#define unlikely(x)   __builtin_expect(!!(x),0)
#endif /* ICERA_SDK_EV_AT_LEAST */
#else /* __dxp__ */
#define unlikely(x) (x)
#endif /* __dxp__ */
#endif /* !defined(unlikely) */
#else
#define likely(x)   (x)
#define unlikely(x)   (x)
#endif

/**
 * Attribute to declare that a function never returns
 *
 */
#ifdef __dxp__
#define DXP_NEVER_RETURNS   __attribute__((noreturn))
#else
#define DXP_NEVER_RETURNS
#endif

/**
 * @defgroup icera_assert Assertion support
 * @{
 */
#ifndef DXP_RELEASE_CODE

/**
 * Assertion macro, compiled only in non-release code.
 * @param cond Condition to check.
 */
    #define DEV_ASSERT(cond)    REL_ASSERT(cond)
    #define DEV_ASSERT_EXTRA(cond, n, ...)    REL_ASSERT_EXTRA(cond, n, __VA_ARGS__)
/**
 * Failure macro, compiled only in non-release code.
 * @param msg Message failure string.
 */
    #define DEV_FAIL(msg)       REL_FAIL(msg)
    #define DEV_FAIL_EXTRA(msg, n, ...)    REL_FAIL_EXTRA(msg, n, __VA_ARGS__)
#else
    #define DEV_ASSERT(cond)
    #define DEV_ASSERT_EXTRA(cond, n, ...)
    #define DEV_FAIL(msg)
    #define DEV_FAIL_EXTRA(msg, n, ...)
#endif

/**
 * We hope to label and suppress false positives from Klocwork static checking
 * by putting asserts into the code.
 * We use the UNVERIFIED form when we haven't done a serious code review.
 * In the code review we might choose to re-write (even for a false positive)
 * to improve the structure or legibility of the code.
 */
#define KW_ASSERT_FALSE_POSITIVE(cond)  DEV_ASSERT(cond)
#define KW_ASSERT_UNVERIFIED(cond)      DEV_ASSERT(cond)

#if (defined (__dxp__))
/**
 * Assertion macro, always compiled.
 * @param cond Condition to check.
 */
struct com_AssertInfo
{
    const int line;
    const char *filename;
    const char condition[];
};


#if (ICERA_SDK_EV_AT_LEAST(4,3,'a'))
/* EV4.3a only!

  SDK team implemented: Auto-generated thunks for invokation of assert function (requires EV4.3a compiler)

  - to reduce codesize in the caller of assert (and thus improve I-Cache performance or reduce I-Mem consumption)
    the code in the caller of assert is a simple fn call (no args are formed to the actual assert function)
  - the simple fn call is to an auto-generated thunk function which contains the invokation of the underlying assert fail code
  - the think is auto-generated using a new cpp extension (pragma deferred_output) so the source text for the thunk body
    is expanded *outside* the function which calls the assert macro (the alternative would be to use GCC nested
    functions, but these can't have attributes
  - it uses attributes to ensure its uncached (i.e. not in I-Mem and not close to important I-Cached functions),
    not instrumented, doesn't return etc. etc
  - the thunk's name is unique using another new cpp extension (__CANON_BASE_FILE__ and __CANON_LINE__)
    so that the thunk names are globally unique
 Updated by Eddy for EV4.5a: initialising the condition field is not allowed when condition is a
 flexible array member. So declare a local structure with exactly the right size. The same code is
 generated, but the debugging .size now includes the true length of the condition string rather than
 0 for a variable array element.
*/

#define INNER_NAME_OF_ASSERT_THUNK(canonBaseFileArg, canonLineArg) _AssertThunk_##canonBaseFileArg##_##canonLineArg

#define GENERATE_NAME_OF_ASSERT_THUNK(canonBaseFileArg, canonLineArg) INNER_NAME_OF_ASSERT_THUNK(canonBaseFileArg, canonLineArg)

#define INNER_GENERATE_REL_ASSERT_THUNK(cond, canonBaseFileArg, canonLineArg, fileArg, lineArg) \
void GENERATE_NAME_OF_ASSERT_THUNK(canonBaseFileArg, canonLineArg) (void)  __attribute__((noreturn, unused, noinline, no_instrument_function, placement(uncached))); \
void GENERATE_NAME_OF_ASSERT_THUNK(canonBaseFileArg, canonLineArg) (void) {\
struct sizedAssertInfo {const int line; const char *filename; const char condition[sizeof(#cond)];}; \
static const struct sizedAssertInfo    aInfoThunk __attribute__((placement(uncached))) ={.filename=fileArg, .line=lineArg, .condition=#cond};\
com_AssertFailStruct ((const struct com_AssertInfo*)&aInfoThunk); }

#define GENERATE_REL_ASSERT_THUNK(cond, canonBaseFileArg, canonLineArg, fileArg, lineArg) \
  INNER_GENERATE_REL_ASSERT_THUNK(cond, canonBaseFileArg, canonLineArg, fileArg, lineArg)

/* Icera-invented pragma (must occur *after* the assert define)
*/
#pragma GCC deferred_output GENERATE_REL_ASSERT_THUNK


/* Restricted to 4.5a until all warnings induced by the use of cbreak are solved */
#if ((ICERA_SDK_EV_AT_LEAST(4,5,'a')) && defined(BT2_NO_ASSERT_THUNKS))
#define INNER_GENERATE_REL_ASSERT_IF(cond, canonBaseFileArg, canonLineArg) \
  if ((cond)==0) {\
    __asm__ volatile ("cbreak");\
  }
#else
#define INNER_GENERATE_REL_ASSERT_IF(cond, canonBaseFileArg, canonLineArg) \
  if ((cond)==0) {\
    void GENERATE_NAME_OF_ASSERT_THUNK(canonBaseFileArg, canonLineArg) (void) __attribute__((noreturn, unused, noinline, no_instrument_function, placement(uncached))); \
    GENERATE_NAME_OF_ASSERT_THUNK(canonBaseFileArg, canonLineArg) (); \
  }
#endif

#define GENERATE_REL_ASSERT_IF(cond, canonBaseFileArg, canonLineArg) \
  INNER_GENERATE_REL_ASSERT_IF(cond, canonBaseFileArg, canonLineArg)


#define INNER_REL_ASSERT(cond, canonBaseFileArg, canonLineArg) \
  GENERATE_REL_ASSERT_THUNK(cond, canonBaseFileArg, canonLineArg, __FILE__, __LINE__) \
  GENERATE_REL_ASSERT_IF(cond, canonBaseFileArg, canonLineArg)

/* Icera-invented __CANON_BASE_FILE__ to form a valid globally unique function name
*/
#define REL_ASSERT(cond) \
  INNER_REL_ASSERT(cond, __CANON_BASE_FILE__, __CANON_LINE__)


/* ASSERT_EXTRA routines take varargs, and these can potentially reference local variables
   - they wouldn't be in scope in the thunk, and thus the thunk-approach won't work.
   Thus we'll use a non-thunk approach for ASSERT_EXTRA ;-(
*/
#define REL_ASSERT_EXTRA(cond, n, ...)  if (unlikely(!(cond))) { \
struct sizedAssertInfo {const int line; const char *filename; const char condition[sizeof(#cond)];}; \
static const struct sizedAssertInfo ainfo __attribute__((placement(uncached))) = {.filename=__FILE__, .line=__LINE__, .condition=#cond};\
com_ExtendedAssertionStruct((const struct com_AssertInfo*)&ainfo, n, __VA_ARGS__);}

#else
/* SDK tweaked version of old assert macros
   - Using static data in a placement (internal) or overlay function *will place static data in D-Mem*. Thus we
     use an attribute here to ensure its in uncached memory (as its not performance critical) so it will interfere
     less with normal cached memory, and will avoid wasting D-Mem from internal functions.

   - Using const with (tweaks to the ainfo types) gets it into the best section and avoids duplication for dual-build
     compilation units

   - Further codespace could be saved by improving the com_ExtendedAssertionStruct interface to pass args differently, or
       to call another generated function to setup the args (this function *not* being in I-Mem!). See "auto-generated
       assert thunk" above for an idea of how we can improve this with EV4.3a
*/
  #define REL_ASSERT(cond)                if (unlikely(!(cond))) {static const struct com_AssertInfo ainfo __attribute__((placement(uncached))) = {.filename=__FILE__, .line=__LINE__, .condition=#cond}; com_AssertFailStruct(&ainfo);}

  #define REL_ASSERT_EXTRA(cond, n, ...)  if (unlikely(!(cond))) {static const struct com_AssertInfo ainfo  __attribute__((placement(uncached))) = {.filename=__FILE__, .line=__LINE__, .condition=#cond}; com_ExtendedAssertionStruct(&ainfo, n, __VA_ARGS__);}

#endif

#else //linux
    #define REL_ASSERT(cond)                assert(cond)
    #define REL_ASSERT_EXTRA(cond, n, ...)  REL_ASSERT(cond)
#endif

/**
 * Failure macro, always compiled.
 * @param msg Message failure string..
 */
#define REL_FAIL(msg)        REL_ASSERT(!(msg))
#define REL_FAIL_EXTRA(msg, n, ...)  REL_ASSERT_EXTRA(!(msg), n, __VA_ARGS__)

/**
 * Various tool libraries insist on using asm rather than __asm__. So bodge in a switch to __asm__
 */
#ifdef __dxp__
    #define asm __asm__
#endif

/**
 * @}
 */

/**
 * @defgroup icera_attributes Compiler attributes
 * @{
 */

/**
 * Indicates if a function is forced to be not inlined
 */
#ifdef __dxp__
#define DXP_NOINLINE  __attribute__((noinline))
#define DXP_FORCEINLINE __attribute__((always_inline))
#else
#define DXP_NOINLINE
#define DXP_FORCEINLINE
#endif

/**
 * Indicates to the compiler that a symbol is deliberately unused.
 */
#define NOT_USED(SYMBOL)    (void)(SYMBOL);

/*
 * Defines to specify the section for functions and variables;
 */


#ifdef __dxp__

/* NOTE: From EV4.5a, these values must not be used directly as arguments to mphal_overlay functions.
         Use tOverlayId enumeration instead */

#define DXP_OVERLAY_NONE    0
#define DXP_LTE_OVERLAY_SECTION 1
#define DXP_2G_OVERLAY_SECTION 2
#define DXP_3G_OVERLAY_SECTION 3

/**
 * Attribute definition to place code in internal memory. Use like the following:
 *
 * static short DXP_ICODE someFunction(void)
 * {
 * };
 */
#if defined (DEBUG_NO_ONCHIP_CODE) || defined (LTE_FULL_UPLANE_TESTBENCH)
    #define DXP_COMMONICODE
    #define DXP_LTEICODE
    #define DXP_2GICODE
    #define DXP_3GICODE
    #define DXP_8X_COMMONICODE
    #define DXP0_COMMONICODE
    #define DXP2_COMMONICODE
    #define DXP_COMMONGCODE
    #define DXP_LTEGCODE
    #define DXP_2GGCODE
    #define DXP_3GGCODE
#else
/* Common onchip code */
    #ifdef TARGET_DXP8060
        #define DXP_COMMONGCODE
        #define DXP_8X_COMMONICODE  __attribute__((placement(internal)))
        #define DXP0_COMMONICODE
        #define DXP2_COMMONICODE
    #else
        #define DXP_COMMONGCODE     __attribute__((placement(gmem)))
        #define DXP_8X_COMMONICODE
        #define DXP0_COMMONICODE    __attribute__((placement(imem)))
        #define DXP2_COMMONICODE    __attribute__((placement(imem2)))
    #endif

    #ifdef USE_MEMORY_OVERLAYS
        #if (ICERA_SDK_EV_AT_LEAST(4,5,'a'))
            /* the next line should be "imem", - update after EV4.5a fix to BT2 */
    #define DXP_COMMONICODE __attribute__((placement(internal)))
            #define DXP_LTEICODE    __attribute__((placement(imem), overlay(DXP_LTE_OVERLAY_SECTION)))
            #define DXP_2GICODE     __attribute__((placement(imem), overlay(DXP_2G_OVERLAY_SECTION)))
            #define DXP_3GICODE     __attribute__((placement(imem), overlay(DXP_3G_OVERLAY_SECTION)))

            #ifdef TARGET_DXP8060
                #define DXP0_LTEICODE
                #define DXP2_LTEICODE
                #define DXP_8X_LTEICODE __attribute__((placement(imem), overlay(DXP_LTE_OVERLAY_SECTION)))
                #define DXP_LTEGCODE

                #define DXP0_3GICODE
                #define DXP2_3GICODE
                #define DXP_8X_3GICODE  __attribute__((placement(imem), overlay(DXP_3G_OVERLAY_SECTION)))
                #define DXP_3GGCODE
            #else
                #define DXP0_LTEICODE   __attribute__((placement(imem), overlay(DXP_LTE_OVERLAY_SECTION)))
                #define DXP2_LTEICODE   __attribute__((placement(imem2), overlay(DXP_LTE_OVERLAY_SECTION)))
                #define DXP_8X_LTEICODE
                #define DXP_LTEGCODE    __attribute__((placement(gmem), overlay(DXP_LTE_OVERLAY_SECTION)))

                #define DXP0_3GICODE    __attribute__((placement(imem), overlay(DXP_3G_OVERLAY_SECTION)))
                #define DXP2_3GICODE    __attribute__((placement(imem2), overlay(DXP_3G_OVERLAY_SECTION)))
                #define DXP_8X_3GICODE
                #define DXP_3GGCODE     __attribute__((placement(gmem), overlay(DXP_3G_OVERLAY_SECTION)))
            #endif
        #else
            #define DXP_COMMONICODE __attribute__((placement(internal)))
        #define DXP_LTEICODE     __attribute__((placement(internal,DXP_LTE_OVERLAY_SECTION)))
        #define DXP_2GICODE     __attribute__((placement(internal,DXP_2G_OVERLAY_SECTION)))
        #define DXP_3GICODE     __attribute__((placement(internal,DXP_3G_OVERLAY_SECTION)))
        #endif
    #else
/* When there are no overlays, put everything internal. If it doesn't fit, you need overlays! */
        #define DXP_LTEICODE     __attribute__((placement(internal)))
        #define DXP_2GICODE     __attribute__((placement(internal)))
        #define DXP_3GICODE     __attribute__((placement(internal)))
    #endif  /* #ifdef USE_MEMORY_OVERLAYS */
#endif  /* #ifdef DEBUG_NO_ONCHIP_CODE */

/**
 * Attribute definitions to place data in internal memory. Use like the following:
 *
 * static short DXP_COMMONIDATA DXP_A16 dxp_taps[] = {1,2,3,4};
 *
 * NOTE: _NOSR postfix is used to indicate memory which does not
 * need saving and restoring across hibernate cycles.
 */
/* Once overlay memory is fully supported, the following will no longer be required */
#ifdef DEBUG_NO_ONCHIP_DATA
    #define DXP_OFFCHIPDATA
    #define DXP_COMMONIDATA
    #define DXP_COMMONIDATA_NOSR
    #define DXP_2GIDATA
    #define DXP_2GIDATA_NOSR
    #define DXP_3GIDATA
    #define DXP_3GIDATA_NOSR
    #define DXP_2GIDATA_M0
    #define DXP_2GIDATA_M1
    #define DXP_3GIDATA_M0
    #define DXP_3GIDATA_M1
    #define DXP_LTEIDATA_M0
    #define DXP_LTEIDATA_M1
    #define DXP_LTEIDATA
    #define DXP_LTEIDATA_FORCEINIT
#elif (ICERA_SDK_EV_AT_LEAST(4,5,'a'))
    #ifdef USE_MEMORY_OVERLAYS
        #define DXP_COMMONIDATA         __attribute__((placement(dmem), has_dmem_latency))
        #define DXP_COMMONIDATA_NOSR    __attribute__((placement(dmem), norestore, has_dmem_latency))
        #define DXP_LTEIDATA            __attribute__((placement(dmem), overlay(DXP_LTE_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_LTEIDATA_NOSR       __attribute__((placement(dmem), overlay(DXP_LTE_OVERLAY_SECTION), norestore, has_dmem_latency))
        #define DXP_2GIDATA             __attribute__((placement(dmem), overlay(DXP_2G_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_2GIDATA_NOSR        __attribute__((placement(dmem), overlay(DXP_2G_OVERLAY_SECTION), norestore, has_dmem_latency))
        #define DXP_3GIDATA             __attribute__((placement(dmem), overlay(DXP_3G_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_3GIDATA_NOSR        __attribute__((placement(dmem), overlay(DXP_3G_OVERLAY_SECTION), norestore, has_dmem_latency))
        #define DXP_2GIDATA_M0          __attribute__((placement(dmem), multidalign(0),overlay(DXP_2G_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_2GIDATA_M1          __attribute__((placement(dmem), multidalign(1),overlay(DXP_2G_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_3GIDATA_M0          __attribute__((placement(dmem), multidalign(0),overlay(DXP_3G_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_3GIDATA_M1          __attribute__((placement(dmem), multidalign(1),overlay(DXP_3G_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_LTEIDATA_M0         __attribute__((placement(dmem), multidalign(0),overlay(DXP_LTE_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_LTEIDATA_M1         __attribute__((placement(dmem),  multidalign(1), overlay(DXP_LTE_OVERLAY_SECTION),has_dmem_latency))

// DXP (old) placement apply to all builds and place code/data in DXP0 memory
// DXP_8X (new) placement defs only apply to 8xxx builds
// DXP0 / DXP2 placement defs only apply to 9xxx builds.
// Common cases are:
// 8x:external, 9x:Dmem 0 = DXP0
// 8x:external, 9x:Dmem 2 = DXP2
// 8x:Dmem0, 9x Dmem0 = DXP_8X  DXP0
// 8x:Dmem0, 9x Dmem2 = DXP_8X  DXP2
        #if defined(TARGET_DXP8060)
            #define DXP0_COMMONIDATA
            #define DXP0_COMMONIDATA_NOSR
            #define DXP0_LTEIDATA
            #define DXP0_LTEIDATA_NOSR
            #define DXP0_LTEIDATA_M0
            #define DXP0_LTEIDATA_M1
            #define DXP2_COMMONIDATA
            #define DXP2_COMMONIDATA_NOSR
            #define DXP2_LTEIDATA
            #define DXP2_LTEIDATA_NOSR
            #define DXP2_LTEIDATA_M0
            #define DXP2_LTEIDATA_M1
            #define DXP_8X_COMMONIDATA      __attribute__((placement(dmem), has_dmem_latency))
            #define DXP_8X_COMMONIDATA_NOSR __attribute__((placement(dmem), norestore, has_dmem_latency))
            #define DXP_8X_LTEIDATA         __attribute__((placement(dmem), overlay(DXP_LTE_OVERLAY_SECTION), has_dmem_latency))
            #define DXP_8X_LTEIDATA_NOSR    __attribute__((placement(dmem), overlay(DXP_LTE_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP_8X_LTEIDATA_M0      __attribute__((placement(dmem), multidalign(0), overlay(DXP_LTE_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP_8X_LTEIDATA_M1      __attribute__((placement(dmem), multidalign(0), overlay(DXP_LTE_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP_8X_2GIDATA          __attribute__((placement(dmem), overlay(DXP_2G_OVERLAY_SECTION), has_dmem_latency))
            #define DXP_8X_2GIDATA_NOSR     __attribute__((placement(dmem), overlay(DXP_2G_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP_8X_2GIDATA_M0       __attribute__((placement(dmem), multidalign(0), overlay(DXP_2G_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP_8X_2GIDATA_M1       __attribute__((placement(dmem), multidalign(0), overlay(DXP_2G_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP_8X_3GIDATA          __attribute__((placement(dmem), overlay(DXP_3G_OVERLAY_SECTION), has_dmem_latency))
            #define DXP_8X_3GIDATA_NOSR     __attribute__((placement(dmem), overlay(DXP_3G_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP_8X_3GIDATA_M0       __attribute__((placement(dmem), multidalign(0), overlay(DXP_3G_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP_8X_3GIDATA_M1       __attribute__((placement(dmem), multidalign(0), overlay(DXP_3G_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP0_GDATA
            #define DXP1_GDATA
            #define DXP2_GDATA
            #define DXP0_LTEGDATA
            #define DXP1_LTEGDATA
            #define DXP2_LTEGDATA
            #define DXP0_2GGDATA
            #define DXP1_2GGDATA
            #define DXP2_2GGDATA
            #define DXP0_3GGDATA
            #define DXP1_3GGDATA
            #define DXP2_3GGDATA

        #else  /* target 9x */
            #define DXP0_COMMONIDATA        __attribute__((placement(dmem), has_dmem_latency))
            #define DXP0_COMMONIDATA_NOSR   __attribute__((placement(dmem), norestore, has_dmem_latency))
            #define DXP0_LTEIDATA           __attribute__((placement(dmem), overlay(DXP_LTE_OVERLAY_SECTION), has_dmem_latency))
            #define DXP0_LTEIDATA_NOSR      __attribute__((placement(dmem), overlay(DXP_LTE_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP0_LTEIDATA_M0        __attribute__((placement(dmem), multidalign(0),overlay(DXP_LTE_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP0_LTEIDATA_M1        __attribute__((placement(dmem), multidalign(1),overlay(DXP_LTE_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP0_2GIDATA            __attribute__((placement(dmem), overlay(DXP_2G_OVERLAY_SECTION), has_dmem_latency))
            #define DXP0_2GIDATA_NOSR       __attribute__((placement(dmem), overlay(DXP_2G_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP0_2GIDATA_M0         __attribute__((placement(dmem), multidalign(0),overlay(DXP_2G_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP0_2GIDATA_M1         __attribute__((placement(dmem), multidalign(1),overlay(DXP_2G_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP0_3GIDATA            __attribute__((placement(dmem), overlay(DXP_3G_OVERLAY_SECTION), has_dmem_latency))
            #define DXP0_3GIDATA_NOSR       __attribute__((placement(dmem), overlay(DXP_3G_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP0_3GIDATA_M0         __attribute__((placement(dmem), multidalign(0),overlay(DXP_3G_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP0_3GIDATA_M1         __attribute__((placement(dmem), multidalign(1),overlay(DXP_3G_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP2_COMMONIDATA        __attribute__((placement(dmem2), has_dmem_latency))
            #define DXP2_COMMONIDATA_NOSR   __attribute__((placement(dmem2), norestore, has_dmem_latency))
            #define DXP2_LTEIDATA           __attribute__((placement(dmem2), overlay(DXP_LTE_OVERLAY_SECTION), has_dmem_latency))
            #define DXP2_LTEIDATA_NOSR      __attribute__((placement(dmem2), overlay(DXP_LTE_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP2_LTEIDATA_M0        __attribute__((placement(dmem2), multidalign(0),overlay(DXP_LTE_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP2_LTEIDATA_M1        __attribute__((placement(dmem2), multidalign(1),overlay(DXP_LTE_OVERLAY_SECTION), norestore, has_dmem_latency))
            #define DXP8X_COMMONIDATA
            #define DXP8X_COMMONIDATA_NOSR
            #define DXP8X_LTEIDATA
            #define DXP8X_LTEIDATA_NOSR
            #define DXP8X_LTEIDATA_M0
            #define DXP8X_LTEIDATA_M1
            #define DXP8X_2GIDATA
            #define DXP8X_2GIDATA_NOSR
            #define DXP8X_2GIDATA_M0
            #define DXP8X_2GIDATA_M1
            #define DXP8X_3GIDATA
            #define DXP8X_3GIDATA_NOSR
            #define DXP8X_3GIDATA_M0
            #define DXP8X_3GIDATA_M1
            #define DXP0_GDATA          __attribute__((placement(gmem), uni(0)))
            #define DXP1_GDATA          __attribute__((placement(gmem), uni(1)))
            #define DXP2_GDATA          __attribute__((placement(gmem), uni(2)))
            #define DXP0_LTEGDATA       __attribute__((placement(gmem), overlay(DXP_LTE_OVERLAY_SECTION), uni(0)))
            #define DXP1_LTEGDATA       __attribute__((placement(gmem), overlay(DXP_LTE_OVERLAY_SECTION), uni(1)))
            #define DXP2_LTEGDATA       __attribute__((placement(gmem), overlay(DXP_LTE_OVERLAY_SECTION), uni(2)))
            #define DXP0_2GGDATA        __attribute__((placement(gmem), overlay(DXP_2G_OVERLAY_SECTION), uni(0)))
            #define DXP1_2GGDATA        __attribute__((placement(gmem), overlay(DXP_2G_OVERLAY_SECTION), uni(1)))
            #define DXP2_2GGDATA        __attribute__((placement(gmem), overlay(DXP_2G_OVERLAY_SECTION), uni(2)))
            #define DXP0_3GGDATA        __attribute__((placement(gmem), overlay(DXP_3G_OVERLAY_SECTION), uni(0)))
            #define DXP1_3GGDATA        __attribute__((placement(gmem), overlay(DXP_3G_OVERLAY_SECTION), uni(1)))
            #define DXP2_3GGDATA        __attribute__((placement(gmem), overlay(DXP_3G_OVERLAY_SECTION), uni(2)))
        #endif
    #else
        /* When there are no overlays, put everything internal. If it doesn't fit, you need overlays! */
        #define DXP_COMMONIDATA         __attribute__((placement(dmem), has_dmem_latency))
        #define DXP_COMMONIDATA_NOSR    DXP_COMMONIDATA
        #define DXP_LTEIDATA            DXP_COMMONIDATA
        #define DXP_LTEIDATA_NOSR       DXP_COMMONIDATA
        #define DXP_2GIDATA             DXP_COMMONIDATA
        #define DXP_2GIDATA_NOSR        DXP_COMMONIDATA
        #define DXP_3GIDATA             DXP_COMMONIDATA
        #define DXP_3GIDATA_NOSR        DXP_COMMONIDATA
        #define DXP_2GIDATA_M0          DXP_COMMONIDATA
        #define DXP_2GIDATA_M1          DXP_COMMONIDATA
        #define DXP_3GIDATA_M0          DXP_COMMONIDATA
        #define DXP_3GIDATA_M1          DXP_COMMONIDATA
        #define DXP_LTEIDATA_M0         DXP_COMMONIDATA
        #define DXP_LTEIDATA_M1         DXP_COMMONIDATA
        #define DXP0_COMMONIDATA        DXP_COMMONIDATA
        #define DXP0_COMMONIDATA_NOSR   DXP_COMMONIDATA
        #define DXP0_LTEIDATA           DXP_COMMONIDATA
        #define DXP0_LTEIDATA_NOSR      DXP_COMMONIDATA
        #define DXP0_LTEIDATA_M0        DXP_COMMONIDATA
        #define DXP0_LTEIDATA_M1        DXP_COMMONIDATA
        #define DXP2_COMMONIDATA        DXP_COMMONIDATA
        #define DXP2_COMMONIDATA_NOSR   DXP_COMMONIDATA
        #define DXP2_LTEIDATA           DXP_COMMONIDATA
        #define DXP2_LTEIDATA_NOSR      DXP_COMMONIDATA
        #define DXP2_LTEIDATA_M0        DXP_COMMONIDATA
        #define DXP2_LTEIDATA_M1        DXP_COMMONIDATA
        #define DXP_8X_COMMONIDATA      DXP_COMMONIDATA
        #define DXP_8X_COMMONIDATA_NOSR DXP_COMMONIDATA
        #define DXP_8X_LTEIDATA         DXP_COMMONIDATA
        #define DXP_8X_LTEIDATA_NOSR    DXP_COMMONIDATA
        #define DXP_8X_LTEIDATA_M0      DXP_COMMONIDATA
        #define DXP_8X_LTEIDATA_M1      DXP_COMMONIDATA

    #endif

/* pre EV4.5a */
#else
    #define DXP_OFFCHIPDATA __attribute__((placement(cached)))
    #define DXP_COMMONIDATA     __attribute__((placement(internal), has_dmem_latency))
    #define DXP_COMMONIDATA_NOSR     __attribute__((placement(internal_norestore), has_dmem_latency))

    #define DXP_8X_COMMONIDATA      __attribute__((placement(internal), has_dmem_latency))
    #define DXP_8X_COMMONIDATA_NOSR __attribute__((placement(internal), norestore, has_dmem_latency))

    #ifdef USE_MEMORY_OVERLAYS
/* Memory overlay definitions */
        #define DXP_LTEIDATA         __attribute__((placement(internal,DXP_LTE_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_LTEIDATA_NOSR    __attribute__((placement(internal_norestore,DXP_LTE_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_2GIDATA         __attribute__((placement(internal,DXP_2G_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_2GIDATA_NOSR    __attribute__((placement(internal_norestore,DXP_2G_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_3GIDATA         __attribute__((placement(internal,DXP_3G_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_3GIDATA_NOSR    __attribute__((placement(internal_norestore,DXP_3G_OVERLAY_SECTION), has_dmem_latency))

            #define DXP_2GIDATA_M0 __attribute__((placement(multidalign,DXP_2G_OVERLAY_SECTION,0), has_dmem_latency))
            #define DXP_2GIDATA_M1 __attribute__((placement(multidalign,DXP_2G_OVERLAY_SECTION,1), has_dmem_latency))
            #define DXP_3GIDATA_M0 __attribute__((placement(multidalign,DXP_3G_OVERLAY_SECTION,0), has_dmem_latency))
            #define DXP_3GIDATA_M1 __attribute__((placement(multidalign,DXP_3G_OVERLAY_SECTION,1), has_dmem_latency))
            #define DXP_LTEIDATA_M0 __attribute__((placement(multidalign,DXP_LTE_OVERLAY_SECTION,0), has_dmem_latency))
            #define DXP_LTEIDATA_M1 __attribute__((placement(multidalign,DXP_LTE_OVERLAY_SECTION,1), has_dmem_latency))
        #define DXP0_COMMONIDATA
        #define DXP0_COMMONIDATA_NOSR
        #define DXP0_LTEIDATA
        #define DXP0_LTEIDATA_NOSR
        #define DXP0_LTEIDATA_M0
        #define DXP0_LTEIDATA_M1
        #define DXP2_COMMONIDATA
        #define DXP2_COMMONIDATA_NOSR
        #define DXP2_LTEIDATA
        #define DXP2_LTEIDATA_NOSR
        #define DXP2_LTEIDATA_M0
        #define DXP2_LTEIDATA_M1
        #define DXP_8X_LTEIDATA         __attribute__((placement(internal,DXP_LTE_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_8X_LTEIDATA_NOSR    __attribute__((placement(internal_norestore,DXP_LTE_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_8X_LTEIDATA_M0      __attribute__((placement(multidalign,DXP_LTE_OVERLAY_SECTION,0), has_dmem_latency))
        #define DXP_8X_LTEIDATA_M1      __attribute__((placement(multidalign,DXP_LTE_OVERLAY_SECTION,1), has_dmem_latency))
        #define DXP_8X_2GIDATA          __attribute__((placement(internal,DXP_2G_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_8X_2GIDATA_NOSR     __attribute__((placement(internal_norestore,DXP_2G_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_8X_2GIDATA_M0       __attribute__((placement(multidalign,DXP_2G_OVERLAY_SECTION,0), has_dmem_latency))
        #define DXP_8X_2GIDATA_M1       __attribute__((placement(multidalign,DXP_2G_OVERLAY_SECTION,1), has_dmem_latency))
        #define DXP_8X_3GIDATA          __attribute__((placement(internal,DXP_3G_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_8X_3GIDATA_NOSR     __attribute__((placement(internal_norestore,DXP_3G_OVERLAY_SECTION), has_dmem_latency))
        #define DXP_8X_3GIDATA_M0       __attribute__((placement(multidalign,DXP_3G_OVERLAY_SECTION,0), has_dmem_latency))
        #define DXP_8X_3GIDATA_M1       __attribute__((placement(multidalign,DXP_3G_OVERLAY_SECTION,1), has_dmem_latency))
#else
/* When there are no overlays, put everything internal. If it doesn't fit, you need overlays! */
        #define DXP_LTEIDATA        __attribute__((placement(internal), has_dmem_latency))
        #define DXP_LTEIDATA_NOSR   __attribute__((placement(internal_norestore), has_dmem_latency))
        #define DXP_2GIDATA         __attribute__((placement(internal), has_dmem_latency))
        #define DXP_2GIDATA_NOSR    __attribute__((placement(internal_norestore), has_dmem_latency))
        #define DXP_3GIDATA         __attribute__((placement(internal), has_dmem_latency))
        #define DXP_3GIDATA_NOSR    __attribute__((placement(internal_norestore), has_dmem_latency))
            #define DXP_2GIDATA_M0 __attribute__((placement(multidside,0), has_dmem_latency))
            #define DXP_2GIDATA_M1 __attribute__((placement(multidside,1), has_dmem_latency))
            #define DXP_3GIDATA_M0 __attribute__((placement(multidside,0), has_dmem_latency))
            #define DXP_3GIDATA_M1 __attribute__((placement(multidside,1), has_dmem_latency))
            #define DXP_LTEIDATA_M0 __attribute__((placement(multidside,0), has_dmem_latency))
            #define DXP_LTEIDATA_M1 __attribute__((placement(multidside,1), has_dmem_latency))

    #endif  /* #ifdef USE_MEMORY_OVERLAYS */
#endif  /* #ifdef DEBUG_NO_ONCHIP_CODE */


    #define DXP_IN_DMEM __attribute__((has_dmem_latency))

/**
 * Attribute definition to align storage on a 256-bit / 32-byte boundary.
 *  - this is the length of a DMEM cache line.
 *
 */
    #define DXP_A256 __attribute__((aligned (32)))

/**
 * Attribute definition to align storage on a 128 bit boundary.. Use like the following:
 *
 * static uint16 DXP_COMMONIDATA DXP_A128 dxp_taps[] = {1,2,3,4};
 */
    #define DXP_A128 __attribute__((aligned (16)))

/**
 * Attribute definition to align storage on a 64 bit boundary.. Use like the following:
 *
 * static uint16 DXP_COMMONIDATA DXP_A64 dxp_taps[] = {1,2,3,4};
 */
    #define DXP_A64 __attribute__((aligned (8)))
/**
 * Attribute definition to align storage on a 32 bit boundary.. Use like the following:
 *
 * static int8 DXP_COMMONIDATA DXP_A32 dxp_taps[] = {1,2,3,4};
 */
    #define DXP_A32 __attribute__((aligned (4)))
/**
 * Attribute definition to align storage on a 16 bit boundary.. Use like the following:
 *
 * static int8 DXP_COMMONIDATA DXP_A16 dxp_taps[] = {1,2,3,4};
 */
    #define DXP_A16 __attribute__((aligned (2)))

/**
 * Attribute definition to place data in external memory referenced relative to each DXP $dp pointer..
 * There will exist 2 instances of the variables, one on each DXP. Standard use for same code (thread) executing on both DXPs.
 * Variables are placed in a non-cacheable region.
 * Use like the following:
 * int8 DXP_UNCACHED_DUAL array[10];
 */
#if (ICERA_SDK_EV_AT_LEAST(4,5,'a'))
    #define DXP_UNCACHED_DUAL __attribute__((placement(uncached), multi))
#else
    #define DXP_UNCACHED_DUAL __attribute__((placement(uncached), dual))
#endif

/**
 * Attribute definition to place data in external memory referenced relative to each DXP $dp pointer..
 * There will exist 2 instances of the variables, one on each DXP. Standard use for same code (thread) executing on both DXPs.
 * Use like the following:
 * int8 DXP_DUAL array[10];
 */
#if (ICERA_SDK_EV_AT_LEAST(4,5,'a'))
    #define DXP_DUAL __attribute__((placement(cached), multi))
    #define DXP_MULTI __attribute__((placement(cached), multi))
#else
    #define DXP_DUAL __attribute__((placement(cached), dual))
    /* DXP_MULTI on 8060 with 4.4 SDK really means DXP_DUAL */
    #define DXP_MULTI __attribute__((placement(cached), dual))
#endif

/**
 * Attribute definition to place data in uncached external memory.. Use like the following:
 * c.f. also DXP_UNCACHED_DUAL for uncached dual variables
 *
 * int8 DXP_UNCACHED array[10];
 */
    #define DXP_UNCACHED __attribute__((placement(uncached), uni))

/**
 * Attribute definition to place data in uncached non initialised external memory .. Use like the following:
 *
 * int8 DXP_UNCACHED_NOINIT array[10];
 */
    #define DXP_UNCACHED_NOINIT __attribute__((placement("platform_noninit"), uni))

/**
 * Attribute definition to place data in cached external memory for DXP0.. Use like the following:
 *
 * int8 DXP_CACHED_UNI0 array[10];
 */
    #define DXP_CACHED_UNI0 __attribute__((placement(cached), uni(0)))

/**
 * Attribute definition to place data in cached external memory for DXP1.. Use like the following:
 *
 * int8 DXP_CACHED_UNI1 array[10];
 */
    #define DXP_CACHED_UNI1 __attribute__((placement(cached), uni(1)))

#else
/* If run elsewhere (eg x86/linux) use default allocation */
    #define DXP_ICODE
    #define DXP_2GICODE
    #define DXP_3GICODE
    #define DXP_LTEICODE
    #define DXP_2GIDATA
    #define DXP_3GIDATA
    #define DXP_3GIDATA_M0
    #define DXP_3GIDATA_M1
    #define DXP_LTEIDATA
    #define DXP_LTEIDATA_M0
    #define DXP_LTEIDATA_M1
    #define DXP_COMMONIDATA
    #define DXP_COMMONIDATA_NOSR
    #define DXP_COMMONICODE
    #define DXP_COMMONGCODE
    #define DXP_8X_COMMONICODE
    #define DXP_8X_LTEICODE
    #define DXP_8X_3GICODE
    #define DXP0_COMMONICODE
    #define DXP2_COMMONICODE
    #define DXP0_2GICODE
    #define DXP0_3GICODE
    #define DXP0_LTEICODE
    #define DXP2_2GICODE
    #define DXP2_3GICODE
    #define DXP2_LTEICODE

    #define DXP_A256
    #define DXP_A128
    #define DXP_A64
    #define DXP_A32
    #define DXP_A16

    #define DXP_UNCACHED_DUAL
    #define DXP_DUAL
    #define DXP_MULTI
    #define DXP_CACHED_UNI0
    #define DXP_CACHED_UNI1
    #define DXP_UNCACHED

#endif

#ifndef __dxp__
    /* Read a register */
    #define REG_RD1(_BASE, _OFFSET)
    #define REG_RD2(_BASE, _OFFSET)
    #define REG_RD4(_BASE, _OFFSET)
    /* Write a register */
    #define REG_WR1(_BASE, _OFFSET, _VALUE)
    #define REG_WR2(_BASE, _OFFSET, _VALUE)
    #define REG_WR4(_BASE, _OFFSET, _VALUE)
#else
    /* Read a register */
    #define REG_RD1(_BASE, _OFFSET) (((volatile int8 *) (_BASE))[(_OFFSET)/sizeof(int8)])
    #define REG_RD2(_BASE, _OFFSET) (((volatile int16 *)(_BASE))[(_OFFSET)/sizeof(int16)])
    #define REG_RD4(_BASE, _OFFSET) (((volatile int32 *)(_BASE))[(_OFFSET)/sizeof(int32)])
    /* Write a register */
    #define REG_WR1(_BASE, _OFFSET, _VALUE) ((((volatile int8 *) (_BASE))[(_OFFSET)/sizeof(int8)]) = (_VALUE))
    #define REG_WR2(_BASE, _OFFSET, _VALUE) ((((volatile int16 *)(_BASE))[(_OFFSET)/sizeof(int16)]) = (_VALUE))
    #define REG_WR4(_BASE, _OFFSET, _VALUE) ((((volatile int32 *)(_BASE))[(_OFFSET)/sizeof(int32)]) = (_VALUE))
#endif

/** @} */

#ifndef USE_STDINT_H
/**
 * @defgroup icera_common_types Fundamental types
 *
 * @{
 */

/**
 * Minimum value that can be held by type int8.
 * @see INT8_MAX
 * @see int8
 */
#define INT8_MIN        (-128)
/**
 * Maximum value that can be held by type int8.
 * @see INT8_MIN
 * @see int8
 */
#define INT8_MAX        (127)
/**
 * Maximum value that can be held by type uint8.
 * @see uint8
 */
#define UINT8_MAX       (255)
/**
 * Minimum value that can be held by type int16.
 * @see INT16_MAX
 * @see int16
 */
#define INT16_MIN       (-32768)
/**
 * Maximum value that can be held by type int16.
 * @see INT16_MIN
 * @see int16
 */
#define INT16_MAX       (32767)
/**
 * Maximum value that can be held by type uint16.
 * @see uint16
 */
#define UINT16_MAX      (65535)
/**
 * Minimum value that can be held by type int32.
 * @see INT32_MAX
 * @see int32
 */
#define INT32_MIN       (-2147483648L)
/**
 * Maximum value that can be held by type int32.
 * @see INT32_MIN
 * @see int32
 */
#define INT32_MAX       (2147483647L)
/**
 * Maximum value that can be held by type uint32.
 * @see uint32
 */
#define UINT32_MAX      (4294967295UL)
/**
 * Minimum value that can be held by type int64.
 * @see INT64_MAX
 * @see int64
 */
#define INT64_MIN        (-0x7fffffffffffffffLL-1)
/**
 * Maximum value that can be held by type int64.
 * @see INT64_MIN
 * @see int64
 */
#define INT64_MAX       (0X7fffffffffffffffLL)
/**
 * Maximum value that can be held by type uint64.
 * @see uint64
 */
#define UINT64_MAX      (0xffffffffffffffffULL)
#endif /* USE_STDINT_H */

#define com_Swift1Entry()                                           \
  {                                                                 \
    uint32 cycles, dgm0_prologue=4;                                 \
    __asm volatile ("get %0, $DBGCNT\n" : "=b" (cycles));           \
    __asm volatile ("put $DBGDQM0, %0\n" : : "b" (dgm0_prologue));  \
    __asm volatile ("put $DBGDQM1, %0\n" : :  "b" (cycles));        \
  }
#define com_Swift1Exit()                                            \
  {                                                                 \
    uint32 cycles, dgm0_epilogue=5;                                 \
    __asm volatile ("get %0, $DBGCNT\n" : "=b" (cycles));           \
    __asm volatile ("put $DBGDQM0, %0\n" : : "b" (dgm0_epilogue));  \
    __asm volatile ("put $DBGDQM1, %0\n"  : : "b" (cycles));        \
  }

#define com_Swift0Entry()                                           \
  {                                                                 \
    register uint32 dgm0_prologue=4;                                \
    __asm volatile ("put $DBGDQM0, %0\n" : : "b" (dgm0_prologue));  \
  }
#define com_Swift0Exit()                                            \
  {                                                                 \
    register uint32 dgm0_epilogue=5;                                \
    __asm volatile ("put $DBGDQM0, %0\n" : : "b" (dgm0_epilogue));  \
  }

/**
 * Converts n to ceil(n/8)
 */
#define COM_SCRATCH_BYTES_TO_DOUBLES(n) (((n)+7) >> 3)


/*************************************************************************************************
 * Public Types
 ************************************************************************************************/

/**
 * 8 bit signed integer
 * @see INT8_MAX
 * @see INT8_MIN
 */
typedef signed   char int8;
/**
 * 8 bit unsigned integer
 * @see UINT8_MAX
 */
typedef unsigned char uint8;

/**
 * 16 bit signed integer
 * @see INT16_MAX
 * @see INT16_MIN
 */
typedef signed   short int16;
/**
 * 16 bit unsigned integer
 * @see UINT16_MAX
 */
typedef unsigned short uint16;

/**
 * 32 bit signed integer
 * @see INT32_MAX
 * @see INT32_MIN
 */
typedef signed   int int32;

/**
 * 32 bit unsigned integer
 * @see UINT32_MAX
 */
typedef unsigned int uint32;

/**
 * 64 bit signed integer
 * @see INT64_MAX
 * @see INT64_MIN
 */
#if !defined(int64)
typedef signed   long long int64;
#endif

/**
 * 64 bit unsigned integer
 * @see UINT64_MAX
 */
typedef unsigned long long uint64;




/**
 * Complex number type with fixed point 8 bit members.
 */
typedef struct complex8Tag
{
    int8 real;                                                                 /*!< Real part. */
    int8 imag;                                                                 /*!< Imaginary part. */

} DXP_A16 complex8;

/**
 * Complex number type with fixed point 16 bit members.
 */
typedef struct complex16Tag
{
    int16 real;                                                                /*!< Real part. */
    int16 imag;                                                                /*!< Imaginary part. */

} DXP_A32 complex16;

/**
 * Complex number type with fixed point 32 bit members.
 */
typedef struct complex32Tag
{
    int32 real;                                                                /*!< Real part. */
    int32 imag;                                                                /*!< Imaginary part. */

} DXP_A64 complex32;

/**
 * Structure to hold a pointer into a circular buffer
 *
 * This structure allows the representation of a position in a circular buffer to be managed. The
 * position is managed in atoms
 */
typedef struct circ_ptr_s
{
    void *base;
    uint16 offset;                                                             /**< in atoms */
    uint16 length;                                                             /**< in atoms */
} circ_ptr_s;

#define MAX_SCRATCH_STACK_DEPTH 30

typedef struct scratch_stack
{
    uint32 addr;
    uint32 bytes;
} scratch_stack;


#ifdef __dxp__
#if (ICERA_SDK_EV_AT_LEAST(4,5,'a'))

#include "mphal_overlay.h"

/* Enumerated type for specifying memory overlay IDs - use enumerations defined by mphal_overlay.h */
typedef enum
{
    OVERLAY_NONE        = DXP_OVERLAY_NONE,
    OVERLAY_ID_LTE      = MPHAL_OVERLAYID_DXP0_L1MEM_OVERLAY1,   /* LTE Memory overlay */
    OVERLAY_ID_2G       = MPHAL_OVERLAYID_DXP0_L1MEM_OVERLAY2,    /* GSM Memory overlay */
    OVERLAY_ID_3G       = MPHAL_OVERLAYID_DXP0_L1MEM_OVERLAY3     /* UMTS memory overlay */
} tOverlayId;
#else
/* Enumerated type for specifying memory overlay IDs (must not exceed MPHAL_NUM_OVERLAYS) */
typedef enum
{
    OVERLAY_NONE        = DXP_OVERLAY_NONE,
    OVERLAY_ID_LTE      = DXP_LTE_OVERLAY_SECTION,   /* LTE Memory overlay */
    OVERLAY_ID_2G       = DXP_2G_OVERLAY_SECTION,    /* GSM Memory overlay */
    OVERLAY_ID_3G       = DXP_3G_OVERLAY_SECTION     /* UMTS memory overlay */
} tOverlayId;
#endif
#endif

/*************************************************************************************************
 * Public function declarations
 ************************************************************************************************/

/**
 * Assertion function, called on assertion failure. The struct version produces better code, but
 * the non-struct version is retained for compatibility
 *
 * @param filename  The name of the file containing the error
 * @param line_no   Line number
 * @param message   Text string of the assertion that failed
 */
extern void DXP_NEVER_RETURNS
com_AssertFail(const char *filename, int line_no, const char *message);
#ifdef __dxp__
extern void DXP_NEVER_RETURNS
com_AssertFailStruct(const struct com_AssertInfo *assert_info);
#endif

/**
 * Extended assertion function, called on assertion failure
 *
 * @param filename  The name of the file containing the error
 * @param line_no   Line number
 * @param message   Text string of the assertion that failed
 * @param n_extra_fields    Number of extra information fields
 * @param ...               int32s carrying information to help diagnose the error
 */
extern void DXP_NEVER_RETURNS
com_ExtendedAssertion(const char *filename,
                      int line_no,
                      const char *message,
                      int n_extra_fields,
                      ...);
#if (defined (__dxp__))
extern void DXP_NEVER_RETURNS
com_ExtendedAssertionStruct(const struct com_AssertInfo *assert_info,
                      int n_extra_fields,
                      ...);
#endif

/*************************************************************************************************
 * Public variable declarations
 ************************************************************************************************/


/*************************************************************************************************
 * Public function declarations
 ************************************************************************************************/
#define COM_MAX_SCRATCH_INSTANCES           2

/**
 * Initialise the scratch memory allocator
 *
 * @ingroup dmem_scratch
 * @param base_i64ptr Aligned pointer to the base of the internal scratch area
 * @param num_bytes   Number of bytes available. Should be a multiple of 8.
 * @param inst        Scratch memory area to initialise
 *
 * There must be no outstanding scratch use when this function is called.
 *
 * @return The maximum number of bytes allocated from the previously configured scratch buffer, or
 * 0 if SCRATCH_WATERMARKING is disabled
 *
 * @see scratch_free
 */
//#if defined (KI_TEST_TOOL_SIGNAL_INFO)
extern int com_ScratchInit(void *base_i64ptr, uint32 n_bytes, int32 inst);
//#endif

/**
 * Reserves a memory area from the scratch stack.Use
 * com_ScratchAlloc(num_doubles,0) to avoid duplication, instead
 * of scratch_alloc(num_doubles). Example call:
 * <pre> int8 *data = com_ScratchAlloc(100,0);  // reserve 800
 * bytes
 * </pre>
 *
 * @ingroup dmem_scratch
 * @param num_doubles
 *               Number of doubles (8-byte blocks) to reserve.
 * @param inst scratch memory area to allocate from
 *
 * @return Pointer to newly reserved area.
 *
 * @see scratch_free
 */
extern void *com_ScratchAlloc(uint32 num_doubles, int32 inst);
/**
 * Return a memory area to the scratch stack, use
 * com_ScratchFree(num_doubles,0) to avoid duplication, instead
 * of scratch_free(num_doubles). Example call:
 * <pre>
 * com_ScratchFree(100,0);                     // free 800 bytes
 * </pre>
 *
 * @ingroup dmem_scratch
 * @param num_doubles
 *               Number of 8 byte blocks to return.
 * @param inst scratch memory area to free from
 * @see scratch_alloc
 */
extern void com_ScratchFree(uint32 num_doubles, int32 inst);

/**
 * Check that the specified scratch memory "scratch_inst" can be freed
 */
extern void scratch_check(uint32 num_doubles, int scratch_inst);
/**
 * Read the current scratch pointer
 *  @param scratch_inst scratch memory area ptr (upper / lower) to return
 *  @return current scratch pointer
 */
extern void *com_ScratchGetCurrentPtr(int scratch_inst);


/*************************************************************************************************
 * Useful macro declarations
 ************************************************************************************************/

/* BIT macro define a mask for a bit a the _INDEX position in the word */
#define BIT(_INDEX) (1L<<(_INDEX))


#endif

/** @} END OF FILE */

