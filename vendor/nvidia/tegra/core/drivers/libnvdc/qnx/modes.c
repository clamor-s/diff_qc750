/*
 * Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <errno.h>
#include <string.h>
#include <qnx/nvdisp_devctls.h>
#include <nvdc.h>
#include "nvdc_priv.h"

int nvdcGetMode(struct nvdcState *state, int head, struct nvdcMode *mode)
{
    int ret;

    if (!NVDC_VALID_HEAD(head) || (mode == NULL) || (state == NULL)) {
        nvdc_error("%s failed: invalid head/NULL ptr\n",__func__);
        return EINVAL;
    }

    ret = devctl(state->dispFd[head], NVDISP_GET_MODE,
                 mode, sizeof(*mode), NULL);
    if (ret != EOK) {
        nvdc_error("%s: failed: devctl failure\n",__func__);
    } else {
        nvdc_info("MODE:\n \
                hActive = %d\n \
                vActive = %d\n \
                hSyncWidth = %d\n \
                vSyncWidth = %d\n \
                hFrontPorch = %d\n \
                vFrontPorch = %d\n \
                hBackPorch = %d\n \
                vBackPorch = %d\n \
                hRefToSync = %d\n \
                vRefToSync = %d\n \
                pclkKHz = %d\n", \
                mode->hActive, \
                mode->vActive, \
                mode->hSyncWidth, \
                mode->vSyncWidth, \
                mode->hFrontPorch, \
                mode->vFrontPorch, \
                mode->hBackPorch, \
                mode->vBackPorch, \
                mode->hRefToSync, \
                mode->vRefToSync, \
                mode->pclkKHz);
    }

    return ret;
}

int nvdcValidateMode(struct nvdcState *state, int head, struct nvdcMode *mode)
{
    nvdc_info("SKIPPING %s \n",__func__);
    return 0;
}

int nvdcSetMode(struct nvdcState *state, int head, struct nvdcMode *mode)
{
    int ret;

    if (!NVDC_VALID_HEAD(head) || (mode == NULL) || (state == NULL)) {
        nvdc_error("%s failed: invalid head/NULL ptr\n",__func__);
        return EINVAL;
    }

    ret = devctl(state->dispFd[head], NVDISP_SET_MODE,
                 mode, sizeof(*mode), NULL);
    if (ret != EOK) {
        nvdc_error("%s failed: devctl failure\n", __func__);
    }

    return ret;
}

int nvdcDPMS(struct nvdcState *state, int head, enum nvdcDPMSmode mode)
{
    nvdc_error("%s unimplemented\n",__func__);
    return ENOANO;
}

int nvdcQueryHeadStatus(struct nvdcState *state, int head,
                        struct nvdcHeadStatus *nvdcStatus)
{
    /*
     * FIXME: Currently, under QNX we assume both heads are enabled.
     */
    if ((state == NULL) || (nvdcStatus == NULL))
        return EINVAL;

    nvdcStatus->enabled = true;
    return 0;
}
