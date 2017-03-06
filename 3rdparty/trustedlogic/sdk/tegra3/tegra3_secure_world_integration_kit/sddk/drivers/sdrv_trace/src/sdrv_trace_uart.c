/*
 * Copyright (c) 2007-2012 Trusted Logic S.A.
 * All Rights Reserved.
 *
 * This software is the confidential and proprietary information of
 * Trusted Logic S.A. ("Confidential Information"). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered
 * into with Trusted Logic S.A.
 *
 * TRUSTED LOGIC S.A. MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. TRUSTED LOGIC S.A. SHALL
 * NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

/*
 * File            : sdrv_trace_uart.c
 *
 * Last-Author     : Trusted Logic S.A.
 * Created         : December 08, 2010
 *
 */


#include <stdio.h>
#include <string.h>
#include "sddi.h"

/* Segment 1 gives the memory space of the UART */
#define SDRV_UART_A_SEGMENT_ID 1
/* Segment 1 gives the memory space of the UART */
#define SDRV_UART_B_SEGMENT_ID 2
/* Segment 1 gives the memory space of the UART */
#define SDRV_UART_C_SEGMENT_ID 3
/* Segment 1 gives the memory space of the UART */
#define SDRV_UART_D_SEGMENT_ID 4
/* Segment 1 gives the memory space of the UART */
#define SDRV_UART_E_SEGMENT_ID 5

/* Segment 2 gives the memory space of the Clocks */
#define SDRV_CLOCK_SEGMENT_ID 6




/* Struct used to Store traces info */
typedef struct{
uint32_t nUART_id;
uint32_t *traces_base_va;
uint32_t *clocks_base_va;
} TRACES_INFO;

static void printChar(void* pInstanceData, char c);



S_RESULT SDRV_EXPORT SDrvCreate(uint32_t nParam0, uint32_t nParam1)
{
   TRACES_INFO *pInfo;

   pInfo = SMemAlloc(sizeof(TRACES_INFO));
   if (pInfo == NULL)
   {
      return S_ERROR_OUT_OF_MEMORY;
   }

   /* Retrieve the Base VA */
   SServiceGetPropertyAsInt("config.register.uart.id",&pInfo->nUART_id); /* will be "0" on error */
   if (pInfo->nUART_id == 0)
   {
      SMemFree(pInfo);
      return S_ERROR_UNREACHABLE;
   }
   
   /* Retrieve the Base VA */
   if ((pInfo->traces_base_va = (uint32_t*)SMemGetVirtual(pInfo->nUART_id)) == NULL)
   {
      SMemFree(pInfo);
      return S_ERROR_UNREACHABLE;
   }

   if ((pInfo->clocks_base_va = (uint32_t*)SMemGetVirtual(SDRV_CLOCK_SEGMENT_ID)) == NULL)
   {
      SMemFree(pInfo);
      return S_ERROR_UNREACHABLE;
   }

   SInstanceSetData(pInfo);   
   return S_SUCCESS;
}

void SDRV_EXPORT SDrvDestroy(void)
{
   TRACES_INFO *pInfo = (TRACES_INFO *)SInstanceGetData();
   SMemFree(pInfo);
}

void SDRV_EXPORT SDrvTracePrint(
           IN     void*   pInstanceData,
         IN const char*   pString)
{
   while ((*pString)!=0)
   {
      printChar(pInstanceData, *pString);
      pString++;
   }
   printChar(pInstanceData, '\r');
//   printChar(pInstanceData, '\n');
}

void SDRV_EXPORT SDrvTraceSignal(void* pInstanceData, uint32_t nSignal, uint32_t nReserved)
{
   const char* pString;
   
   S_VAR_NOT_USED(nReserved);
   
   switch(nSignal)
   {
   case SDRV_SIGNAL_SW_BOOT:
      pString = "SDRV_SIGNAL_SW_BOOT\n";
      break;
   case SDRV_SIGNAL_SW_PANIC:
      pString = "SDRV_SIGNAL_SW_PANIC\n";
      break;
   default:
      pString = NULL;
      break;
   }
   if (pString != NULL)
   {
      SDrvTracePrint(pInstanceData, pString);
   }
}

bool SDRV_EXPORT SDrvHandleInterrupt(
                  void*     pInstanceData,
                  uint32_t  nInterruptId)
{
   S_VAR_NOT_USED(pInstanceData);
   S_VAR_NOT_USED(nInterruptId);

   /* We don't receive interrupts */
   return false;
}

static void printChar(
                  void*   pInstanceData,
                  char c)
{
   TRACES_INFO *pInfo = (TRACES_INFO *)pInstanceData;
   volatile unsigned int* pUartStatus=(volatile unsigned int*)(pInfo->traces_base_va+(0x14>>2));
   volatile unsigned int* pUartWrite= (volatile unsigned int*)(pInfo->traces_base_va);

   /* Check UART is turned ON */
   // if ((pInfo->clocks_base_va[0x320>>2]&0x40)==0)
   // {
   //   return;
   // }

   while (1)
   {
      if (((*pUartStatus)&0x60)!=0) break;
   }
   *pUartWrite=c;
}
