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

#ifndef __CERSTORE_PROTOCOL_H__
#define __CERSTORE_PROTOCOL_H__

/** Identifier of the cerstore example service. This identifier
    globally identifies the service. If you use this example as
    a basis to develop your service, please do not forget
    to change the service UUID */
/* {01B70C61-5EAF-4aa2-B399-80F351C8F0E0} */
#define SERVICE_CERTSTORE_UUID  { 0x01B70C61, 0x5EAF, 0x4aa2, { 0xB3, 0x99, 0x80, 0xF3, 0x51, 0xC8, 0xF0, 0xE0 } }


/**
*  Command CMD_INSTALL_CERT:
*    
*  PARAMS
*  [0] VALUE OUT : a = nObjectId
*  [1] BUFFER OUT : distinguished name
*  [2] BUFFER IN  : certificate
*  Command message:
*    - uint8_t[]: content of the certificate to install
*  Response message on success:
*    - uint32_t: persistent identifier of the certifcate
*    - String*: sequence of strings describing the certificate's Distinguished Name
**/
#define CMD_INSTALL_CERT       0x00000001


/**
 *  Command CMD_DELETE_CERT:
 *  PARAMS
 *  [0] VALUE IN : a = nObjectId
 *  Command message:
 *    - uint32_t: identifier of the certificate to delete
 *  Response message:
 *    none
 **/
#define CMD_DELETE_CERT        0x00000002
 

/**
 *  Command CMD_DELETE_ALL:
 *  Command message:
 *    none
 *  Response message:
 *    none
 **/
#define CMD_DELETE_ALL           0x00000003


/**
 *  Command CMD_LIST_ALL:
 *  PARAMS
 *  [0] BUFFER OUT : list of all nCertId
 *  Command message:
 *    none
 *  Response message on success:
 *    - uint32_t*: a sequence of all the certificate identifiers in the store
 **/
#define CMD_LIST_ALL             0x00000004


/**
 *  Command CMD_GET_CERT:
 *  [0] VALUE IN : a = nObjectId (certificate id) b = issuer certificate id
 *  [1] BUFFER OUT : certificate
 *  Command message:
 *   - uint32_t: identifier of the certificate to be retrieved
 *  Response message on success:
 *   - uint8_t[]: the content of the certificate
 **/
#define CMD_GET_CERT           0x00000005


/**
 *  Command CMD_READ_DN:
 *  Command message:
 *   - uint32_t: identifier of the certificate 
 *  Response message on success:
 *   - String*: a sequence of strings each describing a part of the certificate Distinguished Name
 **/
#define CMD_READ_DN              0x00000006

/**
 *  Command CMD_VERIFY_CERT:
 *  Command message:
 *   - uint32_t: identifier of the certificate 
 *   - uint32_t: identifier of the issuer certificate 
 *  Response message on success:
 *   - uint32_t: SM_SUCCESS if the verification of the certificate was successful
 *               or an error code otherwise
 **/
#define CMD_VERIFY_CERT              0x00000007

#endif /* __CERSTORE_PROTOCOL_H__ */
