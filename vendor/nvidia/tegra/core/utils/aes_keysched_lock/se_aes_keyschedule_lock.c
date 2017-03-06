/* Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvos.h"
#include "t30/arse.h"
#include "t30/arapbpm.h"
#include "nvrm_drf.h"
#include "se_aes_keyschedule_lock.h"

#define SE_BASE_ADDRESS 0x70012000
#define PMC_PA_BASE     0x7000E400

#define NV_READ32(a)        *((const volatile NvU32 *)(a))
#define NV_WRITE32(a,d)     *((volatile NvU32 *)(a)) = (d)

#define SE_REGR(reg) \
     NV_READ32(SE_BASE_ADDRESS + ((SE_##reg##_0)))

#define SE_REGW(reg, data) \
do \
{ \
    NV_WRITE32((SE_BASE_ADDRESS + (SE_##reg##_0)), (data)); \
}while (0)

static void NvTestSeAesKeyScheduleLock(void);
void NvLockSeAesKeySchedule(void)
{
    volatile NvU32 reg_val;

    reg_val = SE_REGR(SE_SECURITY);
    reg_val = NV_FLD_SET_DRF_NUM(SE, SE_SECURITY,KEY_SCHED_READ,\
                                  SE_SE_SECURITY_0_KEY_SCHED_READ_DISABLE, reg_val);
    SE_REGW(SE_SECURITY, reg_val);
    NvTestSeAesKeyScheduleLock();
}

static void NvTestSeAesKeyScheduleLock(void)
{
    NvU32 reg_val;

    reg_val = SE_REGR(SE_SECURITY);
    if (NV_DRF_VAL(SE, SE_SECURITY, KEY_SCHED_READ, reg_val)
            != SE_SE_SECURITY_0_KEY_SCHED_READ_DISABLE)
    {
        //Reset the chip
        reg_val = NV_READ32(PMC_PA_BASE + APBDEV_PMC_CNTRL_0);
        reg_val = NV_FLD_SET_DRF_DEF(APBDEV_PMC, CNTRL, MAIN_RST, ENABLE, reg_val);
        NV_WRITE32(PMC_PA_BASE + APBDEV_PMC_CNTRL_0, reg_val);
        while(1);
    }
}
