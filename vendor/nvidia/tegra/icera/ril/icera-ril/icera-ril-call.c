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
#include <telephony/ril.h>

#include <assert.h>

#include "atchannel.h"
#include "at_tok.h"
#include "icera-ril.h"
#include "icera-ril-call.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>

#include "version.h"
#include <cutils/properties.h>
/* declaration of static functions */
static void countHeldAndWaitingCalls(int *held_count_ptr, int *waiting_count_ptr);
static int callFromCLCCLine(char *line, RIL_Call *p_call);
static int clccStateToRILState(int state, RIL_CallState *p_state);

/* Record of the last error cause */
int last_voice_call_error_cause = 0;

/* State of the audioloopback mode */
int AudioLoopback = 0;

static void startAudioLoopback(void)
{
    ALOGD("Starting audioloop");
    AudioLoopback = 1;
    at_send_command("AT%IAUDCNF=9", NULL);
}
static void terminateAudioLoopback(void)
{
    ALOGD("Terminating audioloop");
    AudioLoopback = 0;
    at_send_command("AT%IAUDCTL=0",NULL);
    at_send_command("AT%IAUDCNF=0", NULL);
}

void ResetLastCallErrorCause(void)
{
    last_voice_call_error_cause = 0;
}
/* These commands will allow the user to alter the modem behaviour from the UI
 * **icera**<cmd>
 *
 * These can be authorised in sim less/network less mode by adding the number to ril.ecclist prop,
 * for example in system.prop in the device/<target> folder
 * Be aware that if the radio is off, the framework will try to turn it on
 *
 */
#define icera_escape_prefix "**42372**"
char * DialEscape[][2]=
{
/* Do not change the order of first sequences as
 * the index is used for additionnal operations
 * (beside the actual AT command)
 */
    {"1","AT"},   /* Dump RIL version in radio logcat (otherwise it's only visible at startup)*/
    {"2","AT"}, /* Start a call with the audioloop */

/* From here the order can be altered */
    {"3","AT%IDBGTEST=1"}, /* Force modem assert */
    {"4","AT%IDMPCFG=globalConfig,2"},    /* Dump config files to logs. */
    {"5","AT%MODE=2"},    /* Force mode change to Factory test */
/*
 * Carefull: The 2 following commands can conflict with RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE
 */
    {"6","AT%INWMODE=0,UG,1"},    /* Disable LTE band */
    {"7","AT%INWMODE=0,EUG,1"},   /* Reenable LTE band */
};

void checkAndAmendEmergencyNumbers(void)
{
    int err;
    unsigned int cmdId = 0;
    char * line, *cmd, *number;
    int index;
    char ecclist[PROPERTY_VALUE_MAX]={0};
    ATResponse *p_response = NULL;
    ATLine *p_cur;

    ALOGI("checkAndAmendEmergencyNumbers");

    /*
     * The following mecanism is to avoid endlessly expending
     * the ecc list every time the ril crashes
     */
    if(property_get("ril.ecclist.backup", ecclist, NULL)==0)
    {
        /* Check (in the same order as the framework) */
        if(property_get("ril.ecclist", ecclist, NULL) == 0)
        {
            /* Put the same default as those used by the framework in case no property exists */
            property_get("ro.ril.ecclist", ecclist, "112,911");
        }
        property_set("ril.ecclist.backup",ecclist);
    }

    /* obtain the number list from the modem */
    err = at_send_command("AT+CPBS=\"EN\"", &p_response);
    if (err != 0 || p_response->success == 0)
    {
        goto error;
    }
    at_response_free(p_response);

    err = at_send_command_singleline("AT+CPBR=?","+CPBR:", &p_response);
    if (err != 0 || p_response->success == 0)
    {
        goto error;
    }

    line = p_response->p_intermediates->line;
    line = strstr(line, "(1-");
    if(line == NULL)
    {
        goto error;
    }

    index = atoi(&line[3]);
    at_response_free(p_response);

    asprintf(&cmd,"AT+CPBR=1,%d",index);
    err = at_send_command_multiline (cmd, "+CPBR:", &p_response);
    free(cmd);

    if (err == 0 && p_response->success != 0)
    {
        for (p_cur = p_response->p_intermediates; p_cur != NULL;
         p_cur = p_cur->p_next)
        {
            char *line = p_cur->line;
            int cid;

            err = at_tok_start(&line);
            if (err < 0)
                goto error;

            err = at_tok_nextint(&line, &index);
            if (err < 0)
                goto error;

            /* Actual number */
            err = at_tok_nextstr(&line, &number);
            /* Check that old list + new number + comma + termination fits */
            if((strlen(ecclist) + strlen(number)+ 2)< PROPERTY_VALUE_MAX)
            {
                sprintf(ecclist,"%s,%s",ecclist,number);
            }
        }
    }
    error:
    at_response_free(p_response);

#ifdef ESCAPE_IN_ECCLIST
    /*
     * We'll add as many maintenance string as possible,
     * Knowing that the property value is limited to 92 chars
     */
    while((strlen(ecclist)+sizeof(icera_escape_prefix)+5  < PROPERTY_VALUE_MAX)
        &&(cmdId < sizeof(DialEscape)/(2*sizeof(char*))))
    {
        sprintf(ecclist,"%s,%s%s",ecclist,icera_escape_prefix,DialEscape[cmdId][0]);
        cmdId++;
    }
#endif

    err = property_set("ril.ecclist",ecclist);

    if(err==0)
    {
        ALOGD("Emergency call list updated");
    }
}

void requestGetCurrentCalls(void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response = NULL;
    ATLine *p_cur;
    int countCalls;
    int countValidCalls;
    RIL_Call *p_calls;
    RIL_Call **pp_calls;
    int i;
    int needRepoll = 0;

    err = at_send_command_multiline ("AT+CLCC", "+CLCC:", &p_response);

    if (err != 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        at_response_free(p_response);
        return;
    }

    /* count the calls */
    for (countCalls = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next
    ) {
        countCalls++;
    }

    ALOGD("requestGetCurrentCalls: %d calls\n", countCalls);

    /* yes, there's an array of pointers and then an array of structures */

    pp_calls = (RIL_Call **)alloca(countCalls * sizeof(RIL_Call *));
    p_calls = (RIL_Call *)alloca(countCalls * sizeof(RIL_Call));
    memset (p_calls, 0, countCalls * sizeof(RIL_Call));

    /* init the pointer array */
    for(i = 0; i < countCalls ; i++) {
        pp_calls[i] = &(p_calls[i]);
    }

    for (countValidCalls = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next
    ) {
        err = callFromCLCCLine(p_cur->line, p_calls + countValidCalls);

        if (err != 0) {
            continue;
        }

        if (p_calls[countValidCalls].state != RIL_CALL_ACTIVE
            && p_calls[countValidCalls].state != RIL_CALL_HOLDING
        ) {
            needRepoll = 1;
        }
        countValidCalls++;
    }

    ALOGD("requestGetCurrentCalls: %d valid calls\n", countValidCalls);

    /* If there is no current calls in audioloopback,
     * simulate the progress of an outgoing call
     */
    if((AudioLoopback != 0) && (countValidCalls == 0))
    {
        ALOGD("Loopback mode, simulating call setup");
        pp_calls = (RIL_Call **)alloca(sizeof(RIL_Call *));
        p_calls = (RIL_Call *)alloca(sizeof(RIL_Call));
        pp_calls[0] = &(p_calls[0]);
        memset (p_calls, 0, sizeof(RIL_Call));
        p_calls->index = 1;
        p_calls->isMT = 0;  // outgoing call
        p_calls->state = 0; // call active
        p_calls->isVoice = 1;
        p_calls->isMpty = 0;
        p_calls->number= NULL;
        p_calls->toa = 129;

        countValidCalls=1;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, pp_calls,
            countValidCalls * sizeof (RIL_Call *));
    at_response_free(p_response);
    return;
}

void requestHangup(void *data, size_t datalen, RIL_Token t)
{
    int *p_line;
    int err;
    char *cmd;
    ATResponse *p_response = NULL;

    p_line = (int *)data;

    /* Take this clue to terminate a potential loopback call */
    if(AudioLoopback)
    {
        terminateAudioLoopback();
    }

    // 3GPP 22.030 6.5.5
    // "Releases a specific active call X"
    asprintf(&cmd, "AT+CHLD=1%d", p_line[0]);
    err = at_send_command(cmd, &p_response);
    free(cmd);

    if (err < 0 || p_response->success == 0) {
        last_voice_call_error_cause = CALL_FAIL_ERROR_UNSPECIFIED;
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
    else
    {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}


/**
 * RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND
 *
 * Hang up waiting or held (like AT+CHLD=0)
*/
void requestHangupWaitingOrBackground(RIL_Token t)
{
    int waiting_count=0;
    int held_count=0;
    int   err = 0;
    ATResponse *p_response = NULL;

    /* Take this clue to terminate a potential loopback call */
    if(AudioLoopback)
    {
        terminateAudioLoopback();
    }

    /* count number of held/waiting calls */
    countHeldAndWaitingCalls(&held_count, &waiting_count);

    ALOGD("requestHangupWaitingOrBackground %d held calls, %d waiting calls",held_count,waiting_count );

    if ( !held_count && !waiting_count)
    {
        // there are no held/waiting calls, use ATH
        err = at_send_command("ATH", &p_response );
    }
    else
    {
        // 3GPP 22.030 6.5.5
        // "Releases all held calls or sets User Determined User Busy
        //    (UDUB) for a waiting call."
        err = at_send_command("AT+CHLD=0", &p_response);
    }

    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}




void requestHangupForegroundResumeBackground(RIL_Token t)
{
    int waiting_count=0;
    int held_count=0;
    int   err = 0;
    ATResponse *p_response = NULL;

    /* Take this clue to terminate a potential loopback call */
    if(AudioLoopback)
    {
        terminateAudioLoopback();
    }

    /* count number of held/waiting calls */
    countHeldAndWaitingCalls(&held_count, &waiting_count);
    ALOGD("requestHangupForegroundResumeBackground %d held calls, %d waiting calls",held_count,waiting_count );

    if ( !held_count && !waiting_count)
    {
        // there are no held/waiting calls, use ATH
        err = at_send_command("ATH", &p_response );
    }
    else
    {
        // 3GPP 22.030 6.5.5
        // "Releases all active calls (if any exist) and accepts
        //    the other (held or waiting) call."
        err = at_send_command("AT+CHLD=1", &p_response);
    }

    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}

void requestSwitchWaitingOrHoldingAndActive(RIL_Token t)
{
    char * cmd;
    int   err = 0;
    ATResponse *p_response = NULL;
    // 3GPP 22.030 6.5.5
    // "Places all active calls (if any exist) on hold and accepts
    //    the other (held or waiting) call."
    err = at_send_command("AT+CHLD=2", &p_response);

    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}



/**
 * RIL_REQUEST_CONFERENCE
 *
 * Conference holding and active (like AT+CHLD=3)
*/
void requestConference(void *data, size_t datalen, RIL_Token t)
{
    // 3GPP 22.030 6.5.5
    // "Adds a held call to the conversation"
    ATResponse *p_response = NULL;
    int err;
    err = at_send_command("AT+CHLD=3", &p_response);

    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}

/**
 * RIL_REQUEST_EXPLICIT_CALL_TRANSFER
 *
 * Connects the two calls and disconnects the subscriber from both calls.
 *
 * "data" is NULL
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */
void requestExplicitCallTransfer(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;

    /* Explicit Call Transfer (connect the active and
     * held party and disconnect from both parties) */
    err = at_send_command("AT+CHLD=4", &p_response);
    if (err < 0 || p_response->success == 0)
    {
        ALOGE("ERROR: requestExplicitCallTransfer() failed\n");
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
    else
    {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}

/**
 * RIL_REQUEST_UDUB
 *
 * Send UDUB (user determined used busy) to ringing or
 * waiting call answer (RIL_BasicRequest r).
*/
void requestUDUB(void *data, size_t datalen, RIL_Token t)
{
    /* user determined user busy */
    /* sometimes used: ATH */
    at_send_command("ATH", NULL);

    /* success or failure is ignored by the upper layer here.
            it will call GET_CURRENT_CALLS and determine success that way */
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;
}

/**
 * RIL_REQUEST_SET_MUTE
 *
 * Turn on or off uplink (microphone) mute.
 *
 * Will only be sent while voice call is active.
 * Will always be reset to "disable mute" when a new voice call is initiated.
*/
void requestSetMute(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int mute = ((int *) data)[0];
    int err = 0;
    char *cmd = NULL;

    assert(mute == 0 || mute == 1);

    asprintf(&cmd, "AT+CMUT=%d", mute);
    err = at_send_command(cmd, &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0)
    {
        ALOGE("ERROR: %s failed\n", __FUNCTION__);
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
    else
    {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}

/**
 * RIL_REQUEST_GET_MUTE
 *
 * Queries the current state of the uplink mute setting.
*/
void requestGetMute(void *data, size_t datalen, RIL_Token t)
{
    char *line = NULL;
    int err = 0;
    int response = 0;
    ATResponse *p_response = NULL;

    err = at_send_command_singleline("AT+CMUT?", "+CMUT:", &p_response);
    if (err < 0 || p_response->success == 0)
        goto error;

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &response);
    if (err < 0)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));
    at_response_free(p_response);
    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR: requestGetMute() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * RIL_REQUEST_LAST_CALL_FAIL_CAUSE
 *
 * Requests the failure cause code for the most recently terminated call.
*
 * See also: RIL_REQUEST_LAST_PDP_FAIL_CAUSE
 */
void requestLastCallFailCause(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int errno = CALL_FAIL_NORMAL;


    /*
     * If we already know why the call failed
     * Just return that cause
     */
    if(last_voice_call_error_cause != 0)
    {
        errno = last_voice_call_error_cause;
    }
    else
    {
        /*
         * Else, find out the network cause from the modem
         */

        int err = 0;
        int nwErrRegistration = 0;
        char *line = NULL;

        err = at_send_command_singleline("AT%IVCER?", "%IVCER:", &p_response);

        /* check if p_response is NULL */
        if(err < 0 || p_response->success == 0)
            goto error;

        line = p_response->p_intermediates->line;

        err = at_tok_start(&line);
        if(err < 0)
            goto error;

        err = at_tok_nextint(&line, &nwErrRegistration);
        if(err < 0)
            goto error;

        err = at_tok_nextint(&line, &errno);
        if(err < 0)
            goto error;

        switch(errno)
        {
            /* Supported  by the framework, return as is */
#if RIL_VERSION >= 4
            /* the following symbol was introduced in version 4 */
            case CALL_FAIL_UNOBTAINABLE_NUMBER:
#endif
            case CALL_FAIL_NORMAL:
            case CALL_FAIL_BUSY:
            case CALL_FAIL_CONGESTION:
            case CALL_FAIL_ACM_LIMIT_EXCEEDED:
            case CALL_FAIL_CALL_BARRED:
            case CALL_FAIL_IMSI_UNKNOWN_IN_VLR:
            case CALL_FAIL_IMEI_NOT_ACCEPTED:
                break;

            /*
             * Modem will return 0 if no error is encountered.
             * return a success result in that case.
             */
            case 0:
                errno = CALL_FAIL_NORMAL;
                break;

            default:
                /* Error code is not one supported by the framework
                 * Check for registration issues first and then return generic error
                 */
                if (nwErrRegistration == CALL_FAIL_IMSI_UNKNOWN_IN_VLR)
                    errno = CALL_FAIL_IMSI_UNKNOWN_IN_VLR;
                else if (nwErrRegistration == CALL_FAIL_IMEI_NOT_ACCEPTED)
                    errno = CALL_FAIL_IMEI_NOT_ACCEPTED;
                else
                    errno = CALL_FAIL_ERROR_UNSPECIFIED;
                break;
        }
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &errno, sizeof(int));
    at_response_free(p_response);

    return;

 error:
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    return;
}

/**
 * RIL_REQUEST_SEPARATE_CONNECTION
 *
 * Separate a party from a multiparty call placing the multiparty call
 * (less the specified party) on hold and leaving the specified party
 * as the only other member of the current (active) call
 *
 * Like AT+CHLD=2x
 *
 * See TS 22.084 1.3.8.2 (iii)
 * TS 22.030 6.5.5 "Entering "2X followed by send"
 * TS 27.007 "AT+CHLD=2x"
*/

void requestSeparateConnection(void *data, size_t datalen, RIL_Token t)
{
    char  cmd[12];
    int   party = ((int*)data)[0];
    int   err = 0;
    ATResponse *p_response = NULL;

    // Make sure that party is in a valid range.
    // (Note: The Telephony middle layer imposes a range of 1 to 7.
    // It's sufficient for us to just make sure it's single digit.)
    if (party > 0 && party < 10) {
        sprintf(cmd, "AT+CHLD=2%d", party);
        err = at_send_command(cmd, &p_response);
        if (err < 0 || p_response->success == 0)
        {
            RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        }
        else
        {
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
        }
        at_response_free(p_response);
        return;
    }
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * RIL_REQUEST_DIAL
 *
 * Initiate voice call.
*/
void requestDial(void *data, size_t datalen, RIL_Token t)
{
    RIL_Dial *p_dial;
    char *cmd = NULL;
    const char *clir;
    int err;
    ATResponse *p_response = NULL;

    /* reset the last failure cause as this is a new attempt */
    ResetLastCallErrorCause();

    p_dial = (RIL_Dial *)data;

    switch (p_dial->clir) {
        case 1: clir = "I"; break;    /*invocation*/
        case 2: clir = "i"; break;    /*suppression*/
        default:
        case 0: clir = ""; break;    /*subscription default*/
    }

    if(strncmp(p_dial->address,icera_escape_prefix, sizeof(icera_escape_prefix)-1)==0)
    {
        unsigned int cmdId=0;
        ALOGD("Escape call command detected...");

        while (cmdId < sizeof(DialEscape)/(2*sizeof(char*)))
        {
            if(strcmp(p_dial->address+sizeof(icera_escape_prefix)-1,DialEscape[cmdId][0]) == 0)
            {
                asprintf(&cmd, "%s", DialEscape[cmdId][1]);

                /* Special case for commands that require more than just an AT command */
                switch(cmdId)
                {
                    case 0: /* ril version dump */
                        ALOGD("Ril version:\nVersion: \"%s\"", RIL_VERSION_DETAILS);
                        ALOGD("%s", RIL_MODIFIED_FILES);
                        break;
                    case 1: /*Audio loopback on */
                        startAudioLoopback();
                        break;
                    default:
                        break;
                }

                break;
            }
            cmdId++;
        }
    }

    if(cmd == NULL)
    {
        asprintf(&cmd, "ATD%s%s;", p_dial->address, clir);

        err = at_send_command(cmd, &p_response);

        /*
         * We need to check if there is an non network originated
         * error code and store it so it can be used later
         */
        if (err < 0 || p_response->success == 0)
        {
            /* There was a problem */
            last_voice_call_error_cause = CALL_FAIL_ERROR_UNSPECIFIED;

            if((p_response!=NULL) &&
               (at_get_cme_error(p_response) == CME_ERROR_SS_FDN_FAILURE))
            {
                last_voice_call_error_cause= CALL_FAIL_FDN_BLOCKED;
            }
        }
    }
    else
    {
        /* We're not interrested by the response to the escape
         * commands
         */
        err = at_send_command(cmd, &p_response);
    }

    free(cmd);

    /* Result is supposed to be ignored, but for consistency's sake */
    if (err < 0 || p_response->success == 0)
    {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
    else
    {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}


/**
 * RIL_REQUEST_ANSWER
 *
 * Answer incoming call.
 *
 * Will not be called for WAITING calls.
 * RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE will be used in this case
 * instead.
*/
void requestAnswer(RIL_Token t)
{
    ATResponse *p_response = NULL;

    int err = at_send_command("ATA", &p_response);

    if (err < 0 || p_response->success == 0) {
        last_voice_call_error_cause = CALL_FAIL_ERROR_UNSPECIFIED;
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
    else
    {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);

}

/**
 * RIL_REQUEST_DTMF
 *
 * Send a DTMF tone
 *
 * If the implementation is currently playing a tone requested via
 * RIL_REQUEST_DTMF_START, that tone should be cancelled and the new tone
 * should be played instead.
*/
void requestDTMF(void *data, size_t datalen, RIL_Token t)
{
    char c = ((char *) data)[0];
    char *cmd = NULL;
    ATResponse *p_response = NULL;
    int err = 0;

    /* Set duration to default (manufacturer specific, 70ms in our case). */
    err = at_send_command("AT+VTD=0", &p_response);
    if (err < 0 || p_response->success == 0) goto error;

    at_response_free(p_response);
    p_response = NULL;

    /* DTMF tone. */
    asprintf(&cmd, "AT+VTS=%c", (int) c);
    err = at_send_command(cmd, &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0) goto error;

    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR: requestDTMF() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * RIL_REQUEST_DTMF_START
 *
 * Start playing a DTMF tone. Continue playing DTMF tone until
 * RIL_REQUEST_DTMF_STOP is received .
 *
 * If a RIL_REQUEST_DTMF_START is received while a tone is currently playing,
 * it should cancel the previous tone and play the new one.
 *
 * See also: RIL_REQUEST_DTMF, RIL_REQUEST_DTMF_STOP.
 */
void requestDTMFStart(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    char c = ((char *) data)[0];
    char *cmd = NULL;
    int err = 0;

    /* Set duration to 50  n/10 ms = 5s. */
    //err = at_send_command("AT+VTD=100", &p_response);

    /* As a workaround for bgz056097, set duration to default (manufacturer specific, 70ms in our case).  */
    err = at_send_command("AT+VTD=0", &p_response);
    if (err < 0 || p_response->success == 0) goto error;

    at_response_free(p_response);
    p_response = NULL;

    /* Start the DTMF tone. */
    asprintf(&cmd, "AT+VTS=%c", (int) c);
    err = at_send_command(cmd, &p_response);
    free(cmd);
    if (err < 0 || p_response->success == 0) goto error;

    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR: requestDTMFStart() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * RIL_REQUEST_DTMF_STOP
 *
 * Stop playing a currently playing DTMF tone.
 *
 * See also: RIL_REQUEST_DTMF, RIL_REQUEST_DTMF_START.
 */
void requestDTMFStop(void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    ATResponse *p_response = NULL;

    /* Stop the DTMF tone, set duration to default (70ms) */
    err = at_send_command("AT+VTD=0", &p_response);
    if (err < 0 || p_response->success == 0) goto error;

    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR: requestDTMFStop() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

static void countHeldAndWaitingCalls(int *held_count_ptr, int *waiting_count_ptr)
{
    int err;
    int countCalls = 0;
    ATResponse *p_response=NULL;
    ATLine *p_cur;

    assert( held_count_ptr != NULL );
    assert( waiting_count_ptr != NULL );

    *held_count_ptr = 0;
    *waiting_count_ptr = 0;

    err = at_send_command_multiline ("AT+CLCC", "+CLCC:", &p_response);

    if (err != 0 || p_response->success == 0) {
        goto error;
    }

    /* count the waiting/held calls */
    for (countCalls = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next
    ) {
        char *line = p_cur->line;
        int flag;

        /* expected line: +CLCC:<id>,<dir>,<stat>,<mode>,...
           where stat=0: active, 1:held, 2:dialing, 3:alerting, 4:incoming, 5:waiting
        */

        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        /* skip id */
        err = at_tok_nextint(&line, &flag);
        if (err < 0)
            goto error;

        /* skip dir */
        err = at_tok_nextint(&line, &flag);
        if (err < 0)
            goto error;

        /* get stat */
        err = at_tok_nextint(&line, &flag);
        if (err < 0)
            goto error;

        if (flag==1)
        {
            /* held call */
            *held_count_ptr=*held_count_ptr+1;
        }

        if (flag==5)
        {
            /* waiting call */
            *waiting_count_ptr=*waiting_count_ptr+1;
        }
    }
error:
    at_response_free(p_response);
}


void requestSetCallForward(void *data, RIL_Token t)
{
    int err = 0;
    char *cmd = NULL;
    RIL_CallForwardInfo *info = NULL;
    ATResponse *p_response=NULL;

    info = ((RIL_CallForwardInfo *) data);

    if(data == NULL) goto error;

    if (info->number == NULL) {
        asprintf(&cmd, "AT+CCFC=%d,%d",
                 info->reason, info->status);
    } else {
        if (info->serviceClass!=0)
        {
            asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",%d,%d",
                 info->reason, info->status, info->number, info->toa, info->serviceClass);
        }
        else
        {
            /* from ril.h: 0 means user doesn't input class */
            asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",%d",
                 info->reason, info->status, info->number, info->toa);
        }
    }

    err = at_send_command(cmd, &p_response);

    if (err < 0 || p_response->success == 0) goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    free(cmd);
    at_response_free(p_response);

    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    free(cmd);
    at_response_free(p_response);
}

/**
 * Note: directly modified line and has *p_call point directly into
 * modified line
 */
static int callFromCLCCLine(char *line, RIL_Call *p_call)
{
        //+CLCC: 1,0,2,0,0,\"+18005551212\",145
        //     index,isMT,state,mode,isMpty(,number,TOA)?

    int err;
    int state;
    int mode;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(p_call->index));
    if (err < 0) goto error;

    err = at_tok_nextbool(&line, &(p_call->isMT));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &state);
    if (err < 0) goto error;

    err = clccStateToRILState(state, &(p_call->state));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &mode);
    if (err < 0) goto error;

    p_call->isVoice = (mode == 0);

    err = at_tok_nextbool(&line, &(p_call->isMpty));
    if (err < 0) goto error;

    if (at_tok_hasmore(&line)) {
        err = at_tok_nextstr(&line, &(p_call->number));

        /* tolerate null here */
        if (err < 0) return 0;

        // Some lame implementations return strings
        // like "NOT AVAILABLE" in the CLCC line
        if (p_call->number != NULL
            && 0 == strspn(p_call->number, "*+0123456789")
        ) {
            p_call->number = NULL;
        }

        err = at_tok_nextint(&line, &p_call->toa);
        if (err < 0) goto error;
    }

    p_call->uusInfo = NULL;

    return 0;

error:
    ALOGE("invalid CLCC line\n");
    return -1;
}

void requestQueryCallWaiting(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    char *cmd = NULL;
    char *line;
    int class;
    int response[2];

    class = ((int *)data)[0];

    asprintf(&cmd, "AT+CCWA=1,2,%d",class);

    err = at_send_command_singleline(cmd, "+CCWA:", &p_response);

    if (err < 0 || p_response->success == 0) goto error;

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(response[0]));
    if (err < 0) goto error;

    if (at_tok_hasmore(&line)) {
        err = at_tok_nextint(&line, &(response[1]));
        if (err < 0) goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
    at_response_free(p_response);
    free(cmd);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    free(cmd);
}

void requestSetCallWaiting(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    char *cmd = NULL;
    int enabled, class;

    if((datalen<2)||(data==NULL)) goto error;

    enabled = ((int *)data)[0];
    class = ((int *)data)[1];

    asprintf(&cmd, "AT+CCWA=0,%d,%d",enabled,class);

    err = at_send_command(cmd, NULL);

    if (err < 0 ) goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    free(cmd);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    free(cmd);
}


static int clccStateToRILState(int state, RIL_CallState *p_state)
{
    switch(state) {
        case 0: *p_state = RIL_CALL_ACTIVE;   return 0;
        case 1: *p_state = RIL_CALL_HOLDING;  return 0;
        case 2: *p_state = RIL_CALL_DIALING;  return 0;
        case 3: *p_state = RIL_CALL_ALERTING; return 0;
        case 4: *p_state = RIL_CALL_INCOMING; return 0;
        case 5: *p_state = RIL_CALL_WAITING;  return 0;
        default: return -1;
    }
}


void unsolicitedCallStateChange(const char *s)
{
    int result = 0;
    char *line = NULL;
    char *tok = NULL;
    int err;
    int state;
    int mode;
    RIL_Call call;

    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED,
                              NULL, 0);

    line = tok = strdup(s);

    err = at_tok_start(&tok);
    if (err < 0) goto error;

    err = at_tok_nextint(&tok, &(call.index));
    if (err < 0) goto error;

    err = at_tok_nextbool(&tok, &(call.isMT));
    if (err < 0) goto error;

    err = at_tok_nextint(&tok, &state);
    if (err < 0) goto error;

    err = clccStateToRILState(state, &(call.state));
    if (err < 0) goto error;

    err = at_tok_nextint(&tok, &mode);
    if (err < 0) goto error;

    call.isVoice = (mode == 0);

    if (call.isVoice && (call.state == RIL_CALL_INCOMING)) {
        result = 1;
    }

error:
    free(line);
    RIL_onUnsolicitedResponse(RIL_UNSOL_RINGBACK_TONE,
                              &result, sizeof(result));
}
