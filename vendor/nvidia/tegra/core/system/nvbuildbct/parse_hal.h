/**
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/*
 * The file contains apis for the hal parsing code.
 */


#ifndef INCLUDED_PARSE_HAL_H
#define INCLUDED_PARSE_HAL_H

#include "nvbuildbct.h"
#include "data_layout.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/* Set of function pointers to be used to access the hardware
 * interface for performing bct operations
*/
typedef struct BuildBctParseInterfaceRec
{

    void (*ProcessConfigFile)(BuildBctContext *Context);

    void (*GetBctSize)(BuildBctContext *Context);

    int  (*SetValue)(BuildBctContext *Context,
                      ParseToken Token,
                      NvU32 Value);

    void (*UpdateBct)(BuildBctContext *Context,
                    UpdateEndState EndState);

    void (*UpdateBl)(BuildBctContext *Context,
                    UpdateEndState EndState);

}BuildBctParseInterface;

void Ap20GetBuildBctInterf(BuildBctParseInterface *BuildBctInterf);
void t30GetBuildBctInterf(BuildBctParseInterface *BuildBctInterf);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_PARSE_HAL_H */

