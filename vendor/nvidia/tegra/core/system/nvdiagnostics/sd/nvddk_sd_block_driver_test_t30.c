/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/** @file
 * @brief <b>NVIDIA Driver Development Kit: DDK SD Block Driver Testcase API</b>
 *
 * @b Description: Contains the testcases for the nvddk SD block driver.
 */

#include "nvddk_sdio.h"
#include "nvddk_blockdevmgr.h"
#include "nvddk_blockdev.h"
#include "nvddk_sd_block_driver_test.h"

NvDdkBlockDevHandle hDdkSd = NULL;
NvDdkBlockDevInfo DeviceInfo;


NvError SdDiagTest(NvU32 Type)
{
    NvError ErrStatus;
    NvOsDebugPrintf("SdDiagTest: %d\n", Type);
    switch (Type)
    {
    case SdDiagType_WriteRead:
        ErrStatus =SdDiagVerifyWriteRead();
        break;
    case SdDiagType_WriteReadFull:
        ErrStatus =SdDiagVerifyWriteReadFull();
        break;
    case SdDiagType_WriteReadSpeed:
        ErrStatus =SdDiagVerifyWriteReadSpeed();
        break;
    case SdDiagType_WriteReadSpeed_BootArea:
        ErrStatus =SdDiagVerifyWriteReadSpeed_BootArea();
        break;
    case SdDiagType_Erase:
        ErrStatus =SdDiagVerifyErase();
        break;
    default:
        ErrStatus = NvError_NotSupported;
    }
    return ErrStatus;
}

NvError SdDiagInit(NvU32 Instance)
{
    NvError ErrStatus = NvSuccess;

    ErrStatus = NvDdkBlockDevMgrInit();
    if (ErrStatus)
    {
        NvOsDebugPrintf("NvDdkSDBlockDevinit FAILED\n");
        return ErrStatus;
    }

    //Instance = 2 for enterprise and 0 for cardhu, Minor Instance = 0
    ErrStatus = NvDdkBlockDevMgrDeviceOpen(NvRmModuleID_Sdio, Instance, 0,
                                              &hDdkSd);
    if (ErrStatus)
    {
        NvOsDebugPrintf("NvDdkSDBlockDevOpen FAILED\n");
        return ErrStatus;
    }

    if (hDdkSd->NvDdkBlockDevGetDeviceInfo)
    {
        hDdkSd->NvDdkBlockDevGetDeviceInfo(hDdkSd, &DeviceInfo);
    }
    NvOsDebugPrintf("Device Info:\n \
            Total Blocks: %d\n \
            DeviceType: %s\n \
            BytesPerSector: %d\n \
            SectorsPerBlock:%d\n", DeviceInfo.TotalBlocks,
            ((DeviceInfo.DeviceType == 1)?"Fixed":"Removable"),
            DeviceInfo.BytesPerSector,
            DeviceInfo.SectorsPerBlock);
    return ErrStatus;
}


NvError SdDiagVerifyWriteRead(void)
{
    NvError ErrStatus = NvSuccess;
    NvS32 DataSizes[] = {4, 64, 128, 512, 1024, 2048, -1};
    NvU64 DataSize = 0;
    NvU32 SizeIndex = 0;
    NvU8 *pReadBuffer = NULL;
    NvU8 *pWriteBuffer = NULL;
    NvU32 StartSector = 0, NumSectors = 0;
    NvU64 StartTime=0, EndTime=0;
    NvU32 ReadTime=0, WriteTime=0;
    NvU32 Speed;
    NvU32 i, j=0, k=0;
    NvU32 passes =0, fails =0;

    NumSectors = DeviceInfo.TotalBlocks * DeviceInfo.SectorsPerBlock;

    NvOsDebugPrintf("==================================================\n");
    NvOsDebugPrintf("          Starting Write Read VerifyTest\n");
    NvOsDebugPrintf("==================================================\n");

    for(k = 0; k < 20; k++ )
    {
        StartSector = NvOsGetTimeUS() % (NumSectors - 1000);
        NvOsDebugPrintf("Testing with start sector = %d\n", StartSector);
        SizeIndex = 0;

        while(DataSizes[SizeIndex] != -1)
        {
            DataSize = DataSizes[SizeIndex] * NV_BYTES_IN_KB;
            NvOsDebugPrintf("=============================================\n");
            NvOsDebugPrintf("Testing with %d KB data\n", DataSizes[SizeIndex]);
            NvOsDebugPrintf("=============================================\n");

           // Allocate memory for write buffer
            pWriteBuffer = NvOsAlloc(DataSize);
            if (!pWriteBuffer)
            {
                NvOsDebugPrintf("NvOsAlloc FAILED\n");
                goto SdDiagVerifyWriteRead_Exit;
            }

            // Allocate memory for read buffer
            pReadBuffer = NvOsAlloc(DataSize);
            if (!pReadBuffer)
            {
                NvOsDebugPrintf("NvOsAlloc FAILED\n");
                goto SdDiagVerifyWriteRead_Exit;
            }

            // Fill the write buffer with the timestamp
            for(i=0;i<DataSize;i++)
            {
                pWriteBuffer[i] = NvOsGetTimeUS();
                pReadBuffer[i] = 0;
            }

            // Write the buffer to SD
            NvOsDebugPrintf("Writting the data ...\n");
            StartTime = NvOsGetTimeUS();
            ErrStatus = hDdkSd->NvDdkBlockDevWriteSector(hDdkSd,
                                                         StartSector,
                                                         pWriteBuffer,
                                      (DataSize/NV_DIAG_SD_SECTOR_SIZE));
            EndTime = NvOsGetTimeUS();
            if (ErrStatus)
            {
                NvOsDebugPrintf("Write FAILED\n");
                goto SdDiagVerifyWriteRead_Exit;
            }
            WriteTime = (NvU32)(EndTime - StartTime);

            // Read into the buffer from SD
            NvOsDebugPrintf("Reading the data ...\n");
            StartTime = NvOsGetTimeUS();
            ErrStatus = hDdkSd->NvDdkBlockDevReadSector(hDdkSd,
                                                        StartSector,
                                                        pReadBuffer,
                                       (DataSize / NV_DIAG_SD_SECTOR_SIZE));
            EndTime = NvOsGetTimeUS();
            if (ErrStatus)
            {
                NvOsDebugPrintf("Read FAILED\n");
                goto SdDiagVerifyWriteRead_Exit;
            }
            ReadTime = (NvU32)(EndTime - StartTime);


            // Verify the write and read data
            NvOsDebugPrintf("Write-Read-Verify ...\n");
            j = 0;
            for (i = 0; i < DataSize; i++)
            {
                if (pWriteBuffer[i] != pReadBuffer[i])
                {
                    NvOsDebugPrintf("**ERROR: Compare Failed @ offset %d\n", i);
                    j = 1;
                    break;
                }
            }
            if (j == 0)
            {
                NvOsDebugPrintf("Write-Read-Verify PASSED\n");
                // Calulcate and print the write time and speed
                Speed = ((DataSize /NV_BYTES_IN_KB) * NV_MICROSEC_IN_SEC) /
                         WriteTime;
                NvOsDebugPrintf("Write Time = %d us\n", WriteTime);
                NvOsDebugPrintf("Write Speed = %d KB/s\n", Speed);

                // Calculate and print the write time and speed
                Speed = ((DataSize /NV_BYTES_IN_KB) * NV_MICROSEC_IN_SEC) /
                         ReadTime;
                NvOsDebugPrintf("Read Time  = %d us\n", ReadTime);
                NvOsDebugPrintf("Read Speed = %d KB/s\n", Speed);
                passes++;
            }
            else
            {
                NvOsDebugPrintf("Write-Read-Verify FAILED\n");
                fails++;
            }
            // Free the allocated buffer
            if (pWriteBuffer)
                NvOsFree(pWriteBuffer);
            if (pReadBuffer)
                NvOsFree(pReadBuffer);
            pReadBuffer = NULL;
            pWriteBuffer = NULL;

            // Change index to verify with next data size
            SizeIndex++;
        }
    }
    NvOsDebugPrintf("==================================================\n");
    NvOsDebugPrintf("Write Read VerifyTest Completed\n");
    NvOsDebugPrintf("Total Tests = %d\nTotal Passes = %d\nTotal Fails = %d\n",
                           passes+fails, passes, fails);
    NvOsDebugPrintf("==================================================\n");

SdDiagVerifyWriteRead_Exit:
    if (pWriteBuffer)
        NvOsFree(pWriteBuffer);
    if (pReadBuffer)
        NvOsFree(pReadBuffer);
    return ErrStatus;
}


NvError SdDiagVerifyWriteReadFull(void)
{
    NvError ErrStatus = NvSuccess;
    NvU64 DataSize = 0;
    NvU8 *pReadBuffer = NULL;
    NvU8 *pWriteBuffer = NULL;
    NvU32 StartSector = 0, NumSectors = 0;
    NvU32 i, j=0;

    NumSectors = DeviceInfo.TotalBlocks * DeviceInfo.SectorsPerBlock;
    StartSector = 0;
    NvOsDebugPrintf("==================================================\n");
    NvOsDebugPrintf("Starting Write Read VerifyTest for entire card\n");
    NvOsDebugPrintf("==================================================\n");
    DataSize = NV_DIAG_SD_SECTOR_SIZE;
    while(StartSector < NumSectors)
    {
        j = 0;
        // Allocate memory for write buffer
        pWriteBuffer = NvOsAlloc(DataSize);
        if (!pWriteBuffer)
        {
            NvOsDebugPrintf("NvOsAlloc FAILED\n");
            goto SdDiagVerifyWriteReadFull_Exit;
        }

        // Allocate memory for read buffer
        pReadBuffer = NvOsAlloc(DataSize);
        if (!pReadBuffer)
        {
            NvOsDebugPrintf("NvOsAlloc FAILED\n");
            goto SdDiagVerifyWriteReadFull_Exit;
        }

        // Fill the write buffer with known bytes
        for(i=0;i<DataSize;i++)
        {
            pWriteBuffer[i] = NvOsGetTimeUS();
            pReadBuffer[i] = 0;
        }

        // Write the buffer to SD
        NvOsDebugPrintf("Writting the data ...\n");
        ErrStatus = hDdkSd->NvDdkBlockDevWriteSector(hDdkSd,
                                                     StartSector,
                                                     pWriteBuffer,
                                                     1);
        if (ErrStatus)
        {
            NvOsDebugPrintf("Write FAILED\n");
            goto SdDiagVerifyWriteReadFull_Exit;
        }

        // Read into the buffer from SD
        NvOsDebugPrintf("Reading the data ...\n");
        ErrStatus = hDdkSd->NvDdkBlockDevReadSector(hDdkSd,
                                                    StartSector,
                                                    pReadBuffer,
                                                    1);
        if (ErrStatus)
        {
            NvOsDebugPrintf("Read FAILED\n");
            goto SdDiagVerifyWriteReadFull_Exit;
        }

        // Verify the write and read data
        for(i=0;i<DataSize;i++)
        {
            if(pWriteBuffer[i] != pReadBuffer[i])
            {
                NvOsDebugPrintf("**ERROR: Compare Failed @ offset %d\n", i);
                j = 1;
                break;
            }
        }
        if(j == 0)
            NvOsDebugPrintf("Write-Read-Verify for Sector %d PASSED\n", StartSector);
        else
            NvOsDebugPrintf("Write-Read-Verify for Sector %d FAILED\n", StartSector);

       // Free the allocated buffer
        if (pWriteBuffer)
            NvOsFree(pWriteBuffer);
        if (pReadBuffer)
            NvOsFree(pReadBuffer);
        pReadBuffer = NULL;
        pWriteBuffer = NULL;

        // Change index to verify with next data size
        StartSector++;
    }
    NvOsDebugPrintf("==================================================\n");
    NvOsDebugPrintf("Write Read VerifyTest for entire card Completed\n");
    NvOsDebugPrintf("==================================================\n");

SdDiagVerifyWriteReadFull_Exit:
    if (pWriteBuffer)
        NvOsFree(pWriteBuffer);
    if (pReadBuffer)
        NvOsFree(pReadBuffer);
    return ErrStatus;
}


NvError SdDiagVerifyWriteReadSpeed(void)
{
    NvError ErrStatus = NvSuccess;
    NvS32 DataSizes[] = {4, 8, 16, 32, 64, -1};
    NvU64 DataSize = 0;
    NvU32 SizeIndex = 0;
    NvU8 *pReadBuffer = NULL;
    NvU8 *pWriteBuffer = NULL;
    NvU32 StartSector = 0, NumSectors = 0;
    NvU64 StartTime=0, EndTime=0;
    NvU32 ReadTime=0, WriteTime=0;
    NvU32 Speed;
    NvU32 i, j=0;

    NumSectors = DeviceInfo.TotalBlocks * DeviceInfo.SectorsPerBlock;
    NvOsDebugPrintf("==================================================\n");
    NvOsDebugPrintf("         Starting Write Read Speed Test\n");
    NvOsDebugPrintf("==================================================\n");
    SizeIndex = 0;

    while(DataSizes[SizeIndex] != -1)
    {
        NvOsDebugPrintf("=================================================\n");
        NvOsDebugPrintf("Testing with %d MB data\n", DataSizes[SizeIndex]);
        NvOsDebugPrintf("=================================================\n");

        DataSize = DataSizes[SizeIndex] * NV_BYTES_IN_MB;
        StartSector = NvOsGetTimeUS() % (NumSectors - 1000);
        j = 0;

        // Allocate memory for write buffer
        pWriteBuffer = NvOsAlloc(DataSize);
        if (!pWriteBuffer)
        {
            NvOsDebugPrintf("NvOsAlloc FAILED\n");
            goto SdDiagVerifyWriteReadSpeed_Exit;
        }

        // Allocate memory for read buffer
        pReadBuffer = NvOsAlloc(DataSize);
        if (!pReadBuffer)
        {
            NvOsDebugPrintf("NvOsAlloc FAILED\n");
            goto SdDiagVerifyWriteReadSpeed_Exit;
        }

        // Fill the write buffer with known bytes
        for(i = 0; i < DataSize; i++)
        {
            pWriteBuffer[i] = i;
            pReadBuffer[i] = 0;
        }

        // Write the buffer to SD
        NvOsDebugPrintf("Writting the data ...\n");
        StartTime = NvOsGetTimeUS();
        ErrStatus = hDdkSd->NvDdkBlockDevWriteSector(hDdkSd,
                                                     StartSector,
                                                     pWriteBuffer,
                                    (DataSize / NV_DIAG_SD_SECTOR_SIZE));
        EndTime = NvOsGetTimeUS();
        if (ErrStatus)
        {
            NvOsDebugPrintf("Write FAILED\n");
            goto SdDiagVerifyWriteReadSpeed_Exit;
        }
        WriteTime = (NvU32)(EndTime - StartTime);

        // Read into the buffer from SD
         NvOsDebugPrintf("Reading the data ...\n");
        StartTime = NvOsGetTimeUS();
        ErrStatus = hDdkSd->NvDdkBlockDevReadSector(hDdkSd,
                                                    StartSector,
                                                    pReadBuffer,
                                    (DataSize / NV_DIAG_SD_SECTOR_SIZE));
        EndTime = NvOsGetTimeUS();

        if (ErrStatus)
        {
            NvOsDebugPrintf("Read FAILED\n");
            goto SdDiagVerifyWriteReadSpeed_Exit;
        }
        ReadTime = (NvU32)(EndTime - StartTime);

#if NV_WRITE_READ_VERIFY_NEEDED_FOR_SPEED_TEST
        // Verify the write and read data
        NvOsDebugPrintf("Write-Read-Verify ...\n");
        j = 0;
        for (i = 0; i < DataSize; i++)
        {
            if (pWriteBuffer[i] != pReadBuffer[i])
            {
                NvOsDebugPrintf("**ERROR: Compare Failed @ offset %d\n", i);
                j = 1;
                break;
            }
        }
        if (j == 0)
        {
            NvOsDebugPrintf("Write-Read-Verify PASSED\n");
#endif
            // Calulcate and print the write time and speed
            Speed = ((DataSize /NV_BYTES_IN_KB) * NV_MICROSEC_IN_SEC) /
                     WriteTime;
            NvOsDebugPrintf("Write Time = %d us\n", WriteTime);
            NvOsDebugPrintf("Write Speed = %d KB/s\n", Speed);

            // Calculate and print the write time and speed
            Speed = ((DataSize /NV_BYTES_IN_KB) * NV_MICROSEC_IN_SEC) /
                     ReadTime;
            NvOsDebugPrintf("Read Time  = %d us\n", ReadTime);
            NvOsDebugPrintf("Read Speed = %d KB/s\n", Speed);
#if NV_WRITE_READ_VERIFY_NEEDED_FOR_SPEED_TEST
        }
        else
        {
            NvOsDebugPrintf("Write-Read-Verify FAILED\n");
        }
#endif
       // Free the allocated buffer
       if (pWriteBuffer)
            NvOsFree(pWriteBuffer);
        if (pReadBuffer)
            NvOsFree(pReadBuffer);
        pReadBuffer = NULL;
        pWriteBuffer = NULL;

        // Change index to verify with next data size
        SizeIndex++;
    }
    NvOsDebugPrintf("==================================================\n");
    NvOsDebugPrintf("        Write Read Speed Test Completed\n");
    NvOsDebugPrintf("==================================================\n");

SdDiagVerifyWriteReadSpeed_Exit:
    if (pWriteBuffer)
        NvOsFree(pWriteBuffer);
    if (pReadBuffer)
        NvOsFree(pReadBuffer);
    return ErrStatus;
}

NvError SdDiagVerifyWriteReadSpeed_BootArea(void)
{
    NvError ErrStatus = NvSuccess;
    NvS32 DataSizes[] = {4, 8, 16, 64, 128, 512, -1};
    NvU64 DataSize = 0;
    NvU32 SizeIndex = 0;
    NvU8 *pReadBuffer = NULL;
    NvU8 *pWriteBuffer = NULL;
    NvU32 StartSector = 0, NumSectors = 0;
    NvU64 StartTime=0, EndTime=0;
    NvU32 ReadTime=0, WriteTime=0;
    NvU32 Speed;
    NvU32 i, j=0;
    NvU32 passes =0, fails =0;

    NumSectors = DeviceInfo.TotalBlocks * DeviceInfo.SectorsPerBlock;

    NvOsDebugPrintf("==================================================\n");
    NvOsDebugPrintf("Starting Write Read Boot Area VerifyTest\n");
    NvOsDebugPrintf("==================================================\n");

    while(DataSizes[SizeIndex] != -1)
    {
        DataSize = DataSizes[SizeIndex] * NV_BYTES_IN_KB;
        NvOsDebugPrintf("=============================================\n");
        NvOsDebugPrintf("Testing with %d KB data\n", DataSizes[SizeIndex]);
        NvOsDebugPrintf("=============================================\n");

       // Allocate memory for write buffer
        pWriteBuffer = NvOsAlloc(DataSize);
        if (!pWriteBuffer)
        {
            NvOsDebugPrintf("NvOsAlloc FAILED\n");
            goto SdDiagVerifyWriteReadSpeed_BootArea_Exit;
        }

        // Allocate memory for read buffer
        pReadBuffer = NvOsAlloc(DataSize);
        if (!pReadBuffer)
        {
            NvOsDebugPrintf("NvOsAlloc FAILED\n");
            goto SdDiagVerifyWriteReadSpeed_BootArea_Exit;
        }

        // Fill the write buffer with the timestamp
        for(i=0;i<DataSize;i++)
        {
            pWriteBuffer[i] = NvOsGetTimeUS();
            pReadBuffer[i] = 0;
        }

        // Write the buffer to SD
        StartTime = NvOsGetTimeUS();
        ErrStatus = hDdkSd->NvDdkBlockDevWriteSector(hDdkSd,
                                                     StartSector,
                                                     pWriteBuffer,
                                  (DataSize/NV_DIAG_SD_SECTOR_SIZE));
        EndTime = NvOsGetTimeUS();
        if (ErrStatus)
        {
            NvOsDebugPrintf("Write FAILED\n");
            goto SdDiagVerifyWriteReadSpeed_BootArea_Exit;
        }
        WriteTime = (NvU32)(EndTime - StartTime);

        // Read into the buffer from SD
        StartTime = NvOsGetTimeUS();
        ErrStatus = hDdkSd->NvDdkBlockDevReadSector(hDdkSd,
                                                    StartSector,
                                                    pReadBuffer,
                                   (DataSize/NV_DIAG_SD_SECTOR_SIZE));
        EndTime = NvOsGetTimeUS();
        if (ErrStatus)
        {
            NvOsDebugPrintf("Read FAILED\n");
            goto SdDiagVerifyWriteReadSpeed_BootArea_Exit;
        }
        ReadTime = (NvU32)(EndTime - StartTime);

        // Verify the write and read data
        NvOsDebugPrintf("Write-Read-Verify ...\n");
        j = 0;
        for (i = 0; i < DataSize; i++)
        {
            if (pWriteBuffer[i] != pReadBuffer[i])
            {
                NvOsDebugPrintf("**ERROR: Compare Failed @ offset %d\n", i);
                j = 1;
                break;
            }
        }
        if (j == 0)
        {
            NvOsDebugPrintf("Write-Read-Verify PASSED\n");
            // Calulcate and print the write time and speed
            Speed = ((DataSize /NV_BYTES_IN_KB) * NV_MICROSEC_IN_SEC) /
                     WriteTime;
            NvOsDebugPrintf("Write Time = %d us\n", WriteTime);
            NvOsDebugPrintf("Write Speed = %d KB/s\n", Speed);

            // Calculate and print the write time and speed
            Speed = ((DataSize /NV_BYTES_IN_KB) * NV_MICROSEC_IN_SEC) /
                     ReadTime;
            NvOsDebugPrintf("Read Time  = %d us\n", ReadTime);
            NvOsDebugPrintf("Read Speed = %d KB/s\n", Speed);
            passes++;
        }
        else
        {
            NvOsDebugPrintf("Write-Read-Verify FAILED\n");
            fails++;
        }

        // Free the allocated buffer
        if (pWriteBuffer)
            NvOsFree(pWriteBuffer);
        if (pReadBuffer)
            NvOsFree(pReadBuffer);
        pReadBuffer = NULL;
        pWriteBuffer = NULL;

        // Change index to verify with next data size
        SizeIndex++;
    }

    NvOsDebugPrintf("==================================================\n");
    NvOsDebugPrintf("Write Read Boot Area VerifyTest Completed\n");
    NvOsDebugPrintf("Total Tests = %d\nTotal Passes = %d\nTotal Fails = %d\n",
                           passes+fails, passes, fails);
    NvOsDebugPrintf("==================================================\n");

SdDiagVerifyWriteReadSpeed_BootArea_Exit:
    if (pWriteBuffer)
        NvOsFree(pWriteBuffer);
    if (pReadBuffer)
        NvOsFree(pReadBuffer);
    return ErrStatus;
}


NvError SdDiagVerifyErase(void)
{
    NvError ErrStatus = NvSuccess;
    NvS32 DataSizes[] = {16, 32, 64, 128, 192, -1};
    NvU64 DataSize = 0;
    NvU32 SizeIndex = 0;
    NvU8 *pReadBuffer = NULL;
    NvU8 *pWriteBuffer = NULL;
    NvU32 StartSector = 0, NumSectors = 0;
    NvU64 StartTime=0, EndTime=0;
    NvU32 i, j=0;
    NvDdkBlockDevIoctl_EraseLogicalSectorsInputArgs ErasePartArgs;

    NumSectors = DeviceInfo.TotalBlocks * DeviceInfo.SectorsPerBlock;
    NvOsDebugPrintf("==================================================\n");
    NvOsDebugPrintf("         Starting Erase Test\n");
    NvOsDebugPrintf("==================================================\n");
    SizeIndex = 0;
    while(DataSizes[SizeIndex] != -1)
    {
        NvOsDebugPrintf("=================================================\n");
        NvOsDebugPrintf("Testing with %d MB data\n", DataSizes[SizeIndex]);
        NvOsDebugPrintf("=================================================\n");

        DataSize = DataSizes[SizeIndex] * NV_BYTES_IN_MB;
        StartSector = NvOsGetTimeUS() % (NumSectors - 1000);
        j = 0;

        // Allocate memory for write buffer
        pWriteBuffer = NvOsAlloc(DataSize);
        if (!pWriteBuffer)
        {
            NvOsDebugPrintf("NvOsAlloc FAILED\n");
            goto SdDiagVerifyErase_Exit;
        }

        // Allocate memory for read buffer
        pReadBuffer = NvOsAlloc(DataSize);
        if (!pReadBuffer)
        {
            NvOsDebugPrintf("NvOsAlloc FAILED\n");
            goto SdDiagVerifyErase_Exit;
        }

        // Fill the write buffer with timestamps
        for(i = 0; i < DataSize; i++)
        {
            pWriteBuffer[i] = i;
            pReadBuffer[i] = 0;
        }

        // Write the buffer to SD
        NvOsDebugPrintf("Writing the data ...\n");
        ErrStatus = hDdkSd->NvDdkBlockDevWriteSector(hDdkSd,
                                                     StartSector,
                                                     pWriteBuffer,
                                    (DataSize / NV_DIAG_SD_SECTOR_SIZE));
        if (ErrStatus)
        {
            NvOsDebugPrintf("Write FAILED\n");
            goto SdDiagVerifyErase_Exit;
        }

        // Erase the data
        NvOsDebugPrintf("Erasing the data ...\n");
        ErasePartArgs.StartLogicalSector = (NvU32)StartSector;
        ErasePartArgs.NumberOfLogicalSectors =(NvU32)DataSize /
                                               NV_DIAG_SD_SECTOR_SIZE;
        ErasePartArgs.IsPTpartition = NV_FALSE;
        ErasePartArgs.IsSecureErase = NV_FALSE;
        ErasePartArgs.IsTrimErase = NV_FALSE;

        StartTime = NvOsGetTimeUS();
        ErrStatus = hDdkSd->NvDdkBlockDevIoctl(hDdkSd,
                                    NvDdkBlockDevIoctlType_EraseLogicalSectors,
                    sizeof(NvDdkBlockDevIoctl_EraseLogicalSectorsInputArgs), 0,
                                    &ErasePartArgs, NULL);
        EndTime = NvOsGetTimeUS();
        if (ErrStatus)
        {
            NvOsDebugPrintf("Erase FAILED\n");
            goto SdDiagVerifyErase_Exit;
        }

        // Read into the buffer from SD
        NvOsDebugPrintf("\nReading the data ...\n");
        ErrStatus = hDdkSd->NvDdkBlockDevReadSector(hDdkSd,
                                                    StartSector,
                                                    pReadBuffer,
                                    (DataSize / NV_DIAG_SD_SECTOR_SIZE));
        if (ErrStatus)
        {
            NvOsDebugPrintf("Read FAILED\n");
            goto SdDiagVerifyErase_Exit;
        }

        // Verify the write and read data
        for (i = 0; i < DataSize; i++)
        {
            if (( pReadBuffer[i] != 0x00) && (pReadBuffer[i] != 0xFF) )
            {
                NvOsDebugPrintf("**ERROR: Erase Failed @ offset %d\n", i);
                j = 1;
                break;
            }
        }
        if (j == 0)
        {
            NvOsDebugPrintf("Verify Erase PASSED\n");
            NvOsDebugPrintf("Erase Time = %d us\n", (EndTime - StartTime));
        }
        else
        {
            NvOsDebugPrintf("Verify Erase FAILED\n");
        }

        // Free the allocated buffer
        if (pWriteBuffer)
            NvOsFree(pWriteBuffer);
        if (pReadBuffer)
            NvOsFree(pReadBuffer);
        pReadBuffer = NULL;
        pWriteBuffer = NULL;

        // Change index to verify with next data size
        SizeIndex++;
    }
    NvOsDebugPrintf("==================================================\n");
    NvOsDebugPrintf("            Erase Test Completed\n");
    NvOsDebugPrintf("==================================================\n");

SdDiagVerifyErase_Exit:
    if (pWriteBuffer)
        NvOsFree(pWriteBuffer);
    if (pReadBuffer)
        NvOsFree(pReadBuffer);
    return ErrStatus;
}



void SdDiagClose(void)
{
    if (hDdkSd)
        hDdkSd->NvDdkBlockDevClose(hDdkSd);
    NvDdkBlockDevMgrDeinit();
}
