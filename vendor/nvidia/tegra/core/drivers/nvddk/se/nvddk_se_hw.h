/*
 * Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#ifndef INCLUDED_NVDDK_SE_HW_H
#define INCLUDED_NVDDK_SE_HW_H

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * Disables the Key Schedule read
 *
 * @retval NvSuccess Key schedule read is disabled.
 * @retval NvError_NotInitialized Context is not initialized.
 */
NvError SeKeySchedReadDisable(void);

/**
 * Clears all h/w interrupts.
 */
void  SeClearInterrupts(void);

/**
 * Processes the Buffers specified in SHA context
 *
 * @param hSeShaHwCtx SHA module context
 *
 * @retval NvSuccess Buffer have been successfully processed.
 * @retval NvError_NotInitialized Context is not initialized.
 */
NvError SeSHAProcessBuffer(NvDdkSeShaHWCtx *hSeShaHwCtx);

/**
 * Takes backup of MSGLEFT and MSGLENGTH register contents
 *
 * @param hSeShaHwCtx SHA module context
 *
 * @retval NvSuccess Backup is done without any error.
 * @retval NvError_NotInitialized Context is not initialized.
 */
NvError SeSHABackupRegisters(NvDdkSeShaHWCtx *hSeShaHwCtx);

/**
 * Waits for the operation to complete
 *
 * @param args module context when called by Interrupt handler
 *
 */
void SeIsr(void* args);

/**
 * Sets the Aes key in the keyslot specified.
 *
 * @param hSeAesHwCtx Aes Module Context
 *
 * @retval NvSuccess Buffer has been successfully processed
 * @retval NvError_NotInitialized Context is not initialized.
 */
NvError SeAesSetKey(NvDdkSeAesHWCtx *hSeAesHwCtx);

/**
 * Processes the buffers specified in Aes Context
 *
 * @param hSeAesHwCtx AES module Context
 *
 * @retval NvSuccess Buffers have been successfully processed.
 * @retval NvError_NotInitialized Context is not initialized.
 */
NvError SeAesProcessBuffer(NvDdkSeAesHWCtx *hSeAesHwCtx);

/**
 * Processes the buffers specified in Aes Context
 *
 * @param hSeAesHwCtx AES module Context
 * @retval NvSuccess Buffers have been successfully processed
 * @retval NvError_NotInitialized Context is not initialized.
 */
NvError SeAesCMACProcessBuffer(NvDdkSeAesHWCtx *hSeAesHwCtx);

/**
 * Processes the buffers specified in Aes Context
 *
 * @param hSeAesHwCtx AES module Context
 * @retval NvSuccess Buffers have been successfully processed
 * @retval NvError_NotInitialized Context is not initialized.
 */
NvError CollectCMAC(NvU32 *pBuffer);

/**
 * Write locks the key in specified key slot
 *
 * @param KeySlotIndex to write lock particular key in the slot
 */
void SeAesWriteLockKeySlot(SeAesHwKeySlot KeySlotIndex);

/**
 * Read locks the key in the specified key slot
 *
 * @param KeySlotIndex to Read lock particular key in the slot
 */
void SeAesReadLockKeySlot(NvU32 KeySlotIndex);

/**
 * Writes the Iv in the specified key slot
 *
 * @param hSeAesHwCtx AES module Context
 */
void SeAesSetIv(NvDdkSeAesHWCtx *hSeAesHwCtx);
#if defined(__cplusplus)
}
#endif

#endif
