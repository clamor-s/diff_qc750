/*
 * NVIDIA_COPYRIGHT_BEGIN
 *
 * Copyright 2006-2007 by NVIDIA Corporation.  All rights reserved.  All
 * information contained herein is proprietary and confidential to NVIDIA
 * Corporation.  Any use, reproduction, or disclosure without the written
 * permission of NVIDIA Corporation is prohibited.
 *
 * NVIDIA_COPYRIGHT_END
 */

#ifndef INCLUDED_CHIPLIB_INTERFACE_H
#define INCLUDED_CHIPLIB_INTERFACE_H

#include "nvcommon.h"

// IIfaceObject and bootstrapping logic
typedef enum
{
    IID_QUERY_IFACE         = 0,
    IID_CHIP_IFACE          = 1,
    IID_INTERRUPT_IFACE     = 8,
    IID_BUSMEM_IFACE        = 16,
    IID_LAST_IFACE          = 0xFFFF
} IID_TYPE;

struct IIfaceObjectRec;

typedef struct IIfaceObjectVtableRec
{
    void *Unused1;
    void *Unused2;

    // IIfaceObject interface
    void (*AddRef)(struct IIfaceObjectRec *pThis);
    void (*Release)(struct IIfaceObjectRec *pThis);
    struct IIfaceObjectRec *(*QueryIface)(struct IIfaceObjectRec *pThis,
        IID_TYPE id);
} IIfaceObjectVtable;

typedef struct IIfaceObjectRec
{
    IIfaceObjectVtable *pVtable;
} IIfaceObject;

typedef IIfaceObject *(*QueryIfaceFn)(IID_TYPE id);
#define QUERY_PROC_NAME "QueryIface"

// IChip
typedef enum
{
    ELEVEL_UNKNOWN    = 0,
    ELEVEL_HW         = 1,
    ELEVEL_RTL        = 2,
    ELEVEL_CMODEL     = 3
} ELEVEL;

struct IChipRec;

typedef struct IChipVtableRec
{
    void *Unused1;
    void *Unused2;

    // IIfaceObject interface
    void (*AddRef)(struct IChipRec *pThis);
    void (*Release)(struct IChipRec *pThis);
    IIfaceObject *(*QueryIface)(struct IChipRec *pThis, IID_TYPE id);

    void *Unused3;

    // IChip interface
    int (*Startup)(struct IChipRec *pThis, IIfaceObject* system, char** argv,
        int argc);
    void (*Shutdown)(struct IChipRec *pThis);
    int (*AllocSysMem)(struct IChipRec *pThis, int numBytes, NvU32* physAddr);
    void (*FreeSysMem)(struct IChipRec *pThis, NvU32 physAddr);
    void (*ClockSimulator)(struct IChipRec *pThis, NvS32 numClocks);
    void (*Delay)(struct IChipRec *pThis, NvU32 numMicroSeconds);
    int (*EscapeWrite)(struct IChipRec *pThis, char* path, NvU32 index,
        NvU32 size, NvU32 value);
    int (*EscapeRead)(struct IChipRec *pThis, char* path, NvU32 index,
        NvU32 size, NvU32* value);
    int (*FindPCIDevice)(struct IChipRec *pThis, NvU16 vendorId,
        NvU16 deviceId, int index, NvU32* address);
    int (*FindPCIClassCode)(struct IChipRec *pThis, NvU32 classCode, int index,
        NvU32* address);
    int (*GetSimulatorTime)(struct IChipRec *pThis, NvU64* simTime);
    double (*GetSimulatorTimeUnitsNS)(struct IChipRec *pThis);
    int (*GetPCIBaseAddress)(struct IChipRec *pThis, NvU32 cfgAddr, int index,
        NvU32* pAddress, NvU32* pSize);
    ELEVEL (*GetChipLevel)(struct IChipRec *pThis);
} IChipVtable;

typedef struct IChipRec
{
    IChipVtable *pVtable;
} IChip;

// IBusMem
typedef enum
{
    BUSMEM_HANDLED      = 0,
    BUSMEM_NOTHANDLED   = 1,
} BusMemRet;

struct IBusMemRec;

typedef struct IBusMemVtableRec
{
    void *Unused1;
    void *Unused2;

    // IIfaceObject interface
    void (*AddRef)(struct IBusMemRec *pThis);
    void (*Release)(struct IBusMemRec *pThis);
    IIfaceObject *(*QueryIface)(struct IBusMemRec *pThis, IID_TYPE id);

    void *Unused3;

    // IBusMem interface
    BusMemRet (*BusMemWrBlk)(struct IBusMemRec *pThis, NvU64 address,
        const void *appdata, NvU32 count);
    BusMemRet (*BusMemRdBlk)(struct IBusMemRec *pThis, NvU64 address,
        void *appdata, NvU32 count);
    BusMemRet (*BusMemCpBlk)(struct IBusMemRec *pThis, NvU64 dest,
        NvU64 source, NvU32 count);
    BusMemRet (*BusMemSetBlk)(struct IBusMemRec *pThis, NvU64 address,
        NvU32 size, void* data, NvU32 data_size);
} IBusMemVtable;

typedef struct IBusMemRec
{
    IBusMemVtable *pVtable;
} IBusMem;

struct IInterruptRec;

typedef struct IInterruptVtableRec
{
    void *Unused1;
    void *Unused2;

    // IIfaceObject interface
    void (*AddRef)(struct IInterruptRec *pThis);
    void (*Release)(struct IInterruptRec *pThis);
    IIfaceObject *(*QueryIface)(struct IInterruptRec *pThis, IID_TYPE id);

    void *Unused3;

    // IInterrupt interface
    void (*HandleInterrupt)( struct IInterruptRec *pThis );

} IInterruptVtable;

typedef struct IInterruptRec
{
    IInterruptVtable *pVtable;
} IInterrupt;

#endif // INCLUDED_CHIPLIB_INTERFACE_H
