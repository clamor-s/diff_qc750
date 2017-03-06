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
#include <errno.h>
#include "atchannel.h"
#include "at_tok.h"
#include <cutils/properties.h>

/* For manual IP address setup */
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/sockios.h>
//#include <linux/ipv6_route.h>

#include <linux/route.h>

#include <sys/socket.h>

#include "icera-ril.h"
#include "icera-ril-data.h"

#include "at_channel_writer.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>

#define PPP_CONNECTION_POLLING_PERIOD      100000 /*microsecs*/
#define PPP_CONNECTION_TIMEOUT_COUNTER   60000000 /*microsecs */

#define IPDPCFG_PAP 1
#define IPDPCFG_CHAP 2

#define MAX_CID_STRING_LENGTH 4

#define IPV4_NULL_ADDR "0.0.0.0"
/* Different modem report different null ipv6 address in different way */
#define IPV6_NULL_ADDR_1 "0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0"
#define IPV6_NULL_ADDR_2 "::"

/* includes have changed between kernel versions */
#if RIL_VERSION <= 4
    #include <../libnetutils/ifc_utils.h>
    #ifndef _LINUX_IPV6_ROUTE_H
struct in6_ifreq {
    struct in6_addr ifr6_addr;
    __u32           ifr6_prefixlen;
    int             ifr6_ifindex;
};
    #endif
#elif RIL_VERSION >= 6
    #include <netutils/ifc.h>
#endif

/* Defined in ipv6_route.h - re-defined here to avoid the maintenance of Bionic until this header is included in it. */
#ifndef _LINUX_IPV6_ROUTE_H
struct in6_rtmsg {
    struct in6_addr     rtmsg_dst;
    struct in6_addr     rtmsg_src;
    struct in6_addr     rtmsg_gateway;
    __u32                   rtmsg_type;
    __u16                   rtmsg_dst_len;
    __u16                   rtmsg_src_len;
    __u32                   rtmsg_metric;
    unsigned long           rtmsg_info;
    __u32                   rtmsg_flags;
    int                     rtmsg_ifindex;
};
#endif

static void requestOrSendDataCallList(RIL_Token *t);
/*
 * Global variable to be able to send the response once the ip address
 * has been obtained and set
 */

static RIL_Token *SetupDataCall_token = NULL;
static int DataCallSetupOngoing = 0;
static int LastErrorCause = PDP_FAIL_NONE;

static int ExplicitRilRouting = 0;
int s_use_ppp = 0;
int s_pppd_running = 0;

static RIL_Data_Call_Response_v6 s_network_interface[MAX_NETWORK_IF];

static  RIL_Data_Call_Response_v6 *s_network_interface_alias_of[MAX_NETWORK_IF];

static int v_NmbOfConcurentContext = 0;
extern int ipv6_supported;


static int IsIPV6NullAddress(char *AddrV6)
{
    return ((strcmp(AddrV6, IPV6_NULL_ADDR_1) == 0) || ((strcmp(AddrV6, IPV6_NULL_ADDR_2) == 0)));
}

int GetNmbConcurentProfile(void)
{
    return v_NmbOfConcurentContext;
}

void ResetPppdState()
{
    s_pppd_running = 0;

}

/*
 * Only used for test suite to request route handling at RIL level
 * rather than in the framework (which is not used in that configuration
 */
void ConfigureExplicitRilRouting(int Set)
{
    ExplicitRilRouting = Set;
}

static void CompleteDataCall(int Status, int nwIf)
{
    int response_size=0;
    ALOGD("CompleteDataCall");
    if(SetupDataCall_token == NULL)
    {
        /* No token, this isn't expected... */
        ALOGE("Error: No setupDataCall token!");
        return;
    }

#if (RIL_VERSION <= 4)
    char *response[5];

#ifndef MULTI_PDP_GB
    /* Ril v3 only supports one PDP */
    if(nwIf != 0)
    {
        ALOGE("Wrong nwIf: %d",nwIf);
        goto error;
    }
#endif

    /* Ril v3 expects a string whereas ril v6 expects a int */
    response[0] = alloca(MAX_CID_STRING_LENGTH);
    if(snprintf(response[0],MAX_CID_STRING_LENGTH,"%d",s_network_interface[nwIf].cid) >= MAX_CID_STRING_LENGTH)
    {
        ALOGE("Cid string overflow: %d", s_network_interface[nwIf].cid);
        goto error;
    }
    response[1] = s_network_interface[nwIf].ifname;
    response[2] = s_network_interface[nwIf].addresses;
    response[3] = s_network_interface[nwIf].dnses;
    response[4] = s_network_interface[nwIf].gateways;

    response_size = sizeof(response);


#elif RIL_VERSION >= 6
    RIL_Data_Call_Response_v6 *response = &s_network_interface[nwIf];
    response_size = sizeof(RIL_Data_Call_Response_v6);
#endif
    RIL_onRequestComplete(SetupDataCall_token, Status, response, response_size);
    SetupDataCall_token = NULL;
    DataCallSetupOngoing = 0;
    return;

#if (RIL_VERSION <= 4)
error:
    RIL_onRequestComplete(SetupDataCall_token, RIL_E_GENERIC_FAILURE, NULL, 0);
    SetupDataCall_token = NULL;
    DataCallSetupOngoing = 0;
#endif
}

/*
 * Helper functions
 */
static int getCurrentApnConf(int this_cid, char** apn, int * authtype, char ** user, char** password, char ** ip_type)
{
    int err;
    ATResponse *p_response   = NULL;
    ATLine *p_cur;
    char *out;
    *apn =NULL;
    *user=NULL;
    *password=NULL;
    *ip_type =NULL;

    ALOGD("getCurrentApnConf()");
    /* First get the APN */
    err = at_send_command_multiline ("AT+CGDCONT?", "+CGDCONT:", &p_response);
    if (err == 0 && p_response->success != 0)
    {
        /* Browse through the contexts */
        for (p_cur = p_response->p_intermediates; p_cur != NULL;
         p_cur = p_cur->p_next) {
            char *line = p_cur->line;
            int cid;
            char *type;
            char *address;

            err = at_tok_start(&line);
            if (err < 0)
                goto error;

            err = at_tok_nextint(&line, &cid);
            if (err < 0)
                goto error;

            /* Focus only on the cId of interest */
            if(cid != this_cid)
            {
                continue;
            }

            /* Type */
            err = at_tok_nextstr(&line, &out);
            if (err < 0)
                goto error;

            asprintf(ip_type, "%s",out);

            /* APN */
            err = at_tok_nextstr(&line, &out);
            if (err < 0)
                goto error;

            asprintf(apn, "%s",out);

            break;
        }
    }
    at_response_free(p_response);
    p_response = NULL;

    /* Same for authentication cfg */
    err = at_send_command_multiline ("AT%IPDPCFG?", "%IPDPCFG:", &p_response);
    if (err == 0 && p_response->success != 0)
    {
        /* Browse through the contexts */
        for (p_cur = p_response->p_intermediates; p_cur != NULL;
         p_cur = p_cur->p_next) {
            char *line = p_cur->line;
            int cid;
            int skip;

            err = at_tok_start(&line);
            if (err < 0)
                goto error;

            err = at_tok_nextint(&line, &cid);
            if (err < 0)
                goto error;

            /* Focus only on the cId of interest */
            if(cid != this_cid)
            {
                continue;
            }

            /* Skip (nbns) */
            err = at_tok_nextint(&line, &skip);
            if (err < 0)
                goto error;

            /*Auth type */
            err = at_tok_nextint(&line, authtype);
            if (err < 0)
                goto error;

            /* user */
            err = at_tok_nextstr(&line, &out);
            if (err < 0)
                goto error;

            asprintf(user, "%s",out);

            /* password */
            err = at_tok_nextstr(&line, &out);
            if (err < 0)
                goto error;

            asprintf(password, "%s",out);

            break;
        }
    }
    at_response_free(p_response);

    error:
    return err;
}

static int getCurrentCallState(int this_cid)
{
    /* Answer 0 if no call listed for the given cid */
    int callState = 0;
    int cid;
    int err;
    ATResponse *p_response   = NULL;
    ATLine *p_cur;
    ALOGD("getCurrentCallState()");

    err = at_send_command_multiline ("AT%IPDPACT?", "%IPDPACT:", &p_response);

    if (err == 0 && p_response->success != 0) {
        for (p_cur = p_response->p_intermediates; p_cur != NULL;
         p_cur = p_cur->p_next) {
            char *line = p_cur->line;
            int cid;

            err = at_tok_start(&line);
            if (err < 0)
                goto error;

            err = at_tok_nextint(&line, &cid);
            if (err < 0)
                goto error;

            /* Focus only on the cId of interrest */
            if(cid != this_cid)
            {
                continue;
            }

            /* Call state */
            err = at_tok_nextint(&line, &callState);
        }
    }
    error:
    return callState;
}

void requestSetupDataCall(void *data, size_t datalen, RIL_Token t)
{
    const char *apn,*username, *password, *type;
    int needToReattach = 0;
    int authType = 0;
    char *cmd;
    int err, nwIf;
    ATResponse *p_response   = NULL;
    ATLine *p_cur;
    char * currentApn, *currentUsername,*currentPassword, *currentType;
    int currentAuth;
    int error_code = RIL_E_GENERIC_FAILURE;

    /*
     * Check if we're in the middle of processing a connection request
     * We need to complete the current request before handling another one
     */
    if(DataCallSetupOngoing != 0)
    {
        ALOGD("Call request ongoing, delaying request");
        /* Wait 1 second before reprocessing the request */
        void *req = (void *)ril_request_object_create(RIL_REQUEST_SETUP_DATA_CALL, data, datalen, t);
        if(req != NULL)
        {
            postDelayedRequest(req, 1);
        }

        return;
    }

    /*
     * store the token so we can answer the request later
     * We need to return from this so we can process internal requests in
     * order to obtain the ip address.
     */
    SetupDataCall_token = t;
    DataCallSetupOngoing++;

    /* Find a suitable nwIf */
    for(nwIf=0;nwIf < GetNmbConcurentProfile();nwIf++)
    {
        if((s_network_interface[nwIf].active == 0)
        ||(s_network_interface[nwIf].active == 1))

            break;
    }

    if(nwIf >= GetNmbConcurentProfile())
    {
        ALOGE("Too many concurent calls");
#ifdef MULTI_PDP_GB
        error_code = RIL_E_NO_NET_INTERFACE;
#endif
        goto error;
    }
    ALOGD("Assinging call to nwIf #%d (%s)", nwIf, s_network_interface[nwIf].ifname);

    /* Reset the error cause */
    s_network_interface[nwIf].status = PDP_FAIL_NONE;
    LastErrorCause = PDP_FAIL_NONE;

    /*
     * debug socket only gives the apn (in the wrong place as well)
     * need a specific treatment
     */
    if(datalen == sizeof(char*))
    {
        ALOGD("Request from debug socket, best effort attempt only");
        apn = ((const char **)data)[0];
        username = "";
        password = "";
        type = "IP";
        authType = 0;
    }
    else
    {
        /* extract request's parameters */
        apn = ((const char **)data)[2];
        username = ((const char **)data)[3];
        password = ((const char **)data)[4];
        type = ((const char **)data)[6];

        /* Check that parameters are there. */
        if(username == NULL)
            username= "";
        if(password== NULL)
            password = "";
        if(apn == NULL)
            apn = "";
        if(type == NULL)
            type = "IP";

        /* Convert requested authentication to the encoding used by our at commands */
        switch (*((const char **)data)[5])
        {
            case '1':
                authType = IPDPCFG_PAP;
                break;
            case '2':
            case '3':
                authType = IPDPCFG_CHAP;
                break;
            default:
                /* No authentication */
                authType = 0;
        }
    }
    /*
     * Test the curent configuration:
     * If it is identical to the current one, skip setting the APN and carry on,
     */
    getCurrentApnConf(s_network_interface[nwIf].cid,&currentApn,&currentAuth,&currentUsername,&currentPassword,&currentType);

    ALOGD("CurrentApn: %s, type: %s",(currentApn!=NULL)?currentApn:"NULL",(currentType!=NULL)?currentType:"NULL");
    ALOGD("CurrentAuth: %d, %s, %s",currentAuth,(currentUsername!=NULL)?currentUsername:"NULL",(currentPassword!=NULL)?currentPassword:"NULL");

    /* We need this info to be able to return it to the framework */
    if((s_network_interface[nwIf].type == NULL)&&(currentType !=NULL))
    {
        asprintf(&s_network_interface[nwIf].type, "%s",currentType);
    }

    if((currentApn == NULL) || (strcmp(currentApn,apn) != 0)
     ||(currentAuth != authType)
     ||((currentUsername == NULL) || (strcmp(currentUsername,username) != 0))
     ||((currentPassword == NULL) || (strcmp(currentPassword,password) != 0))
     ||((currentType == NULL) || (strcmp(type,currentType)!=0)))
    {

        /*
         * In LTE mode, only option to modify the APN config is to detach
         * (or drop to cfun=4, which takes longer), if context is active
         * Detach/Attach should be very quick.
         */
        int callstate = getCurrentCallState(s_network_interface[nwIf].cid);

        if((callstate != 0)&&(callstate != 3))
        {
            at_send_command("AT+CGATT=0", NULL);
            needToReattach = 1;
        }

        if(ipv6_supported)
        {
            ALOGD("requesting data connection to APN '%s - IPV6 supported'", apn);
            asprintf(&cmd, "AT+CGDCONT=%d,\"%s\",\"%s\"", s_network_interface[nwIf].cid, type, apn);
        }
        else
        {
            ALOGD("requesting data connection to APN '%s - IPV6 not supported'", apn);
            asprintf(&cmd, "AT+CGDCONT=%d,\"IP\",\"%s\"", s_network_interface[nwIf].cid, apn);
            type = "IP";
        }

        free(s_network_interface[nwIf].type);
        asprintf(&s_network_interface[nwIf].type, "%s",type);

        err = at_send_command(cmd, &p_response);
        free(cmd);

        if (err < 0 || p_response->success == 0)
        {
            ALOGE("Problem setting the profile");
            /*
             * although we will abandon this particular attempt,
             * try some recovery steps for next attempt
             */
            if(s_use_ppp == 0)
            {
                asprintf(&cmd,"AT%%IPDPACT=%d,0",s_network_interface[nwIf].cid);
                err = at_send_command(cmd, NULL);
                free(cmd);
            }
            else
            {
                asprintf(&cmd,"AT%%IPDPACT=%d,0,\"PPP\",1",s_network_interface[nwIf].cid);
                err = at_send_command(cmd, NULL);
                free(cmd);
            }
            goto error;
        }
        at_response_free(p_response);

        if(authType != 0)
        {
            asprintf(&cmd,"AT%%IPDPCFG=%d,0,%d,\"%s\",\"%s\"",s_network_interface[nwIf].cid, authType,username,password);
        }
        else
        {
            /*
             * this is needed as this setup is persistent between calls,
             * we couldn't unset it otherwise
             */
            asprintf(&cmd,"AT%%IPDPCFG=%d,0,0",s_network_interface[nwIf].cid);
        }
        err = at_send_command(cmd, &p_response);
        free(cmd);

        if (err < 0 || p_response->success == 0)
        {
            goto error;
        }
        at_response_free(p_response);

        if(needToReattach)
        {
            at_send_command("AT+CGATT=1", NULL);
        }
    }

    free(currentApn);
    free(currentUsername);
    free(currentPassword);
    free(currentType);

    if(s_use_ppp == 0)
    {
        /*
         * Icera-specific command to activate PDP context #1
         * The request will conclude when the IP address has been obtained or the call fails.
         */
        asprintf(&cmd,"AT%%IPDPACT=%d,1,\"IP\",%d",s_network_interface[nwIf].cid, nwIf);
        err = at_send_command(cmd, &p_response);
        free(cmd);
        if (err < 0 || p_response->success == 0) {
            goto error;
        }
    }
    else
    {
        while(s_pppd_running)
        {
            /* Wait that pppd close previous connection.*/
            sleep(1);
        }

        // Activate PDP context for PPP connection.
        asprintf(&cmd,"AT%%IPDPACT=%d,1,\"PPP\",1",s_network_interface[nwIf].cid);
        err = at_send_command(cmd, &p_response);
        free(cmd);
        if (err < 0 || p_response->success == 0) {
            goto error;
        }
        /* Start ppp daemon*/
        err = property_set("ctl.start", "pppd_gprs");
        ALOGD("starting service pppd_gprs...");
        if (err < 0)
        {
            ALOGD("### error in starting service pppd_gprs: err %d", err);
            goto error;
        };
        s_pppd_running = 1;
    }

    at_response_free(p_response);

    return;
error:
    /*
     * Need to keep track of the early failure or framework won't
     * reattempt to setup the datacall
     */
    s_network_interface[nwIf].status = PDP_FAIL_ERROR_UNSPECIFIED;

    /*
     * Give the modem a bit of breathing space: 3 seconds should do,
     * otherwise framework will retry immediately.
     */
    s_network_interface[nwIf].suggestedRetryTime = 3000;

    /*
     * This may be queried from LastDatafailCause (which doesn't
     * give a call id, so we can't look it up easilly in the
     * datacall list)
     */
    LastErrorCause = PDP_FAIL_ERROR_UNSPECIFIED;

#if RIL_VERSION >= 6
    /*
     * Ril > v6 must return success to the request, but with the error
     * in the status field of the call list.
     */
    CompleteDataCall(RIL_E_SUCCESS, nwIf);
#else
    RIL_onRequestComplete(t, error_code, NULL, 0);
#endif
    /* Allow future calls to be made */
    DataCallSetupOngoing = 0;
    SetupDataCall_token = NULL;
    at_response_free(p_response);
}

void requestDeactivateDataCall(void *data, size_t datalen, RIL_Token t)
{
    char * cid;
    char *cmd;
    int err;
    ATResponse *p_response = NULL;

    cid = ((char **)data)[0];

    if(s_use_ppp == 0)
    {
        asprintf(&cmd, "AT%%IPDPACT=%s,0", cid);
    }
    else
    {
        asprintf(&cmd, "AT%%IPDPACT=%s,0,\"PPP\",1", cid);
    }

    err = at_send_command(cmd, &p_response);
    free(cmd);

    if (err < 0 || p_response->success == 0) {
        at_response_free(p_response);
        goto error;
    }
    at_response_free(p_response);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

static int getCallFailureCause(void)
{
    char *line = NULL;
    int nwErrRegistration, nwErrAttach, nwErrActivation;
    int err = 0;
    int response = 0;
    ATResponse *p_response;

    err = at_send_command_singleline("AT%IER?", "%IER:", &p_response);
    if (err < 0 || p_response->success == 0)
        goto error;

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;
    err = at_tok_nextint(&line, &nwErrRegistration);
    if (err < 0)
       goto error;

    err = at_tok_nextint(&line, &nwErrAttach);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &nwErrActivation);
    if (err < 0)
        goto error;

    goto finally;
    error:
    nwErrActivation = -1;
    finally:
    at_response_free(p_response);
    return nwErrActivation;
}

void requestLastDataFailCause(void *data, size_t datalen, RIL_Token t)
{
    int nwErrActivation;
    if(LastErrorCause != PDP_FAIL_NONE)
    {
        nwErrActivation = LastErrorCause;
    }
    else
    {
        nwErrActivation = getCallFailureCause();
    }

    if(nwErrActivation>=0)
    {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, &nwErrActivation, sizeof(int));
    }
    else
    {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void onDataCallListChanged(void *param)
{
    requestOrSendDataCallList(NULL);
}


void requestDataCallList(void *data, size_t datalen, RIL_Token t)
{
    requestOrSendDataCallList(&t);
}

static void requestOrSendDataCallList(RIL_Token *t)
{
    void *data;
    int datalen;

#if RIL_VERSION <= 4
    /*
     * Convert To the old format in this case, only one datacall...
     */
    int i;
    int NmbContext = GetNmbConcurentProfile();
    datalen = NmbContext * sizeof(RIL_Data_Call_Response_v4);

    RIL_Data_Call_Response_v4 *Response = alloca(datalen);
    for(i=0; i<NmbContext;i++)
    {
        Response[i].cid = s_network_interface[i].cid;
        Response[i].active = s_network_interface[i].active;
        Response[i].type = s_network_interface[i].type;
        Response[i].address = s_network_interface[i].addresses;
        /* Apn is ignored */
        Response[i].apn = NULL;
    }
    data =(void*)Response;
#else
    /* Return the list that is maintained */
    data=(void *)s_network_interface;
    datalen = sizeof(RIL_Data_Call_Response_v6)*GetNmbConcurentProfile();
#endif

    if (t != NULL)
        RIL_onRequestComplete(*t, RIL_E_SUCCESS, data, datalen);
    else
        RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED, data, datalen);
}

void AddNetworkInterface(char * NwIF)
{
    char *alias;
    int i;

    if(v_NmbOfConcurentContext >= MAX_NETWORK_IF)
    {
        ALOGE("AddNetworkInterface: Too many network interfaces");
        return;
    }

    /* Initialize the structure */
    memset(&s_network_interface[v_NmbOfConcurentContext],0,sizeof(RIL_Data_Call_Response_v6));
    s_network_interface[v_NmbOfConcurentContext].ifname = NwIF;

     /* Check if the interface is an alias (for route configuration)*/
    s_network_interface_alias_of[v_NmbOfConcurentContext]=NULL;
    alias =  strchr(s_network_interface[v_NmbOfConcurentContext].ifname, ':');
    if(alias!= NULL)
    {
         // Interface is an alias. Find the parent interface.
         for(i=0; i<v_NmbOfConcurentContext; i++)
         {
             int name_length = (int) (alias - s_network_interface[v_NmbOfConcurentContext].ifname);
             // Compare interface name before alias ID separator ':"
             if(strncmp(s_network_interface[v_NmbOfConcurentContext].ifname,
                        s_network_interface[i].ifname,
                        name_length) == 0)
             {
                  // Check we have not find another alias of the same interface
                  if(s_network_interface_alias_of[i] == NULL)
                  {
                      s_network_interface_alias_of[v_NmbOfConcurentContext] = &s_network_interface[i];
                  }
             }
         }
         // Ensure the parent interface always exists for an alias.
         if( s_network_interface_alias_of[v_NmbOfConcurentContext] == NULL)
         {
             ALOGE("AddNetworkInterface: Parent interface must exist before defining its alias");
         }
    }

    /* For LTE support, start cids at 5 */
    s_network_interface[v_NmbOfConcurentContext].cid = 5 + v_NmbOfConcurentContext;

    s_use_ppp = (strncmp(s_network_interface[v_NmbOfConcurentContext].ifname,"ppp",3)==0);

    v_NmbOfConcurentContext++;
}

void ParseIpdpact(const char* s)
{
    ALOGD("ParseIpdpact");
    char *line = (char*)s;
    int cid;
    int state;
    int err;

    int nwIf;

    /* expected line:
        %IPDPACT: <cid>,<state>,<ndisid>
        where state=0:deactivated,1:activated,2:activating,3:activation failed, 4:LTE mode (%IPDPAUTO)
    */

    err = at_tok_start(&line);
    if (err < 0)
    {
        ALOGE("invalid IPDPACT line %s\n", s);
        goto error;
    }
    err = at_tok_nextint(&line, &cid);
    if (err < 0)
    {
        ALOGE("Error Parsing IPDPACT line %s\n", s);
        goto error;
    }

    err = at_tok_nextint(&line, &state);
    if (err < 0)
    {
        ALOGE("Error Parsing IPDPACT line %s\n", s);
        goto error;
    }

    /*
     * third parameter is not reliable to figure out the nwIf
     * as it is not reported consistently (on call drop it seems to
     * become the error code).
     *
     * Work the nwIf out from the cId instead.
     */
    for(nwIf=0;nwIf < GetNmbConcurentProfile();nwIf++)
    {
        if(s_network_interface[nwIf].cid == cid)
            break;
    }

    if(nwIf >= v_NmbOfConcurentContext)
    {
        ALOGE("Unknown network If...");
        goto error;
    }
    /*
     * we need to move to a suitable command thread for
     * the rest of the processing.
     */
    int data[2];
    data[0] = nwIf;
    data[1] = state;
    ril_request_object_t* req = ril_request_object_create(IRIL_REQUEST_ID_PROCESS_DATACALL_EVENT, data, sizeof(data), NULL);
    if(req != NULL)
    {
        requestPutObject(req);
        return;
    }
    error:
        ALOGE("Error, aborting");
}

static void getIpAddress(int nwIf)
{
    int err;
    ATResponse *p_response = NULL;
    ALOGD("getIpAddress");
    char *devAddr = NULL;
    char *gwProperty = NULL;
    char *gwAddr, *dns1Addr, *dns2Addr, *dns1AddrV4, *dns2AddrV4, *dummy, *netmask;
    char *devAddrV6, *gwAddrV6, *dns1AddrV6, *dns2AddrV6;
    char *line, *tmp;
    int prefix_length;

    err = at_send_command_multiline("AT%IPDPADDR?","%IPDPADDR:", &p_response);

    if((err==0) && (p_response->success != 0))
    {
        int cId;
        char *cmd = NULL;
        ATLine *p_cur = p_response->p_intermediates;

        /* Find ip addresses in the response */
        while(p_cur !=NULL)
        {
            line = p_cur->line;

            err = at_tok_start(&line);
            if (err < 0) {
                goto error;
            }
            err = at_tok_nextint(&line, &cId);

            if (err < 0) {
                goto error;
            }

            /* Stop searching if we've got the right call */
            if(cId == s_network_interface[nwIf].cid)
                break;

            p_cur = p_cur->p_next;
        }

        if(p_cur == NULL)
        {
            ALOGE("Couldn't find a matching nwIf");
            goto error;
        }

        // The device address (IPv4)
        err = at_tok_nextstr(&line,&devAddr);
        if(err<0) goto error;

        // The gateway (IPv4)
        err = at_tok_nextstr(&line,&gwAddr);
        if(err<0) goto error;

        /*
         * The 1st & 2nd DNS (IPv4)
         */
        err = at_tok_nextstr(&line,&dns1AddrV4);
        if(err<0) goto error;
        err = at_tok_nextstr(&line,&dns2AddrV4);
        if(err<0) goto error;

        /*
         *  Skip the nbns if they're provided (IPv4)
         */
        err = at_tok_nextstr(&line,&dummy);
        if(err<0) goto error;
        err = at_tok_nextstr(&line,&dummy);
        if(err<0) goto error;

        /*
         *  Check if we have a netmask, use default if not. (IPv4)
         */
        err = at_tok_nextstr(&line,&netmask);

        /*
         * Skip dhcp
         */
        err = at_tok_nextstr(&line,&dummy);
        if(err<0) goto error;

        /* Default in case there is no IPV6 address provided in the answer.*/
        devAddrV6   = IPV6_NULL_ADDR_1;
        gwAddrV6    = IPV6_NULL_ADDR_1;
        dns1AddrV6 = IPV6_NULL_ADDR_1;
        dns2AddrV6 = IPV6_NULL_ADDR_1;
/*
 * Not an LTE limitation, but the CMW gives out
 * very strange IPV6 addresses, let's ignore it
 * for the time being.
 */
#ifndef LTE_TEST

        /* Check if IPV6 addresses are present*/
        if(at_tok_hasmore(&line))
        {
            /*
             * IPV6 device address
             */
            err = at_tok_nextstr(&line,&devAddrV6);
            if(err<0) goto error;

            /*
             * IPV6 Gateway
             */
            err = at_tok_nextstr(&line,&gwAddrV6);
            if(err<0) goto error;

            /*
             * IPV6 1st & 2nd DNS (IPv4)
             */
            err = at_tok_nextstr(&line,&dns1AddrV6);
            if(err<0) goto error;
            err = at_tok_nextstr(&line,&dns2AddrV6);
            if(err<0) goto error;

            /* Convert to a suitable format that the framework expect */
            tmp = ConvertIPV6AddressToStandard(devAddrV6);
            devAddrV6 = (tmp!=NULL)?tmp:strdup(devAddrV6);
            tmp = ConvertIPV6AddressToStandard(gwAddrV6);
            gwAddrV6 = (tmp!=NULL)?tmp:strdup(gwAddrV6);
            tmp = ConvertIPV6AddressToStandard(dns1AddrV6);
            dns1AddrV6 = (tmp!=NULL)?tmp:strdup(dns1AddrV6);
            tmp = ConvertIPV6AddressToStandard(dns2AddrV6);
            dns2AddrV6 = (tmp!=NULL)?tmp:strdup(dns2AddrV6);

            /*
             *  Don't use last parameters
             */

            ALOGD("Addresses device (%s): %s\ngw: %s\ndns1: %s\ndns2: %s\nmask: %s\nipv6 address: %s\nipv6 gw: %s\nipv6 dns1: %s\nipv6 dns2: %s\n",s_network_interface[nwIf].ifname, devAddr, gwAddr,dns1AddrV4,dns2AddrV4,netmask,  devAddrV6, gwAddrV6,dns1AddrV6,dns2AddrV6);
        }
        else
#endif /* LTE_TEST */
        {
            /* Print only IPV4 addresses */
            ALOGD("Addresses device (%s): %s\ngw: %s\ndns1: %s\ndns2: %s\nmask: %s\n",s_network_interface[nwIf].ifname, devAddr, gwAddr,dns1AddrV4,dns2AddrV4,netmask);
        }

        /* Update the nwIf structure */
        if(s_network_interface[nwIf].addresses)
            free(s_network_interface[nwIf].addresses);
        if(s_network_interface[nwIf].dnses)
            free(s_network_interface[nwIf].dnses);
        if(s_network_interface[nwIf].gateways)
            free(s_network_interface[nwIf].gateways);

        if(IsIPV6NullAddress(devAddrV6))
        {
            asprintf(&s_network_interface[nwIf].addresses, "%s",devAddr);
            asprintf(&s_network_interface[nwIf].dnses, "%s %s",dns1AddrV4,dns2AddrV4);
            asprintf(&s_network_interface[nwIf].gateways,"%s",gwAddr);
        }
        else
        {
            asprintf(&s_network_interface[nwIf].addresses, "%s %s",devAddr,devAddrV6);
            asprintf(&s_network_interface[nwIf].dnses, "%s %s %s %s",dns1AddrV4,dns2AddrV4,dns1AddrV6,dns2AddrV6);
            asprintf(&s_network_interface[nwIf].gateways,"%s %s",gwAddr,gwAddrV6);
        }
        /* We don't concern ourselves with gateways and masks as we're dealing with point to point connections */

        /*
         * Configure IPV4 interface.
         */
        ALOGI("Starting IF configure");
        if (ifc_init() != 0) {
            ALOGE("ifc_init failed");
            goto error;
        }

        in_addr_t ip = inet_addr(devAddr);
        in_addr_t mask = inet_addr(netmask);
        in_addr_t gw = inet_addr(gwAddr);

        if(s_network_interface_alias_of[nwIf])
        {
            // If interface is an alias. turn up the parent interface instead of the interface itself.
            if(ifc_up(s_network_interface_alias_of[nwIf]->ifname))
            {
                ALOGE("failed to turn on parent interface %s: %s\n",
                     s_network_interface_alias_of[nwIf]->ifname, strerror(errno));
                goto error;
            }
        }
        else
        {
            // Interface is not an alias. turn it on.
            if(ifc_up(s_network_interface[nwIf].ifname)) {
                ALOGE("failed to turn on interface %s: %s\n",
                s_network_interface[nwIf].ifname, strerror(errno));
                goto error;
            }
        }


        if(strcmp(devAddr, IPV4_NULL_ADDR) != 0)
        {
            if (ifc_set_addr(s_network_interface[nwIf].ifname, ip)) {
                ALOGE("failed to set ipaddr %s: %s\n", devAddr, strerror(errno));
                goto error;
            }

#if RIL_VERSION <= 4
            if (ifc_set_mask(s_network_interface[nwIf].ifname, mask)) {
                ALOGE("failed to set netmask %s: %s\n", netmask, strerror(errno));
                goto error;
            }
#else
            /* get prefix length from mask*/
            prefix_length = 0;
            while(ntohl(mask) & (0x80000000 >> prefix_length))
            {
                prefix_length++;
            }

            if(ifc_set_prefixLength(s_network_interface[nwIf].ifname, prefix_length))
            {
                ALOGE("failed to set prefix length %d: %s\n",prefix_length , strerror(errno));
                goto error;
            }
#endif

            if(ExplicitRilRouting)
            {
                if (ifc_create_default_route(s_network_interface[nwIf].ifname, gw)) {
                    ALOGE("failed to set default route %s: %s\n", gwAddr, strerror(errno));
                    goto error;
                }
            }
        }

        if(!IsIPV6NullAddress(devAddrV6)) {

            /*
             * In case a V6 address is present we must add the default route for the IPV6 traffic.
             * ifc_xxx API does not provide IPV6 support yet (Android 2.3 and before) so do it manually...
             */
            struct sockaddr_in6 sa6;
            struct in6_rtmsg rt;
            int skfd, err;
            struct ifreq ifr;
            struct in6_ifreq ifr6;

            /* Create a socket to the INET6 kernel. */
            skfd = socket(AF_INET6, SOCK_DGRAM, 0);

            /* Get index of the interface. */
            memset(&ifr, 0, sizeof(ifr));
            strncpy(ifr.ifr_name, s_network_interface[nwIf].ifname,IFNAMSIZ);
            err = ioctl(skfd, SIOGIFINDEX, &ifr);

            /* Set interface address */
            ifr6.ifr6_ifindex = ifr.ifr_ifindex;
            ifr6.ifr6_prefixlen = 64;
            inet_pton(AF_INET6, devAddrV6, &ifr6.ifr6_addr);
            err = ioctl(skfd, SIOCSIFADDR,&ifr6);

            if(ExplicitRilRouting)
            {
                /* Add the default route for IPV6 (all fields to 0 except index) */
                memset(&rt, 0, sizeof(rt));
                rt.rtmsg_ifindex = ifr.ifr_ifindex;
                err = ioctl(skfd, SIOCADDRT, &rt);
            }

            /* close the socket.*/
            close(skfd);
        }
#if RIL_VERSION <= 4
        /*
         * Select which DNS to provide to Android.
         */
        if(!IsIPV6NullAddress(dns1AddrV6))
        {
            /* A DNS V6 is present - use it as first DNS*/
            dns1Addr = dns1AddrV6;

            if(strcmp(dns1AddrV4, IPV4_NULL_ADDR) != 0)
            {
                /* Use DNSV4 as 2nd DNS if present*/
                dns2Addr = dns1AddrV4;
            }
            else
            {
                /* if No DNS V4 present then use DNS2 V6*/
                dns2Addr = dns2AddrV6;
            }
        }

        else
        {
            /* No DNS V6 present - use only DNSV4*/
            dns1Addr = dns1AddrV4;
            dns2Addr = dns2AddrV4;
        }

        /*
         * Set the DNS address:
         * "net.gprs.dns1" is monitored by MobileDataStateTracker and user radio can modify
         * "net.usbX" or "dhcp.usbx" are not within radio user reach.
         * So use that
         */
        property_set("net.gprs.dns1", dns1Addr);
        property_set("net.gprs.dns2", dns2Addr);

        if(!IsIPV6NullAddress(dns1AddrV6))
        {
            /* report the V6 address to android if it is present.*/
            devAddr = devAddrV6;
            gwAddr = gwAddrV6;
        }

        asprintf(&gwProperty, "net.%s.gw",s_network_interface[nwIf].ifname);
        if(gwProperty)
        {
              property_set(gwProperty, gwAddr);
              free(gwProperty);
        }
 #endif
        if(!IsIPV6NullAddress(devAddrV6)) free(devAddrV6);
        if(!IsIPV6NullAddress(gwAddrV6)) free(gwAddrV6);
        if(!IsIPV6NullAddress(dns1AddrV6)) free(dns1AddrV6);
        if(!IsIPV6NullAddress(dns2AddrV6)) free(dns2AddrV6);

        ALOGD("Network configuration succedded\n");;
    }
    else
    {
        ALOGE("IPDPADDR problem\n");
    }

    CompleteDataCall(RIL_E_SUCCESS,nwIf);

    return;

    error:
        ALOGE("Network configuration problem");
        at_response_free(p_response);
        CompleteDataCall(RIL_E_GENERIC_FAILURE,nwIf);
}

void ProcessDataCallEvent(int nwIf, int State)
{
    switch(State)
    {
        case 1:
        {
            /*
             * Call is connected... Get the ip address
             * and acknoledge the request completion.
             */
            ALOGD("PDP Context activated, activating network interface %d",nwIf);
            s_network_interface[nwIf].active = 2;
            s_network_interface[nwIf].status = 0;
            getIpAddress(nwIf);

            break;
        }
        case 2:
        {
            if(SetupDataCall_token == NULL)
            {
                /*
                 * Must be LTE setup. Block other data call
                 * attempt for the time being as it seems to
                 * affect the modem.
                 */
                DataCallSetupOngoing++;
            }
            break;
        }
        case 4:
        {
            ALOGD("PDP Context activated, activating network interface %d",nwIf);
            s_network_interface[nwIf].active = 1;
            /* Allow datacall to be setup from now on */
            DataCallSetupOngoing = 0;

            break;
        }

        case 3:
        {
            /*
             * Allow further call to be made...
             */
            if(s_use_ppp == 1)
            {
                /* Let time to pppd to complete before notifying Android that connection is closed*/
                sleep(5);
                s_pppd_running = 0;
            }

            /*
             * Problem starting the call...
             */
            s_network_interface[nwIf].active = 0;
            s_network_interface[nwIf].status = PDP_FAIL_NONE;

#if RIL_VERSION >= 6
            /* In RIL v6, we now need to find out why it failed */
            s_network_interface[nwIf].status = getCallFailureCause();

            /*
             * If no specific error returned by the modem, put a generic
             * error cause
             */
            if(s_network_interface[nwIf].status == 0)
            {
                s_network_interface[nwIf].status = PDP_FAIL_ERROR_UNSPECIFIED;
            }
            /*
             * Now need to return success in failure cases with failure
             * details in response.
             */
            CompleteDataCall(RIL_E_SUCCESS,nwIf);
#else
            CompleteDataCall(RIL_E_GENERIC_FAILURE,nwIf);
#endif
            break;
        }
        case 0:
        {
            /*
             *  Call was terminated or dropped...
             */
            ALOGD("PDP Context de-activated, de-activating network interface %d",nwIf);
            int status = ifc_down(s_network_interface[nwIf].ifname);

            ALOGD("taking down if returned status %d\n", status);

            s_network_interface[nwIf].active = 0;
            s_network_interface[nwIf].status = PDP_FAIL_NONE;

            if(s_use_ppp == 1)
            {
                /* Let time to pppd to complete*/
                sleep(5);
                s_pppd_running = 0;
            }

#if RIL_VERSION >= 6
            /* In RIL v6, we now need to find out why it failed */
            s_network_interface[nwIf].status = getCallFailureCause();
#endif
            onDataCallListChanged(NULL);

            break;
        }
    }
}

/*
 * Converts from 0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0 to
 * 0:0:0:0:0:0:0:0
 * returns NULL if not the expected format
 * Caller to free the string.
 */
char * ConvertIPV6AddressToStandard(char * src)
{
    int val[8]={0,0,0,0,0,0,0,0};
    int i=0;
    char * ptr = src-1;
    char * dst = NULL;

    do{
        ptr++;
        val[i/2]+= atoi(ptr)<<(8*((i+1)%2));
        i++;
        ptr = strstr(ptr, ".");
    }while((ptr!=NULL)&&(i<=16));

    if(i==16)
        asprintf(&dst,"%x:%x:%x:%x:%x:%x:%x:%x",val[0],val[1],val[2],val[3],val[4],val[5],val[6],val[7]);
    return dst;
}
