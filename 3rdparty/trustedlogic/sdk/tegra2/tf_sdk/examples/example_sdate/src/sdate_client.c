/**
 * Copyright (c) 2008-2011 Trusted Logic S.A.
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

/* ----------------------------------------------------------------------------
*   Includes
* ---------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tee_client_api.h"
#include "sdate_protocol.h"

#if defined(__SYMBIAN32__)
extern void symbianPrintf(const char* format, ...);
extern void symbianFprintf(FILE *stream, const char* format, ...);
extern void symbianPutc(int c, FILE *stream);
#define printf symbianPrintf
#define fprintf symbianFprintf
#define putc symbianPutc
#endif


/**
 *    Structure representing the date and time
 **/
typedef struct STRUCT_CALENDAR
{
    uint32_t nYear;
    uint32_t nMonth;  /* from 1 to 12 */
    uint32_t nDay;    /* from 1 to 31 */
    uint32_t nHour;   /* from 0 to 23 */
    uint32_t nMinute; /* from 0 to 59 */
    uint32_t nSecond; /* from 0 to 59 */
} STRUCT_CALENDAR;

/****************************************************************************
 ****************************************************************************
 **
 ** In this example, the client is very simple and includes the implementation
 ** of the "stub", but more complex services may decide to put the functions
 ** in a separate static or dynamic library.
 **
 ****************************************************************************
 ****************************************************************************/

/*---------------------------------------------------------------------------
                      Connection-related functions
  ---------------------------------------------------------------------------*/

/**
*  Function sDateInitialize:
*  Description:
*           Creates a TEEC context.
*  Output : S_HANDLE*     phDevice     = points to the TEEC context handle
*
* A Device Context represents a connection to the Secure World but not
* to any particular service in the secure world.
**/
static TEEC_Result sDateInitialize(OUT S_HANDLE* phDevice)
{
   TEEC_Result    nError;
   TEEC_Context* pContext;

   *phDevice = S_HANDLE_NULL;
   pContext = (TEEC_Context *)malloc(sizeof(TEEC_Context));
   if (pContext == NULL)
   {
      return TEEC_ERROR_BAD_PARAMETERS;
   }
   memset(pContext, 0, sizeof(TEEC_Context));

   /* Create context  */
   nError = TEEC_InitializeContext(NULL, pContext);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: TEEC_InitializeContext failed [0x%08x].\n", nError);
      free(pContext);
   }
   else
   {
      *phDevice = (S_HANDLE)pContext;
   }
   return nError;
}


/**
 *  Function sDateOpenSession:
 *  Description:
 *           Opens a client session with the service.
 *  Input :  S_HANDLE      hDevice      = the device handle
 *           TEEC_UUID*       pServiceUUID = the identifier of the service
 *  Output:  S_HANDLE*     phSession    = points to the session handle
 *
 * A "Client Session" is a connection between the client (this application)
 * and the service. When the client opens a session, it can specify
 * which login method it wants to use.
 * In this example the login type will be set to PUBLIC, as no special rights are requested
 * when manipulating the date of a service.
 **/
static TEEC_Result sDateOpenSession(
                                 IN    S_HANDLE       hDevice,
                                 IN    const TEEC_UUID*  pServiceUUID,
                                 OUT   S_HANDLE*      phSession)
{
   TEEC_Operation sOperation;
   TEEC_Result    nError;
   TEEC_Session*  pSession;
   TEEC_Context*  pContext = (TEEC_Context *)hDevice;

   uint32_t       nLoginType = TEEC_LOGIN_PUBLIC;

   pSession = (TEEC_Session*)malloc(sizeof(TEEC_Session));
   if (pSession == NULL)
   {
      return TEEC_ERROR_OUT_OF_MEMORY;
   }

   memset(pSession, 0, sizeof(TEEC_Session));

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

   nError = TEEC_OpenSession(pContext,
                             pSession,                   /* OUT session */
                             pServiceUUID,               /* destination UUID */
                             nLoginType,                 /* connectionMethod */
                             NULL,                       /* connectionData */
                             &sOperation,                /* IN OUT operation */
                             NULL                        /* OUT returnOrigin, optional */
                             );
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: Could not open session [0x%08x].\n", nError);
      free(pSession);
      return nError;
   }
   else
   {
      *phSession = (S_HANDLE)pSession;
   }
   return TEEC_SUCCESS;
}

/**
 *  Function sDateCloseSession:
 *  Description:
 *           Closes the client session.
 *  Input :  S_HANDLE hSession = session handle
 **/
static TEEC_Result sDateCloseSession(IN S_HANDLE hSession)
{
   TEEC_Session*  pSession = (TEEC_Session *)hSession;

   if(pSession == NULL)
   {
      fprintf(stderr, "Error: Invalid session handle.\n");
      return TEEC_ERROR_BAD_PARAMETERS;
   }

   TEEC_CloseSession(pSession);
   free(pSession);
   return TEEC_SUCCESS;
}


/**
 *
 *  Function sDateFinalize:
 *  Description:
 *           Finalize: delete the TEEC context.
 *  Input :  S_HANDLE      hDevice     = the TEEC context handle
 *
 * Note that a Device Context can be deleted only when all
 * sessions have been closed in the device context.
 **/
static TEEC_Result sDateFinalize(IN S_HANDLE hDevice)
{
   TEEC_Context* pContext = (TEEC_Context *)hDevice;

   if (pContext == NULL)
   {
      fprintf(stderr, "Error: Handle invalid.\n");
      return TEEC_ERROR_BAD_PARAMETERS;
   }

   TEEC_FinalizeContext(pContext);
   free(pContext);
   return TEEC_SUCCESS;
}


/*---------------------------------------------------------------------------
   Each function corresponds to a command exchanged with the service.
  ---------------------------------------------------------------------------*/

/**
 *  Function clientGetDate:
 *  Description:
 *           Returns the date of a service.
 *  Input :  S_HANDLE        hSession  = session handle
 *
 *  Output:  STRUCT_CALENDAR* pCalendar = the date and time of the service.
 *
 **/
static TEEC_Result clientGetDate(IN S_HANDLE hSession,
                              OUT STRUCT_CALENDAR* pCalendar)
{
   TEEC_Result    nError;
   TEEC_Operation sOperation;
   TEEC_Session*  pSession = (TEEC_Session *)hSession;

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_VALUE_OUTPUT, TEEC_VALUE_OUTPUT, TEEC_NONE);
   nError = TEEC_InvokeCommand(pSession,
                               CMD_GET_DATE,
                               &sOperation,       /* IN OUT operation */
                               NULL               /* OUT returnOrigin, optional */
                              );
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "   Error: Could not invoke command [0x%08x].\n", nError);
      return nError;
   }

   /* Decode time and date */
   pCalendar->nYear   = sOperation.params[0].value.a;
   pCalendar->nMonth  = sOperation.params[0].value.b;
   pCalendar->nDay    = sOperation.params[1].value.a;
   pCalendar->nHour   = sOperation.params[1].value.b;
   pCalendar->nMinute = sOperation.params[2].value.a;
   pCalendar->nSecond = sOperation.params[2].value.b;

   printf("   Service Date: %d/%d/%d - %d:%d:%d\n",pCalendar->nYear, pCalendar->nMonth, pCalendar->nDay,
                   pCalendar->nHour, pCalendar->nMinute, pCalendar->nSecond);

   return nError;
}

/**
 *  Function clientSetDate:
 *  Description:
 *           Changes the date of a service.
 *  Input :  S_HANDLE        hSession  = session handle
 *           STRUCT_CALENDAR* pCalendar = the future date of the service.
 *
 **/
static TEEC_Result clientSetDate(IN S_HANDLE hSession,
                              IN STRUCT_CALENDAR* pCalendar)
{
   TEEC_Result    nError;
   TEEC_Operation sOperation;
   TEEC_Session*  pSession = (TEEC_Session *)hSession;

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_NONE);
   sOperation.params[0].value.a = pCalendar->nYear;
   sOperation.params[0].value.b = pCalendar->nMonth;
   sOperation.params[1].value.a = pCalendar->nDay;
   sOperation.params[1].value.b = pCalendar->nHour;
   sOperation.params[2].value.a = pCalendar->nMinute;
   sOperation.params[2].value.b = pCalendar->nSecond;

   nError = TEEC_InvokeCommand(pSession,
                               CMD_SET_DATE,
                               &sOperation,       /* IN OUT operation */
                               NULL               /* OUT returnOrigin, optional */
                              );
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "   Error: Could not invoke command [0x%08x].\n", nError);
      return nError;
   }

   printf("   Date was changed to %d/%d/%d - %d:%d:%d \n", pCalendar->nYear,pCalendar->nMonth,pCalendar->nDay,
   pCalendar->nHour, pCalendar->nMinute, pCalendar->nSecond);

   return TEEC_SUCCESS;
}


/**
 *  Function main:
 *  Description:
 *     Main entry point of the application
 **/
#if defined(__SYMBIAN32__)
int __main__(void)
#else
int main(void)
#endif
{
   TEEC_Result    nError;
   S_HANDLE       hSession_A;
   S_HANDLE       hSession_B;

   S_HANDLE          hDevice = S_HANDLE_NULL;
   STRUCT_CALENDAR   sCalendar;
   STRUCT_CALENDAR   sCalendarToSet;

   static const TEEC_UUID SERVICE_A_UUID = SERVICE_SDATE_A_UUID;
   static const TEEC_UUID SERVICE_B_UUID = SERVICE_SDATE_B_UUID;

   printf( "\nTrusted Logic SDATE Example Application\n");

   /* Initialize */
   printf("Initialize device.\n");
   nError = sDateInitialize(&hDevice);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: SDateInitialize() failed [0x%08x].\n", nError);
      return 1;
   }

   /* Open a session with the first service */
   printf("Open a session with SERVICE_A\n");
   nError = sDateOpenSession(hDevice, &SERVICE_A_UUID, &hSession_A);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: sDateOpenSession() failed [0x%08x].\n", nError);
      return 1;
   }

   /* Get the current date of the first service */
   nError = clientGetDate(hSession_A, &sCalendar);
   if(nError != TEEC_SUCCESS)
   {
      return 1;
   }

   /* Change the date on the first service */
   sCalendarToSet.nYear = 1999;
   sCalendarToSet.nMonth = 1;
   sCalendarToSet.nDay = 30;
   sCalendarToSet.nHour = 11;
   sCalendarToSet.nMinute = 12;
   sCalendarToSet.nSecond = 13;
   nError = clientSetDate(hSession_A, &sCalendarToSet);
   if(nError != TEEC_SUCCESS)
   {
      return 1;
   }

   nError = clientGetDate(hSession_A,&sCalendar);
   if(nError != TEEC_SUCCESS)
   {
      return 1;
   }

   /* Get the current date of the second service */
   printf("Open a session with SERVICE_B\n");
   nError = sDateOpenSession(hDevice, &SERVICE_B_UUID, &hSession_B);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: sDateOpenSession() failed [0x%08x].\n", nError);
      return 1;
   }

   nError = clientGetDate(hSession_B, &sCalendar);
   if(nError != TEEC_SUCCESS)
   {
      return 1;
   }


   /* Close the session with the first service */
   printf("Close session with SERVICE_A\n");
   nError = sDateCloseSession(hSession_A);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: sDateCloseSession() failed [0x%08x].\n", nError);
      return 1;
   }

   /* Close the session with the second service */
   printf("Close session with SERVICE_B\n");
   nError = sDateCloseSession(hSession_B);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: sDateCloseSession() failed [0x%08x].\n", nError);
      return 1;
   }

   /* Close the device context */
   nError = sDateFinalize(hDevice);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: sDateFinalize() failed [0x%08x].\n", nError);
      return 1;
   }

   printf("SDate example was successfully performed.\n");
   return 0;
}
