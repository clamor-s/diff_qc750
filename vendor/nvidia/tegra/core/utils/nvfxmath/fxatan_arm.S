/*
 * Copyright (c) 2005 - 2008 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "fxmacros.h"

  SECTION TEXT

  EXPORT  NvSFxAtan2D

//
// r0 = y, r1 = x
//
NvSFxAtan2D COLON
  orrs  r2, r1, r0          // (x | y)
  bxeq  lr                  // if (!(x | y)) return y;
  str   lr, [sp, #-4]!      // save return address on stack
  mov   r2, #0              // angle = 0
  cmp   r1, #0              // x < 0
  rsblt r1, r1, #0          // if (x < 0) x = -x
  rsblt r0, r0, #0          // if (x < 0) y = -y
  addlt r2, r2, #1509949440 // if (x < 0) angle = 180*128*65536
  cmp   r0, #0              // y > 0
  rsbgt r2, r2, #0          // if (y > 0) angle = -angle
  eor   r3, r1, r1, ASR #31 //
  sub   r3, r3, r1, ASR #31 // absx = labs(x)
  eor   ip, r0, r0, ASR #31 //
  sub   ip, ip, r0, ASR #31 // absy = labs(y)
  cmp   r3, ip              // (absx > absy)
  movlo r3, ip              // max (absx, absy)
2 COLON
  add   ip, pc, #LBLFWD(1)-LBLBCK(2)-8  // ip = &atab

  cntlz r3, r3, lr          // lz = clz (max (absx, absy))
  subs  r3, r3, #1          // lz > 1
  movgt r1, r1, LSL r3      // if (lz > 1) x <<= lz - 1
  movgt r0, r0, LSL r3      // if (lz > 1) y <<= lz - 1
  mov   r1, r1, ASR #2      // x=x/4 (avoid overflow in following)
  mov   r0, r0, ASR #2      // y=y/4 (avoid overflow in following)

  ldr   lr, [ip], #4        // atn = *p++
  subs  r3, r0, #0          // yold = y, y >= 0 ?
  subge r0, r0, r1          // if (y >= 0) y = y - x
  addlt r0, r0, r1          // if (y < 0) y = y + x
  addge r1, r1, r3          // if (y >= 0) x = x + yold
  sublt r1, r1, r3          // if (y < 0) x = x - yold
  addge r2, r2, lr          // if (y >= 0) angle = angle + atn
  sublt r2, r2, lr          // if (y < 0) angle = angle + atn

#if defined(__GNUC__)
  .irp tag,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
  ldr   lr, [ip], #4        // atn = *p++
  subs  r3, r0, #0          // yold = y, y >= 0 ?
  subge r0, r0,r1, ASR #\tag// if (y >= 0) y = y - (x >> step)
  addlt r0, r0,r1, ASR #\tag// if (y < 0) y = y + (x >> step)
  addge r1, r1,r3, ASR #\tag// if (y >= 0) x = x + (yold >> step)
  sublt r1, r1,r3, ASR #\tag// if (y < 0) x = x - (yold >> step)
  addge r2, r2, lr          // if (y >= 0) angle = angle + atn
  sublt r2, r2, lr          // if (y < 0) angle = angle - atn
  .endr  
#else
  GBLA tag
tag SETA 1
  WHILE tag <= 23
    ldr   lr, [ip], #4        // atn = *p++
    subs  r3, r0, #0          // yold = y, y >= 0 ?
    subge r0, r0,r1, ASR #tag // if (y >= 0) y = y - (x >> step)
    addlt r0, r0,r1, ASR #tag // if (y < 0) y = y + (x >> step)
    addge r1, r1,r3, ASR #tag // if (y >= 0) x = x + (yold >> step)
    sublt r1, r1,r3, ASR #tag // if (y < 0) x = x - (yold >> step)
    addge r2, r2, lr          // if (y >= 0) angle = angle + atn
    sublt r2, r2, lr          // if (y < 0) angle = angle - atn
tag SETA tag+1
  WEND
#endif

  ldr   lr, [sp], #4        // return address stored in top of stack
  mov   r0, r2, ASR #7      // angle >> 7
  bx    lr                  // return

1 COLON
  DW 377487360
  DW 222843801
  DW 117744544
  DW 59768969
  DW 30000467
  DW 15014858
  DW 7509261
  DW 3754860
  DW 1877459
  DW 938733
  DW 469367
  DW 234683
  DW 117342
  DW 58671
  DW 29335
  DW 14668
  DW 7334
  DW 3667
  DW 1833
  DW 917
  DW 458
  DW 229
  DW 115
  DW 57
        
  END
