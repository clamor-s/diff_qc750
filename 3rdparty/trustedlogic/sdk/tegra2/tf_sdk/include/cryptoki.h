/**
* Copyright (c) 2006-2008 Trusted Logic S.A.
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
#ifndef __CRYPTOKI_H__
#define __CRYPTOKI_H__

#include "s_type.h"

/* Define CRYPTOKI_EXPORTS during the build of cryptoki libraries. Do
 * not define it in applications.
 */
#ifdef CRYPTOKI_EXPORTS
#define PKCS11_EXPORT S_DLL_EXPORT
#else
#define PKCS11_EXPORT S_DLL_IMPORT
#endif

#define CKV_TOKEN_SYSTEM                           0x00000001
#define CKV_TOKEN_SYSTEM_SHARED                    0x00000000
#define CKV_TOKEN_USER                             0x00004004
#define CKV_TOKEN_USER_SHARED                      0x00004012

#define CKV_TOKEN_SYSTEM_GROUP(gid)                (0x00010000 | (gid))
#define CKV_TOKEN_USER_GROUP(gid)                  (0x00020000 | (gid))

#include "pkcs11.h"

#endif /* __CRYPTOKI_H__ */
