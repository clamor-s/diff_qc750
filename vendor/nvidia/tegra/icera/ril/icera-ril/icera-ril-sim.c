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


#include <telephony/ril.h>
#include <assert.h>
#include <stdio.h>
#include "atchannel.h"
#include "at_tok.h"
#include <icera-ril.h>
#include <icera-ril-sim.h>

#define LOG_TAG "RIL"
#include <utils/Log.h>

#define CRSM_CHUNK_SIZE 255
#define READ_BINARY 176

static int getCardStatus(RIL_CardStatus **pp_card_status);
static void freeCardStatus(RIL_CardStatus *p_card_status);

typedef struct pinRetries_{
    int pin1Retries;
    int puk1Retries;
    int pin2Retries;
    int puk2Retries;
} pinRetriesStruct;

static int getNumOfRetries(pinRetriesStruct * pinRetries);

void pollSIMState(void *param)
{
    /*
     * Do not change the system state because of a sim event
     * if the current state is RADIO_STATE_OFF
     */
    if(currentState() != RADIO_STATE_OFF)
    {
        switch(getSIMStatus()) {
            case SIM_ABSENT:
            case SIM_PIN:
            case SIM_PUK:
            case SIM_NETWORK_PERSONALIZATION:
            default:
                setRadioState(RADIO_STATE_SIM_LOCKED_OR_ABSENT);
            return;

            case SIM_NOT_READY:
            case SIM_ERROR:
                /* Error on SIM - reset the state before restarting to poll.*/
                setRadioState(RADIO_STATE_SIM_NOT_READY);
            return;

            case SIM_READY:
                setRadioState(RADIO_STATE_SIM_READY);
            return;
        }
    }
}

/**
 * RIL_REQUEST_GET_SIM_STATUS
 *
 * Requests status of the SIM interface and the SIM card.
 *
 * Valid errors:
 *  Must never fail.
 */
void requestGetSimStatus(void *data, size_t datalen, RIL_Token t)
{
    RIL_CardStatus *p_card_status;
    char *p_buffer;
    int buffer_size;
    int result = getCardStatus(&p_card_status);

    if (result == RIL_E_SUCCESS) {
        p_buffer = (char *)p_card_status;
        buffer_size = sizeof(*p_card_status);
    } else {
        p_buffer = NULL;
        buffer_size = 0;
    }

    RIL_onRequestComplete(t, result, p_buffer, buffer_size);
    freeCardStatus(p_card_status);
    return;
}


/**
 * RIL_REQUEST_SIM_IO
 *
 * Request SIM I/O operation.
 * This is similar to the TS 27.007 "restricted SIM" operation
 * where it assumes all of the EF selection will be done by the
 * callee.
 *
 * "data" is a const RIL_SIM_IO *
 * Please note that RIL_SIM_IO has a "PIN2" field which may be NULL,
 * or may specify a PIN2 for operations that require a PIN2 (eg
 * updating FDN records)
 *
 * "response" is a const RIL_SIM_IO_Response *
 *
 * Arguments and responses that are unused for certain
 * values of "command" should be ignored or set to NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *  SIM_PIN2
 *  SIM_PUK2
 */
void requestSIM_IO(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    RIL_SIM_IO_Response sr;
    int err;
    int chunks_num = 1;
    char *cmd = NULL;
    RIL_SIM_IO *p_args;
    char *line;
    int p1, p2, p3;
    int i;

    memset(&sr, 0, sizeof(sr));
    sr.simResponse = (char *)malloc(1);
    if (!sr.simResponse) goto error;
    *sr.simResponse = 0; // empty string

    p_args = (RIL_SIM_IO *)data;

    if (p_args->pin2 != NULL) {
        asprintf(&cmd, "AT+CPWD=\"P2\",\"%s\",\"%s\"", p_args->pin2, p_args->pin2);

        err = at_send_command(cmd, &p_response);
        free(cmd);

        if (err < 0 || p_response->success == 0) goto error;

        at_response_free(p_response);
        p_response = NULL;
    }

    // The condition for READ_BINARY allow to read huge files on SIM
    p1 = p_args->p1;
    p2 = p_args->p2;
    p3 = p_args->p3;

    if (p_args->command == READ_BINARY) {
        chunks_num = ((p3 - 1) / CRSM_CHUNK_SIZE) + 1;
    }

    for (i = 0; i < chunks_num; i++) {
        if (p_args->data == NULL) {
            // The condition for READ_BINARY allow to read huge files on SIM
            if (p_args->command == READ_BINARY) {
                p1 = ((i * CRSM_CHUNK_SIZE + p_args->p2) >> 8) + p_args->p1;
                p2 = (i * CRSM_CHUNK_SIZE + p_args->p2) & 0xFF;
                p3 = (i == chunks_num - 1 ? (p_args->p3 - 1) % CRSM_CHUNK_SIZE + 1 : CRSM_CHUNK_SIZE);
            }

            asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d,,\"%s\"",
                        p_args->command, p_args->fileid,
                        p1, p2, p3,p_args->path);
        } else {
            asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d,\"%s\",\"%s\"",
                        p_args->command, p_args->fileid,
                        p1, p2, p3, p_args->data,p_args->path);
        }

        err = at_send_command_singleline(cmd, "+CRSM:", &p_response);
        free(cmd);

        if (err < 0 || p_response->success == 0) goto error;



        line = p_response->p_intermediates->line;

        err = at_tok_start(&line);
        if (err < 0) goto error;

        err = at_tok_nextint(&line, &(sr.sw1));
        if (err < 0) goto error;

        err = at_tok_nextint(&line, &(sr.sw2));
        if (err < 0) goto error;

        if (at_tok_hasmore(&line)) {
            char *str;

            err = at_tok_nextstr(&line, &str);
            if (err < 0) goto error;

            sr.simResponse = (char *)realloc(sr.simResponse, strlen(sr.simResponse) + strlen(str) + 1);
            if (!sr.simResponse) goto error;

            strcat(sr.simResponse, str);
        }
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &sr, sizeof(sr));
    at_response_free(p_response);
    free(sr.simResponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    free(sr.simResponse);
}


/**
 * RIL_REQUEST_ENTER_SIM_PIN
 *
 * Supplies SIM PIN. Only called if RIL_CardStatus has RIL_APPSTATE_PIN state
 *
 * "data" is const char **
 * ((const char **)data)[0] is PIN value
 *
 * "response" is int *
 * ((int *)response)[0] is the number of retries remaining, or -1 if unknown
 *
 * Valid errors:
 *
 * SUCCESS
 * RADIO_NOT_AVAILABLE (radio resetting)
 * GENERIC_FAILURE
 * PASSWORD_INCORRECT
 */
void requestEnterSimPin(void *data, size_t  datalen, RIL_Token  t, int request)
{
    ATResponse   *p_response = NULL;
    int           err;
    int           numberOfRetries = -1;
    char         *cmd = NULL;
    char         *cmd_secure = NULL;
    const char  **strings = (const char **)data;
    RIL_Errno     failure = RIL_E_GENERIC_FAILURE;
    pinRetriesStruct pinRetries;

#if RIL_VERSION >= 6
    /* Extra parameter AppId for RIL v6*/
    datalen -= sizeof(char*);
#endif

    switch(datalen / sizeof(char*))
    {
        case 1: /* Pin entry*/
            asprintf(&cmd, "AT+CPIN=\"%s\"", strings[0]);
            asprintf(&cmd_secure, "AT+CPIN=\"%s\"", "****");
            break;
        case 2: /* Puk entry */
            asprintf(&cmd, "AT+CPIN=\"%s\",\"%s\"", strings[0],strings[1]);
            asprintf(&cmd_secure, "AT+CPIN=\"%s\",\"%s\"", "****","****");

            break;
        default:
            goto end;
    }
    err = at_send_command_secure(cmd, cmd_secure, &p_response);
    free(cmd);
    free(cmd_secure);

    getNumOfRetries(&pinRetries);

    switch(request)
    {
        case RIL_REQUEST_ENTER_SIM_PIN:
            numberOfRetries = pinRetries.pin1Retries;
            break;
        case RIL_REQUEST_ENTER_SIM_PUK:
            numberOfRetries = pinRetries.puk1Retries;
            break;
        default:
            numberOfRetries = -1;
    }

    ALOGE("requestEnterSimPin: number of retries left: %d", numberOfRetries);

    if (err < 0)
        goto end;

    if (p_response->success == 0) {
        if ((at_get_cme_error(p_response) == CME_ERROR_INCORRECT_PASSWORD) ||
            (at_get_cme_error(p_response) == CME_ERROR_PUK_REQUIRED)) {
            failure = RIL_E_PASSWORD_INCORRECT;
        }
    } else {
        failure = RIL_E_SUCCESS;
        /* Notify that SIM is ready */
        setRadioState(RADIO_STATE_SIM_READY);
    }

end:
    RIL_onRequestComplete(t, failure, (void *)&numberOfRetries, sizeof(numberOfRetries));
    at_response_free(p_response);
}

/**
 * RIL_REQUEST_ENTER_SIM_PIN2
 *
 * Supplies SIM PIN. Only called if RIL_CardStatus has RIL_APPSTATE_PIN state
 *
 * "data" is const char **
 * ((const char **)data)[0] is PIN value
 *
 * "response" is int *
 * ((int *)response)[0] is the number of retries remaining, or -1 if unknown
 *
 * Valid errors:
 *
 * SUCCESS
 * RADIO_NOT_AVAILABLE (radio resetting)
 * GENERIC_FAILURE
 * PASSWORD_INCORRECT
 */
void requestEnterSimPin2(void *data, size_t  datalen, RIL_Token  t, int request)
{
    ATResponse   *p_response = NULL;
    int           err;
    int           numberOfRetries = -1;
    char         *cmd = NULL;
    char         *cmd_secure = NULL;
    const char  **strings = (const char **)data;
    RIL_Errno     failure = RIL_E_GENERIC_FAILURE;
    pinRetriesStruct pinRetries;

#if RIL_VERSION >= 6
    /* Extra parameter AppId for RIL v6*/
    datalen -= sizeof(char*);
#endif

    switch(datalen / sizeof(char*))
    {
        case 1: /* Pin2 entry: this will force pin2 check at sim level */
            asprintf(&cmd, "AT+CPWD=\"P2\",\"%s\",\"%s\"", strings[0], strings[0]);
            asprintf(&cmd_secure, "AT+CPWD=\"P2\",\"%s\",\"%s\"", "****", "****");
            break;
        case 2: /* Puk2 entry, need to first check that PUK2 is needed */
        {
            char * cpinResult, * line;

            err = at_send_command_singleline("AT+CPIN?", "+CPIN:", &p_response);
            if (err < 0 || p_response->success == 0) {
                goto end;
            }

            line = p_response->p_intermediates->line;

            err = at_tok_start(&line);

            if (err < 0) {
                goto end;
            }

            err = at_tok_nextstr(&line, &cpinResult);

            if(strcmp (cpinResult, "SIM PUK2")) {
                /*
                 * Cpin is not expecting PUK2,
                 * Only way to enter it is via SS
                 */
                asprintf(&cmd, "ATD**052*%s*%s*%s#", strings[0],strings[1],strings[1]);
                asprintf(&cmd_secure, "ATD**052*%s*%s*%s#", "****","****","****");
            }
            else
            {
                asprintf(&cmd, "AT+CPIN=\"%s\",\"%s\"", strings[0],strings[1]);
                asprintf(&cmd_secure, "AT+CPIN=\"%s\",\"%s\"", "****","****");
            }
            at_response_free(p_response);
            break;
        }
        default:
            goto end;
    }
    err = at_send_command_secure(cmd, cmd_secure, &p_response);
    free(cmd);
    free(cmd_secure);

    getNumOfRetries(&pinRetries);
    switch(request)
    {
        case RIL_REQUEST_ENTER_SIM_PIN2:
            numberOfRetries = pinRetries.pin2Retries;
            break;
        case RIL_REQUEST_ENTER_SIM_PUK2:
            numberOfRetries = pinRetries.puk2Retries;
            break;
        default:
            numberOfRetries = -1;
            break;
    }

    ALOGE("requestEnterSimPin2: number of retries left: %d", numberOfRetries);

    if (err < 0)
        goto end;

    if(p_response->success == 0) {
        if ((at_get_cme_error(p_response) == CME_ERROR_INCORRECT_PASSWORD) ||
            (at_get_cme_error(p_response) == CME_ERROR_PUK2_REQUIRED)) {
            failure = RIL_E_PASSWORD_INCORRECT;
        }
    } else {
        failure = RIL_E_SUCCESS;
        /* Notify that SIM is ready */
        setRadioState(RADIO_STATE_SIM_READY);
    }

end:
    RIL_onRequestComplete(t, failure, (void *)&numberOfRetries, sizeof(numberOfRetries));
    at_response_free(p_response);
}


/**
 * RIL_REQUEST_QUERY_FACILITY_LOCK
 *
 * Query the status of a facility lock state
 *
 * "data" is const char **
 * ((const char **)data)[0] is the facility string code from TS 27.007 7.4
 *                      (eg "AO" for BAOC, "SC" for SIM lock)
 * ((const char **)data)[1] is the password, or "" if not required
 * ((const char **)data)[2] is the TS 27.007 service class bit vector of
 *                           services to query
 *
 * "response" is an int *
 * ((const int *)response) 0 is the TS 27.007 service class bit vector of
 *                           services for which the specified barring facility
 *                           is active. "0" means "disabled for all"
 *
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 */
void requestQueryFacilityLock(void *data, size_t datalen, RIL_Token t)
{
    int err, rat, response;
    ATResponse *p_response = NULL;
    char *cmd = NULL;
    char *cmd_secure = NULL;
    char *line = NULL;
    char *facility_string = NULL;
    char *facility_password = NULL;
    char *facility_class = NULL;

    ALOGD("FACILITY");
    assert (datalen >= (3 * sizeof(char **)));

    facility_string   = ((char **)data)[0];
    facility_password = ((char **)data)[1];
    facility_class    = ((char **)data)[2];


    asprintf(&cmd, "AT+CLCK=\"%s\",2,\"%s\",%s", facility_string, facility_password, facility_class);
    asprintf(&cmd_secure, "AT+CLCK=\"%s\",2,\"%s\",%s", facility_string, "****", facility_class);
    err = at_send_command_singleline_secure(cmd, cmd_secure, "+CLCK:", &p_response);
    free(cmd);
    free(cmd_secure);

    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);

    if (err < 0) {
        goto error;
    }

    err = at_tok_nextint(&line, &response);

    if (err < 0) {
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));
    at_response_free(p_response);
    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR: requestQueryFacilityLock() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


/**
 * RIL_REQUEST_SET_FACILITY_LOCK
 *
 * Enable/disable one facility lock
 *
 * "data" is const char **
 *
 * ((const char **)data)[0] = facility string code from TS 27.007 7.4
 * (eg "AO" for BAOC)
 * ((const char **)data)[1] = "0" for "unlock" and "1" for "lock"
 * ((const char **)data)[2] = password
 * ((const char **)data)[3] = string representation of decimal TS 27.007
 *                            service class bit vector. Eg, the string
 *                            "1" means "set this facility for voice services"
 *
 * "response" is int *
 * ((int *)response)[0] is the number of retries remaining, or -1 if unknown
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *  PASSWORD_INCORRECT
 *
 */
void requestSetFacilityLock(void *data, size_t datalen, RIL_Token t)
{
    /* It must be tested if the Lock for a particular class can be set without
     * modifing the values of the other class. If not, first must call
     * requestQueryFacilityLock to obtain the previus value
     */
    int err = 0;
    int numberOfRetries = -1;
    char *cmd = NULL;
    char *cmd_secure = NULL;
    char *code = NULL;
    char *lock = NULL;
    char *password = NULL;
    char *class = NULL;
    ATResponse *p_response = NULL;
    RIL_Errno failure = RIL_E_GENERIC_FAILURE;
    pinRetriesStruct pinRetries;

    assert (datalen >= (4 * sizeof(char **)));

    code = ((char **)data)[0];
    lock = ((char **)data)[1];
    password = ((char **)data)[2];
    class = ((char **)data)[3];

    asprintf(&cmd, "AT+CLCK=\"%s\",%s,\"%s\",%s", code, lock, password, class);
    asprintf(&cmd_secure, "AT+CLCK=\"%s\",%s,\"%s\",%s", code, lock, "****", class);

    err = at_send_command_secure(cmd, cmd_secure, &p_response);
    free(cmd);
    free(cmd_secure);

    getNumOfRetries(&pinRetries);

    numberOfRetries = pinRetries.pin1Retries;

    ALOGE("requestSetFacilityLock: number of retries left: %d", numberOfRetries);

    if (err < 0)
        goto end;

    if(p_response->success == 0) {
        if ((at_get_cme_error(p_response) == CME_ERROR_INCORRECT_PASSWORD) ||
            (at_get_cme_error(p_response) == CME_ERROR_PUK_REQUIRED)) {
            failure = RIL_E_PASSWORD_INCORRECT;
        }
    } else {
        failure = RIL_E_SUCCESS;
    }

end:
    RIL_onRequestComplete(t, failure, (void *)&numberOfRetries, sizeof(numberOfRetries));
    at_response_free(p_response);
}


/**
 * RIL_REQUEST_CHANGE_BARRING_PASSWORD
 *
 * Change call barring facility password
 *
 * "data" is const char **
 *
 * ((const char **)data)[0] = facility string code from TS 27.007 7.4
 * (eg "AO" for BAOC)
 * ((const char **)data)[1] = old password
 * ((const char **)data)[2] = new password
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 */
void requestChangeBarringPassword(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err = 0;
    char *cmd = NULL;
    char *cmd_secure = NULL;
    char *string = NULL;
    char *old_password = NULL;
    char *new_password = NULL;

    assert (datalen >= (3 * sizeof(char **)));

    string       = ((char **)data)[0];
    old_password = ((char **)data)[1];
    new_password = ((char **)data)[2];

    asprintf(&cmd, "AT+CPWD=\"%s\",\"%s\",\"%s\"", string, old_password, new_password);
    asprintf(&cmd_secure, "AT+CPWD=\"%s\",\"%s\",\"%s\"", string, "****", "****");
    err = at_send_command_secure(cmd, cmd_secure, &p_response);
    free(cmd);
    free(cmd_secure);

    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR: requestChangeBarringPassword() failed, err = %d\n", err);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


/** Returns SIM_NOT_READY on error */
SIM_Status getSIMStatus()
{
    ATResponse *p_response = NULL;
    int err;
    int ret;
    char *cpinLine;
    char *cpinResult;

    if (currentState() == RADIO_STATE_OFF ||
        currentState() == RADIO_STATE_UNAVAILABLE) {
        ret = SIM_NOT_READY;
        goto done;
    }

    err = at_send_command_singleline("AT+CPIN?", "+CPIN:", &p_response);

    if ((err != 0) ||(p_response == NULL)){
        ret = SIM_NOT_READY;
        goto done;
    }

    switch (at_get_cme_error(p_response)) {
        case CME_SUCCESS:
            break;

        case CME_SIM_NOT_INSERTED:
            ret = SIM_ABSENT;
            goto done;

        case CME_ERROR_SIM_FAILURE:
            ret = SIM_ERROR;
            goto done;

        case CME_ERROR_ME_DEPERSONALIZATION_BLOCKED:
            ret = SIM_NETWORK_PERSONALIZATION_BRICKED;
            goto done;

        default:
            ret = SIM_NOT_READY;
            goto done;
    }

    /* CPIN? has succeeded, now look at the result */

    cpinLine = p_response->p_intermediates->line;
    err = at_tok_start (&cpinLine);

    if (err < 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

    err = at_tok_nextstr(&cpinLine, &cpinResult);

    if (err < 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

    if ((0 == strcmp (cpinResult, "SIM PIN")) ||
        (0 == strcmp (cpinResult, "SIM PIN2"))) {
        ret = SIM_PIN;
        goto done;
    } else if ((0 == strcmp (cpinResult, "SIM PUK")) ||
               (0 == strcmp (cpinResult, "SIM PUK2"))) {
        ret = SIM_PUK;
        goto done;
    } else if (0 == strcmp (cpinResult, "PH-NET PIN")) {
        ret = SIM_NETWORK_PERSONALIZATION;
        goto done;
    } else if (0 != strcmp (cpinResult, "READY")) {
        /* we're treating unsupported lock types as "sim absent" */
        ret = SIM_ABSENT;
        goto done;
    }

    ret = SIM_READY;

done:
    at_response_free(p_response);
    return ret;
}

static RIL_AppType getCardType(void)
{
    ATResponse *p_response = NULL;
    int err;
    RIL_AppType ret =  RIL_APPTYPE_UNKNOWN;
    char *line;
    int sim;

    err = at_send_command_singleline("AT*TSIMINS?", "*TSIMINS:", &p_response);
    if((err<0)||(p_response->success== 0))      goto done;

    line =   p_response->p_intermediates->line;
    err = at_tok_start (&line);
    if(err<0)        goto done;

    err = at_tok_nextint(&line, &sim);
    if(err<0)        goto done;
    /*  1: SIM Present 0: SIM not present */
    err = at_tok_nextint(&line, &sim);
    if((err<0) || (sim==0))        goto done;

    err = at_tok_nextint(&line, &sim);
    if(err<0)        goto done;

   switch(sim)
   {
       case  0:
           ret =   RIL_APPTYPE_SIM;
           break;
       case 1:
           ret = RIL_APPTYPE_USIM;
           break;
       default:
            ret =   RIL_APPTYPE_UNKNOWN;
            break;
   }

done:
    at_response_free(p_response);
    return ret;

}


/**
 * Get the current card status.
 *
 * This must be freed using freeCardStatus.
 * @return: On success returns RIL_E_SUCCESS
 */
static int getCardStatus(RIL_CardStatus **pp_card_status)
{
    RIL_AppStatus app_status_array[] = {
        // SIM_ABSENT = 0
        { RIL_APPTYPE_UNKNOWN, RIL_APPSTATE_UNKNOWN, RIL_PERSOSUBSTATE_UNKNOWN,
          NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN },
        // SIM_NOT_READY = 1
        { RIL_APPTYPE_UNKNOWN, RIL_APPSTATE_DETECTED, RIL_PERSOSUBSTATE_UNKNOWN,
          NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN },
        // SIM_READY = 2
        { RIL_APPTYPE_UNKNOWN, RIL_APPSTATE_READY, RIL_PERSOSUBSTATE_READY,
          NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN },
        // SIM_PIN = 3
        { RIL_APPTYPE_UNKNOWN, RIL_APPSTATE_PIN, RIL_PERSOSUBSTATE_UNKNOWN,
          NULL, NULL, 0, RIL_PINSTATE_ENABLED_NOT_VERIFIED, RIL_PINSTATE_UNKNOWN },
        // SIM_PUK = 4
        { RIL_APPTYPE_UNKNOWN, RIL_APPSTATE_PUK, RIL_PERSOSUBSTATE_UNKNOWN,
          NULL, NULL, 0, RIL_PINSTATE_ENABLED_BLOCKED, RIL_PINSTATE_UNKNOWN },
        // SIM_NETWORK_PERSONALIZATION = 5
        { RIL_APPTYPE_UNKNOWN, RIL_APPSTATE_SUBSCRIPTION_PERSO, RIL_PERSOSUBSTATE_SIM_NETWORK,
          NULL, NULL, 0, RIL_PINSTATE_ENABLED_NOT_VERIFIED, RIL_PINSTATE_UNKNOWN },
        // SIM_NETWORK_PERSONALIZATION_BRICKED = 6
        { RIL_APPTYPE_UNKNOWN, RIL_APPSTATE_SUBSCRIPTION_PERSO, RIL_PERSOSUBSTATE_SIM_NETWORK,
          NULL, NULL, 0, RIL_PINSTATE_ENABLED_PERM_BLOCKED, RIL_PINSTATE_UNKNOWN }

    };
    RIL_CardState card_state;
    int num_apps;
    int sim_status = getSIMStatus();
    int i;
    pinRetriesStruct pinRetries;

    switch(sim_status)
    {
        case SIM_ABSENT:
            card_state = RIL_CARDSTATE_ABSENT;
            num_apps = 0;
            break;
        case SIM_ERROR:
            card_state = RIL_CARDSTATE_ERROR;
            num_apps = 0;
            break;
        default:
            card_state = RIL_CARDSTATE_PRESENT;
            num_apps = 1;
            break;
    }

    // Allocate and initialize base card status.
    RIL_CardStatus *p_card_status = malloc(sizeof(RIL_CardStatus));
    p_card_status->card_state = card_state;
    p_card_status->universal_pin_state = RIL_PINSTATE_UNKNOWN;
    p_card_status->gsm_umts_subscription_app_index = RIL_CARD_MAX_APPS;
    p_card_status->cdma_subscription_app_index = RIL_CARD_MAX_APPS;
#if RIL_VERSION >= 6
    p_card_status->ims_subscription_app_index = RIL_CARD_MAX_APPS;
#endif
    p_card_status->num_applications = num_apps;

    // Initialize application status
    for (i = 0; i < RIL_CARD_MAX_APPS; i++) {
        p_card_status->applications[i] = app_status_array[SIM_ABSENT];
    }

    // Pickup the appropriate application status
    // that reflects sim_status for gsm.
    if (num_apps != 0) {
        int err = -1;
        // Only support one app, gsm
        p_card_status->num_applications = 1;
        p_card_status->gsm_umts_subscription_app_index = 0;

        // Get the correct app status
        p_card_status->applications[0] = app_status_array[sim_status];
        p_card_status->applications[0].app_type = getCardType();

        /* Compute the pin state */
        err = getNumOfRetries(&pinRetries);

        if(err != 0)
        {
            if(pinRetries.puk1Retries == 0)
            {
                p_card_status->applications[0].pin1 = RIL_PINSTATE_ENABLED_PERM_BLOCKED;
            }
            else if(pinRetries.pin1Retries == 0)
            {
                p_card_status->applications[0].pin1 = RIL_PINSTATE_ENABLED_BLOCKED;
            }

            if(pinRetries.puk2Retries == 0)
            {
                p_card_status->applications[0].pin2 = RIL_PINSTATE_ENABLED_PERM_BLOCKED;
            }
            else if(pinRetries.pin2Retries == 0)
            {
                p_card_status->applications[0].pin2 = RIL_PINSTATE_ENABLED_BLOCKED;
            }
        }
    }

    *pp_card_status = p_card_status;
    return RIL_E_SUCCESS;
}

/**
 * Free the card status returned by getCardStatus
 */
static void freeCardStatus(RIL_CardStatus *p_card_status)
{
    free(p_card_status);
}

/**
 * RIL_UNSOL_SIM_REFRESH
 *
 * Indicates that file(s) on the SIM have been updated, or the SIM
 * has been reinitialized.
 *
 * "data" is an int *
 * ((int *)data)[0] is a RIL_SimRefreshResult.
 * ((int *)data)[1] is the EFID of the updated file if the result is
 * SIM_FILE_UPDATE or NULL for any other result.
 *
 * Note: If the radio state changes as a result of the SIM refresh (eg,
 * SIM_READY -> SIM_LOCKED_OR_ABSENT), RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED
 * should be sent.
 */
void unsolSIMRefresh(const char *s)
{
    int response[2];
    int n, inserted=0, err;

    char *line = strdup(s);
    char *p = line;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &n);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &inserted);
    if (err < 0) goto error;

    if(inserted == 1)
    {
        /* SIM initialized.  All files should be re-read. */
        response[0] = SIM_INIT;
        response[1] = 0;

        RIL_onUnsolicitedResponse(RIL_UNSOL_SIM_REFRESH,
                                    response, sizeof(response));
    }
    error:
    free(p);
}


/**
 * RIL_REQUEST_CHANGE_SIM_PIN
 *
 * Supplies old SIM PIN and new PIN.
 *
 * "data" is const char **
 * ((const char **)data)[0] is old PIN value
 * ((const char **)data)[1] is new PIN value
 *
 * "response" is int *
 * ((int *)response)[0] is the number of retries remaining, or -1 if unknown
 *
 * Valid errors:
 *
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 *  PASSWORD_INCORRECT
 *     (old PIN is invalid)
 *
 */
void requestChangePasswordPin(void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    int numberOfRetries = -1;
    int simCheckEnabled = 0;
    char *line;
    char *cmd = NULL;
    char *cmd_secure = NULL;
    char *old_password = NULL;
    char *new_password = NULL;
    ATResponse *p_response = NULL;
    RIL_Errno failure = RIL_E_GENERIC_FAILURE;
    pinRetriesStruct pinRetries;

#if RIL_VERSION >= 6
    /* Let's ignore AppId */
    datalen -= sizeof(char *);
#endif

    if (datalen != 2 * sizeof(char *))
        goto end;

    old_password = ((char **)data)[0];
    new_password = ((char **)data)[1];

    asprintf(&cmd, "AT+CLCK=\"%s\",2", "SC");
    err = at_send_command_singleline(cmd,"+CLCK:", &p_response);
    free(cmd);

    if (err < 0 || p_response->success == 0)
    {
        goto end;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);

    if (err < 0)
    {
        goto end;
    }

    err = at_tok_nextint(&line, &simCheckEnabled);

    if (err < 0)
    {
        goto end;
    }
    at_response_free(p_response);

    if(simCheckEnabled == 0)
    {
        asprintf(&cmd, "AT+CLCK=\"%s\",1,\"%s\"", "SC", old_password);
        asprintf(&cmd_secure, "AT+CLCK=\"%s\",1,\"%s\"", "SC", "****");
        err = at_send_command_secure(cmd, cmd_secure, &p_response);
        free(cmd);
        free(cmd_secure);

        if (err < 0)
            goto end;

        if(p_response->success == 0)
        {
            if ((at_get_cme_error(p_response) == CME_ERROR_INCORRECT_PASSWORD) ||
            (at_get_cme_error(p_response) == CME_ERROR_PUK_REQUIRED))
            {
                failure = RIL_E_PASSWORD_INCORRECT;
            }
            goto end;
        }
       at_response_free(p_response);
    }

    asprintf(&cmd, "AT+CPWD=\"%s\",\"%s\",\"%s\"", "SC", old_password,new_password);
    asprintf(&cmd_secure, "AT+CPWD=\"%s\",\"%s\",\"%s\"", "SC", "****","****");
    err = at_send_command_secure(cmd, cmd_secure, &p_response);
    free(cmd);
    free(cmd_secure);

    if (err < 0)
        goto end;

    if(p_response->success == 0)
    {
        if ((at_get_cme_error(p_response) == CME_ERROR_INCORRECT_PASSWORD) ||
            (at_get_cme_error(p_response) == CME_ERROR_PUK_REQUIRED))
        {
            failure = RIL_E_PASSWORD_INCORRECT;
        }
        goto end;
    }
    else
    {
        failure = RIL_E_SUCCESS;
    }

    if(simCheckEnabled == 0)
    {
        at_response_free(p_response);
        asprintf(&cmd, "AT+CLCK=\"%s\",0,\"%s\"", "SC", new_password);
        asprintf(&cmd_secure, "AT+CLCK=\"%s\",0,\"%s\"", "SC", "****");
        err = at_send_command_secure(cmd, cmd_secure, &p_response);
        free(cmd);
        free(cmd_secure);

        if (err < 0)
        {
            failure = RIL_E_GENERIC_FAILURE;
            goto end;
        }

        if(p_response->success == 0)
        {
            if ((at_get_cme_error(p_response) == CME_ERROR_INCORRECT_PASSWORD) ||
            (at_get_cme_error(p_response) == CME_ERROR_PUK_REQUIRED))
            {
                failure = RIL_E_PASSWORD_INCORRECT;
            }
            else
                failure = RIL_E_GENERIC_FAILURE;
            goto end;
        }
    }


end:
    getNumOfRetries(&pinRetries);

    numberOfRetries = pinRetries.pin1Retries;
    ALOGE("requestChangePassword: number of retries left: %d", numberOfRetries);
    RIL_onRequestComplete(t, failure, (void *)&numberOfRetries, sizeof(numberOfRetries));
    at_response_free(p_response);
    return;
}

/**
 * RIL_REQUEST_CHANGE_SIM_PIN2
 *
 * Supplies old SIM PIN2 and new PIN2.
 *
 * "data" is const char **
 * ((const char **)data)[0] is old PIN2 value
 * ((const char **)data)[1] is new PIN2 value
 *
 * "response" is int *
 * ((int *)response)[0] is the number of retries remaining, or -1 if unknown
 *
 * Valid errors:
 *
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 *  PASSWORD_INCORRECT
 *     (old PIN2 is invalid)
 *
 */
void requestChangePasswordPin2(void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    int numberOfRetries = -1;
    int simCheckEnabled = 0;
    char *line;
    char *cmd = NULL;
    char *cmd_secure = NULL;
    char *old_password = NULL;
    char *new_password = NULL;
    ATResponse *p_response = NULL;
    RIL_Errno failure = RIL_E_GENERIC_FAILURE;
    pinRetriesStruct pinRetries;

#if RIL_VERSION >= 6
    /* Let's ignore AppId */
    datalen -= sizeof(char *);
#endif

    if (datalen != 2 * sizeof(char *))
        goto end;

    old_password = ((char **)data)[0];
    new_password = ((char **)data)[1];

    asprintf(&cmd, "AT+CPWD=\"%s\",\"%s\",\"%s\"", "P2", old_password,new_password);
    asprintf(&cmd_secure, "AT+CPWD=\"%s\",\"%s\",\"%s\"", "P2", "****","****");
    err = at_send_command_secure(cmd, cmd_secure, &p_response);
    free(cmd);
    free(cmd_secure);

    if (err < 0)
        goto end;

    if(p_response->success == 0)
    {
        if ((at_get_cme_error(p_response) == CME_ERROR_INCORRECT_PASSWORD) ||
            (at_get_cme_error(p_response) == CME_ERROR_PUK2_REQUIRED))
        {
            failure = RIL_E_PASSWORD_INCORRECT;
        }
        goto end;
    }
    else
    {
        failure = RIL_E_SUCCESS;
    }

end:

    getNumOfRetries(&pinRetries);
    numberOfRetries = pinRetries.pin2Retries;
    ALOGE("requestChangePassword: number of retries left: %d", numberOfRetries);

    RIL_onRequestComplete(t, failure, (void *)&numberOfRetries, sizeof(numberOfRetries));
    at_response_free(p_response);
    return;
}
/**
 * Get the number of retries left for pin functions
 */
static int getNumOfRetries(pinRetriesStruct *pinRetries)
{
    ATResponse *p_response = NULL;
    int err = 0;
    char *line;
    int num_of_retries = -1;

    pinRetries->pin1Retries = -1;
    pinRetries->puk1Retries = -1;
    pinRetries->pin2Retries = -1;
    pinRetries->puk2Retries = -1;

    err = at_send_command_singleline("AT%PINNUM?", "%PINNUM:", &p_response);

    if (err < 0 || p_response->success == 0)
        goto end;

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto end;
    err = at_tok_nextint(&line, &num_of_retries);
    if (err < 0)
       goto end;
    pinRetries->pin1Retries = num_of_retries;

    err = at_tok_nextint(&line, &num_of_retries);
    if (err < 0)
        goto end;
    pinRetries->puk1Retries = num_of_retries;

    err = at_tok_nextint(&line, &num_of_retries);
    if (err < 0)
        goto end;
    pinRetries->pin2Retries = num_of_retries;

    err = at_tok_nextint(&line, &num_of_retries);
    if (err < 0)
        goto end;
    pinRetries->puk2Retries = num_of_retries;

end:
    at_response_free(p_response);
    return err;
}
