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
*   Includes
* ---------------------------------------------------------------------------- */
#include "ssdi.h"
#include "sdate_protocol.h"


/* ----------------------------------------------------------------------------
*                     Constants or Global Data
* ---------------------------------------------------------------------------- */

/* 
 * The time origin of the service is set to 
 *      midnight on January 1, 1970  
 */
static const S_CALENDAR  sOrigin ={1970,1,1,1,1,0,0};


/* ----------------------------------------------------------------------------
*                    Main service functions
* ---------------------------------------------------------------------------- */

/** 
 *   Function serviceSetDate:
 *           Sets the date of a service to a date recieved from the client.
 *   Input:  SESSION_CONTEXT* pSessionContext = session context 
 *           S_HANDLE hDecoder = handle of a decoder containing the future date of the service
 *
 **/
static S_RESULT serviceSetDate(void* pSessionContext, uint32_t nParamTypes, S_PARAM pParams[4])
{
     
    S_CALENDAR  sDate;
    int32_t     nSeconds;
    S_RESULT    nError;
    
    S_VAR_NOT_USED(pSessionContext);

    if ( (S_PARAM_TYPE_GET(nParamTypes, 0) != S_PARAM_TYPE_VALUE_INPUT)
       ||(S_PARAM_TYPE_GET(nParamTypes, 1) != S_PARAM_TYPE_VALUE_INPUT)
       ||(S_PARAM_TYPE_GET(nParamTypes, 2) != S_PARAM_TYPE_VALUE_INPUT))
    {
      return S_ERROR_BAD_PARAMETERS;
    }
    /* Read the date elements from client*/
    sDate.nYear   = pParams[0].value.a;
    sDate.nMonth  = pParams[0].value.b;
    sDate.nDay    = pParams[1].value.a;
    sDate.nHour   = pParams[1].value.b;
    sDate.nMinute = pParams[2].value.a;
    sDate.nSecond = pParams[2].value.b;

    /* Convert from a calendar structure to seconds */
    nError = SDateConvertCalendarToSeconds(&sOrigin, &sDate, &nSeconds);
    if(nError != S_SUCCESS)
    {
        SLogError("Can not convert calendar to seconds (0x%80X).");
        return nError;
    }
    
    nError = SDateSet(nSeconds, 0);
    if(nError != S_SUCCESS)
    {
        SLogError("Can not set the service's date (0x%80X).");
        return nError;
    }
    
     SLogTrace("Date was changed to: %d/%d/%d - %d:%d:%d ",sDate.nYear, sDate.nMonth, sDate.nDay,
                                                      sDate.nHour, sDate.nMinute, sDate.nSecond);             
    
    return S_SUCCESS;
}


/** 
 *   Function serviceGetDate:
 *           Retrieve the current date of the service.
 *   Input:  SESSION_CONTEXT* pSessionContext = session context 
 *           S_HANDLE hEcoder = handle of an encoder containg the current date of the service.
 *
 **/
static S_RESULT serviceGetDate(void* pSessionContext, uint32_t nParamTypes, S_PARAM pParams[4])
{
    int32_t     nSeconds;
    uint32_t    nStatus;
    S_RESULT    nError;

    S_CALENDAR  sDate;
    
    S_VAR_NOT_USED(pSessionContext);

    if ( (S_PARAM_TYPE_GET(nParamTypes, 0) != S_PARAM_TYPE_VALUE_OUTPUT)
       ||(S_PARAM_TYPE_GET(nParamTypes, 1) != S_PARAM_TYPE_VALUE_OUTPUT)
       ||(S_PARAM_TYPE_GET(nParamTypes, 2) != S_PARAM_TYPE_VALUE_OUTPUT))
    {
      return S_ERROR_BAD_PARAMETERS;
    }

    /* Retrieve the current date */ 
set_date:
    nError = SDateGet(&nSeconds, &nStatus, 0); 
    if (nError == S_SUCCESS) 
    { 
        if (nStatus == S_DATE_STATUS_NOT_SET) 
        { 
            /* 
            * If the date has not been set, set the date reference 
            * with the current wall clock time. 
            */ 

            SLogWarning("Current date was not set.");
            nError = SDateSet(SClockGet(), 0); 
            if (nError != S_SUCCESS) 
            { 
                SLogError("Cannot synchronize date with local device time. ");
                return nError;
            }
            SLogWarning("Date was synchronized to local device time.");
            
            /* Try reading the date again */
            goto set_date;
        } 
        else if (nStatus == S_DATE_STATUS_NEEDS_RESET)
        { 
            /*
             *  Date may have been corrupted.
             */

            SLogWarning("Current date may have been corrupted. Date will be resynchronized.");         
            nError = SDateSet(SClockGet(), 0); 
            if (nError != S_SUCCESS) 
            { 
                SLogError("Cannot synchronize date with local device time. ");
                return nError;
            }
            
            /* Try reading the date again */
            goto set_date;

        } else 
        {    
            /* 
             * Date successfully retrieved 
             */ 
            SDateConvertSecondsToCalendar(nSeconds, &sOrigin, &sDate );

            /* encode the time and date */
            SLogTrace("Service Date : %d/%d/%d - %d:%d:%d ",sDate.nYear, sDate.nMonth, sDate.nDay, 
                                                    sDate.nHour, sDate.nMinute, sDate.nSecond);             
            pParams[0].value.a = sDate.nYear;
            pParams[0].value.b = sDate.nMonth;
            pParams[1].value.a = sDate.nDay;
            pParams[1].value.b = sDate.nHour;
            pParams[2].value.a = sDate.nMinute;
            pParams[2].value.b = sDate.nSecond;
        }
    }
    return  S_SUCCESS;
}


/* ----------------------------------------------------------------------------
*                     Service Entry Points
* ---------------------------------------------------------------------------- */

/**
 *  Function SRVXCreate:
 *  Description:
 *        The function SSPICreate is the service constructor, which the system
 *        calls when it creates a new instance of the service.
 *        Here this function implements nothing.
 **/
S_RESULT SRVX_EXPORT SRVXCreate(void)
{
   SLogTrace("SRVXCreate");

   return S_SUCCESS;
}

/**
 *  Function SRVXDestroy:
 *  Description:
 *        The function SSPIDestroy is the service destructor, which the system
 *        calls when the instance is being destroyed.
 *        Here this function implements nothing.
 **/
void SRVX_EXPORT SRVXDestroy(void)
{
   SLogTrace("SRVXDestroy");

   return;
}

/**
 *  Function SRVXOpenClientSession:
 *  Description:
 *        The system calls the function SSPIOpenClientSession when a new client
 *        connects to the service instance.
 *        Here this function implements nothing. 
 **/
S_RESULT SRVX_EXPORT SRVXOpenClientSession(
   uint32_t nParamTypes,
   IN OUT S_PARAM pParams[4],
   OUT void** ppSessionContext )
{
   S_VAR_NOT_USED(nParamTypes);
   S_VAR_NOT_USED(pParams);
   S_VAR_NOT_USED(ppSessionContext);

   SLogTrace("SRVXOpenClientSession");
   return S_SUCCESS;
}

/**
 *  Function SRVXCloseClientSession:
 *  Description:
 *        The system calls this function to indicate that a session is being closed. 
 **/
void SRVX_EXPORT SRVXCloseClientSession(IN OUT void* pSessionContext)
{
   S_VAR_NOT_USED(pSessionContext);

   SLogTrace("SRVXCloseClientSession");
}

/**
 *  Function SRVXInvokeCommand:
 *  Description:
 *        The system calls this function when the client invokes a command within
 *        a session of the instance.
 *        Here this function perfoms a switch on the command Id received as input,
 *        and returns the main function matching the command id.
 *
 **/
S_RESULT SRVX_EXPORT SRVXInvokeCommand(IN OUT void* pSessionContext,
                                       uint32_t nCommandID,
                                       uint32_t nParamTypes,
                                       IN OUT S_PARAM pParams[4])
{
    
   switch(nCommandID)
   {
   case CMD_SET_DATE:
       return serviceSetDate(pSessionContext, nParamTypes, pParams);
   case CMD_GET_DATE:
       return serviceGetDate(pSessionContext, nParamTypes, pParams);
   default:
      SLogError("invalid command ID: 0x%X", nCommandID);
      return S_ERROR_BAD_PARAMETERS;
   }
}
