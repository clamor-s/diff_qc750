/* Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved. */
/*
** Copyright 2010, Icera Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** You may not use this file except in compliance with the License.
** You may obtain a copy of the License at:
**
** http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** imitations under the License.
*/
/*
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <telephony/ril.h>
#include "atchannel.h"
#include "at_tok.h"
#include "icera-ril.h"
#include "icera-ril-utils.h"
#include "icera-ril-services.h"
#include "icera-ril-data.h"
#include "icera-util.h"

#include "icera-ril-net.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>

/**
 * RIL_REQUEST_CANCEL_USSD
 *
 * Cancel the current USSD session if one exists.
 */
void requestCancelUSSD(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;

    err = at_send_command("AT+CUSD=2", &p_response);

    if (err < 0 || p_response->success == 0) {
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);

    return;
}

/**
 * RIL_REQUEST_SEND_USSD
 *
 * Send a USSD message
 *
 * If a USSD session already exists, the message should be sent in the
 * context of that session. Otherwise, a new session should be created.
 *
 * The network reply should be reported via RIL_UNSOL_ON_USSD
 *
 * Only one USSD session may exist at a time, and the session is assumed
 * to exist until:
 *   a) The android system invokes RIL_REQUEST_CANCEL_USSD
 *   b) The implementation sends a RIL_UNSOL_ON_USSD with a type code
 *      of "0" (USSD-Notify/no further action) or "2" (session terminated)
 *
 * "data" is a const char * containing the USSD request in UTF-8 format
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  FDN_CHECK_FAILURE
 *  GENERIC_FAILURE
 *
 * See also: RIL_REQUEST_CANCEL_USSD, RIL_UNSOL_ON_USSD
 */
void requestSendUSSD(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    char *cmd = NULL;
    char *ussdhex = NULL;
    char *ussd = (char *)data;
    int len = strlen(ussd);

    ussdhex = (char *)alloca(len * 2 + 1);
    asciiToHex(ussd, ussdhex, len);

    asprintf(&cmd, "AT+CUSD=1,\"%s\",15", ussdhex);
    err = at_send_command_singleline(cmd,"+CUSD:", &p_response);
    free(cmd);

    if (err < 0) goto error;

    if((p_response->success == 0)
     &&(at_get_cme_error(p_response) == CME_ERROR_SS_FDN_FAILURE))
    {
        RIL_onRequestComplete(t, RIL_E_FDN_CHECK_FAILURE, NULL, 0);
        at_response_free(p_response);
        return;
    }
    else
    {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }

    /*
     * The response to the AT may indicate error but still have
     * a network response.
     *
     * Check if we have an answer with some content.
     */
    if(p_response->p_intermediates)
    {
        onUSSDReceived(p_response->p_intermediates->line);
    }
    else
    {
        /* If no content, simulate an (empty) answer */
        char * response[] = {"2"};
        ALOGD("Simulating Network USSD response");
        RIL_onUnsolicitedResponse(RIL_UNSOL_ON_USSD, response, sizeof(char *));
    }

    at_response_free(p_response);

    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR: requestSendUSSD() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


void requestQueryCLIP(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    char *line;
    int response;

    err = at_send_command_singleline("AT+CLIP?","+CLIP:",&p_response);
    if(err < 0 || p_response->success == 0) goto error;

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if(err < 0) goto error;

    /* The first number is discarded */
    err = at_tok_nextint(&line, &response);
    if(err < 0) goto error;

    err = at_tok_nextint(&line, &response);
    if(err < 0) goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));
    at_response_free(p_response);

    return;
error:
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


void requestGetCLIR(void *data, size_t datalen, RIL_Token t)
{
   /* Even though data and datalen must be NULL and 0 respectively this
      * condition is not verified
      */
    ATResponse *p_response = NULL;
    int response[2];
    char *line = NULL;
    int err = 0;

    err = at_send_command_singleline("AT+CLIR?", "+CLIR:", &p_response);

    if (err < 0 || p_response->success == 0) goto error;

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(response[0]));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(response[1]));
    if (err < 0) goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
    at_response_free(p_response);

    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

void requestSetCLIR(void *data, size_t datalen, RIL_Token t)
{
    char *cmd = NULL;
    int err = 0;
    ATResponse *p_response = NULL;

    asprintf(&cmd, "AT+CLIR=%d", ((int *)data)[0]);

    err = at_send_command(cmd, &p_response);
    free(cmd);

    if (err < 0 || p_response->success == 0)
      RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
    else
      RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}


/**
 * RIL_REQUEST_QUERY_CALL_FORWARD_STATUS
 */
void requestQueryCallForwardStatus(void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    int i = 0;
    int n = 0;
    int tmp = 0;
    char *strtmp;
    ATResponse *p_response = NULL;
    ATLine *p_cur;
    int reason = ((RIL_CallForwardInfo *) data)->reason;
    char *cmd;

    RIL_CallForwardInfo *p_cfresponse;
    RIL_CallForwardInfo **pp_cfresponse;

    asprintf(&cmd, "AT+CCFC=%d,2", reason);

    err = at_send_command_multiline(cmd, "+CCFC:", &p_response);

    free(cmd);

    if(err != 0 || p_response->success == 0)
      goto error;

    for (p_cur = p_response->p_intermediates; p_cur != NULL; p_cur = p_cur->p_next)
      n++;

    p_cfresponse = alloca(n*sizeof(RIL_CallForwardInfo));
    pp_cfresponse = alloca(n*sizeof(RIL_CallForwardInfo *));
    memset(p_cfresponse, 0, sizeof(RIL_CallForwardInfo) * n);

    for (i = 0; i < n; i++) {
        pp_cfresponse[i] = &(p_cfresponse[i]);
    }

    for (i = 0,p_cur = p_response->p_intermediates; p_cur != NULL; p_cur = p_cur->p_next, i++) {

      char *line = p_cur->line;
      err = at_tok_start(&line);
      if (err < 0) goto error;

      err = at_tok_nextint(&line, &p_cfresponse[i].status);
      if (err < 0) goto error;

      err = at_tok_nextint(&line, &p_cfresponse[i].serviceClass);
      if (err < 0) goto error;

      if(!at_tok_hasmore(&line)) continue;

      err = at_tok_nextstr(&line,&p_cfresponse[i].number);
      if (err < 0) goto error;

      if(!at_tok_hasmore(&line)) continue;

      err = at_tok_nextint(&line, &p_cfresponse[i].toa);
      if (err < 0) goto error;

      if(!at_tok_hasmore(&line)) continue;
      at_tok_nextstr(&line,&strtmp);

      if(!at_tok_hasmore(&line)) continue;
      at_tok_nextint(&line,&tmp);

      if(!at_tok_hasmore(&line)) continue;
      err = at_tok_nextint(&line, &p_cfresponse[i].timeSeconds);
      if (err < 0) goto error;

    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, pp_cfresponse, n*sizeof(RIL_CallForwardInfo *));

    /*pp_cfresponse contains a pointer to the location of the number in p_response.
      So free p_response only after sending the message to Android to avoid memory re-use.*/
    at_response_free(p_response);
    return;

error:
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


/**
 * RIL_REQUEST_SET_TTY_MODE
 *
 * Request to set the TTY mode
 *
 * "data" is int *
 * ((int *)data)[0] is == 0 for TTY off
 * ((int *)data)[0] is == 1 for TTY on
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
*/
void requestSetTtyMode(void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response = NULL;
    int ttyMode = ((int *) data)[0];

    if (ttyMode) {
        err = at_send_command("AT%ICTMSET=1,1,,", &p_response);
    }
    else {
        err = at_send_command("AT%ICTMSET=0,0,,", &p_response);
    }

    if (err < 0 || p_response->success == 0)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR: requestSetTtyMode() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * RIL_REQUEST_QUERY_TTY_MODE
 *
 * Request the setting of TTY mode
 *
 * "data" is NULL
 *
 * "response" is int *
 * ((int *)response)[0] is == 0 for TTY off
 * ((int *)response)[0] is == 1 for TTY on
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
*/
void requestQueryTtyMode(void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response = NULL;
    char *line = NULL;
    int response[1];

    err = at_send_command_singleline("AT%ICTMSET?", "%ICTMSET:", &p_response);

    if (err < 0 || p_response->success == 0)
        goto error;

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &response[0]);
    if (err < 0) goto error;

    if (response[0] == 0) {
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0) goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
    at_response_free(p_response);
    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR: requestQueryTtyMode() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


/**
 * RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION
 *
 * Enables/disables supplementary service related notifications
 * from the network.
 *
 * Notifications are reported via RIL_UNSOL_SUPP_SVC_NOTIFICATION.
 *
 * "data" is int *
 * ((int *)data)[0] is == 1 for notifications enabled
 * ((int *)data)[0] is == 0 for notifications disabled
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_UNSOL_SUPP_SVC_NOTIFICATION.
 */
void requestSetSuppSVCNotification(void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    int enabled = 0;
    char *cmd = NULL;
    enabled = ((int *)data)[0];
    ATResponse *p_response = NULL;

    assert(enabled == 0 || enabled == 1);

    asprintf(&cmd, "AT+CSSN=%d,%d", enabled, enabled);
    err = at_send_command(cmd, &p_response);
    free(cmd);

    if (err < 0 || p_response->success == 0) goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR: requestSetSuppSVCNotification() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


/**
 * RIL_REQUEST_SCREEN_STATE
 *
 * Indicates the current state of the screen.  When the screen is off, the
 * RIL should notify the baseband to suppress certain notifications (eg,
 * signal strength and changes in LAC/CID or BID/SID/NID/latitude/longitude)
 * in an effort to conserve power.  These notifications should resume when the
 * screen is on.
 *
 * "data" is int *
 * ((int *)data)[0] is == 1 for "Screen On"
 * ((int *)data)[0] is == 0 for "Screen Off"
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  GENERIC_FAILURE
 */
void requestScreenState(void *data, size_t datalen, RIL_Token t, int fast_dormancy_to)
{
    int err = 0, i = 0;
    ATResponse *p_response = NULL;
    char * cmd;
    int screenState = ((int*)data)[0];
    int LteSupported = GetLteSupported();
    int LocUpdateEnabled = GetLocupdateState();

    LogKernelTimeStamp();
    
    if (screenState == 1) {
        /* Screen is on - be sure to enable all unsolicited notifications again. */
        const char* ScreenOnCmdList[]={
                            "AT+CGEREP=1",
                            "AT+CMER=3,0,0,1",
                            "AT*TUNSOL=\"SQ\",1",
                            "AT*TUNSOL=\"SM\",1",
                            "AT+CSQ",
                            NULL
                            };
        while(ScreenOnCmdList[i] != NULL)
        {
            err = at_send_command(ScreenOnCmdList[i], &p_response);
            if (err < 0 || p_response->success == 0)
                goto error;
            i++;
            at_response_free(p_response);
        }

        if(LocUpdateEnabled)
        {
            at_send_command(LteSupported?"AT+CREG=2;+CGREG=2;+CEREG=2":"AT+CREG=2;+CGREG=2",NULL);
        }
        
        at_send_command(useIcti()?"AT%ICTI=1":"AT%NWSTATE=3",NULL);
        
        if(fast_dormancy_to > 0)
        {
            at_send_command("AT%IFD=0",NULL);
        }

        /* Specific case for this multiline response */
        err = at_send_command_multiline_no_prefix("AT%IDRVSTAT", &p_response);
        if (err < 0 || p_response->success == 0)
            goto error;
        at_response_free(p_response);

        /* Resend network state */
        RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                                   NULL, 0);
    } else if (screenState == 0) {
        /* Screen is off - disable all unsolicited notifications. */
        const char* ScreenOffCmdList[]={
                                        "AT+CMER=3,0,0,0",
                                        "AT*TUNSOL=\"SQ\",0",
                                        "AT*TUNSOL=\"SM\",0",
                                        NULL};
        while(ScreenOffCmdList[i] != NULL)
        {
            err = at_send_command(ScreenOffCmdList[i], &p_response);
            if (err < 0 || p_response->success == 0)
                goto error;
            i++;
            at_response_free(p_response);
        }

        /*
         * Still need to receive notification so we can reconnect, should network get lost 
         * No change if locupdate is not enabled. 
         */
        if(LocUpdateEnabled)
        {
            at_send_command(LteSupported?"AT+CREG=1;+CGREG=1;+CEREG=1":"AT+CREG=1;+CGREG=1",NULL);
        }

        at_send_command(useIcti()?"AT%ICTI=0":"AT%NWSTATE=0",NULL);

        if(fast_dormancy_to > 0)
        {
            asprintf(&cmd,"AT%%IFD=%d", fast_dormancy_to);
            at_send_command(cmd,NULL);
            free(cmd);
        }
    } else {
        /* Not a defined value - error. */
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);

    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR: requestScreenState() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * RIL_REQUEST_OEM_HOOK_RAW
 *
 * This request reserved for OEM-specific uses.
 * data[0] is used to differentiate several OEM specific requests
 *
 * Valid errors:
 *  All
 */

void requestOEMHookRaw(void *data, size_t datalen, RIL_Token t)
{
    int *loc_data;
    loc_data = (int *)data;

    ALOGD("requestOEMHookRaw: request %d\n", loc_data[0]);
    switch (loc_data[0])
    {
        case RIL_REQUEST_OEM_HOOK_SHUTDOWN_RADIO_POWER:
            requestShutdownRadioPower(&loc_data[1], datalen-sizeof(int), t);
            break;
        case RIL_REQUEST_OEM_HOOK_ACOUSTIC_PROFILE:
            requestAcousticProfileCmd(&loc_data[1], datalen-sizeof(int), t);
            break;
        case RIL_REQUEST_OEM_HOOK_EXPLICIT_ROUTE_BY_RIL:
            ConfigureExplicitRilRouting(loc_data[1]);
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            break;
    case RIL_REQUEST_OEM_HOOK_EXPLICIT_DATA_ABORT:
            /* Fall through to unsupported unless in test mode. */
            if(isRunningTestMode())
            {
                /* de-reference pointer to invalid address */
                *((unsigned int*)0xDEADBEEF) = 0;
            }
            else
            {
                goto error;
            }
            break;

        default:
            goto error;
            break;
    }
    return;

    error:
        ALOGE("ERROR: requestOEMHookRaw: unknown request %d\n", loc_data[0]);
        RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, loc_data, sizeof(int));
}

/**
 * RIL_REQUEST_OEM_HOOK_STRINGS
 *
 * This request reserved for OEM-specific uses. It passes strings
 * back and forth.
 *
 * It can be invoked on the Java side from
 * com.android.internal.telephony.Phone.invokeOemRilRequestStrings()
 *
 * "data" is a const char **, representing an array of null-terminated UTF-8
 * strings copied from the "String[] strings" argument to
 * invokeOemRilRequestStrings()
 *
 * "response" is a const char **, representing an array of null-terminated UTF-8
 * stings that will be returned via the caller's response message here:
 *
 * (String[])(((AsyncResult)response.obj).result)
 *
 * An error response here will result in
 * (((AsyncResult)response.obj).result) == null and
 * (((AsyncResult)response.obj).exception) being an instance of
 * com.android.internal.telephony.gsm.CommandException
 *
 * Valid errors:
 *  All
 */

void requestOEMHookStrings(void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    char * category = ((char **)data)[0];
    char *cmd = ((char **)data)[1];
    char *p_prefix = NULL;
    char **p_response_strings = NULL;
    ATResponse *p_response = NULL;
    int countLines=0;
    int count;
    ATLine *p_cur;

    if(strcmp(category, "singleline") == 0)
    {
        p_prefix = ((char **)data)[2];
        err = at_send_command_singleline(cmd, p_prefix, &p_response);
    }
    else if (strcmp(category, "no_result") == 0)
    {
        err = at_send_command(cmd, &p_response);
    }
    else if (strcmp(category, "multiline_no_prefix") == 0)
    {
        err = at_send_command_multiline_no_prefix(cmd, &p_response);
    }
    else if (strcmp(category, "numeric") == 0)
    {
        err = at_send_command_numeric(cmd, &p_response);
    }
    else if (strcmp(category, "multiline") == 0)
    {
        p_prefix = ((char **)data)[2];
        err = at_send_command_multiline(cmd, p_prefix, &p_response);
    }
    else if (strcmp(category, "tofile") == 0)
    {
        char * p_path = ((char **)data)[2];
        err = at_send_command_tofile(cmd, p_path);

        if(err < 0)
            goto error;
        /* No response to process, we can return immediately */
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
        return;
    }
    else
    {
        ALOGE("ERROR: requestOEMHookString: unknown category \"%s\"\n", category);
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    if (err < 0 || p_response->success == 0) goto error;

    /* count numbers of lines */
    for (countLines = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next
    ) {
        countLines++;
    }

    if (countLines > 0){
        p_response_strings = malloc(sizeof(char*) * countLines);

        p_cur = p_response->p_intermediates;
        for (count= 0; count < countLines; count++)
        {
            p_response_strings[count] = p_cur->line;
            p_cur = p_cur->p_next;
        }
        RIL_onRequestComplete(t, RIL_E_SUCCESS, &p_response_strings[0], sizeof(char*) * count);

        free(p_response_strings);

    }
    else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }

    at_response_free(p_response);
    return;

    error:
    if(p_response!=NULL)
    {
        at_response_free(p_response);
    }
    ALOGE("ERROR: requestOEMHookString() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    return;
}

/**
 * RIL_UNSOL_ON_USSD
 *
 * Called when a new USSD message is received.
 *
 * "data" is const char **
 * ((const char **)data)[0] points to a type code, which is
 *  one of these string values:
 *      "0"   USSD-Notify -- text in ((const char **)data)[1]
 *      "1"   USSD-Request -- text in ((const char **)data)[1]
 *      "2"   Session terminated by network
 *      "3"   other local client (eg, SIM Toolkit) has responded
 *      "4"   Operation not supported
 *      "5"   Network timeout
 *
 * The USSD session is assumed to persist if the type code is "1", otherwise
 * the current session (if any) is assumed to have terminated.
 *
 * ((const char **)data)[1] points to a message string if applicable, which
 * should always be in UTF-8.
 */
void onUSSDReceived(const char *s)
{
    char **response;
    char *p = NULL;
    char *line = NULL;
    char *hexstr = NULL;
    char *ucs2str = NULL;
    int err = -1;
    int i = 0;
    int n = 1;
    int len;

    line = strdup(s);
    p = line;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &i);
    if (err < 0) goto error;

    if (i < 0 || i > 5) goto error;

    response = alloca(2 * sizeof(char *));
    response[0] = alloca(2 * sizeof(char));
    sprintf(response[0], "%d", i);

    /* "0" or "1" USSD-Notify */
    if ((i == 0) || (i == 1)) {
        n = 2;

        err = at_tok_nextstr(&line, &hexstr);
        if (err < 0) goto error;

        /* encode Hex string to Char string */
        len = strlen(hexstr) / 2;
        ucs2str = (char *)alloca(len + 1);
        hexToAscii(hexstr, ucs2str, len);
        response[1] = ucs2str;

        if (at_tok_hasmore(&line)) {
            err = at_tok_nextint(&line, &i);
            if (err < 0) goto error;

              /* Coding Scheme is GSM alphabet */
            if(((i&0xF0) == 0) || ((i&0xF0) == 0x20) || ((i&0xF0) == 0x30)) {
                /* Worst case need to allocate 3 time the length (euro sign)*/
                response[1] = (char *)alloca(len * 3 + 1);
                GSMToUTF8(ucs2str, response[1], len);
            }
            /* Coding Scheme is UCS2 - encode UCS2 to UTF-8 */
            else if ((i & 0x0C) == 0x08) {
                len /= 2;
                response[1] = (char *)alloca(len * 3 + 1);
                UCS2ToUTF8(ucs2str, response[1], len);
            }
        }
    }

    free(p);
    RIL_onUnsolicitedResponse(RIL_UNSOL_ON_USSD, response, n * sizeof(char *));
    return;

error:
    free(p);
    ALOGE("Failed to parse +CUSD.");
}


/**
 * RIL_UNSOL_SUPP_SVC_NOTIFICATION
 *
 * Reports supplementary service related notification from the network.
 *
 * "data" is a const RIL_SuppSvcNotification *
 *
 */
void onSuppServiceNotification(const char *s, int type)
{
    #define CSSI_CUG_CALL_CODE  4  /* this is a CUG call (also <index> present) */
    #define CSSU_CUG_CALL_CODE  1  /* this is a CUG call (also <index> present) (MT call setup),
                                    * not implemented by ICERA modem */

    RIL_SuppSvcNotification ssnResponse;
    char *line = NULL;
    char *tok = NULL;
    int err;

    line = tok = strdup(s);

    memset(&ssnResponse, 0, sizeof(ssnResponse));
    ssnResponse.notificationType = type;

    err = at_tok_start(&tok);
    if (err < 0) goto error;

    err = at_tok_nextint(&tok, &ssnResponse.code);
    if (err < 0) goto error;

    if ((type == 0 && ssnResponse.code == CSSI_CUG_CALL_CODE) ||
        (type == 1 && ssnResponse.code == CSSU_CUG_CALL_CODE)) {
        err = at_tok_nextint(&tok, &ssnResponse.index);
        if (err < 0) goto error;
    }

    free(line);
    RIL_onUnsolicitedResponse(RIL_UNSOL_SUPP_SVC_NOTIFICATION,
                              &ssnResponse, sizeof(ssnResponse));
    return;

error:
    free(line);
    if (type == 0) {
        ALOGE("Failed to parse +CSSI.");
    }
    else {
        ALOGE("Failed to parse +CSSU.");
    }
}


/**
 * RIL_REQUEST_SHUTDOWN_RADIO_POWER
 *
 * Request the shutdown of the radio *
 * "data" is NULL
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  GENERIC_FAILURE
*/
void requestShutdownRadioPower(void *data, size_t datalen, RIL_Token t)
{
    int err;
    int response = 0;

    /* Disable error handling callbacks because shutdown of the modem can cause them to detect errors and reboot the modem*/
    at_set_on_reader_closed(NULL);
    at_set_on_timeout(NULL);

    /* Tells upper layer than system shutdown can continue. */
    setRadioState(RADIO_STATE_MODEM_OFF);

    response = RIL_REQUEST_OEM_HOOK_SHUTDOWN_RADIO_POWER;
    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));

    /* Send AT+CFUN=0 to make a "clean" shutdown of the modem. */
    err = at_send_command("AT+CFUN=0", NULL);
    if(err<0){ goto error;}

    /*Cut modem power (not necesssary to do it here because AP will cut all power when shutting down). */
    IceraModemPowerControl(MODEM_POWER_OFF, 0);

    return;

error:
    ALOGE("ERROR: requestShutdownRadioPower() failed\n");
    response = RIL_REQUEST_OEM_HOOK_SHUTDOWN_RADIO_POWER;
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, &response, sizeof(int));
}


/**
 * RIL_REQUEST_ACOUSTIC_PROFILE
 *
 * Request the shutdown of the radio *
 * "data" is NULL
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  GENERIC_FAILURE
 *  MODE_NOT_SUPPORTED
*/
void requestAcousticProfileCmd(void *data, size_t datalen, RIL_Token t)
{

    int err;
    int mode;
    int response = 0;
    extern int AudioLoopback;

    assert (datalen >= sizeof(int *));
    mode = ((int *)data)[0];

    ALOGI("requestAcousticProfileCmd: Routing path:%X.....\n",mode);

    /* Check if in AudioLoopback mode,
     * As setting another audio profile would cancel it.
     */
    if(AudioLoopback != 0)
        goto error1;

     switch(mode)
     {
        case 0x02: /* HEADSET */
            err = at_send_command("at%iaudcnf=1", NULL);
            if (err < 0){ goto error;}
            break;
        case 0x01: /* SPEAKER */
            err = at_send_command("at%iaudcnf=2",  NULL);
            if (err < 0){ goto error;}
            break;
        case 0x03:/* BLUETOOTH_SCO */
            err = at_send_command("at%iaudcnf=3",  NULL);
            if (err < 0){ goto error;}
            break;
        case 0x00: /* EARPIECE */
            err = at_send_command("at%iaudcnf=0",  NULL);
            if (err < 0){ goto error;}
            break;
        default:
            goto error1;
            break;
     }

     /* Success case */
     response = RIL_REQUEST_OEM_HOOK_ACOUSTIC_PROFILE;
     RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));
     return;

     /* Failure case */
     error:
         ALOGE("ERROR: requestAcousticProfileCmd() failed\n");
         response = RIL_REQUEST_OEM_HOOK_ACOUSTIC_PROFILE;
         RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, &response, sizeof(int));
         return;

     error1:
         ALOGE("ERROR: requestAcousticProfileCmd() failed\n");
         response = RIL_REQUEST_OEM_HOOK_ACOUSTIC_PROFILE;
         RIL_onRequestComplete(t, RIL_E_MODE_NOT_SUPPORTED, &response, sizeof(int));
         return;
}



