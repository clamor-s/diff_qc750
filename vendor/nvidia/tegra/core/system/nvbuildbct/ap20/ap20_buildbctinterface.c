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
 * set.c - State setting support for the buildimage tool
 */

/*
 * TODO / Notes
 * - Add doxygen commentary
 */

#include "math.h"


#include "nvapputil.h"
#include "nvassert.h"

#include "ap20_set.h"
#include "ap20_parse.h"
#include "ap20/nvboot_bct.h"
#include "../nvbuildbct.h"
#include "ap20_data_layout.h"

#include "../parse_hal.h"

static void Ap20GetBctSize(BuildBctContext *Context)
{
    Context->NvBCTLength =  sizeof(NvBootConfigTable);
}

void Ap20GetBuildBctInterf(BuildBctParseInterface *pBuildBctInterf)
{
    pBuildBctInterf->ProcessConfigFile = Ap20ProcessConfigFile;
    pBuildBctInterf->GetBctSize = Ap20GetBctSize;
    pBuildBctInterf->SetValue = Ap20SetValue;
    pBuildBctInterf->UpdateBct = Ap20UpdateBct;
    pBuildBctInterf->UpdateBl = Ap20UpdateBl;

}
