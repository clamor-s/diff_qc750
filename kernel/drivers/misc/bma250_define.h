/*
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2010 Bosch Sensortec GmbH
 * All Rights Reserved
 */

/* EasyCASE V6.5 14/04/2010 16:02:49 */
/* EasyCASE O
If=horizontal
LevelNumbers=no
LineNumbers=no
Colors=16777215,0,12582912,12632256,0,0,0,16711680,8388736,0,33023,32768,0,0,0,0,0,32768,12632256,255,65280,255,255,16711935
ScreenFont=System,,100,1,-13,0,700,0,0,0,0,0,0,1,2,1,34,96,96
PrinterFont=Courier,,100,1,-85,0,400,0,0,0,0,0,0,1,2,1,49,600,600
LastLevelId=3 */
/* EasyCASE ( 1 */
/* EasyCASE < */
/*	$Date: 2008/10/07 18:33:57 $
 *	$Revision: 1.21 $
 */

/*! \file bma250_define.h 
	\brief Header file for all define, constants and function prototypes
*/
#ifndef __DEFINE_HEADER
#define __DEFINE_HEADER

/*-----------------------------------------------------------------------------------------------*/
/* Includes*/
/*-----------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------*/
/* Defines*/
/*-----------------------------------------------------------------------------------------------*/

#ifndef NULL
#ifdef __cplusplus		// EC++
#define NULL	0
#else
#define NULL	((void *) 0)
#endif
#endif

typedef char S8;
typedef unsigned char U8;
typedef short S16;
typedef unsigned short U16;
typedef int S32;
typedef unsigned int U32;
typedef long long S64;
typedef unsigned long long U64;
typedef unsigned char BIT;
typedef unsigned int BOOL;
typedef double F32;

#define ON					1	/**< Define for "ON" */
#define OFF					0	/**< Define for "OFF" */
#define TRUE					1	/**< Define for "TRUE" */
#define FALSE					0	/**< Define for "FALSE" */
#define ENABLE					1	/**< Define for "ENABLE" */
#define DISABLE					0	/**< Define for "DISABLE" */
#define LOW					0	/**< Define for "Low" */
#define HIGH					1	/**< Define for "High" */
#define INPUT					0	/**< Define for "Input" */
#define OUTPUT					1	/**< Define for "Output" */

#define	 C_Null_U8X				(U8)0
#define	 C_Zero_U8X				(U8)0
#define	 C_One_U8X				(U8)1
#define	 C_Two_U8X				(U8)2
#define	 C_Three_U8X				(U8)3
#define	 C_Four_U8X				(U8)4
#define	 C_Five_U8X				(U8)5
#define	 C_Six_U8X				(U8)6
#define	 C_Seven_U8X				(U8)7
#define	 C_Eight_U8X				(U8)8
#define	 C_Nine_U8X				(U8)9
#define	 C_Ten_U8X				(U8)10
#define	 C_Eleven_U8X				(U8)11
#define	 C_Twelve_U8X				(U8)12
#define	 C_Sixteen_U8X				(U8)16
#define	 C_TwentyFour_U8X			(U8)24
#define	 C_ThirtyTwo_U8X			(U8)32
#define	 C_Hundred_U8X				(U8)100
#define	 C_OneTwentySeven_U8X			(U8)127
#define	 C_TwoFiftyFive_U8X			(U8)255
#define	 C_TwoFiftySix_U16X			(U16)256
/* Return type is True */
#define C_Successful_S8X			(S8)0
/* return type is False */
#define C_Unsuccessful_S8X			(S8)-1

typedef enum {
	E_False,
	E_True
} te_Boolean;
/* EasyCASE > */
/* EasyCASE - */

#endif				/*	__DEFINE_HEADER */
/* EasyCASE ) */

