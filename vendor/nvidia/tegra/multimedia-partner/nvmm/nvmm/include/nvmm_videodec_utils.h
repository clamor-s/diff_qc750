/* draft version : 2.0 */
/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVMM_VIDEODEC_UTILS_H
#define INCLUDED_NVMM_VIDEODEC_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Function to copy SDRAM to IRAM using AHB DMA.
 * pSdram[input] : SDRAM address
 * pIram[input] : IRAM address
 * Size : Size in words
 */
#define AHB_DMA_BASE                    0x60008000 /* AHB DMA controlller */
#define AHB_DMA_CMD                     ((AHB_DMA_BASE + 0x00)) /* DMA command register         */
#define CC_Base                         0x6000C000      // Cache Control for direct access to cache RAMs
#define CACHE_CTRL                      ((volatile unsigned *) (CC_Base     ))  // Cache Control
#define AHB_DMA_CH0                     0x60009000 /* AHB DMA Channel 0 */
#define AHB_DMA_CH1                     0x60009020 /* AHB DMA Channel 1 */
#define AHB_DMA_CH2                     0x60009040 /* AHB DMA Channel 2 */
#define AHB_DMA_CH3                     0x60009060 /* AHB DMA Channel 3 */
#define HDMA_CHx_STR_MEM_PTR            ((ChannelBaseAdd + 0x10))      /* DMA CH0 Starting memory address */
#define HDMA_CHx_MEM_SEQ                ((ChannelBaseAdd + 0x14))      /* DMA CH0 Memory sequencer */
#define HDMA_CHx_STR_XMEM_PTR           ((ChannelBaseAdd + 0x18))      /* DMA CH0 external memory starting pointer */
#define HDMA_CHx_XMEM_SEQ               ((ChannelBaseAdd + 0x1c))      /* DMA CH0 external Memory sequencer */
#define HDMA_CHx_CSR                    ((ChannelBaseAdd + 0x00))      /* DMA CH0 control register     */
#define HDMA_CHx_STA                    ((ChannelBaseAdd + 0x04))      /* DMA CH0 Status register  */
#define CLK_BASE                        0x60006000
#define CLK_OUT_ENB_H_SET_0             ((CLK_BASE + 0x328))
#define CLK_OUT_ENB_H_CLR_0             ((CLK_BASE + 0x32c))
#define CLK_OUT_RST_H_SET_0             ((CLK_BASE + 0x308))
#define CLK_OUT_RST_H_CLR_0             ((CLK_BASE + 0x30c))

#define HDMA_CH0_STR_MEM_PTR            ((AHB_DMA_CH0 + 0x10))      /* DMA CH0 Starting memory address */
#define HDMA_CH0_MEM_SEQ                ((AHB_DMA_CH0 + 0x14))      /* DMA CH0 Memory sequencer */
#define HDMA_CH0_STR_XMEM_PTR           ((AHB_DMA_CH0 + 0x18))      /* DMA CH0 external memory starting pointer */
#define HDMA_CH0_XMEM_SEQ               ((AHB_DMA_CH0 + 0x1c))      /* DMA CH0 external Memory sequencer */
#define HDMA_CH0_CSR                    ((AHB_DMA_CH0 + 0x00))      /* DMA CH0 control register     */
#define HDMA_CH0_STA                    ((AHB_DMA_CH0 + 0x04))      /* DMA CH0 Status register  */

#define HDMA_CH1_STR_MEM_PTR            ((AHB_DMA_CH1 + 0x10))      /* DMA CH0 Starting memory address */
#define HDMA_CH1_MEM_SEQ                ((AHB_DMA_CH1 + 0x14))      /* DMA CH0 Memory sequencer */
#define HDMA_CH1_STR_XMEM_PTR           ((AHB_DMA_CH1 + 0x18))      /* DMA CH0 external memory starting pointer */
#define HDMA_CH1_XMEM_SEQ               ((AHB_DMA_CH1 + 0x1c))      /* DMA CH0 external Memory sequencer */
#define HDMA_CH1_CSR                    ((AHB_DMA_CH1 + 0x00))      /* DMA CH0 control register     */
#define HDMA_CH1_STA                    ((AHB_DMA_CH1 + 0x04))      /* DMA CH0 Status register  */

#define HDMA_CH2_STR_MEM_PTR            ((AHB_DMA_CH2 + 0x10))      /* DMA CH0 Starting memory address */
#define HDMA_CH2_MEM_SEQ                ((AHB_DMA_CH2 + 0x14))      /* DMA CH0 Memory sequencer */
#define HDMA_CH2_STR_XMEM_PTR           ((AHB_DMA_CH2 + 0x18))      /* DMA CH0 external memory starting pointer */
#define HDMA_CH2_XMEM_SEQ               ((AHB_DMA_CH2 + 0x1c))      /* DMA CH0 external Memory sequencer */
#define HDMA_CH2_CSR                    ((AHB_DMA_CH2 + 0x00))      /* DMA CH0 control register     */
#define HDMA_CH2_STA                    ((AHB_DMA_CH2 + 0x04))      /* DMA CH0 Status register  */

#define HDMA_CH3_STR_MEM_PTR            ((AHB_DMA_CH3 + 0x10))      /* DMA CH0 Starting memory address */
#define HDMA_CH3_MEM_SEQ                ((AHB_DMA_CH3 + 0x14))      /* DMA CH0 Memory sequencer */
#define HDMA_CH3_STR_XMEM_PTR           ((AHB_DMA_CH3 + 0x18))      /* DMA CH0 external memory starting pointer */
#define HDMA_CH3_XMEM_SEQ               ((AHB_DMA_CH3 + 0x1c))      /* DMA CH0 external Memory sequencer */
#define HDMA_CH3_CSR                    ((AHB_DMA_CH3 + 0x00))      /* DMA CH0 control register     */
#define HDMA_CH3_STA                    ((AHB_DMA_CH3 + 0x04))      /* DMA CH0 Status register  */

NvError NvVideoDecExternalAlloc(NvRmDeviceHandle hRmDevice, NvU32 Size, NvU32 Alignment,
                                NvVideoDecMemory *mem);

NvError NvVideoDecIramAlloc(NvRmDeviceHandle hRmDevice, NvU32 Size, NvU32 Alignment, 
                            NvVideoDecMemory *mem);

void NvVideoDecMemFreeNew(NvVideoDecMemory mem);

#if NV_IS_AVP

NvError NvVideoDecScratchIramAlloc(VideoDecBlockContext *pCommonVideoDecBlockContext,
                                    NvVideoDecMemory *mem);

void NvVideoDecScratchIramFree(VideoDecBlockContext *pCommonVideoDecBlockContext,
                                    NvVideoDecMemory *mem);
#endif

NvError NvMMVideoDecCheckVdeCapabilities(NvRmDeviceHandle RmHandle, NvRmModuleID ModuleId, VidoeoDecFeature *FeatureSupported);

NvBool VideoDecBlockCheckResolution(NvRmDeviceHandle hRmDevice, NvU32 NumMbs, NvMMBlockType eType);

void SdramIramMemCopyAhbDma(NvU32 *pSdram1, NvU32 *pIram1, NvU32 Size1, NvU32 channel1,NvU32 *pSdram2, NvU32 *pIram2, NvU32 Size2, NvU32 channel2);

void SdramIramMemCopyAhbDmaSingleChannel(NvU32 *pSdram1, NvU32 *pIram1, NvU32 Size1, NvU32 channel1);
void SdramIramMemCopyAhbDmaSingleChannelU8(NvU8 *pSdram1, NvU8 *pIram1, NvU32 Size1, NvU32 channel1);
void SdramIramMemCopyAhbDmaSingleChannelTiled(NvU32 *pSdram1, NvU32 *pIram1, NvU32 sdramSeq, NvU32 iramSeq, NvU32 csrVal, NvU32 channel1);
void AhbDmaEnable(NvU32 *Channel0BaseAddress, NvU32 *Channel1BaseAddress, 
            NvU32 *Channel2BaseAddress, NvU32 *Channel3BaseAddress);
            
void AhbDmaDisable(void);

void WaitForDmaDone(NvU32 channel1,NvU32 channel2);

void WaitForDmaDoneSingleChannel(NvU32 channel1);

#ifdef __cplusplus
}
#endif

#endif  // INCLUDED_NVMM_VIDEODEC_UTILS_H

