/*
 * Copyright (c) 2012, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>

#include "nvassert.h"
#include "nvcommon.h"
#include "nvhwc_didim.h"

#define PATH_TEGRA_DC_0                  "/sys/class/graphics/fb0/device/"
#define PATH_SMARTDIMMER                 PATH_TEGRA_DC_0 "smartdimmer/"
#define PATH_SMARTDIMMER_ENABLE          PATH_SMARTDIMMER "enable"
#define PATH_SMARTDIMMER_AGGRESSIVENESS  PATH_SMARTDIMMER "aggressiveness"

#define NV_PROPERTY_DIDIM_ENABLE                 TEGRA_PROP(didim.enable)
#define NV_PROPERTY_DIDIM_ENABLE_DEFAULT         1

/* Copied from nvhwc.c: This should be moved to a common header file */
#define TEGRA_PROP_PREFIX "persist.tegra."
#define TEGRA_PROP(s) TEGRA_PROP_PREFIX #s

/* These properties control the smartdimmer aggressiveness levels for
 * normal and video content.  The value range is 0 (off) through 5
 * (most aggressive).
 */
#define NV_PROPERTY_DIDIM_NORMAL                TEGRA_PROP(didim.normal)
#define NV_PROPERTY_DIDIM_NORMAL_DEFAULT        1

#define NV_PROPERTY_DIDIM_VIDEO                 TEGRA_PROP(didim.video)
#define NV_PROPERTY_DIDIM_VIDEO_DEFAULT         3

#define DIDIM_LEVEL_MIN 0
#define DIDIM_LEVEL_MAX 5

#define HWC_PRIORITY_LVL (1 << 3)

enum didimEnable {
    didimEnableOff = 0,
    didimEnableOn,
};

enum didimLevel {
    didimLevelOff = 0,
    didimLevel1,
    didimLevel2,
    didimLevel3,
    didimLevel4,
    didimLevel5,
};

enum didimVersion {
    didimVersionNone = 0,
    didimVersion1,
};

struct didim_state {
    struct didim_client client;

    enum didimVersion version;

    union {
        /* DIDIM 1 */
        struct {
            enum didimLevel normal;
            enum didimLevel video;
            enum didimLevel current;
        } level;
    };
};

static int sysfs_read_int(const char *pathname, int *value)
{
    int len, fd = open(pathname, O_RDONLY);
    char buf[20];

    if (fd < 0) {
        ALOGE("%s: open failed: %s", pathname, strerror(errno));
        return -1;
    }

    len = read(fd, buf, sizeof(buf)-1);
    if (len < 0) {
        ALOGE("%s: read failed: %s", pathname, strerror(errno));
    } else {
        buf[len] = '\0';
        *value = atoi(buf);
    }

    close(fd);

    return len < 0 ? len : 0;
}

static int sysfs_write_int(const char *pathname, int value)
{
    int len, fd = open(pathname, O_WRONLY);
    char buf[20];

    if (fd < 0) {
        ALOGE("%s: open failed: %s", pathname, strerror(errno));
        return -1;
    }

    snprintf(buf, sizeof(buf), "%d", value);

    len = write(fd, buf, strlen(buf));
    if (len < 0) {
        ALOGE("%s: write failed: %s", pathname, strerror(errno));
    }

    close(fd);

    return len < 0 ? len : 0;
}

static int sysfs_write_string(const char *pathname, const char *buf)
{
    int len, fd = open(pathname, O_WRONLY);

    if (fd < 0) {
        return -1;
    }

    len = write(fd, buf, strlen(buf));
    if (len < 0) {
        ALOGE("%s: write failed: %s", pathname, strerror(errno));
    }

    close(fd);

    return len < 0 ? len : 0;
}

static int get_property_int(const char *key, int default_value)
{
    char value[PROPERTY_VALUE_MAX];

    if (property_get(key, value, NULL) > 0) {
        return strtol(value, NULL, 0);
    }

    return default_value;
}

static int didim_get_env_property(const char *key, int default_value)
{
    const char *env = getenv(key);
    int value;

    if (env) {
        value = atoi(env);
    } else {
        value = get_property_int(key, default_value);
    }

    return value;
}

static int didim_get_level_property(const char *key, int default_value)
{
    int value;

    value = didim_get_env_property(key, default_value);

    if (DIDIM_LEVEL_MIN > value) {
        value = DIDIM_LEVEL_MIN;
    } else if (value > DIDIM_LEVEL_MAX) {
        value = DIDIM_LEVEL_MAX;
    }
    return value;
}

static inline int r_equal(const hwc_rect_t* r1,
                          const hwc_rect_t* r2)
{
    return (r1->left == r2->left && r1->right == r2->right &&
            r1->top == r2->top && r1->bottom == r2->bottom);
}

#define r_width(r) ((r)->right - (r)->left)
#define r_height(r) ((r)->bottom - (r)->top)

static int didim_get_version(void)
{
    int enable;
    int version;
    int status;

    enable = sysfs_read_int(PATH_SMARTDIMMER_ENABLE, &version);

    if (enable < 0) {
        version = didimVersionNone;
    } else {
        version = didimVersion1;
    }

    /* Get enable property, if there is one defined */
    enable = didim_get_env_property(
        NV_PROPERTY_DIDIM_ENABLE, NV_PROPERTY_DIDIM_ENABLE_DEFAULT);

    /* Set the enable parameter for didim */
    status = sysfs_write_int(PATH_SMARTDIMMER_ENABLE, enable);

    if (status) {
        ALOGE("Failed to set initial DIDIM status to %s",
              enable ? "enable" : "disable");
    }

    /* If enable property is set to 0, we want to set version to None */
    if (!enable) {
        version = didimVersionNone;
    }

    return version;
}

static inline void didim_level(struct didim_state *didim, int level)
{
    int enable, status;

    NV_ASSERT(DIDIM_LEVEL_MIN <= level && level <= DIDIM_LEVEL_MAX);

    status = sysfs_write_int(PATH_SMARTDIMMER_AGGRESSIVENESS,
                             level | HWC_PRIORITY_LVL);
    if (!status) {
        didim->level.current = level;
    } else {
        ALOGE("Failed to set DIDIM aggressiveness\n");
    }
}

static void didim_window_level(struct didim_client *client, hwc_rect_t *rect)
{
    struct didim_state *didim = (struct didim_state *) client;
    enum didimLevel level = rect ? didim->level.video : didim->level.normal;

    NV_ASSERT(didim && didim->version == didimVersion1);

    if (level != didim->level.current) {
        didim_level(didim, level);
    }
}

struct didim_client *didim_open(void)
{
    struct didim_state *didim;
    int version = didim_get_version();

    if (version == didimVersionNone) {
        return NULL;
    }

    didim = (struct didim_state *) malloc(sizeof(struct didim_state));
    if (!didim) {
        return NULL;
    }

    memset(didim, 0, sizeof(struct didim_state));

    didim->version = version;

    switch (didim->version) {
    case didimVersion1:
        /* Invalidate cached state */
        didim->level.current = -1;

        /* Get aggressiveness level for normal content */
        didim->level.normal = didim_get_level_property(
            NV_PROPERTY_DIDIM_NORMAL, NV_PROPERTY_DIDIM_NORMAL_DEFAULT);

        /* Get aggressiveness level for video content */
        didim->level.video = didim_get_level_property(
            NV_PROPERTY_DIDIM_VIDEO, NV_PROPERTY_DIDIM_VIDEO_DEFAULT);

        didim->client.set_window = didim_window_level;
        break;
    default:
        NV_ASSERT(0);
        break;
    }

    /* Make sure the client is first so we can cast between the types */
    NV_ASSERT((struct didim_client *) didim == &didim->client);

    return (struct didim_client *) didim;
}

void didim_close(struct didim_client *client)
{
    if (client) {
        struct didim_state *didim = (struct didim_state *) client;

        switch (didim->version) {
        case didimVersion1:
            didim_level(didim, didimLevelOff);
            break;
        default:
            break;
        }

        /* Disable smartdimmer */
        sysfs_write_int(PATH_SMARTDIMMER_ENABLE, 0);

        free(didim);
    }
}
