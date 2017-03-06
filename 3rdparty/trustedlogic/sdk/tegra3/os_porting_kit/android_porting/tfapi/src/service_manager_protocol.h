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

/*
 * File            : service_manager_protocol.h
 *
 * Original-Author : Trusted Logic S.A.
 *
 * Created         : October 13, 2004
 */


#ifndef __SERVICE_MANAGER_PROTOCOL_H__
#define __SERVICE_MANAGER_PROTOCOL_H__

/*---------------------------------------------------------------------------
 * Service Manager Command Identifiers
 *
 * @see your Product Reference Manual for a full specification of the commands
 *---------------------------------------------------------------------------*/

/**
 * The identifier of the service manager's Get All Services command.
 */
#define SERVICE_MANAGER_COMMAND_ID_GET_ALL_SERVICES  0x00000000

/**
 * The identifier of the service manager's Download Service command.
 */
#define SERVICE_MANAGER_COMMAND_ID_DOWNLOAD_SERVICE  0x00000001


/**
 * The identifier of the service manager's Remove Service command.
 */
#define SERVICE_MANAGER_COMMAND_ID_REMOVE_SERVICE  0x00000002


/**
 * The identifier of the service manager Get Property command.
 */
#define SERVICE_MANAGER_COMMAND_ID_GET_PROPERTY  0x00000003


/**
 * The identifier of the service manager Get All Properties command.
 */
#define SERVICE_MANAGER_COMMAND_ID_GET_ALL_PROPERTIES  0x00000004

/**
 * The UUID of the service manager
 *
 */

#define SERVICE_MANAGER_UUID { 0x61419a20, 0x33fe, 0x42ab, { 0x85, 0xfe, 0x88, 0xb0, 0xee, 0xfb, 0xa0, 0x04 } }

#endif  /* !defined(__SERVICE_MANAGER_PROTOCOL_H__) */
