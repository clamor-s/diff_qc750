/*
 * Copyright (c) 2011 Trusted Logic S.A.
 * All Rights Reserved.
 *
 * This software is the confidential and proprietary information of
 * Trusted Logic S.A. ("Confidential Information"). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered
 * into with Trusted Logic S.A.
 *
 * TRUSTED LOGIC S.A. MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. TRUSTED LOGIC S.A. SHALL
 * NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

#ifndef __LIB_BEF_INTERNAL_H__
#define __LIB_BEF_INTERNAL_H__

#include "s_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NDEBUG
#define MD_ASSERT(test)

#else

/*
 * Notifies a failed assertion. This function never returns.
 *
 * mdNotifyFailedAssert should first print out an error message describing the
 * failed assertion and its origin, then abort the current thread.
 *
 * The MD API expects mdNotifyFailedAssert to be implemented by some other
 * module if and only if {MD_DEBUG_ACTIVE} is defined.
 */
extern void mdNotifyFailedAssert(
      const char* pFileName,
      uint32_t nLine,
      const char* pExpression);

#define MD_ASSERT(test)                                     \
   do                                                       \
   {                                                        \
      if (!(test))                                          \
      {                                                     \
         mdNotifyFailedAssert(__FILE__, __LINE__, #test);   \
      }                                                     \
   }                                                        \
   while (0)

#endif /* NDEBUG */

/**
 * Base types (tag's high nibble)
 **/
#ifndef INCLUDE_MINIMAL_BEF
#define LIB_BEF_BASE_TYPE_BOOLEAN     0x00
#endif /* INCLUDE_MINIMAL_BEF */
#define LIB_BEF_BASE_TYPE_UINT8         0x10
#ifndef INCLUDE_MINIMAL_BEF
#define LIB_BEF_BASE_TYPE_UINT16        0x20
#endif /* INCLUDE_MINIMAL_BEF */
#define LIB_BEF_BASE_TYPE_UINT32        0x30
#define LIB_BEF_BASE_TYPE_STRING        0x70
#define LIB_BEF_BASE_TYPE_SEQUENCE      0x90
#ifndef INCLUDE_MINIMAL_BEF
#define LIB_BEF_BASE_TYPE_HANDLE        0xA0
#endif /* INCLUDE_MINIMAL_BEF */
/* Composite is UUID or MemRef */
#define LIB_BEF_BASE_TYPE_COMPOSITE     0xB0

/** 
 * Flags (tag's low nibble)
 **/
#define LIB_BEF_FLAG_VALUE_EXPLICIT          0x06
#define LIB_BEF_FLAG_NULL_VALUE              0x07
#define LIB_BEF_FLAG_NULL_ARRAY              0x08
#define LIB_BEF_FLAG_NON_NULL_ARRAY          0x0F

/**
 * Full tags for some types
 **/
#define LIB_BEF_TAG_FALSE            0x00
#define LIB_BEF_TAG_TRUE             0x01
#define LIB_BEF_TAG_SEQUENCE         0x96
#define LIB_BEF_TAG_COMPOSITE        0xB6

#define LIB_BEF_SUBTAG_MEM_REF       0x01
#define LIB_BEF_SUBTAG_UUID          0x02

#define LIB_BEF_TYPE_BASE_MASK      0xF0
#define LIB_BEF_FLAG_MASK           0x0F

/**
 * Returns the base type of the specified item type, i.e. one of the
 * <tt>LIB_BEF_BASE_TYPE_*</tt> constants.
 */
#define LIB_BEF_GET_BASE_TYPE(nItemType)  \
   ( (nItemType) & LIB_BEF_TYPE_BASE_MASK )

/**
 * Returns the flag of the specified item type, i.e. one of the
 * <tt>LIB_BEF_TYPE_FLAG*</tt> constants.
 */
#define LIB_BEF_GET_FLAG(nItemType)  \
   ( (nItemType) & LIB_BEF_FLAG_MASK )

#ifdef __cplusplus
}
#endif

#endif /* !defined(__LIB_BEF_INTERNAL_H__) */
