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

/* ----------------------------------------------------------------------------
*   Constants or Global Data
* ---------------------------------------------------------------------------- */

/** Service Identifier **/
/* {410574B4-199E-447a-AC91-5706952877C5} */
#define SERVICE_CRYPTOCATALOGUE_UUID  { 0x410574B4, 0x199E, 0x447a, { 0xAC, 0x91, 0x57, 0x06, 0x95, 0x28, 0x77, 0xC5 } }


/** Service-specific Command identifiers **/
#define CMD_CRYPTOCATALOGUE       0x00000001
/**
 *  Command CMD_CRYPTOCATALOGUE: 
 *  The Client sends this command to the service.
 *  The Service performs a series of cryptographic operations to demonstrate the usage
 *  of the Internal Cryptographic 
 *        
 **/
