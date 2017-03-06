#include <errno.h>
#include <sys/ioctl.h>

#include <linux/tegra_dc_ext.h>

#include <nvdc.h>

#include "nvdc_priv.h"

int nvdcGetCursor(struct nvdcState *state, int head)
{
    int ret = 0;

    if (!NVDC_VALID_HEAD(head)) {
        return EINVAL;
    }

    if (ioctl(state->tegraExtFd[head], TEGRA_DC_EXT_GET_CURSOR)) {
        ret = errno;
    }

    return ret;
}

int nvdcPutCursor(struct nvdcState *state, int head)
{
    int ret = 0;

    if (!NVDC_VALID_HEAD(head)) {
        return EINVAL;
    }

    if (ioctl(state->tegraExtFd[head], TEGRA_DC_EXT_PUT_CURSOR)) {
        ret = errno;
    }

    return ret;
}

int nvdcSetCursorImage(struct nvdcState *state, int head,
                       struct nvdcCursorImage *args)
{
    struct tegra_dc_ext_cursor_image image = {  };
    int ret = 0;

    if (!NVDC_VALID_HEAD(head)) {
        return EINVAL;
    }

    image.foreground.r = args->foreground.r;
    image.foreground.g = args->foreground.g;
    image.foreground.b = args->foreground.b;
    image.background.r = args->background.r;
    image.background.g = args->background.g;
    image.background.b = args->background.b;

    image.buff_id = args->bufferId;

    switch (args->size) {
        case NVDC_CURSOR_IMAGE_SIZE_64x64:
            image.flags |= TEGRA_DC_EXT_CURSOR_IMAGE_FLAGS_SIZE_64x64;
            break;
        case NVDC_CURSOR_IMAGE_SIZE_32x32:
            image.flags |= TEGRA_DC_EXT_CURSOR_IMAGE_FLAGS_SIZE_32x32;
            break;
        default:
            return EINVAL;
    }

    if (ioctl(state->tegraExtFd[head], TEGRA_DC_EXT_SET_CURSOR_IMAGE, &image)) {
        ret = errno;
    }

    return ret;
}

int nvdcSetCursor(struct nvdcState *state, int head, int x, int y, int visible)
{
    struct tegra_dc_ext_cursor cursor = { };
    int ret = 0;

    if (!NVDC_VALID_HEAD(head)) {
        return EINVAL;
    }

    cursor.x = x;
    cursor.y = y;
    if (visible) {
        cursor.flags |= TEGRA_DC_EXT_CURSOR_FLAGS_VISIBLE;
    }

    if (ioctl(state->tegraExtFd[head], TEGRA_DC_EXT_SET_CURSOR, &cursor)) {
        ret = errno;
    }

    return ret;
}
