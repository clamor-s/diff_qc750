/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVMM_POWERMANAGER_H
#define INCLUDED_NVMM_POWERMANAGER_H

#include "nvcommon.h"
#include "nvos.h"
#include "nvmm.h"
#include "nvmm_manager.h"

 /**
 * @brief Initializes and opens the NvMM Power manager. 
 * @retval NvSuccess Indicates that the NvmmContextManager has successfully opened.
 * @retval NvError_InsufficientMemory Indicates that function fails to allocate
 * the memory.
 * @retval NvError_NotInitialized Indicates the NvmmContextManager initialization failed.
 */

 NvError NvmmContextManagerOpen(NvmmManagerHandle hNvmmMgr);

 /** 
 * @brief Closes the NvmmContextManager. This function de-initializes the NvmmContextManager. 
 * This API never fails.
 *
 * @param None
 */

 void NvmmContextManagerClose(void); 

/** 
 * @brief Registers NvMMBlock handle with the PM
 *
 * @param hBlock Handle to the NvMMBlock.
 * 
 * @retval NvSuccess Indicates the operation succeeded.
 */
 NvError NvmmContextManagerRegisterBlock(NvMMBlockHandle hBlock);

/** 
 * @brief Unregisters NvMMBlock handle from the PM
 *
 * @param hBlock Handle to the NvMMBlock.
 * @retval NvSuccess Indicates the operation succeeded.
 */
NvError NvmmContextManagerUnregisterBlock(NvMMBlockHandle hBlock);


/** 
 * @brief The block calls back this API when its state change is done. 
 *
 */
NvBool NvmmContextManagerNotify(void);

#endif 

