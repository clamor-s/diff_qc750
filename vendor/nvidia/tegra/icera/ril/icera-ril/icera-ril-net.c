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
#include <telephony/ril.h>
#include <assert.h>
#include "atchannel.h"
#include "at_tok.h"
#include "misc.h"
#include <icera-ril.h>
#include <icera-ril-net.h>
#include <ril_request_object.h>

/*for GetNmbConcurentProfile */
#include <icera-ril-data.h>

#define LOG_TAG "RIL"
#include <utils/Log.h>

#define countryCodeLength 3
#define networkCodeMinLength 2
#define networkCodeMaxLength 3


static const char* networkStatusToRilString(int state);

static void parsePlusCxREG(char *line, int *p_RegState, int* p_Lac,int * p_CId);
static int accessTechToRat(char *rat, int request);
static int GetCurrentNetworkSelectionMode(void);
static void resetCachedOperator(void);

static int CurrentTechno = RADIO_TECH_UNKNOWN;

#if RIL_VERSION <= 4
static int signalStrength[2];
static int ResponseSize = sizeof(signalStrength);
#else
static RIL_SignalStrength_v6 signalStrengthData;
static RIL_SignalStrength_v6 *signalStrength=&signalStrengthData;
static int ResponseSize = sizeof(RIL_SignalStrength_v6);
#endif

enum regState {
    REG_STATE_NO_NETWORK = 0,
    REG_STATE_REGISTERED = 1,
    REG_STATE_SEARCHING  = 2,
    REG_STATE_REJECTED   = 3,
    REG_STATE_UNKNOWN    = 4,
    REG_STATE_ROAMING    = 5,
    REG_STATE_EMERGENCY  = 10,
    };

#define TECH_2G 1
#define TECH_3G 2
static int ActToTechno(int Act)
{
    switch(Act)
    {
        case 0:
        case 1:
        case 3:
            return TECH_2G;
            break;
        case 2:
        case 4:
        case 5:
        case 6:
            return TECH_3G;
            break;
        default:
            return 0;
    }
}

static char * TechnoToString(int Techno)
{
    switch(Techno)
    {
        case TECH_2G:
            return "2G";
        case TECH_3G:
            return "3G";
        case TECH_2G|TECH_3G:
            return "2G/3G";
        default:
            return "";
    }
}

typedef struct _regstate{
    int Reg;
    int Cid;
    int Lac;
} regstate;

typedef struct _nwstate{
    int emergencyOrSearching;
    int mccMnc;
    char * rat;
    char * connState;
} nwstate;

typedef struct _copsstate{
    int valid;
    char* networkOp[3];
}opstate;

/* Cached network details */
static nwstate NwState;
static regstate Creg;
static regstate Cgreg;
static regstate Cereg;
static int cacheInvalid;
static opstate CopsState;

/* Global state variable */
static int LocUpdateEnabled = 1;
static int LteSupported;
static int NetworkSelectionMode;

/**
 * RIL_REQUEST_SET_BAND_MODE
 *
 * Assign a specified band for RF configuration.
 *
 * "data" is int *
 * ((int *)data)[0] is == 0 for "unspecified" (selected by baseband automatically)
 * ((int *)data)[0] is == 1 for "EURO band" (GSM-900 / DCS-1800 / WCDMA-IMT-2000)
 * ((int *)data)[0] is == 2 for "US band" (GSM-850 / PCS-1900 / WCDMA-850 / WCDMA-PCS-1900)
 * ((int *)data)[0] is == 3 for "JPN band" (WCDMA-800 / WCDMA-IMT-2000)
 * ((int *)data)[0] is == 4 for "AUS band" (GSM-900 / DCS-1800 / WCDMA-850 / WCDMA-IMT-2000)
 * ((int *)data)[0] is == 5 for "AUS band 2" (GSM-900 / DCS-1800 / WCDMA-850)
 * ((int *)data)[0] is == 6 for "Cellular (800-MHz Band)"
 * ((int *)data)[0] is == 7 for "PCS (1900-MHz Band)"
 * ((int *)data)[0] is == 8 for "Band Class 3 (JTACS Band)"
 * ((int *)data)[0] is == 9 for "Band Class 4 (Korean PCS Band)"
 * ((int *)data)[0] is == 10 for "Band Class 5 (450-MHz Band)"
 * ((int *)data)[0] is == 11 for "Band Class 6 (2-GMHz IMT2000 Band)"
 * ((int *)data)[0] is == 12 for "Band Class 7 (Upper 700-MHz Band)"
 * ((int *)data)[0] is == 13 for "Band Class 8 (1800-MHz Band)"
 * ((int *)data)[0] is == 14 for "Band Class 9 (900-MHz Band)"
 * ((int *)data)[0] is == 15 for "Band Class 10 (Secondary 800-MHz Band)"
 * ((int *)data)[0] is == 16 for "Band Class 11 (400-MHz European PAMR Band)"
 * ((int *)data)[0] is == 17 for "Band Class 15 (AWS Band)"
 * ((int *)data)[0] is == 18 for "Band Class 16 (US 2.5-GHz Band)"
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
void requestSetBandMode(void *data, size_t datalen, RIL_Token t)
{
    int bandMode = ((int *)data)[0];
    ATResponse *p_response = NULL;
    int err = 0;

    switch (bandMode) {

        /* "unspecified" (selected by baseband automatically) */
        case 0:
            /* All bands are activated except FDD BAND III (1700 UMTS-FDD band),
             * i.e. GSM 900, GSM 1800, GSM 1900, GSM 850, 2100 UMTS-FDD,
             * 1900 UMTS-FDD, 850 UMTS-FDD, 900 UMTS-FDD bans.
             */
            err = at_send_command("AT%IPBM=\"ANY\",1", &p_response);
            if (err < 0 || p_response->success == 0) goto error;
            break;

        /* "EURO band" (GSM-900 / DCS-1800 / WCDMA-IMT-2000) */
        case 1:
            /* EGSM (GSM 900) and FDD BAND I (2100 UMTS-FDD/WCDMA-IMT-2000) set ON */
            err = at_send_command("AT%IPBM=\"ANY\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* GSM 1800 (DCS-1800) set ON */
            err = at_send_command("AT%IPBM=\"DCS\",1", &p_response);
            if (err < 0 || p_response->success == 0) goto error;
            break;

        /* "US band" (GSM-850 / PCS-1900 / WCDMA-850 / WCDMA-PCS-1900) */
        case 2:
            /* EGSM (GSM 900) and FDD BAND I (2100 UMTS-FDD/WCDMA-IMT-2000) set ON */
            err = at_send_command("AT%IPBM=\"ANY\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* GSM 850 set ON */
            err = at_send_command("AT%IPBM=\"G850\",1", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* GSM 1900 (PCS-1900) set ON */
            err = at_send_command("AT%IPBM=\"PCS\",1", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* 850 UMTS-FDD (WCDMA-850) set ON */
            err = at_send_command("AT%IPBM=\"FDD_BAND_V\",1", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* 1900 UMTS-FDD (WCDMA-PCS-1900) set ON */
            err = at_send_command("AT%IPBM=\"FDD_BAND_II\",1", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* GSM 900 set OFF */
            err = at_send_command("AT%IPBM=\"EGSM\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* 2100 UMTS-FDD (WCDMA-IMT-2000) set OFF */
            err = at_send_command("AT%IPBM=\"FDD_BAND_I\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;
            break;

        /* "JPN band" (WCDMA-800 / WCDMA-IMT-2000) */
        case 3:
            /* ATTENTION: WCDMA-800 is not supported by ICERA modem */

            /* EGSM (GSM 900) and FDD BAND I (2100 UMTS-FDD/WCDMA-IMT-2000) set ON */
            err = at_send_command("AT%IPBM=\"ANY\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* GSM 900 set OFF */
            err = at_send_command("AT%IPBM=\"EGSM\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;
            break;

        /* "AUS band" (GSM-900 / DCS-1800 / WCDMA-850 / WCDMA-IMT-2000) */
        case 4:
            /* EGSM (GSM 900) and FDD BAND I (2100 UMTS-FDD/WCDMA-IMT-2000) set ON */
            err = at_send_command("AT%IPBM=\"ANY\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* GSM 1800 (DCS-1800) set ON */
            err = at_send_command("AT%IPBM=\"DCS\",1", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* 850 UMTS-FDD (WCDMA-850) set ON */
            err = at_send_command("AT%IPBM=\"FDD_BAND_V\",1", &p_response);
            if (err < 0 || p_response->success == 0) goto error;
            break;

        /* "AUS band 2" (GSM-900 / DCS-1800 / WCDMA-850) */
        case 5:
            /* EGSM (GSM 900) and FDD BAND I (2100 UMTS-FDD/WCDMA-IMT-2000) set ON */
            err = at_send_command("AT%IPBM=\"ANY\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* GSM 1800 (DCS-1800) set ON */
            err = at_send_command("AT%IPBM=\"DCS\",1", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* 850 UMTS-FDD (WCDMA-850) set ON */
            err = at_send_command("AT%IPBM=\"FDD_BAND_V\",1", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* 2100 UMTS-FDD (WCDMA-IMT-2000) set OFF */
            err = at_send_command("AT%IPBM=\"FDD_BAND_I\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;
            break;

        /* Cellular (800-MHz Band) */
        case 6:
            /* It is not supported by ICERA modem */
            goto not_supported;

        /* PCS (1900-MHz Band) */
        case 7:
            /* EGSM (GSM 900) and FDD BAND I (2100 UMTS-FDD/WCDMA-IMT-2000) set ON */
            err = at_send_command("AT%IPBM=\"ANY\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* GSM 1900 (PCS-1900) set ON */
            err = at_send_command("AT%IPBM=\"PCS\",1", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* GSM 900 set OFF */
            err = at_send_command("AT%IPBM=\"EGSM\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* 2100 UMTS-FDD (WCDMA-IMT-2000) set OFF */
            err = at_send_command("AT%IPBM=\"FDD_BAND_I\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;
            break;

        /* Band Class 3 (JTACS Band) */
        case 8:
            /* It is not supported by ICERA modem */
            goto not_supported;

        /* Band Class 4 (Korean PCS Band) */
        case 9:
            /* It is not supported by ICERA modem */
            goto not_supported;

        /* Band Class 5 (450-MHz Band) */
        case 10:
            /* It is not supported by ICERA modem */
            goto not_supported;

        /* Band Class 6 (2-GMHz IMT2000 Band) */
        case 11:
            /* EGSM (GSM 900) and FDD BAND I (2100 UMTS-FDD/WCDMA-IMT-2000) set ON */
            err = at_send_command("AT%IPBM=\"ANY\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* GSM 900 set OFF */
            err = at_send_command("AT%IPBM=\"EGSM\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;
            break;

        /* Band Class 7 (Upper 700-MHz Band) */
        case 12:
            /* It is not supported by ICERA modem */
            goto not_supported;

        /* Band Class 8 (1800-MHz Band) */
        case 13:
            /* EGSM (GSM 900) and FDD BAND I (2100 UMTS-FDD/WCDMA-IMT-2000) set ON */
            err = at_send_command("AT%IPBM=\"ANY\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* GSM 1800 (DCS-1800) set ON */
            err = at_send_command("AT%IPBM=\"DCS\",1", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* GSM 900 set OFF */
            err = at_send_command("AT%IPBM=\"EGSM\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* 2100 UMTS-FDD (WCDMA-IMT-2000) set OFF */
            err = at_send_command("AT%IPBM=\"FDD_BAND_I\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;
            break;

        /* Band Class 9 (900-MHz Band) */
        case 14:
            /* EGSM (GSM 900) and FDD BAND I (2100 UMTS-FDD/WCDMA-IMT-2000) set ON */
            err = at_send_command("AT%IPBM=\"ANY\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;

            at_response_free(p_response);
            p_response = NULL;

            /* 2100 UMTS-FDD (WCDMA-IMT-2000) set OFF */
            err = at_send_command("AT%IPBM=\"FDD_BAND_I\",0", &p_response);
            if (err < 0 || p_response->success == 0) goto error;
            break;

        /* Band Class 10 (Secondary 800-MHz Band) */
        case 15:
            /* It is not supported by ICERA modem */
            goto not_supported;

        /* Band Class 11 (400-MHz European PAMR Band) */
        case 16:
            /* It is not supported by ICERA modem */
            goto not_supported;

        /* Band Class 15 (AWS Band) */
        case 17:
            /* It is not supported by ICERA modem */
            goto not_supported;

        /* Band Class 16 (US 2.5-GHz Band) */
        case 18:
            /* It is not supported by ICERA modem */
            goto not_supported;

        default:
            goto error;
    }

    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

not_supported:
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_MODE_NOT_SUPPORTED, NULL, 0);
    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR: requestSetBandMode() failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE
 *
 * Query the list of band mode supported by RF.
 *
 * See also: RIL_REQUEST_SET_BAND_MODE
 */
void requestQueryAvailableBandMode(void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    int i;
    int value[12];
    char *cmd = NULL;
    char *line, *c_skip;
    ATLine *nextATLine = NULL;
    ATResponse *p_response = NULL;

    // the stack  has to be powered down by the framework before the band mode is requested.

    err = at_send_command_multiline_no_prefix("AT%IPBM?", &p_response);
    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    nextATLine = p_response->p_intermediates;
    line = nextATLine->line;

    for (i = 0 ; i < 12 ; i++ )
    {
        err = at_tok_nextColon(&line);
        if (err < 0)
            goto error;
        err = at_tok_nextint(&line, &value[i]);
        if (err < 0)
            goto error;
        if(nextATLine->p_next!=NULL)
        {
            nextATLine = nextATLine->p_next;
            line = nextATLine->line;
        }
    }

    if(value[0]==1) //Any Band
    {
        int response[2];

        response[0] = 2;
        response[1] = ANY; //Any Band supported
        RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
        goto IF_END;
    }else if((value[3]==1)||(value[4]==1)||(value[6]==1)||(value[9]==1))  //US Band
    {
        if((value[1]==1)||(value[2]==1)||(value[5]==1))  //EURO Band + US Band
        {
            int response[6];

            response[0] = 6;
            response[1] = EURO; //EURO Band supported
            response[2] = US;   //US Band supported
            response[3] = AUS;  //AUS band supported
            response[4] = AUS2; //AUS-2 band supported
            response[5] = JPN;  //JPN band supported
            RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
             goto IF_END;
        }else  //No EURO Band
        {
            if((value[5]==1)||(value[8]==1)) //JPN Band  + US Band
            {
                int response[5];

                response[0] = 5;
                response[1] = US;   //US Band supported
                response[2] = AUS;  //AUS band supported
                response[3] = AUS2; //AUS-2 band supported
                response[4] = JPN;  //JPN band supported
                RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
                goto IF_END;
            }else  //No JPN Band
            {
                int response[4];

                response[0] = 4;
                response[1] = US;   //US Band supported
                response[2] = AUS;  //AUS band supported
                response[3] = AUS2; //AUS-2 band supported
                RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
                goto IF_END;
            }
        }
    }else if((value[1]==1)||(value[2]==1)||(value[5]==1))
    { //No US Support at all
        int response[6];//check

        response[0] = 6;
        response[1] = EURO; //EURO Band supported
        response[3] = AUS;    //AUS band supported
        response[4] = AUS2; //AUS-2 band supported
        response[5] = JPN;    //JPN band supported
        RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
    }
IF_END:
    // the stack  has to be powered up by the framework after the band mode was requested.

      at_response_free(p_response);
    return;

    error:
    at_response_free(p_response);
    ALOGE("requestQueryAvailableBandMode must never return error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    return;
}

void requestSetNetworkSelectionAutomatic(void *data, size_t datalen, RIL_Token t)
{
    int err = 0;

    /*
     * First query the current network selection mode
     * If already in aurtomatic, then re-applying automatic
     * will make the phone offline for some time
     */
    int currentSelMode = GetCurrentNetworkSelectionMode();

    if(currentSelMode!=0)
    {
        err = at_send_command("AT+COPS=0", NULL);
    }
    else
    {
        ALOGD("Network selection mode already automatic, ignoring. ");
    }

    if(err < 0)
    {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        /*
         * Not sure where we stand, will fetch it from the
         * modem on a per need basis
         */
        NetworkSelectionMode = -1;
    }
    else
    {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
        NetworkSelectionMode = 0;
    }
    return;
}

void requestSetNetworkSelectionManual(void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    int response = -1;
    char *cmd = NULL;
    const char *oper = (const char *) data;
    ATResponse *p_response = NULL;

    //check oper length if valid
    if((strlen(oper) > (countryCodeLength + networkCodeMaxLength))
            ||(strlen(oper) < (countryCodeLength + networkCodeMinLength))){
        ALOGE("requestSetNetworkSelectionManual oper length invalid");
        goto error;
    }

    asprintf(&cmd, "AT+COPS=1,2,\"%s\"", oper);
    err = at_send_command(cmd, &p_response);

    if (err < 0 || p_response->success == 0) goto error;

    free(cmd);
    NetworkSelectionMode = 1;
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
    /* Not sure where we stand anymore, ask the modem again */
    NetworkSelectionMode = -1;
    // check CME Error for handling different error response
    if(p_response)
    {
        switch (at_get_cme_error(p_response))
        {
            case CME_SIM_NOT_INSERTED:
            case CME_ERROR_SIM_INVALID:
            case CME_ERROR_SIM_POWERED_DOWN:
            case CME_ERROR_ILLEGAL_MS:
            case CME_ERROR_ILLEGAL_ME:
                RIL_onRequestComplete(t, RIL_E_ILLEGAL_SIM_OR_ME, NULL, 0);
            break;

            default:
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        }
    }
    else
    {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }

    ALOGE("ERROR - requestSetNetworkSelectionManual() failed");

    free(cmd);
    at_response_free(p_response);
}


void requestQueryNetworkSelectionMode(
                void *data, size_t datalen, RIL_Token t)
{
    int response = GetCurrentNetworkSelectionMode();

    if(response >= 0)
    {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));
    }
    else
    {
        ALOGE("requestQueryNetworkSelectionMode must never return error when radio is on");
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void requestQueryAvailableNetworks(void *data, size_t datalen, RIL_Token t)
{
    /* We expect an answer on the following form:
       +COPS: (2,"AT&T","AT&T","310410",0),(1,"T-Mobile ","TMO","310260",0)
     */

    int err, operators, i, skip, status;
    ATResponse *p_response = NULL;
    char * c_skip, *line, *p = NULL;
    char ** response = NULL;
    int AcT, *AccessTechno;
    char * LongAlpha, *ShortAlpha, * MccMnc, *StatusString;
    int j, found, effectiveOperators = 0;

    err = at_send_command_singleline("AT+COPS=?", "+COPS:", &p_response);

    if (err != 0) goto error;

    if (at_get_cme_error(p_response) != CME_SUCCESS) goto error;

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    /* Count number of '(' in the +COPS response to get number of operators*/
    operators = 0;
    for (p = line ; *p != '\0' ;p++) {
        if (*p == '(') operators++;
    }

    response = (char **)alloca(operators * 4 * sizeof(char *));
    AccessTechno = (int*)alloca(operators * sizeof(int));

    for (i = 0 ; i < operators ; i++ )
    {
        err = at_tok_nextstr(&line, &c_skip);
        if (err < 0) goto error;
        if (strcmp(c_skip,"") == 0)
        {
            operators = i;
            continue;
        }
        status = atoi(&c_skip[1]);
        StatusString = (char*)networkStatusToRilString(status);

        err = at_tok_nextstr(&line, &LongAlpha);
        if (err < 0) goto error;

        err = at_tok_nextstr(&line, &ShortAlpha);
        if (err < 0) goto error;

        err = at_tok_nextstr(&line, &MccMnc);
        if (err < 0) goto error;

        err = at_tok_nextint(&line, &AcT);
        if (err < 0) goto error;
        /*
         * Check if this operator is already in the list for another AcT
         * to avoid confusing doublons
         */

        for(j=0,found=0; (j<effectiveOperators) && (found == 0); j++)
        {

            if(strcmp(response[j*4+2],MccMnc) == 0)
            {
                found++;
                AccessTechno[j] |= ActToTechno(AcT);
            }
        }

        if(!found)
        {
            response[effectiveOperators*4] = LongAlpha;
            response[effectiveOperators*4+1] = ShortAlpha;
            response[effectiveOperators*4+2] = MccMnc;
            response[effectiveOperators*4+3] = StatusString;
            AccessTechno[effectiveOperators] = ActToTechno(AcT);
            effectiveOperators++;
        }
    }

    /* Rewrite the Long Alpha to mention the techno */
    for(j=0; j<effectiveOperators; j++)
    {
        asprintf(&response[j*4], "%s (%s)",response[j*4], TechnoToString(AccessTechno[j]));
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, (effectiveOperators * 4 * sizeof(char *)));

    for(j=0; j<effectiveOperators; j++)
    {
        free(response[j*4]);
    }

    at_response_free(p_response);
    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR - requestQueryAvailableNetworks() failed");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


void requestRegistrationState(int request, void *data,
                                        size_t datalen, RIL_Token t)
{
    int err;
    unsigned int i;
    int Reg, Lac, Cid;
    char * responseStr[15];
    ATResponse *p_response = NULL;
    char *line, *p;
    int ConnectionState;

    memset(responseStr,(int)NULL,sizeof(responseStr));

    /* Size of the response, in char * */
    int ResponseSize = 4;

    if (request == RIL_REQUEST_VOICE_REGISTRATION_STATE) {
        Reg = Creg.Reg;
        Lac = Creg.Lac;
        Cid = Creg.Cid;
    } else if (request == RIL_REQUEST_DATA_REGISTRATION_STATE) {
        Reg = Cgreg.Reg;
        Lac = Cgreg.Lac;
        Cid = Cgreg.Cid;
    } else {
        assert(0);
        goto error;
    }

    if(Reg == -1)
        goto error;

    /*
     * Need to get the LTE state as well
     * Don't bother if 3G already available
     */
    if ((request == RIL_REQUEST_DATA_REGISTRATION_STATE)
     &&(Reg!=1)
     &&(Reg!=5))
    {
        /*
         * Return the information only if more
         * valuable than the current one
         */
        if(Cereg.Reg != 0)
        {
            Reg = Cereg.Reg;
            Cid = Cereg.Cid;
            Lac = Cereg.Lac;
        }

    }

    /*
     * Only fill the following 2 null initialized fields
     * if there is something relevant to report
     */
    if(Lac != -1)
    {
        asprintf(&responseStr[1], "%x", Lac);
    }

    if(Cid != -1)
    {
        asprintf(&responseStr[2], "%x", Cid);
    }

    /*
     * Workout if emergency calls are possible:
     * Rat indication but not registered
     */
    if((request == RIL_REQUEST_VOICE_REGISTRATION_STATE)&&
     ((strcmp(NwState.rat,"0")!=0)&&(IsRegistered()==0)))
    {
        /* Need to indicate that emergency calls are available */
        Reg+=REG_STATE_EMERGENCY;
    }
    asprintf(&responseStr[0], "%d", Reg);

    CurrentTechno = accessTechToRat(NwState.rat, request);

#if RIL_VERSION >= 6
    /* See if we can find an HPSA+ bearer */
    ConnectionState = accessTechToRat(NwState.connState, request);
    /*
     * Rat type are sorted, so this test is sufficient to report
     * the best supported RAT
     */
    if(ConnectionState > CurrentTechno)
        CurrentTechno = ConnectionState;
#endif

    asprintf(&responseStr[3], "%d", (int)CurrentTechno);

#if (RIL_VERSION >= 6)
    /*
     * Ril v6 takes registration failure cause but the info is located
     * in different places depending on the request
     */

    if((Reg == REG_STATE_REJECTED)
     ||(Reg == (REG_STATE_REJECTED+REG_STATE_EMERGENCY)))
    {
        int NwRejCause=0, index;
        if(request == RIL_REQUEST_DATA_REGISTRATION_STATE)
        {
            err = at_send_command_singleline("AT%IER?", "%IER", &p_response);
        }
        else
        {
            err = at_send_command_singleline("AT%IVCER?", "%IVCER:", &p_response);
            ResponseSize = 13;
        }
        if((err < 0)||(p_response->success == 0))
            goto error;
        line = p_response->p_intermediates->line;

        err = at_tok_start(&line);
        if (err < 0) goto error;

        err = at_tok_nextint(&line, &NwRejCause);

        /*
         *  Framework is expecting 24.008 coded cause, so we can transmit as is,
         * note that the info is located in different places in each request
         */
        index = (request == RIL_REQUEST_DATA_REGISTRATION_STATE)?4:13;
        asprintf(&responseStr[index],"%d",NwRejCause);
    }

    /* Ril v6 takes #number of concurent profiles */
    if(request == RIL_REQUEST_DATA_REGISTRATION_STATE)
    {
        ResponseSize = 6;
        asprintf(&responseStr[5],"%d",GetNmbConcurentProfile());
    }
#endif

    RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, ResponseSize*sizeof(char*));

    goto finally;
error:
    ALOGE("requestRegistrationState must never return an error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);

finally:
    /* Generic clean up */
    at_response_free(p_response);

    for(i =0; i<(sizeof(responseStr)/sizeof(char*)); i++)
    {
        free(responseStr[i]);
    }
}

/*
 * Returns the latest technology in use
 */

int GetCurrentTechno(void)
{
    return CurrentTechno;
}

/*
 * Returns the current rat mode (RIL encoding) after querying it from the modem.
 * returns -1 if it can't figure it out
 */
void GetCurrentNetworkMode(int* mode)
{
    int err,nwmode_option,prio;
    ATResponse *p_response = NULL;
    char * line,*current_mode;
    /*
     * Get the current mode
     */
    err = at_send_command_multiline ("AT%INWMODE?", "%INWMODE:", &p_response);

    if (err != 0 || p_response->success == 0) {
        ALOGE("Looks like %%nwmode is not supported by modem FW!");
        goto error;
    }
    /* only interrested in the first line. */
    line = p_response->p_intermediates->line;

    /* expected line: %INWMODE:0,<domain>,<prio> */

    /* skip prefix */
    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    /* get current mode, has to be 0 on the first line */
    err = at_tok_nextint(&line, &nwmode_option);
    if ((err < 0)||(nwmode_option != 0))
        goto error;

    /* Get the RAT */
    err = at_tok_nextstr(&line, &current_mode);
    if (err < 0)
        goto error;
   /* get current prio */
    err = at_tok_nextint(&line, &prio);
    if (err < 0)
        goto error;

    if((strcmp(current_mode,"UG")==0)||(strcmp(current_mode,"GU")==0))
    {
        /*
         * dual mode WCDMA no pref: dual mode WCMDA prefered
         * (dual mode GSM preferred is not supported in Android but
         * unprioritized may be reported as UG or GU)
         */
        *mode=(prio==0)?PREF_NET_TYPE_GSM_WCDMA_AUTO:PREF_NET_TYPE_GSM_WCDMA;
    }else if(strcmp(current_mode,"G") == 0)
    {
        *mode =PREF_NET_TYPE_GSM_ONLY; /* GSM only */
    }else if(strcmp(current_mode,"U") == 0)
    {
        *mode =PREF_NET_TYPE_WCDMA; /* WCDMA only */
    }
    else if(strcmp(current_mode,"EUG") == 0)
    {
        *mode =PREF_NET_TYPE_LTE_GSM_WCDMA;
    }
    else if(strcmp(current_mode,"E") == 0)
    {
        *mode =PREF_NET_TYPE_LTE_ONLY;
    }
    else
    {
        goto error; /* Don't know ! */
    }

    return;

error:
    *mode = -1;
    at_response_free(p_response);
}

/*
 * Obtain the current network selection mode
 * return -1 if it can't be obtained.
 */
static int GetCurrentNetworkSelectionMode(void)
{
    int err;
    ATResponse *p_response = NULL;
    int response = -1;
    char *line;

    if(NetworkSelectionMode == -1)
    {

        err = at_send_command_singleline("AT+COPS?", "+COPS:", &p_response);

        if (err < 0 || p_response->success == 0) {
            goto error;
        }

        line = p_response->p_intermediates->line;

        err = at_tok_start(&line);

        if (err < 0) {
            goto error;
        }

        err = at_tok_nextint(&line, &NetworkSelectionMode);
    }
    else
    {
        ALOGD("Providing cached network selection details");
    }


    response = NetworkSelectionMode;

    error:
    at_response_free(p_response);
    return response;
}

void requestOperator(void *data, size_t datalen, RIL_Token t)
{
    int err;
    int i;
    int skip;
    ATLine *p_cur;
    char *response[3];
    ATResponse *p_response = NULL;

    memset(response, 0, sizeof(response));

    if(CopsState.valid == 0)
    {
        char* value = NULL;


        err = at_send_command_multiline(
            "AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?",
            "+COPS:", &p_response);

        /* we expect 3 lines here:
         * +COPS: 0,0,"T - Mobile"
         * +COPS: 0,1,"TMO"
         * +COPS: 0,2,"310170"
         */

        if ((err != 0) ||(p_response->success==0)) goto error;

        for (i = 0, p_cur = p_response->p_intermediates
                ; p_cur != NULL
                ; p_cur = p_cur->p_next, i++) {
            char *line = p_cur->line;

            err = at_tok_start(&line);
            if (err < 0) goto error;

            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;

            // If we're unregistered, we may just get
            // a "+COPS: 0" response
            if (!at_tok_hasmore(&line)) {
                CopsState.networkOp[i] = NULL;
                continue;
            }

            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;

            // a "+COPS: 0, n" response is also possible
            if (!at_tok_hasmore(&line)) {
                CopsState.networkOp[i] = NULL;
                continue;
            }

            err = at_tok_nextstr(&line, &value);
            if (err < 0) goto error;

            CopsState.networkOp[i] = strdup(value);
        }

        if (i != 3) {
            /* expect 3 lines exactly */
            goto error;
        }
        CopsState.valid = 1;
    }
    else
    {
        ALOGD("Providing cached Operator details");
    }

    for(i=0;i<3;i++)
    {
        response[i] = CopsState.networkOp[i];
    }
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));

    return;
error:
    ALOGE("requestOperator must not return error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

void requestSetPreferredNetworkType(void *data, size_t datalen, RIL_Token t)
{
    int err, requested_prio = 0, current_mode,
        Result = RIL_E_GENERIC_FAILURE, cfun_state = 0;
    char *line = NULL;
    char * cmd = NULL;
    char *requested_rat;
    ATResponse *p_response = NULL;

    assert (datalen >= sizeof(int *));
    int requested_mode = ((int *)data)[0];

    /* CDMA/EVDO modes aren't supported, return an error immediately */
    if((requested_mode >3) && (requested_mode < 7))
    {
        ALOGE("requestSetPreferredNetworkType: unsupported mode (%d)", requested_mode);
        Result = RIL_E_MODE_NOT_SUPPORTED;
        goto error;
    }

    /* Get the current mode from the modem to apply the change only if necessary */
    GetCurrentNetworkMode(&current_mode);

    if (current_mode != requested_mode)
    {
#ifndef LTE_TEST
        switch (requested_mode)
        {
            case PREF_NET_TYPE_GSM_WCDMA:
                requested_rat = "UG";
                requested_prio = 1;
                break;
            case PREF_NET_TYPE_GSM_ONLY:
                requested_rat = "G";
                break;
            case PREF_NET_TYPE_WCDMA:
                requested_rat = "U";
                break;
            case PREF_NET_TYPE_GSM_WCDMA_AUTO:
            case PREF_NET_TYPE_GSM_WCDMA_CDMA_EVDO_AUTO:
                requested_rat = "UG";
                break;
            case PREF_NET_TYPE_LTE_CDMA_EVDO:
            case PREF_NET_TYPE_LTE_ONLY:
                requested_rat = "E";
                break;
            case PREF_NET_TYPE_LTE_CMDA_EVDO_GSM_WCDMA:
            case PREF_NET_TYPE_LTE_GSM_WCDMA:
                requested_rat = "EUG";
                break;
            default: goto error; break; /* Should never happen */
        }
#else
        /* Force RAT to LTE only in LTE_TEST mode*/
        requested_rat = "E";
#endif
        /*
         * We may need to drop a potential call,
         * cfun=4 will do the trick and doesn't involve the
         * potential delay of +cgatt. Do it only if necessary,
         * so we preserve the radio state for airplane mode
         */
        err = at_send_command_singleline("AT+CFUN?","+CFUN:", &p_response);
        if (err != 0 || p_response->success == 0) {
            goto error;
        }

        line = p_response->p_intermediates->line;
        /* skip prefix */
        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &cfun_state);

        if(err < 0)
            goto error;

        if(cfun_state == 1)
        {
            at_send_command("AT+CFUN=4", NULL);
        }

        asprintf(&cmd, "AT%%INWMODE=0,%s,%d", requested_rat, requested_prio);
        err = at_send_command(cmd, NULL);
        free(cmd);

        /* Register on the NW again if needed */
        if(cfun_state == 1)
        {
            at_send_command("AT+CFUN=1", NULL);
        }

        if (err < 0)
            goto error;

        at_response_free(p_response);
    }
    else
    {
        ALOGD("Already in requested mode: No change");
    }
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
    ALOGE("ERROR: requestSetPreferredNetworkType() failed\n");
    at_response_free(p_response);
    RIL_onRequestComplete(t, Result, NULL, 0);
}

void requestGetPreferredNetworkType(void *data, size_t datalen, RIL_Token t)
{
    int current_mode;
    GetCurrentNetworkMode(&current_mode);
    if(current_mode != -1)
    {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, &current_mode, sizeof(int));
        return;
    }
    else
    {
        ALOGE("ERROR: requestGetPreferredNetworkType() failed\n");
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

/**
 * RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION
 *
 * Requests that network personlization be deactivated
 *
 * "data" is const char **
 * ((const char **)(data))[0]] is network depersonlization code
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
 *     (code is invalid)
 */
void requestEnterNetworkDepersonalization(void *data, size_t datalen,
                                          RIL_Token t)
{
    /*
     * AT+CLCK=<fac>,<mode>[,<passwd>[,<class>]]
     *     <fac>    = "PN" = Network Personalization (refer 3GPP TS 22.022)
     *     <mode>   = 0 = Unlock
     *     <passwd> = inparam from upper layer
     */

    int err = 0;
    char *cmd = NULL;
    ATResponse *p_response = NULL;
    const char *passwd = ((const char **) data)[0];
    RIL_Errno rilerr = RIL_E_GENERIC_FAILURE;
    int num_retries = -1;

    /* Check inparameter. */
    if (passwd == NULL) {
        goto error;
    }
    /* Build and send command. */
    asprintf(&cmd, "AT+CLCK=\"PN\",0,\"%s\"", passwd);
    err = at_send_command(cmd, &p_response);

    free(cmd);

    if (err < 0 || p_response->success == 0)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &num_retries, sizeof(int));
    at_response_free(p_response);
    return;

error:
    if (p_response && at_get_cme_error(p_response) == CME_ERROR_INCORRECT_PASSWORD) {
        char *line;
        rilerr = RIL_E_PASSWORD_INCORRECT;

        /* Get the number of remaining attempts */
        at_response_free(p_response);
        p_response = NULL;
        err = at_send_command_singleline("AT%ILOCKNUM=\"PN\",2","%ILOCKNUM:", &p_response);

        if (!(err < 0 || p_response->success == 0))
        {
            line = p_response->p_intermediates->line;
            err = at_tok_start(&line);
            if (err >= 0)
                err = at_tok_nextint(&line, &num_retries);
            if(err <0)
                rilerr = RIL_E_GENERIC_FAILURE;
        }
    } else {
        ALOGE("ERROR: requestEnterNetworkDepersonalization failed\n");
    }
    at_response_free(p_response);
    RIL_onRequestComplete(t,
                                                     rilerr,
                                                     (rilerr==RIL_E_PASSWORD_INCORRECT)?&num_retries:NULL,
                                                     (rilerr==RIL_E_PASSWORD_INCORRECT)?sizeof(int):0);
}

/**
 * RIL_REQUEST_SET_LOCATION_UPDATES
 *
 * Enables/disables network state change notifications due to changes in
 * LAC and/or CID (for GSM) or BID/SID/NID/latitude/longitude (for CDMA).
 * Basically +CREG=2 vs. +CREG=1 (TS 27.007).
 *
 * Note:  The RIL implementation should default to "updates enabled"
 * when the screen is on and "updates disabled" when the screen is off.
 *
 * "data" is int *
 * ((int *)data)[0] is == 1 for updates enabled (+CREG=2)
 * ((int *)data)[0] is == 0 for updates disabled (+CREG=1)
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_REQUEST_SCREEN_STATE, RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED
 */
void requestSetLocationUpdates(void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    char *cmd = NULL;
    ATResponse *p_response = NULL;

    /* Update the global for future screen_state transitions */
    LocUpdateEnabled = ((int *)data)[0];

    if(LocUpdateEnabled)
    {
        at_send_command(LteSupported?"AT+CREG=2;+CGREG=2;+CEREG=2":"AT+CREG=2;+CGREG=2",&p_response);
    }
    else
    {
        at_send_command(LteSupported?"AT+CREG=1;+CGREG=1;+CEREG=1":"AT+CREG=1;+CGREG=1",&p_response);
    }

    if(err < 0 || p_response->success == 0) goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;

error:
    at_response_free(p_response);
    ALOGE("ERROR: requestSetLocationUpdates failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * RIL_REQUEST_GET_NEIGHBORING_CELL_IDS
 *
 * Request neighboring cell id in GSM network
 *
 * "data" is NULL
 * "response" must be a " const RIL_NeighboringCell** "
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
void requestGetNeighboringCellIds(void *data, size_t datalen, RIL_Token t)
{
    int err = 0;
    ATResponse *p_response = NULL;
    ATLine *p_cur;
    RIL_NeighboringCell **all_pp_cells = NULL;
    RIL_NeighboringCell **pp_cells;
    RIL_NeighboringCell *p_cells;
    char *line;
    int type;
    int all_cells_number = 0;
    int cells_number;
    int cell_id;
    int i;

    err = at_send_command_multiline("AT%INCELL", "%INCELL:", &p_response);

    if (err != 0 || p_response->success == 0) goto error;

    /* %INCELL: <type>,<nbCells>,<cellId1>,<SignalLevel1>,...,<cellIdN>,<SignalLevelN> */

    for (p_cur = p_response->p_intermediates; p_cur != NULL; p_cur = p_cur->p_next) {
        line = p_cur->line;

        err = at_tok_start(&line);
        if (err < 0) goto error;

        err = at_tok_nextint(&line, &type);
        if (err < 0) goto error;

        if (type == 2) continue; /* to ignore interRat GSM cells */

        err = at_tok_nextint(&line, &cells_number);
        if (err < 0) goto error;

        if (cells_number > 0) {
            p_cells = (RIL_NeighboringCell *)alloca(cells_number * sizeof(RIL_NeighboringCell));
            pp_cells = (RIL_NeighboringCell **)alloca((all_cells_number + cells_number) * sizeof(RIL_NeighboringCell *));

            if (all_cells_number > 0) {
                memcpy(pp_cells, all_pp_cells, all_cells_number * sizeof(RIL_NeighboringCell *));
            }

            for (i = 0; i < cells_number; i++) {
                err = at_tok_nextint(&line, &cell_id);
                if (err < 0) goto error;

                asprintf(&p_cells[i].cid, "%x", cell_id);

                err = at_tok_nextint(&line, &p_cells[i].rssi);
                if (err < 0) goto error;

                pp_cells[all_cells_number + i] = &p_cells[i];
            }

            all_cells_number += cells_number;
            all_pp_cells = pp_cells;
        }
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, all_pp_cells, all_cells_number * sizeof(RIL_NeighboringCell *));
    goto finally;

error:

    ALOGE("ERROR: requestGetNeighboringCellIds failed\n");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);

finally:
    at_response_free(p_response);
    while (all_cells_number>0)
    {
        all_cells_number --;
        free(all_pp_cells[all_cells_number]->cid);
    }
}

/**
 * RIL_UNSOL_RESTRICTED_STATE_CHANGED
 *
 * Indicates a restricted state change (eg, for Domain Specific Access Control).
 *
 * Radio need send this msg after radio off/on cycle no matter it is changed or not.
 *
 * "data" is an int *
 * ((int *)data)[0] contains a bitmask of RIL_RESTRICTED_STATE_* values.
 */
void unsolRestrictedStateChanged(void)
{
    int response = 0;

    if((Cgreg.Reg != 1)
     &&(Cgreg.Reg != 5)
     &&(Cereg.Reg != 1)
     &&(Cereg.Reg != 5)) {
        response |= RIL_RESTRICTED_STATE_PS_ALL;
    }

    if(!IsRegistered())
    {
        if(strcmp(NwState.rat,"0")&&(!IsRegistered())){
            response |= RIL_RESTRICTED_STATE_CS_NORMAL;
        }else{
            response |= RIL_RESTRICTED_STATE_CS_ALL;
        }
    }

    RIL_onUnsolicitedResponse(RIL_UNSOL_RESTRICTED_STATE_CHANGED,
                              &response, sizeof(response));
    return;
}

/**
 * RIL_UNSOL_NITZ_TIME_RECEIVED
 *
 * Called when radio has received a NITZ time message.
 *
 * "data" is const char * pointing to NITZ time string
 * in the form "yy/mm/dd,hh:mm:ss(+/-)tz,dt"
*/
void unsolNetworkTimeReceived(const char *data)
{
    int err;
    ATResponse *p_response = NULL;
    char *line = (char *)data;

    /* expected line: *TLTS: "yy/mm/dd,hh:mm:ss+/-tz", without "dt" */

    /* skip prefix */
    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    /* Remove spaces and " around the useful string*/
    err =  at_tok_nextstr(&line, &line);
    if (err < 0)
        goto error;

    /* Skip the report to Android if the string is empty (meaning that the modem has no NITZ info available)*/
    if(strcmp(line,"") != 0)
    {
        RIL_onUnsolicitedResponse(RIL_UNSOL_NITZ_TIME_RECEIVED,
                                  line, strlen(line));
    }

    return;
error:
    ALOGE("ERROR: unsolNetworkTimeReceived - invalid time string\n");
    return;
}

void requestSignalStrength(void *data, size_t datalen, RIL_Token t)
{
    if(t)
    {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, signalStrength, ResponseSize);
    }
    else
    {
        RIL_onUnsolicitedResponse(RIL_UNSOL_SIGNAL_STRENGTH, signalStrength, ResponseSize);
    }
}

void unsolSignalStrength(const char *s)
{
    char * p;
    int err;
    int rssi, ber;
    char *line;
    /*
     * Cache the values so they don't need to be requested from the modem again
     */
    line = strdup(s);
    p = line;
    memset(signalStrength,-1,ResponseSize);

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &rssi);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &ber);
    if (err < 0) goto error;

#if RIL_VERSION <= 4
    signalStrength[0] = rssi;
    signalStrength[1] = ber;
#else
    switch(GetCurrentTechno())
    {
#ifdef LTE_SPECIFIC_SIGNAL_REPORT
        /*
         * As of JB MR1, LTE signal level reporting is broken in the framework
         * unless you can report rsnr or rsrp. LTE rrsi is ignored. Defaulting
         * to GSM level should work just fine.
         */
        case RADIO_TECH_LTE:
            signalStrength->GW_SignalStrength.signalStrength = 99;
            signalStrength->GW_SignalStrength.bitErrorRate = 0;
            signalStrength->LTE_SignalStrength.cqi = 0;
            signalStrength->LTE_SignalStrength.rsrp = 0;
            signalStrength->LTE_SignalStrength.rsrq = 0;
            signalStrength->LTE_SignalStrength.rssnr = 0;
            signalStrength->LTE_SignalStrength.signalStrength = rssi;
            break;
#endif
        default:
            signalStrength->GW_SignalStrength.signalStrength = rssi;
            signalStrength->GW_SignalStrength.bitErrorRate = ber;
            signalStrength->LTE_SignalStrength.cqi = 0;
            signalStrength->LTE_SignalStrength.rsrp = -140;
            signalStrength->LTE_SignalStrength.rsrq = 20;
            signalStrength->LTE_SignalStrength.rssnr = -200;
            signalStrength->LTE_SignalStrength.signalStrength = 99;
            break;
    }
#endif
    /* Send unsolicited to framework */
    requestSignalStrength(NULL, 0, 0);

    error:
    free(p);
}

static const char* networkStatusToRilString(int state)
{
    switch(state){
        case 0: return("unknown");   break;
        case 1: return("available"); break;
        case 2: return("current");   break;
        case 3: return("forbidden"); break;
        default: return NULL;
    }
}

static void parsePlusCxREG(char *line, int *p_RegState, int* p_Lac,int * p_CId)
{
    int Lac= -1, CId =-1, RegState = -1, skip, err, commas, unsol;
    char * p;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    /*
     * The solicited version of the CREG response is
     * +CREG: n, stat, [lac, cid]
     * and the unsolicited version is
     * +CREG: stat, [lac, cid]
     * The <n> parameter is basically "is unsolicited creg on?"
     * which it should always be
     *
     * Now we should normally get the solicited version here,
     * but the unsolicited version could have snuck in
     * so we have to handle both
     *
     * Also since the LAC and CID are only reported when registered,
     * we can have 1, 2, 3, or 4 arguments here
     *
     * a +CGREG: answer may have a fifth value that corresponds
     * to the network type, as in;
     *
     *   +CGREG: n, stat [,lac, cid [,networkType]]
     *  This network type is not taken from the +CGREG response, but taken
     *  from the  AT%NWSTATE
     *
     *  a +CGREG: answer may have a sixth value which is ignored
     *
     *  Same for +CEREG
     */

    /* count number of commas
     * and determine if it is sollicited or not
     * lac parameter comes surronded by " " - Check its position
     * after 1st comma means unsollicited otherwise it is a sollicited format
     */

    commas = 0;
    unsol = 0;
    for (p = line ; *p != '\0' ;p++) {
        if (*p == ',') commas++;
        if((commas == 1)&&(*p=='"')) unsol=1;
    }

    switch (commas) {
        /* Unsolicited, no locupdate */
        case 0: /* +CREG: <stat> */
            err = at_tok_nextint(&line, &RegState);
            if (err < 0) goto error;
        break;

        /* Solicited */
        case 1: /* +CREG: <n>, <stat> */
            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &RegState);
            if (err < 0) goto error;
        break;

        /* Unsolicited */
        default:
            if(!unsol)
            {
                err = at_tok_nextint(&line, &skip);
                if (err < 0) goto error;
            }
            err = at_tok_nextint(&line, &RegState);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &Lac);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &CId);
            if (err < 0) goto error;
        break;
    }

    error:
    if(p_RegState!=NULL)
        *p_RegState = RegState;
    if(p_Lac!=NULL)
        *p_Lac = Lac;
    if(p_CId != NULL)
        *p_CId = CId;

}

static int accessTechToRat(char *rat, int request)
{
    unsigned int i;
    typedef struct _RatDesc{
        const char * Rat;
        const int PsRadioTech;
        const int CsRadioTech;
    }RatDesc;

    const RatDesc RatDescList[]=
    /* Rat,             PS eq,              CS eq */
    {
    {"0",               RADIO_TECH_UNKNOWN, RADIO_TECH_UNKNOWN},
    {"2g-gprs",         RADIO_TECH_UNKNOWN, RADIO_TECH_GPRS},
    {"2g-edge",         RADIO_TECH_UNKNOWN, RADIO_TECH_EDGE},
    {"(2G-GPRS)",       RADIO_TECH_GPRS,    RADIO_TECH_UNKNOWN},
    {"(2G-EDGE)",       RADIO_TECH_EDGE,    RADIO_TECH_UNKNOWN},
    {"2G-GPRS",         RADIO_TECH_GPRS,    RADIO_TECH_GPRS},
    {"2G-EDGE",         RADIO_TECH_EDGE,    RADIO_TECH_EDGE},
    {"3g",              RADIO_TECH_UNKNOWN, RADIO_TECH_UMTS},
    {"3g-hsdpa",        RADIO_TECH_UNKNOWN, RADIO_TECH_HSDPA},
    {"3g-hsupa",        RADIO_TECH_UNKNOWN, RADIO_TECH_HSUPA},
    {"3g-hsdpa-hsupa",  RADIO_TECH_UNKNOWN, RADIO_TECH_HSPA},
    {"(3G)",            RADIO_TECH_UMTS,    RADIO_TECH_UNKNOWN},
    {"(3G-HSDPA)",      RADIO_TECH_HSDPA,   RADIO_TECH_UNKNOWN},
    {"(3G-HSUPA)",      RADIO_TECH_HSUPA,   RADIO_TECH_UNKNOWN},
    {"(3G-HSDPA-HSUPA)",RADIO_TECH_HSPA,    RADIO_TECH_UNKNOWN},
    {"3G",              RADIO_TECH_UMTS,    RADIO_TECH_UMTS},
    {"3G-HSDPA",        RADIO_TECH_HSDPA,   RADIO_TECH_HSDPA},
    {"3G-HSUPA",        RADIO_TECH_HSUPA,   RADIO_TECH_HSUPA},
    {"3G-HSDPA-HSUPA",  RADIO_TECH_HSPA,    RADIO_TECH_HSPA},
    {"lte",             RADIO_TECH_UNKNOWN, RADIO_TECH_LTE},
    {"LTE",             RADIO_TECH_LTE,     RADIO_TECH_LTE},
    {"2g",              RADIO_TECH_UNKNOWN, RADIO_TECH_UNKNOWN},
    {"HSDPA-HSUPA-HSPA+",RADIO_TECH_HSPAP,  RADIO_TECH_HSPAP},
    };

    for(i=0; i<sizeof(RatDescList)/sizeof(RatDesc); i++)
    {
        if(strcmp(rat,RatDescList[i].Rat)==0)
        {
            return (request==RIL_REQUEST_VOICE_REGISTRATION_STATE)?
                RatDescList[i].CsRadioTech
                :RatDescList[i].PsRadioTech;
        }
    }
    return RADIO_TECH_UNKNOWN;
}

void parseCreg(const char *s)
{
    int Reg, Lac, Cid, OldReg;
    char * line = (char*) s;

    parsePlusCxREG(line, &Reg, &Lac, &Cid);

    OldReg = Creg.Reg;

    if((Reg != Creg.Reg)||(Lac != Creg.Lac)||(Cid != Creg.Cid))
    {
        Creg.Reg = Reg;
        Creg.Lac = Lac;
        Creg.Cid = Cid;

        /* Notify framework */
        RIL_onUnsolicitedResponse (
            RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
            NULL, 0);
        /* We'll refresh the operator next time we're asked for it */
        resetCachedOperator();

        /* If not searching, update the restricted state */
        if((Reg != OldReg)&&(Reg!=2))
        {
            unsolRestrictedStateChanged();
        }
    }
}

void parseCgreg(const char *s)
{
    int Reg, Lac, Cid, OldReg;
    char * line = (char*) s;

    parsePlusCxREG(line, &Reg, &Lac, &Cid);

    OldReg = Cgreg.Reg;

    if((Reg != Cgreg.Reg)||(Lac != Cgreg.Lac)||(Cid != Cgreg.Cid))
    {
        Cgreg.Reg = Reg;
        Cgreg.Lac = Lac;
        Cgreg.Cid = Cid;

#if RIL_VERSION <= 4
        /*
         * Only voice event are notified in ICS
         * Older versions also notify data events
         */
        RIL_onUnsolicitedResponse (
            RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
            NULL, 0);
#endif
        /* We'll refresh the operator next time we're asked for it */
        resetCachedOperator();

        /* If not searching, update the restricted state */
        if((Reg != OldReg)&&(Reg!=2))
        {
            unsolRestrictedStateChanged();
        }
    }
}

void parseCereg(const char *s)
{
    int Reg, Lac, Cid, OldReg;
    char * line = (char*) s;

    parsePlusCxREG(line, &Reg, &Lac, &Cid);

    OldReg = Cereg.Reg;

    if((Reg != Cereg.Reg)||(Lac != Cereg.Lac)||(Cid != Cereg.Cid))
    {
        Cereg.Reg = Reg;
        Cereg.Lac = Lac;
        Cereg.Cid = Cid;

#if RIL_VERSION <= 4
        /*
         * Only voice event are notified in ICS
         * Older versions also notify data events
         */
        RIL_onUnsolicitedResponse (
            RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
            NULL, 0);
#endif
        /* We'll refresh the operator next time we're asked for it */
        resetCachedOperator();

        /* If not searching, update the restricted state */
        if((Reg != OldReg)&&(Reg!=2))
        {
            unsolRestrictedStateChanged();
        }
    }
}

void parseNwstate(const char *s)
{
    int err;
    int emergencyOrSearching, mccMnc;
    char *rat, *connState;
    char * line = strdup((char*)s);
    char *p = line;
    err = at_tok_start(&line);

    err = at_tok_nextint(&line, &emergencyOrSearching);
    if (err < 0) goto error;
    err = at_tok_nextint(&line, &mccMnc);
    if (err < 0) goto error;
    err = at_tok_nextstr(&line, &rat);
    if (err < 0) goto error;
    err = at_tok_nextstr(&line, &connState);
    if (err < 0) goto error;

    if((emergencyOrSearching != NwState.emergencyOrSearching)||
      (mccMnc != NwState.mccMnc)||
      (strcmp(rat, NwState.rat))||
      (strcmp(connState,NwState.connState)))
    {
        /* Notify a change of network status */
        RIL_onUnsolicitedResponse (
            RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
            NULL, 0);

        /* Cache the info, Framework will request them later */
        free(NwState.connState);
        free(NwState.rat);

        NwState.emergencyOrSearching = emergencyOrSearching;
        NwState.mccMnc = mccMnc;
        NwState.rat = strdup(rat);
        NwState.connState = strdup(connState);
    }
    free(p);
    return;
    error:
        free(p);
        ALOGE("Error parsing nwstate");
}

void parseIcti(const char *s)
{
    char * line = strdup((char*)s);
    char *p = line;
    char *rat;
    int err;
    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextstr(&line, &rat);
    if (err < 0) goto error;

    if(strcmp(rat, NwState.rat))
    {
        /* Notify a change of network status */
        RIL_onUnsolicitedResponse (
            RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
            NULL, 0);

        free(NwState.rat);
        NwState.rat = strdup(rat);
    }
    error:
    free(p);
}
void InitNetworkCacheState(void)
{
    int i;

    NwState.connState = strdup("");
    NwState.rat = strdup("");
    NwState.emergencyOrSearching = 0;
    NwState.mccMnc = -1;
    NetworkSelectionMode = -1;

    Creg.Cid = -1;
    Creg.Lac = -1;
    Creg.Reg = -1;

    Cgreg.Cid = -1;
    Cgreg.Lac = -1;
    Cgreg.Reg = -1;

    Cereg.Cid = -1;
    Cereg.Lac = -1;
    Cereg.Reg = -1;

    CopsState.valid = 0;
    for(i=0;i<3;i++)
    {
        CopsState.networkOp[i] = NULL;
    }
}

static void resetCachedOperator(void)
{
    int i;
    CopsState.valid = 0;
    for(i = 0;i<3;i++)
    {
        free(CopsState.networkOp[i]);
        CopsState.networkOp[i] = NULL;
    }
}

int IsRegistered(void)
{
    return  (Cereg.Reg == 1)||(Cereg.Reg == 5)||
            (Creg.Reg == 1)||(Creg.Reg == 5)||
            (Cgreg.Reg == 1)||(Cgreg.Reg == 5);
}

void InitializeNetworkUnsols(int defaultLocUp)
{
    int err;
    ATResponse *p_response = NULL;

    /*  check if LTE is supported */
    err = at_send_command("AT+CEREG?", &p_response);

    /* Check if LTE supporting FW */
    if (err != 0 || p_response->success == 0)
    {
        LteSupported = 0;
        /* State will always be not searching for LTE */
        Cereg.Reg = 0;
    }
    else
    {
        LteSupported = 1;
    }
    at_response_free(p_response);

    LocUpdateEnabled = defaultLocUp;
    
    /* Initialize unsolicited and force first update with solicited request */
    if(defaultLocUp==0)
    {
        at_send_command(LteSupported?"AT+CREG=1;+CGREG=1;+CEREG=1;+CREG?;+CGREG?;+CEREG?":"AT+CREG=1;+CGREG=1;+CREG?;+CGREG?",NULL);
    }
    else
    {
        at_send_command(LteSupported?"AT+CREG=2;+CGREG=2;+CEREG=2;+CREG?;+CGREG?;+CEREG?":"AT+CREG=2;+CGREG=2;+CREG?;+CGREG?",NULL);
    }
}

int GetLocupdateState(void)
{
    return LocUpdateEnabled;
}

int GetLteSupported(void)
{
    return LteSupported;
}
