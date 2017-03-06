/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_ARM_CP15_H
#define INCLUDED_ARM_CP15_H


//==========================================================================
// ARM CP15 Register Definitions for AP20
//==========================================================================

//-----------------------------------------------------------------------------
/**  @name CP15 c1: Auxiliary Control Register - MRC/MCR p15,0,<Rd>,c1,c0,1
 */
/*@{*/

#define M_CP15_C1_C0_1_FULL_ZERO    0x00000008  /**< Full of zero mode: 0=disable, 1=enable */
#define M_CP15_C1_C0_1_EXCL         0x00000080  /**< L1 & L2 cache exclusivity: 0=disable, 1=enable */

//*@}*/

//-----------------------------------------------------------------------------
/**  @name c0: Multiprocessor Affinity Register (MPIDR) - MRC p15,0,<Rd>,c0,c0,5
 */
/*@{*/

#define M_CP15_C0_C0_5_CPU_ID       0x00000003  /**< MPIDR CPU Id (mask) */

//*@}*/

//-----------------------------------------------------------------------------
/**  @name CP15 c1: Secure Configuration Register - MRC/MCR p15,0,<Rd>,c1,c1,0
 */
/*@{*/

/** non secure shift */
#define S_CP15_C1_C1_0_NS            0
/**< World Select: 0 = secure (default), 1 = non-secure */
#define M_CP15_C1_C1_0_NS            (1 << S_CP15_C1_C1_0_NS)

/** IRQ shift */
#define S_CP15_C1_C1_0_IRQ            1
/**< IRQ behavior: 0 = IRQ mode on IRQ (default),
 * 1 = Monitor mode on IRQ
 */
#define M_CP15_C1_C1_0_IRQ            (1 << S_CP15_C1_C1_0_IRQ)

/** FIQ shift */
#define S_CP15_C1_C1_0_FIQ            2
/**< FIQ behavior: 0 = FIQ mode on FIQ (default), 1 = Monitor mode on FIQ */
#define M_CP15_C1_C1_0_FIQ            (1 << S_CP15_C1_C1_0_FIQ)

/** EA shift */
#define S_CP15_C1_C1_0_EA            3
/**< Abort behavior: 0 = Abort mode on external abort (default),
 * 1 = Monitor mode on external abort
 */
#define M_CP15_C1_C1_0_EA            (1 << S_CP15_C1_C1_0_EA)

/**< CPSR.F behavior: 0 = CPSR.F read-only in non-secure world (default),
 * 1 = CPSR.F writable in non-secure world
 */
#define M_CP15_C1_C1_0_FW           0x00000010

/**< CPSR.A behavior: 0 = CPSR.A read-only in non-secure world (default),
 * 1 = CPSR.A writable in non-secure world
 */
#define M_CP15_C1_C1_0_AW           0x00000020
//*@}*/



#endif /* #ifndef INCLUDED_ARM_CP15_H */
