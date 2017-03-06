/*
 * Copyright (c) 2005 - 2008 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include <stdio.h>
#include "nvfxmath.h"

#define ATAN_ITER(step)             \
    yold = y;                       \
    atn = *p++;                     \
    if (y >= 0) {                   \
        angle = angle + atn;        \
        y = y - (x >> step);        \
        x = x + (yold >> step);     \
    } else {                        \
        angle = angle - atn;        \
        y = y + (x >> step);        \
        x = x - (yold >> step);     \
    }

/*
 * NvSFxAtan2D computes an approximation to the principal value of arctangent 
 * of y/x using the CORDIC algorithm. It uses the signs of both arguments to 
 * determine the quadrant of the return value. The value of the arc tangent 
 * returned is in degrees (where a full circle comprises 360 degrees) and is 
 * in [-180, 180]. Zero is returned if both arguments are zero. The magnitude 
 * of the error should not exceed 2/65536 (determined by comparing a large 
 * number of results to a floating-point reference).
 */
NvSFx NvSFxAtan2D (NvSFx y, NvSFx x)
{
    int absx;
    int absy;
    int angle;
    int yold;
    int atn;
    int lz;
    const int *p;

    static unsigned char lztab[64] = 
        {32, 31, 31, 16, 31, 30,  3, 31, 15, 31, 31, 31, 29, 10,  2, 31,
         31, 31, 12, 14, 21, 31, 19, 31, 31, 28, 31, 25, 31,  9,  1, 31,
         17, 31,  4, 31, 31, 31, 11, 31, 13, 22, 20, 31, 26, 31, 31, 18,
         5, 31, 31, 23, 31, 27, 31,  6, 31, 24,  7, 31 , 8, 31,  0, 31};

    static const int atab[24] = {
        0x16800000, 0x0d485399, 0x0704a3a0, 0x03900089, 
        0x01c9c553, 0x00e51bca, 0x0072950d, 0x00394b6c,
        0x001ca5d3, 0x000e52ed, 0x00072977, 0x000394bb,
        0x0001ca5e, 0x0000e52f, 0x00007297, 0x0000394c,
        0x00001ca6, 0x00000e53, 0x00000729, 0x00000395,
        0x000001ca, 0x000000e5, 0x00000073, 0x00000039
    };
    
    p = atab;

    /* if both inputs are zero, return 0 */
    if (!(x | y)) {
        return x;
    }

    /* map quadrants to right half plane, i.e. x > 0 */
    angle = 0;
    if (x < 0) {
        x = -x;
        y = -y;
        angle= 180*128*65536;
    }
    if (y > 0) {
        angle = -angle;
    }

    /* scale up arguments as much as possible for max accuracy */
    absx = labs(x);
    absy = labs(y);

    /* use Robert Harley's trick for a portable leading zero count */
    lz = (absx > absy) ? absx : absy;
    lz |= lz >> 1;
    lz |= lz >> 2;
    lz |= lz >> 4;
    lz |= lz >> 8;
    lz |= lz >> 16;
    lz *= 7U*255U*255U*255U;
    lz = lztab[(unsigned)lz >> 26];

    if (lz > 1) {
        x <<= lz - 1;
        y <<= lz - 1;
    }

    x >>= 2;
    y >>= 2;

    ATAN_ITER (0);
    ATAN_ITER (1);
    ATAN_ITER (2);
    ATAN_ITER (3);
    ATAN_ITER (4);
    ATAN_ITER (5);
    ATAN_ITER (6);
    ATAN_ITER (7);
    ATAN_ITER (8);
    ATAN_ITER (9);
    ATAN_ITER (10);
    ATAN_ITER (11);
    ATAN_ITER (12);
    ATAN_ITER (13);
    ATAN_ITER (14);
    ATAN_ITER (15);
    ATAN_ITER (16);
    ATAN_ITER (17);
    ATAN_ITER (18);
    ATAN_ITER (19);
    ATAN_ITER (20);
    ATAN_ITER (21);
    ATAN_ITER (22);
    ATAN_ITER (23);

    /* remove scaling of angles */
    return angle >> 7;
}
