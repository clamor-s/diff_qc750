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

#ifndef ICERA_RIL_H
#define ICERA_RIL_H 1
#include <telephony/ril.h>

/* use Icera commands where no standard command exists */
#define USE_ICERA_COMMANDS

/**
 * Name of the wake lock we will request
 */
#define ICERA_RIL_WAKE_LOCK_NAME ("icera-libril")


#define RIL_REQUEST_OEM_HOOK_SHUTDOWN_RADIO_POWER  0
#define RIL_REQUEST_OEM_HOOK_ACOUSTIC_PROFILE      1
#define RIL_REQUEST_OEM_HOOK_EXPLICIT_ROUTE_BY_RIL 2
#define RIL_REQUEST_OEM_HOOK_EXPLICIT_DATA_ABORT   3

/* only used for legacy device (Cello)
 * extends RIL_RadioState in ril.h*/
/* Modem is powered off */
#define RADIO_STATE_MODEM_OFF  10

#define RIL_REQUEST_SHUTDOWN_RADIO_POWER 105
#define RIL_REQUEST_ACOUSTIC_PROFILE 106
/* End legacy device support */

#define MAX_NETWORK_IF 4

//#define ALWAYS_SET_MODEM_STATE_PROPERTY

#if (RIL_VERSION > 7) || (RIL_VERSION < 3)
    #error Version not supported
#elif (RIL_VERSION <= 4)
    #define ANNOUNCED_RIL_VERSION RIL_VERSION

    #ifdef MULTI_PDP_GB
        #define RIL_E_NO_NET_INTERFACE -1
    #endif

        /* These requests have been renamed */
        #define  RIL_REQUEST_VOICE_REGISTRATION_STATE RIL_REQUEST_REGISTRATION_STATE
        #define  RIL_REQUEST_DATA_REGISTRATION_STATE RIL_REQUEST_GPRS_REGISTRATION_STATE
        #define  RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED

        #define PDP_FAIL_NONE 0
        /* These enum are only available in the later RIL versions but
         * they are backward compatible so we use them in earlier versions
         *  as well
         */
        typedef enum {
            RADIO_TECH_UNKNOWN = 0,
            RADIO_TECH_GPRS = 1,
            RADIO_TECH_EDGE = 2,
            RADIO_TECH_UMTS = 3,
            RADIO_TECH_IS95A = 4,
            RADIO_TECH_IS95B = 5,
            RADIO_TECH_1xRTT =  6,
            RADIO_TECH_EVDO_0 = 7,
            RADIO_TECH_EVDO_A = 8,
            RADIO_TECH_HSDPA = 9,
            RADIO_TECH_HSUPA = 10,
            RADIO_TECH_HSPA = 11,
            RADIO_TECH_EVDO_B = 12,
            RADIO_TECH_EHRPD = 13,
            RADIO_TECH_LTE = 14,
            RADIO_TECH_HSPAP = 15 // HSPA+
        } RIL_RadioTechnology;

        typedef enum {
            PREF_NET_TYPE_GSM_WCDMA                = 0, /* GSM/WCDMA (WCDMA preferred) */
            PREF_NET_TYPE_GSM_ONLY                 = 1, /* GSM only */
            PREF_NET_TYPE_WCDMA                    = 2, /* WCDMA  */
            PREF_NET_TYPE_GSM_WCDMA_AUTO           = 3, /* GSM/WCDMA (auto mode, according to PRL) */
            PREF_NET_TYPE_CDMA_EVDO_AUTO           = 4, /* CDMA and EvDo (auto mode, according to PRL) */
            PREF_NET_TYPE_CDMA_ONLY                = 5, /* CDMA only */
            PREF_NET_TYPE_EVDO_ONLY                = 6, /* EvDo only */
            PREF_NET_TYPE_GSM_WCDMA_CDMA_EVDO_AUTO = 7, /* GSM/WCDMA, CDMA, and EvDo (auto mode, according to PRL) */
            PREF_NET_TYPE_LTE_CDMA_EVDO            = 8, /* LTE, CDMA and EvDo */
            PREF_NET_TYPE_LTE_GSM_WCDMA            = 9, /* LTE, GSM/WCDMA */
            PREF_NET_TYPE_LTE_CMDA_EVDO_GSM_WCDMA  = 10, /* LTE, CDMA, EvDo, GSM/WCDMA */
            PREF_NET_TYPE_LTE_ONLY                 = 11  /* LTE only */
        } RIL_PreferredNetworkType;

        typedef struct {
            int             cid;        /* Context ID, uniquely identifies this call */
            int             active;     /* 0=inactive, 1=active/physical link down, 2=active/physical link up */
            char *          type;       /* One of the PDP_type values in TS 27.007 section 10.1.1.
                                            For example, "IP", "IPV6", "IPV4V6", or "PPP". */
            char *          apn;        /* ignored */
            char *          address;    /* An address, e.g., "192.0.1.3" or "2001:db8::1". */
        } RIL_Data_Call_Response_v4;
        typedef struct {
            int             status;     /* A RIL_DataCallFailCause, 0 which is PDP_FAIL_NONE if no error */
            int             cid;        /* Context ID, uniquely identifies this call */
            int             active;     /* 0=inactive, 1=active/physical link down, 2=active/physical link up */
            char *          type;       /* One of the PDP_type values in TS 27.007 section 10.1.1.
                                   For example, "IP", "IPV6", "IPV4V6", or "PPP". If status is
                                   PDP_FAIL_ONLY_SINGLE_BEARER_ALLOWED this is the type supported
                                   such as "IP" or "IPV6" */
            char *          ifname;     /* The network interface name */
            char *          addresses;  /* A space-delimited list of addresses with optional "/" prefix length,
                                   e.g., "192.0.1.3" or "192.0.1.11/16 2001:db8::1/64".
                                   May not be empty, typically 1 IPv4 or 1 IPv6 or
                                   one of each. If the prefix length is absent the addresses
                                   are assumed to be point to point with IPv4 having a prefix
                                   length of 32 and IPv6 128. */
            char *          dnses;      /* A space-delimited list of DNS server addresses,
                                   e.g., "192.0.1.3" or "192.0.1.11 2001:db8::1".
                                   May be empty. */
            char *          gateways;   /* A space-delimited list of default gateway addresses,
                                   e.g., "192.0.1.3" or "192.0.1.11 2001:db8::1".
                                   May be empty in which case the addresses represent point
                                   to point connections. */
        } RIL_Data_Call_Response_v6;
#elif RIL_VERSION >= 6
    #define ANNOUNCED_RIL_VERSION 6

    #define RIL_SIM_IO RIL_SIM_IO_v6
    #define RIL_CardStatus RIL_CardStatus_v6
#endif


#define IRIL_REQUEST_ID_BASE               5000 /* internal communication e.g. for unsolicited events */
#define IRIL_REQUEST_ID_SIM_POLL           (IRIL_REQUEST_ID_BASE + 1) /* request SIM polling in AT Writer thread */
#define IRIL_REQUEST_ID_STK_PRO            (IRIL_REQUEST_ID_BASE + 2) /* request STK PRO commands in AT Writer thread */
#define IRIL_REQUEST_ID_NETWORK_TIME       (IRIL_REQUEST_ID_BASE + 3) /* DEPRECATED - request Network Time command in AT Writer thread */
#define IRIL_REQUEST_ID_RESTRICTED_STATE   (IRIL_REQUEST_ID_BASE + 4) /* request Restricted State Change in AT Writer thread */
#define IRIL_REQUEST_ID_PROCESS_DATACALL_EVENT    (IRIL_REQUEST_ID_BASE + 5) /* Process event during the datacall progress */
#define IRIL_REQUEST_ID_UPDATE_SIM_LOADED    (IRIL_REQUEST_ID_BASE + 6) /* Sim is fully loaded,  ie we're ready to update the ECC list for the framework */
#define IRIL_REQUEST_ID_SMS_ACK_TIMEOUT     (IRIL_REQUEST_ID_BASE + 7) /* SMS ack timeout */
#define IRIL_REQUEST_ID_FORCE_REATTACH   (IRIL_REQUEST_ID_BASE + 8) /* Reattach attempt */

//#define LTE_TEST

#ifdef RIL_SHLIB
const struct RIL_Env *s_rilenv;

#define RIL_onRequestComplete(t, e, response, responselen) s_rilenv->OnRequestComplete(t,e, response, responselen)
#define RIL_onUnsolicitedResponse(a,b,c) s_rilenv->OnUnsolicitedResponse(a,b,c)
#define RIL_requestTimedCallback(a,b,c) s_rilenv->RequestTimedCallback(a,b,c)
#endif

RIL_RadioState currentState(void);
void setRadioState(RIL_RadioState newState);
void requestPutObject(void *param);
void postDelayedRequest(void * req, int delayInSeconds);

const char * internalRequestToString(int request);

void SendStoredQueryCallForwardStatus(void);

int isRunningTestMode( void );
int useIcti(void);

#endif //ICERA_RIL_H

