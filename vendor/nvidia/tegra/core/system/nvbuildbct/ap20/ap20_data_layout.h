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
 * data_layout.h - Definitions for the buildimage data layout code.
 */

/*
 * TODO / Notes
 * - Add doxygen commentary
 */

#ifndef INCLUDED_AP20_DATA_LAYOUT_H
#define INCLUDED_AP20_DATA_LAYOUT_H

#include "../nvbuildbct_config.h"
#include "../nvbuildbct.h"
#include "../data_layout.h"

#if defined(__cplusplus)
extern "C"
{
#endif

void
Ap20UpdateBct(struct BuildBctContextRec *Context, UpdateEndState EndState);

void
Ap20UpdateBl(struct BuildBctContextRec *Context, UpdateEndState EndState);


#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_DATA_LAYOUT_H */
