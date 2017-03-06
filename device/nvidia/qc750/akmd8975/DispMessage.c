/******************************************************************************
 *
 * $Id: DispMessage.c 27 2011-03-23 07:31:31Z yamada.rj $
 *
 * -- Copyright Notice --
 *
 * Copyright (c) 2009 Asahi Kasei Microdevices Corporation, Japan
 * All Rights Reserved.
 *
 * This software program is proprietary program of Asahi Kasei Microdevices
 * Corporation("AKM") licensed to authorized Licensee under Software License
 * Agreement (SLA) executed between the Licensee and AKM.
 *
 * Use of the software by unauthorized third party, or use of the software
 * beyond the scope of the SLA is strictly prohibited.
 *
 * -- End Asahi Kasei Microdevices Copyright Notice --
 *
 ******************************************************************************/
#include "DispMessage.h"
#include "AKCommon.h"

/*!
 Print startup message to Android Log daemon.
 */
void Disp_StartMessage(void)
{
	LOGI("AK8975/B for Android v20110331 "
		 "(Library: v%d.%d.%d.%d) started.",
		 AKSC_GetVersion_Major(),
		 AKSC_GetVersion_Minor(),
		 AKSC_GetVersion_Revision(),
		 AKSC_GetVersion_DateCode());
	LOGI("Debug: %s", ((ENABLE_AKMDEBUG)?("ON"):("OFF")));
	LOGI("Debug level: %d", DBG_LEVEL);
	LOGI("Output to: %s", ((OUTPUT_STDOUT)?("stdout"):("LOGD")));
}

/*!
 Print ending message to Android Log daemon.
 */
void Disp_EndMessage(int ret)
{
	LOGI("AK8975/B for Android end (%d).", ret);
}

/*!
 Print calculated result.
 @param[in] prms A pointer to a #AK8975PRMS structure. The value of member 
 variables of this structure will be printed.
 */
void Disp_MeasurementResult(AK8975PRMS* prms)
{
	AKMDEBUG(DBG_LEVEL2, "FORMATION = %d\n", prms->m_form);

	if (prms->m_ds3Ret & 0x1) {
		AKMDEBUG(DBG_LEVEL1, "THETA=%6.1f, ", DISP_CONV_Q6F(prms->m_theta));
	} else {
		AKMDEBUG(DBG_LEVEL1, "THETA= -    , ");
	}
	if (prms->m_ds3Ret & 0x2) {
		AKMDEBUG(DBG_LEVEL1, "PITCH=%6.1f(%6.1f), ROLL=%6.1f(%6.1f)\n",
				 DISP_CONV_Q6F(prms->m_phi180),
				 DISP_CONV_Q6F(prms->m_phi90),
				 DISP_CONV_Q6F(prms->m_eta180),
				 DISP_CONV_Q6F(prms->m_eta90)
				 );
	} else {
		AKMDEBUG(DBG_LEVEL1, "PITCH= -    ( -    ), ROLL= -    ( -    )\n");
	}

	// hr is quintupled format (x5) .
	// 1LSB = 0.3uT  
	AKMDEBUG(DBG_LEVEL1, "HR[uT]=%5.1f\n", DISP_CONV_AKSCF(prms->m_hr));
	AKMDEBUG(DBG_LEVEL3, "HR HORIZ[uT]=%5.1f\n", DISP_CONV_AKSCF(prms->m_hrhoriz));
	AKMDEBUG(DBG_LEVEL3, "INCLINATION=%6.1f\n", DISP_CONV_Q6F(prms->m_delta));

	if(prms->m_hdoev.hthIdx == AKSC_HDFI_SMA) {	// Small
		AKMDEBUG(DBG_LEVEL2, "HDOE Parameter Set:Small\n");
	}
	else if(prms->m_hdoev.hthIdx == AKSC_HDFI_NOR) { // Normal
		AKMDEBUG(DBG_LEVEL2, "HDOE Parameter Set:Normal\n");
	}
	AKMDEBUG(DBG_LEVEL1, "LEVEL=%2d\n", prms->m_hdst);
	AKMDEBUG(DBG_LEVEL3, "HOFFSET[uT]:  x=%8.1f, y=%8.1f, z=%8.1f\n",
				DISP_CONV_AKSCF(prms->m_ho.u.x),
				DISP_CONV_AKSCF(prms->m_ho.u.y),
				DISP_CONV_AKSCF(prms->m_ho.u.z));
	AKMDEBUG(DBG_LEVEL3, "DOE HR[uT]=%5.1f\n", 
				DISP_CONV_AKSCF(prms->m_hdoev.hrdoeHR));

	AKMDEBUG(DBG_LEVEL1, "\n");
}

/*!
 Output main menu to stdout and wait for user input from stdin.
 @return Selected mode.
 */
MODE Menu_Main(void)
{
	char msg[20];
	memset(msg, 0, sizeof(msg));
	
	AKMDEBUG(DBG_LEVEL1, " --------------------  AK8975 Console Application -------------------- \n");
	AKMDEBUG(DBG_LEVEL1, "   T. Start Factory Shipment Test. \n");
	AKMDEBUG(DBG_LEVEL1, "   1. Start Single Measurement. \n");
	AKMDEBUG(DBG_LEVEL1, "   Q. Quit application. \n");
	AKMDEBUG(DBG_LEVEL1, " --------------------------------------------------------------------- \n\n");
	AKMDEBUG(DBG_LEVEL1, " Please select a number.\n");
	AKMDEBUG(DBG_LEVEL1, "   ---> ");
	fgets(msg, 10, stdin);
	AKMDEBUG(DBG_LEVEL1, "\n");
	
	// BUG : If 2-digits number is input,
	//    only the first character is compared.
	if (strncmp(msg, "T", 1) == 0 || strncmp(msg, "t", 1) == 0) {
		return MODE_FctShipmntTestBody;
	} else if (!strncmp(msg, "1", 1)) {
		return MODE_MeasureSNG;
	} else if (strncmp(msg, "Q", 1) == 0 || strncmp(msg, "q", 1) == 0) {
		return MODE_Quit;
	} else {
		return MODE_ERROR;
	}
}


/*!
 @return If @a testdata is in the range of between @a lolimit and @a hilimit,
 the return value is 1, otherwise -1.
 @param[in] testno   A pointer to a text string.
 @param[in] testname A pointer to a text string.
 @param[in] testdata A data to be tested.
 @param[in] lolimit  The maximum allowable value of @a testdata.
 @param[in] hilimit  The minimum allowable value of @a testdata.
 @param[in,out] pf_total 
 */
int16
TEST_DATA(const char testno[], 
		  const char testname[],
          const int16 testdata, 
		  const int16 lolimit, 
		  const int16 hilimit,
          int16 * pf_total)
{
	int16 pf;                     //Pass;1, Fail;-1
	
	if ((testno == NULL) && (strncmp(testname, "START", 5) == 0)) {
		// Display header
		AKMDEBUG(DBG_LEVEL1, "--------------------------------------------------------------------\n");
		AKMDEBUG(DBG_LEVEL1, " Test No. Test Name    Fail    Test Data    [      Low         High]\n");
		AKMDEBUG(DBG_LEVEL1, "--------------------------------------------------------------------\n");
		
		pf = 1;
	} else if ((testno == NULL) && (strncmp(testname, "END", 3) == 0)) {
		// Display result
		AKMDEBUG(DBG_LEVEL0, "--------------------------------------------------------------------\n");
		if (*pf_total == 1) {
			AKMDEBUG(DBG_LEVEL1, "Factory shipment test was passed.\n\n");
		} else {
			AKMDEBUG(DBG_LEVEL1, "Factory shipment test was failed.\n\n");
		}
		
		pf = 1;
	} else {
		if ((lolimit <= testdata) && (testdata <= hilimit)) {
			//Pass
			pf = 1;
		} else {
			//Fail
			pf = -1;
		}
		
		//display result
		AKMDEBUG(DBG_LEVEL1, " %7s  %-10s      %c    %9d    [%9d    %9d]\n", 
				 testno, testname, ((pf == 1) ? ('.') : ('F')), testdata,
				 lolimit, hilimit);
	}
	
	//Pass/Fail check
	if (*pf_total != 0) {
		if ((*pf_total == 1) && (pf == 1)) {
			*pf_total = 1;            //Pass
		} else {
			*pf_total = -1;           //Fail
		}
	}
	return pf;
}
