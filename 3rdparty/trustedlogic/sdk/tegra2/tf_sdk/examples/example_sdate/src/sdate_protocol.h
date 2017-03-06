/**
 * Copyright (c) 2008-2009 Trusted Logic S.A.
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
 
/* This example depends on the Advanced profile of the Trusted Foundations */

#ifndef __SDATE_PROTOCOL_H__
#define __SDATE_PROTOCOL_H__

/** Identifier of the cerstore example service. This identifier
    globally identifies the service. If you use this example as
    a basis to develop your service, please do not forget
    to change the service UUID */
#define SERVICE_SDATE_A_UUID  { 0x1161D06B, 0x2035, 0x4155, { 0xBC, 0x24, 0x20, 0xF8, 0xA5, 0xEE, 0xDF, 0x71 } }
#define SERVICE_SDATE_B_UUID  { 0x5EB638A8, 0xD9AF, 0x48f5, { 0xB7, 0x9F, 0xAF, 0xD4, 0x2E, 0xC1, 0x65, 0xA1 } }

#define CMD_SET_DATE                 0x00000001

#define CMD_GET_DATE                 0x00000002

#endif /* __SDATE_PROTOCOL_H__ */
