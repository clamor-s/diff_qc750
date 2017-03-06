/*------------------------------------------------------------------------
 *
 * Hybrid Compatibility Header
 * -----------------------------------------
 *
 * (C) 1994-2004 Hybrid Graphics, Ltd.
 * All Rights Reserved.
 *
 * This file consists of unpublished, proprietary source code of
 * Hybrid Graphics, and is considered Confidential Information for
 * purposes of non-disclosure agreement. Disclosure outside the terms
 * outlined in signed agreement may result in irrepairable harm to
 * Hybrid Graphics and legal action against the party in breach.
 *
 * Description:         Compatibility/Portability Header for C and C++
 *                                  applications.
 *
 * $Id: //hybrid/libs/hg/main/hgDefs.c#4 $
 * $Date: 2007/03/16 $
 * $Author: antti $
 *----------------------------------------------------------------------*/

#include "hgdefs.h"

#if (HG_COMPILER == HG_COMPILER_RVCT)
#   include "nvos.h"
#   define MEMCPY(source1, source2, size) NvOsMemcpy(source1, source2, size)
#   define MEMSET(source, value, size) NvOsMemset(source, value, size)
#   define MEMMOVE(destination, source, size) NvOsMemmove(destination, source, size)
#else
#   include <string.h>
#   define MEMCPY(source1, source2, size) memcpy(source1, source2, size)
#   define MEMSET(source, value, size) memset(source, value, size)
#   define MEMMOVE(destination, source, size) memmove(destination, source, size)
#endif

/* Used for stringifying version numbers: */ 
#define xstr(s) str(s) 
#define str(s) #s 

/*-------------------------------------------------------------------*//*!
 * \brief       Return HG's version number as a C string
 *//*-------------------------------------------------------------------*/
const char* hgGetVersion(void) 
{ 
        const char* const s = 
                xstr(HG_VERSION_MAJOR) "." 
                xstr(HG_VERSION_MINOR) "." 
                xstr(HG_VERSION_PATCH); 
        return s; 
}

/*-------------------------------------------------------------------*//*!
 * \brief       Platform specific implementations of hgAssertFail
 *//*-------------------------------------------------------------------*/
#if (HG_COMPILER == HG_COMPILER_MSEC)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
HG_EXTERN_C void hgAssertFail (const char* expr, const char* file, int line, const char* message)
{
    HG_UNREF(expr);
    HG_UNREF(file);
    HG_UNREF(line);
    HG_UNREF(message);

    DebugBreak();
}
#endif

/*-------------------------------------------------------------------*//*!
 * \brief       Tests rvct idiv
 *//*-------------------------------------------------------------------*/

int hgTestIDiv(int a, int b)
{
        return a/b;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Test rvct imod
 *//*-------------------------------------------------------------------*/

int hgTestIMod(int a, int b)
{
        return a%b;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Test rvct memclear
 *//*-------------------------------------------------------------------*/

void hgTestRtMemclr(unsigned char* dst, int size)
{
        MEMSET(dst, 0, size);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Test rvct memclear
 *//*-------------------------------------------------------------------*/

void hgTestRtMemclrW(unsigned int* dst, int size)
{
        MEMSET(dst, 0, size);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Test rvct memcopy
 *//*-------------------------------------------------------------------*/

void hgTestRtMemcpy(unsigned char* dst, unsigned char* src, int size)
{
        MEMCPY(dst, src, size);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Test rvct memcopy
 *//*-------------------------------------------------------------------*/

void hgTestRtMemcpyW(unsigned char* dst, unsigned char* src, int size)
{
        MEMCPY(dst, src, size);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Test rvct memmove
 *//*-------------------------------------------------------------------*/

void hgTestRtMemmove(unsigned char* dst, unsigned char* src, int size)
{
        MEMMOVE(dst, src, size);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Test rvct memcopy
 *//*-------------------------------------------------------------------*/

void hgTestAeabiMemcpy4(unsigned char* dst, unsigned char* src, int size)
{
        MEMCPY(dst, src, size);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Test rvct memcopy
 *//*-------------------------------------------------------------------*/

void hgTestAeabiMemcpy(unsigned char* dst, unsigned char* src, int size)
{
        MEMCPY(dst, src, size);
}
