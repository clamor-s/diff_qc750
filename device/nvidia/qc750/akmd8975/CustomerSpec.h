/******************************************************************************
 *
 * $Id: CustomerSpec.h 43 2011-03-28 09:05:30Z yamada.rj $
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
#ifndef AKMD_INC_CUSTOMERSPEC_H
#define AKMD_INC_CUSTOMERSPEC_H

/*******************************************************************************
 User defines parameters.
 ******************************************************************************/

// Certification information
#define CSPEC_CI_AK_DEVICE	8975
#define CSPEC_CI_LICENSER	"ASAHIKASEI"
#define CSPEC_CI_LICENSEE	"KIONIX_8975_GB"

// Parameters for Average
//	The number of magnetic data block to be averaged.
//	 NBaveh*(*nh) must be 1, 2, 4, 8 or 16.
#define CSPEC_HNAVE		8

// Parameters for Direction Calculation
#define CSPEC_DVEC_X		0
#define CSPEC_DVEC_Y		0
#define CSPEC_DVEC_Z		0

// The number of formation
#define CSPEC_NUM_FORMATION		2

// the counter of Suspend
#define CSPEC_CNTSUSPEND_SNG	8

// Parameters for FctShipmntTest
//  1 : USE SPI
//  0 : NOT USE SPI(I2C)
#define CSPEC_SPI_USE			0     

// Setting file
#define CSPEC_SETTING_FILE	"/data/misc/compass/akmd_set.txt"

#endif

