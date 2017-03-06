
/*************************************************************************************************
 * Copyright (c) 2012, NVIDIA CORPORATION. All rights reserved.
 *************************************************************************************************
 * $Revision: #1 $
 * $Date: $
 * $Author: $
 ************************************************************************************************/

/**
 * @addtogroup agps
 * @{
 */

/**
 * @file agps_test.c NVidia aGPS test cases
 *
 */

/*************************************************************************************************
 * Project header files. The first in this list should always be the public interface for this
 * file. This ensures that the public interface header file compiles standalone.
 ************************************************************************************************/

#include "agps.h"
#include "agps_gpal.h"
#include "agps_gpal_test.h"
#include "agps_frame.h"
#include "agps_port.h"

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/

#include <unistd.h>

/**************************************************************************************************************/
/* Private Macros  */
/**************************************************************************************************************/

#define AGPS_TEST_MAX_PAYLOAD_SIZE ( 1024 )
#define AGPS_PDU_TYPE_DONT_CARE    ( -1 )
#define AGPS_TEST_PARAM_SEPARATOR ":"

/**************************************************************************************************************/
/* Private Functions Declarations (if needed) */
/**************************************************************************************************************/

static void *agps_GpalTestInit(const agps_GpalCallbacks *callbacks, void *agps_handle);

static agps_ReturnCode agps_GpalTestSendDownlinkMsg(void *gps_handle,
                                         agps_GpalProtocol protocol,
                                         const char *msg,
                                         unsigned int size);

static agps_ReturnCode agps_GpalTestSetUeState(void *gps_handle,
                                    agps_GpalProtocol protocol,
                                    agps_GpalUeState state);

static agps_ReturnCode agps_GpalTestResetAssistanceData(void *gps_handle);

static agps_ReturnCode agps_GpalTestDeInit(void *gps_handle);

static agps_ReturnCode agps_GpalTestSetCalibrationStatus(void *gps_handle, int locked);

/**************************************************************************************************************/
/* Private Type Declarations */
/**************************************************************************************************************/

/**
* enumeration of supported test cases
*/
typedef enum
{
    TEST_17_2_2_1,
    TEST_UL_DCCH,
    TEST_CALIBRATION,
    TEST_NOT_SUPPORTED,
} agps_TestCaseId;


/**
* enumeration of states
*/
typedef enum
{
    STATE_17_2_2_1_WAIT_1ST_RRC_RX_MEASURMENT_CONTROL,
    STATE_17_2_2_1_WAIT_2ND_RRC_RX_MEASURMENT_CONTROL,
    STATE_17_2_2_1_WAIT_AGPS_STATE_CHANGE,
    STATE_UL_DCCH,
    STATE_CALIBRATION,
    STATE_INVALID,
} agps_TestCaseState;

/**
* Type definition for a test case
*/
typedef struct
{
    const char *test_case;
    agps_TestCaseId test_case_id;
    agps_TestCaseState initial_state;
} agps_TestCase;

typedef enum
{
    RRC_DL_PDDU_MESSAGETYPE_MEASUREMENT_CONTROL=8,
}
RrcDlPduMessageType;

typedef struct
{
    agps_TestCaseId test_case_id;
    agps_TestCaseState state;
    const agps_GpalCallbacks *callbacks;
    void *agps_handle;
    char *param;
} agps_GpalTestState;

/**************************************************************************************************************/
/* Private Variable Definitions  */
/**************************************************************************************************************/

/**
 * test cases
 */
static const agps_TestCase agps_test_cases[] = {
    { "TC_17_2_2_1", TEST_17_2_2_1,     STATE_17_2_2_1_WAIT_1ST_RRC_RX_MEASURMENT_CONTROL},
    { "CAL",         TEST_CALIBRATION,  STATE_CALIBRATION,                               },
    { "UL_DCCH",     TEST_UL_DCCH,      STATE_UL_DCCH,                                   },
    { NULL, TEST_NOT_SUPPORTED, STATE_INVALID},                               /* terminator */
};

/**
 * Buffer to read frames
 */
static char agps_test_buffer[AGPS_TEST_MAX_PAYLOAD_SIZE];

/**
 * test vectors
 */

//static char MeasReportGPS_17_2_2_1[] = "440000000200000000000000"; // only measurement report
// UL_DCCH: 221386A2034F3880 (missing assistance data)
// UL_DCCH: 22138440 (no GPS satellites)
// UL_DCCH: 2213917978FC22195CEB71B05C001A0408001C30 (with GPS position)
static char UL_DCCH_GPS_17_2_2_1[]="2213917978FC22195CEB71B05C001A0408001C30"; // with GPS position
//static char UL_DCCH_GPS_17_2_2_1[]="205200900000000000000000000000"; // EventResult

/**
 * GPAL functional interface
 */
static const agps_GpalFunctions agps_gpal_test_functions = {
    agps_GpalTestInit,
    agps_GpalTestSendDownlinkMsg,
    agps_GpalTestSetUeState,
    agps_GpalTestResetAssistanceData,
    agps_GpalTestSetCalibrationStatus,
    agps_GpalTestDeInit,
};

/**
 * main state
 */
agps_GpalTestState agps_gpal_test_state;

/*************************************************************************************************
 * Private function definitions
 ************************************************************************************************/

static int agps_ExpectMsg(agps_GpalProtocol expected_protocol, int expected_pdu_type,
                           agps_GpalProtocol protocol, const char *buffer, int len)
{
    int ret = 0;
    unsigned char tmp;

    ALOGD("%s: Expecting protocol=0x%x pdu_type=0x%x \n", __FUNCTION__,
         expected_protocol, expected_pdu_type);

    do
    {
        if (protocol == expected_protocol)
        {
            if (AGPS_PDU_TYPE_DONT_CARE == expected_pdu_type)
            {
                ret = 1;
                break;
            }

            if (len>=1)
            {
                tmp = (unsigned char)buffer[0];

                if (tmp & 0x80)
                {
                    ALOGE("%s: Cannot handle packets with integrity check!!! \n", __FUNCTION__);
                }
                else
                {
                    /* dl packet size: bits[2..6] */
                    tmp = (tmp>>2) & 0x1f;

                    ALOGD("%s: packet pdu type = 0x%x \n", __FUNCTION__, tmp);
                    if (expected_pdu_type == tmp)
                    {
                        ret =1;
                        break;
                    }
                }
            }
        }
    } while (0);

    return ret;
}

static void agps_TestConvertAsciiToBin(char *outptr, const char *input, int *size)
{
    int i, offset1, offset2;
    int input_length = strlen( input )/2;

    for (i=0; i < input_length; i++)
    {
        if ((input[2*i])<=0x39)
            offset1 = 0X30;
        else
            if ((input[2*i])<=0x46)
            offset1 = 0x37;
        else
            offset1 = 0x57;

        if ((input[2*i+1])<=0x39)
            offset2 = 0X30;
        else
            if ((input[2*i+1])<=0x46)
            offset2 = 0x37;
        else
            offset2 = 0x57;

        * (outptr + i) = (unsigned short) ((input[2*i] - offset1) <<4) + (unsigned short) (input[2*i+1] - offset2);
    }

    *size = input_length;
}

static void agps_GpalTestSendVector(agps_GpalTestState *state, char *vector)
{
    char *test_vector;
    int vector_size;

    /* hex-encoded vector will always be smaller than ascii-encoded vector so
       we are allocating more memory than absolutely necessary... */
    test_vector = malloc( strlen(vector) );

    /* prepare test vector to send */
    agps_TestConvertAsciiToBin(test_vector, vector, &vector_size);

    /* send message */
    state->callbacks->send_uplink_msg(AGPS_PROTOCOL_RRC,
                                      test_vector,
                                      vector_size,
                                      state->agps_handle);

    free(test_vector);

}

static void *agps_TestCalibrationThreadFn(void *arg)
{
    sleep(5);

    ALOGI("%s: Start calibration\n", __FUNCTION__);

    agps_gpal_test_state.callbacks->calibration_start(agps_gpal_test_state.agps_handle);

    sleep(20);

    ALOGI("%s: Stop calibration\n", __FUNCTION__);

    agps_gpal_test_state.callbacks->calibration_stop(agps_gpal_test_state.agps_handle);

    return NULL;
}

static void *agps_GpalTestInit(const agps_GpalCallbacks *callbacks, void *agps_handle)
{
    agps_gpal_test_state.callbacks = callbacks;
    agps_gpal_test_state.agps_handle = agps_handle;
    agps_PortThread thread;

    switch (agps_gpal_test_state.test_case_id)
    {
    case TEST_UL_DCCH:
        agps_GpalTestSendVector(&agps_gpal_test_state, agps_gpal_test_state.param);
        break;
    case TEST_CALIBRATION:
        agps_PortCreateThread(&thread, agps_TestCalibrationThreadFn, NULL);
        break;
    default:
        /* do nothing */
        break;
    }

    return (void*)&agps_gpal_test_state;
}

static agps_ReturnCode agps_GpalTestSendDownlinkMsg(void *gpal_handle,
                                         agps_GpalProtocol protocol,
                                         const char *msg,
                                         unsigned int size)
{
    agps_ReturnCode ret = AGPS_OK;
    static char test_vector[100];
    int vector_size = 0;
    agps_GpalTestState *state;

    state = (agps_GpalTestState *)gpal_handle;

    switch (state->test_case_id)
    {
    case TEST_17_2_2_1:
        switch (state->state)
        {
        case STATE_17_2_2_1_WAIT_1ST_RRC_RX_MEASURMENT_CONTROL:
            if (agps_ExpectMsg(AGPS_PROTOCOL_RRC, /* expected protocol */
                               RRC_DL_PDDU_MESSAGETYPE_MEASUREMENT_CONTROL, /* expected PDU type */
                               protocol, /* received protocol */
                               msg, /* received message */
                               size /* received size */
                               ) )
            {
                state->state = STATE_17_2_2_1_WAIT_2ND_RRC_RX_MEASURMENT_CONTROL;
            }
            break;
        case STATE_17_2_2_1_WAIT_2ND_RRC_RX_MEASURMENT_CONTROL:
            if (agps_ExpectMsg(AGPS_PROTOCOL_RRC, /* expected protocol */
                               RRC_DL_PDDU_MESSAGETYPE_MEASUREMENT_CONTROL, /* expected PDU type */
                               protocol, /* received protocol */
                               msg, /* received message */
                               size /* received size */
                               ) )
            {
                state->state = STATE_17_2_2_1_WAIT_AGPS_STATE_CHANGE;

                sleep(2);

                /* send message */
                agps_GpalTestSendVector(state, UL_DCCH_GPS_17_2_2_1);
            }
            break;
        case STATE_17_2_2_1_WAIT_AGPS_STATE_CHANGE:
        case STATE_CALIBRATION:
        case STATE_UL_DCCH:
            /* always stay in that state */
            break;
        default:
            ALOGE("%s: Unsupported test case state (%d) for test case id (%d)\n",
                 __FUNCTION__, state->state, state->test_case_id);
            ret = -AGPS_ERROR_TESTCASE;
        }
        break;
    default:
        ALOGE("%s: Unsupported test case id (%d)\n",
             __FUNCTION__, state->test_case_id);
        ret = -AGPS_ERROR_TESTCASE;
    }

    return ret;
}

/* stub */
static agps_ReturnCode agps_GpalTestSetUeState(void *gps_handle,
                                    agps_GpalProtocol protocol,
                                    agps_GpalUeState state)
{
    return AGPS_OK;
}

/* stub */
static agps_ReturnCode agps_GpalTestResetAssistanceData(void *gps_handle)
{
    return AGPS_OK;
}

/* */
static agps_ReturnCode agps_GpalTestSetCalibrationStatus(void *gps_handle, int locked)
{
    ALOGI("%s: Calibration status = %d\n", __FUNCTION__, locked);
    return AGPS_OK;
}

/* stub */
static agps_ReturnCode agps_GpalTestDeInit(void *gps_handle)
{
    free(agps_gpal_test_state.param);
    return AGPS_OK;
}

/** return test case ID associated with provided test name */
static agps_TestCaseId agps_TestGetId(const char *name)
{
    agps_TestCaseId ret = TEST_NOT_SUPPORTED;
    const agps_TestCase *ptr = agps_test_cases;

    /* loop through all test cases and find the one that
       matches the name provided */
    do
    {
        if (!strcmp(ptr->test_case, name))
        {
            /* found the test case! */
            ret = ptr->test_case_id;
            break;
        }
        ptr++;
    } while (ptr->test_case_id != TEST_NOT_SUPPORTED);

    return ret;
}

/*************************************************************************************************
 * Public function definitions (Not doxygen commented, as they should be exported in the header
 * file)
 ************************************************************************************************/

const agps_GpalFunctions *agps_GpalTestCreate(const char *name)
{
    agps_TestCaseId id;
    const agps_GpalFunctions *ret;
    char *tmp;

    /* check if a parameter was provided */
    if ( (tmp=strstr(name, AGPS_TEST_PARAM_SEPARATOR)) != NULL )
    {
        /* remember parameter */
        agps_gpal_test_state.param = tmp + strlen(AGPS_TEST_PARAM_SEPARATOR);

        /* strip parameter off test name (although we're not supposed to
           modify a const char * ...) */
        tmp[0]='\0';
    }

    ALOGI("%s: looking up test case %s\n", __FUNCTION__, name);
    id = agps_TestGetId(name);

    if (id != TEST_NOT_SUPPORTED)
    {
        ret = &agps_gpal_test_functions;
        agps_gpal_test_state.test_case_id = id;
    }
    else
    {
        ALOGE("%s: test case is not supported\n", __FUNCTION__);
        ret = NULL;
    }

    return ret;
}




