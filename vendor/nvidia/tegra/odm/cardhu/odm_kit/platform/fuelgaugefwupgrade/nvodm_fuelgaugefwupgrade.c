/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_services.h"
#include "nvodm_fuelgaugefwupgrade.h"

/**
 * Finds the mode of the TI fuel gauge bq27510
 *
 * @param hFGI2c [in] A handle to the NvOdm I2C module.
 * @param pI2CAddr [out] Pointer to the I2C device addr.
 * @retval NvSuccess Indicates success else error
 */
static NvError
NvOdmFFUDeviceMode(NvOdmServicesI2cHandle hFGI2c,
    NvU32 *pI2CAddr);

/**
 * Extracts the Line of I2C command from the Linked list of buffer.
 *
 * @param pCurrBuff [in] A handle to the buffer node.
 * @param pFGLine [out] A handle to Line of data.
 * @retval NvSuccess Indicates success else error
 */
static void
NvOdmFFUReadLine(struct NvOdmFFUBuff **pCurrBuff,
    struct NvOdmFFULine *pFGLine);

/**
 * Extracts the decimal value from a line of ASCII codes
 *
 * @param pLine [In] A handle to the object of struct NvOdmFFULine.
 * @param dataIndex [In] pointer to the starting point of the line.
 * @param pCharCnt [Out] pointer to the no of ASCIIs used
 * @retval NvSuccess Indicates success else error
 */
static NvError
NvOdmFFUAsciToDec(struct NvOdmFFULine *pLine,
    NvU32 dataIndex,
    NvU32 *pCharCnt,
    NvU32 *val);

/**
 * Extracts the hexadecimal value from a line of ASCII codes
 *
 * @param pLine [In] A handle to the object of struct NvOdmFFULine.
 * @param dataIndex [In] pointer to the starting point of the line.
 * @param pCharCnt [Out] pointer to the no of ASCIIs used
 * @retval NvSuccess Indicates success else error
 */
static NvError
NvOdmFFUAsciToHex(struct NvOdmFFULine *pLine,
    NvU32 dataIndex,
    NvU32 *pCharCnt,
    NvU32 *val);

/**
 * Frees the memory allocated for the Linked List of Buffer
 *
 * @param ppHead [In] A handle to the Head node of the Linked List.
 */
static void NvOdmFFURemoveBuffNode(struct NvOdmFFUBuff **ppHead);

/**
 * Frees the memory allocated for the Linked List of I2C Commands
 *
 * @param ppHead [In] A handle to the Head node of the Linked List.
 */
static void NvOdmFFURemoveCmdNode(struct NvOdmFFUCommand **ppHead);

/**
 * Validates each I2C commands
 *
 * @param pCurrCmd [In] A handle to the Head node of the Linked List.
 * @retval NvSuccess Indicates success else error
 */
static NvError NvOdmFFUVerifyCmd(struct NvOdmFFUCommand *pCurrCmd);

/**
 * Parses the received buffer
 *
 * @param pHeadBuff [In] A handle to the Head node of the Buffer Linked List.
 * @param pHeadCmd [In] A handle to the Head node of the Command Linked List.
 * @retval NvSuccess Indicates success else error
 */
static NvError
NvOdmFFUParse(struct NvOdmFFUBuff *pHeadBuff,
    struct NvOdmFFUCommand **pHeadCmd);

/**
 * Performs an I2c Write Transaction.
 *
 * @param hFGI2c [In] A handle to the NvOdm I2c Module.
 * @param buf [In] A handle to data buffer.
 * @param bufLen [In] The buffer Length.
 * @param devAddr [In] It specifies the I2c device address.
 * @retval NvSuccess Indicates success else error
 */
static NvError
NvOdmFFUI2cWrite(NvOdmServicesI2cHandle hFGI2c,
    NvU8 *buf,
    NvU32 bufLen,
    NvU32 devAddr);

/**
 * Performs an I2c Read Transaction.
 *
 * @param hFGI2c [In] A handle to the NvOdm I2c Module.
 * @param buf [In] A handle to data buffer.
 * @param bufLen [In] The buffer Length.
 * @param devAddr [In] It specifies the I2c device address.
 * @retval NvSuccess Indicates success else error
 */
static NvError
NvOdmFFUI2cRead(NvOdmServicesI2cHandle hFGI2c,
    NvU8 *buf,
    NvU32 bufLen,
    NvU32 devAddr);

/**
 * Converts NvOdmStatus value to NvError.
 *
 * @param status [In] NvOdmI2cStatus value.
 */
static NvError NvOdmFFUOdmStatusToNvErr(NvOdmI2cStatus status);

/**
 * Checks whether the TI's BQ27510 FuelGuage is sealed or not.
 *
 * @param hFGI2c [In] A handle to the NvOdm I2c Module.
 * @param sealed [Out] If *sealed = 0 then it indicates
 * that the device is not sealed otherwiese sealed.
 * @retval NvSuccess Indicates success else error
 */
static NvError NvOdmFFUCheckSealed(NvOdmServicesI2cHandle hFGI2c, NvU8 *sealed);

/**
 * Changes the mode of the FuelGuage to Rom Mode.
 *
 * @param hFGI2c [In] A handle to the NvOdm I2c Module.
 * @retval NvSuccess Indicates success else error
 */
static NvError NvOdmFFUEnterRomMode(NvOdmServicesI2cHandle hFGI2c);

/**
 * Changes the mode of the FuelGuage to Normal Mode.
 *
 * @param hFGI2c [In] A handle to the NvOdm I2c Module.
 * @retval NvSuccess Indicates success else error
 */
static NvError NvOdmFFUExitRomMode(NvOdmServicesI2cHandle hFGI2c);

/**
 * Starts the firmware upgradation.
 *
 * @param pHeadCmd [In] A handle to the Head node of the Command Linked List.
 * @retval NvSuccess Indicates success else error
 */
static NvError NvOdmFFUFlashing(struct NvOdmFFUCommand *pHeadCmd);

/**
 * Executes the I2C command from the Linked List
 *
 * @param pHeadCmd [In] A handle to the Head node of the Command Linked List.
 * @param hFGI2c [In] A handle to the NvOdm I2C module.
 * @retval NvSuccess Indicates success else error
 */
static NvError
NvOdmFFUExecuteCmd(struct NvOdmFFUCommand *pHeadCmd,
    NvOdmServicesI2cHandle hFGI2c);

void
NvOdmFFUReadLine(struct NvOdmFFUBuff **pCurrBuff,
    struct NvOdmFFULine *pFGLine)
{
    static NvU64 s_fp = 0;
    NvU32 len = 0;

    while (*pCurrBuff && (s_fp < (*pCurrBuff)->data_len) &&
        ((*pCurrBuff)->data[s_fp] != '\r') &&
        ((*pCurrBuff)->data[s_fp] != '\n'))
    {
        pFGLine->data[len] = (*pCurrBuff)->data[s_fp];
        len++;
        s_fp++;
        if (s_fp >= (*pCurrBuff)->data_len && (*pCurrBuff)->pNext != NULL)
        {
            s_fp = 0;
            *pCurrBuff = (*pCurrBuff)->pNext;
        }

    }

    if (s_fp >= (*pCurrBuff)->data_len)
    {
        s_fp = 0;
        *pCurrBuff = (*pCurrBuff)->pNext;
    }

    while (*pCurrBuff && (s_fp < (*pCurrBuff)->data_len) &&
        (((*pCurrBuff)->data[s_fp] == '\r') ||
        ((*pCurrBuff)->data[s_fp] == '\n')))
    {
        s_fp++;
        if (s_fp >= (*pCurrBuff)->data_len)
        {
            s_fp = 0;
            *pCurrBuff = (*pCurrBuff)->pNext;
        }
    }

    pFGLine->len = len;
    pFGLine->data[len] = '\n';
}

NvError
NvOdmFFUAsciToHex(struct NvOdmFFULine *pLine,
    NvU32 dataIndex,
    NvU32 *pCharCnt,
    NvU32 *val)
{
    NvU32 value = 0;
    NvU32 i;
    NvError e = NvSuccess;

    for (i = 0; (i < (sizeof(value) * 2)) &&
        (i < (pLine->len - dataIndex)); ++i)
    {
        if (pLine->data[dataIndex + i] == '\t' ||
            pLine->data[dataIndex + i] == ' ')
            break;

        if (pLine->data[dataIndex + i] >= '0' &&
            pLine->data[dataIndex + i] <= '9')
        {
            value = (value << 4) |  (pLine->data[dataIndex + i] - '0');
        }
        else if (pLine->data[dataIndex + i] >= 'A' &&
                 pLine->data[dataIndex + i] <= 'F')
        {
            value = (value << 4) | ((pLine->data[dataIndex + i] - 'A') + 10);
        }
        else if (pLine->data[dataIndex + i] >= 'a' &&
                 pLine->data[dataIndex + i] <= 'f')
        {
            value = (value << 4) | ((pLine->data[dataIndex + i] - 'a') + 10);
        }
        else
        {
            NvOdmOsDebugPrintf("NvOdmFFUAsciToHex Unknown asci value\n");
            e = NvError_BadValue;
            goto fail;
        }

    }
    *pCharCnt = i;
    *val = value;
    return NvSuccess;
fail:
    return e;
}

NvError
NvOdmFFUAsciToDec(struct NvOdmFFULine *pLine,
    NvU32 dataIndex,
    NvU32 *pCharCnt,
    NvU32 *val)
{
    NvU32 value = 0;
    NvU32 i;
    NvError e = NvSuccess;

    for (i = 0; i < (pLine->len - dataIndex); ++i)
    {
        if (pLine->data[dataIndex + i] == '\t' ||
            pLine->data[dataIndex + i] == ' ')
            break;

        if (pLine->data[dataIndex + i] >= '0' &&
            pLine->data[dataIndex + i] <= '9')
        {
            value = ((value * 10) + (pLine->data[dataIndex + i] - '0'));
        }
        else
        {
            NvOdmOsDebugPrintf("NvOdmFFUAsciToDec Unknown asci value\n");
            e = NvError_BadValue;
            goto fail;
        }

    }
    *pCharCnt = i;
    *val = value;
    return NvSuccess;
fail:
    return e;
}

void NvOdmFFURemoveBuffNode(struct NvOdmFFUBuff **ppHead)
{
    struct NvOdmFFUBuff *pTemp = *ppHead;

    if (*ppHead == NULL)
        return;

    while (*ppHead)
    {
        pTemp = (*ppHead)->pNext;
        NvOdmOsFree(*ppHead);
        *ppHead = pTemp;
    }
    *ppHead = NULL;
}

void NvOdmFFURemoveCmdNode(struct NvOdmFFUCommand **ppHead)
{
    struct NvOdmFFUCommand *pTemp = *ppHead;

    if (*ppHead == NULL)
        return;

    while (*ppHead)
    {
        pTemp = (*ppHead)->pNext;
        NvOdmOsFree(*ppHead);
        *ppHead = pTemp;
    }
    *ppHead = NULL;
}

NvError NvOdmFFUVerifyCmd(struct NvOdmFFUCommand *pCurrCmd)
{
    NvError e = NvSuccess;

    while (pCurrCmd != NULL)
    {
        switch (pCurrCmd->cmd)
        {
            case NvOdmFFU_WRITE:
            case NvOdmFFU_READ:
            case NvOdmFFU_COMPARE:
                if (!pCurrCmd->data_len &&
                    (pCurrCmd->data_len > NvOdmFFU_MAX_DATA__LEN))
                {
                    NvOdmOsDebugPrintf("NvOdmFFUVerifyCmd : invalid data length\n");
                    e = NvError_BadValue;
                    goto fail;
                }
                break;
            case NvOdmFFU_DELAY:
                if (!pCurrCmd->i2c_delay)
                {
                    NvOdmOsDebugPrintf("NvOdmFFUVerifyCmd : invalid i2c delay\n");
                    e = NvError_BadValue;
                    goto fail;
                }
                break;
            default:
                NvOdmOsDebugPrintf("Unknown command\n");
                e = NvError_BadValue;
                goto fail;
        }
        pCurrCmd = pCurrCmd->pNext;
    }
    return NvSuccess;
fail:
    return e;
}

NvError
NvOdmFFUParse(struct NvOdmFFUBuff *pHeadBuff,
    struct NvOdmFFUCommand **pHeadCmd)
{
    struct NvOdmFFUBuff *pCurrBuff = pHeadBuff;
    struct NvOdmFFULine line;
    struct NvOdmFFUCommand *pTailCmd = NULL;
    struct NvOdmFFUCommand *pPrevCmd = NULL;
    NvU32 i;
    NvU32 j;
    NvU32 I2CAddr = 0;
    NvU32 reg_addr = 0;
    NvU32 cnt = 0;
    NvU32 value = 0x00;
    NvU32 debugCnt = 0;
    NvError e = NvSuccess;

    NvOdmOsMemset((void *)&line, 0, sizeof(struct NvOdmFFULine));

    // Creating the Head node for the command list
    *pHeadCmd = (struct NvOdmFFUCommand *)
                    NvOdmOsAlloc(sizeof(struct NvOdmFFUCommand));
    if (pHeadCmd == NULL)
    {
        NvOdmOsDebugPrintf("NvOdmFFUParse : Error in memorry allocation\n");
        e = NvError_InsufficientMemory;
        goto cleanCmd;
    }
    (*pHeadCmd)->pNext = NULL;
    (*pHeadCmd)->cmd = NvOdmFFU_UNUSED;
    pTailCmd = *pHeadCmd;

    do
    {
        debugCnt++;
        // Separating the line of I2C instruction from the current buffer
        NvOdmFFUReadLine(&pCurrBuff, &line);
        if (line.len == 0 || line.data[0] == ';')
        {
            if (pCurrBuff != NULL)
                continue;
            else
                break;
        }
        if (line.data[1] != ':' || line.data[2] != ' ')
        {
            NvOdmOsDebugPrintf("NvOdmFFUParse : syntax error %x %x"
                "debugCnt=%d\n", line.data[1], line.data[2], debugCnt);
            e = NvError_BadValue;
            goto cleanCmd;
        }
        I2CAddr = 0;
        reg_addr = 0;

        switch (line.data[0])
        {
            // Identifying the command type
            case 'W':
            case 'w':
                pTailCmd->cmd = NvOdmFFU_WRITE;
                break;

            case 'C':
            case 'c':
                pTailCmd->cmd = NvOdmFFU_COMPARE;
                break;

            case 'X':
            case 'x':
                pTailCmd->cmd = NvOdmFFU_DELAY;
                break;

            case 'R':
            case 'r':
                pTailCmd->cmd = NvOdmFFU_READ;
                break;

            default:
                NvOdmOsDebugPrintf("NvOdmFFU_Parse : unknown command\n");
                e = NvError_BadValue;
                goto cleanCmd;
        }

        for (i = 2, j = 0; j < NvOdmFFU_MAX_DATA__LEN &&
                line.data[i] != '\n'; i += cnt)
        {
            // Parsing the Dev addr, Reg_addr and data
            cnt = 0;
            if (line.data[i] == ' ') // skip white spaces
            {
                cnt = 1;
                continue;
            }

            if (pTailCmd->cmd == NvOdmFFU_DELAY)
            {
                e = NvOdmFFUAsciToDec(&line, i, &cnt, &pTailCmd->i2c_delay);
                if (e)
                {
                    // Error in NvOdmFFUAsciToDec
                    goto cleanCmd;
                }
                if (line.data[i + cnt] != '\n')
                {
                    NvOdmOsDebugPrintf("NvOdmFFU_Parse :"
                        "Extra parameter for delay\n");
                    e = NvError_BadValue;
                    goto cleanCmd;
                }
                break;
            }

            else if (!I2CAddr)
            {
                e = NvOdmFFUAsciToHex(&line, i, &cnt, &value);
                if (e)
                {
                    goto cleanCmd;
                }
                pTailCmd->device_address = value;
                I2CAddr = 1;
            }
            else if (!reg_addr)
            {
                e = NvOdmFFUAsciToHex(&line, i, &cnt, &value);
                if (e)
                {
                    goto cleanCmd;
                }
                pTailCmd->register_address = value;
                reg_addr = 1;
            }
            else
            {
                e = NvOdmFFUAsciToHex(&line, i, &cnt, &value);
                if (e)
                {
                    goto cleanCmd;
                }
                pTailCmd->data[j++] = value;
            }
        }

        pTailCmd->data_len = j;

        if (pCurrBuff != NULL)
        {
            // We need another command node
            pTailCmd->pNext = (struct NvOdmFFUCommand *)
                               NvOdmOsAlloc(sizeof(struct NvOdmFFUCommand));
            if (pTailCmd->pNext == NULL)
            {
                NvOdmOsDebugPrintf("NvOdmFFUParse :"
                   " Error in memorry allocation\n");
                e = NvError_InsufficientMemory;
                goto cleanCmd;
            }
            pPrevCmd = pTailCmd;
            pTailCmd = pTailCmd->pNext;
            pTailCmd->pNext = NULL;
            pTailCmd->cmd = NvOdmFFU_UNUSED;
        }

    } while (pCurrBuff != NULL);

    if ((pTailCmd->cmd == NvOdmFFU_UNUSED) && (pPrevCmd != NULL))
    {
        pPrevCmd->pNext = NULL;
        NvOdmOsFree(pTailCmd);
        pTailCmd = pPrevCmd;
    }
    if (NvOdmFFUVerifyCmd(*pHeadCmd))
    {
        NvOdmOsDebugPrintf("NvOdmFFU_Parse : error in File format \n");
        e = NvError_BadValue;
        goto cleanCmd;
    }
    return NvSuccess;

cleanCmd:
    // Freeing the command nodes, incase of errors
    NvOdmFFURemoveCmdNode(pHeadCmd);
    return e;
}

NvError
NvOdmFFUI2cWrite(NvOdmServicesI2cHandle hFGI2c,
     NvU8 *buf,
     NvU32 bufLen,
     NvU32 devAddr)
{
    NvOdmI2cTransactionInfo trans;
    NvError e = NvSuccess;
    NvOdmI2cStatus status = NvSuccess;

    trans.Flags = NVODM_I2C_IS_WRITE;
    trans.NumBytes = bufLen;
    trans.Address = devAddr;
    trans.Buf = buf;

    status =  NvOdmI2cTransaction(hFGI2c, &trans, 1,
    NvOdmFFU_I2C_CLOCK_FREQ_KHZ, NvOdmFFU_I2C_TRANSACTION_TIMEOUT);

    if(status)
        e = NvOdmFFUOdmStatusToNvErr(status);
    else
        e = NvSuccess;

    return e;
}

NvError
NvOdmFFUI2cRead(NvOdmServicesI2cHandle hFGI2c,
     NvU8 *buf,
     NvU32 bufLen,
     NvU32 devAddr)
{
    NvOdmI2cTransactionInfo trans[2];
    NvError e = NvSuccess;
    NvOdmI2cStatus status = NvSuccess;

    trans[0].Flags = NVODM_I2C_IS_WRITE | NVODM_I2C_USE_REPEATED_START;
    trans[0].NumBytes = 1;
    trans[0].Address = devAddr;
    trans[0].Buf = buf;
    trans[1].Flags = 0;
    trans[1].NumBytes = bufLen - 1;
    trans[1].Address = devAddr;
    trans[1].Buf = (buf+1);

    status = NvOdmI2cTransaction(hFGI2c, trans, 2,
    NvOdmFFU_I2C_CLOCK_FREQ_KHZ, NvOdmFFU_I2C_TRANSACTION_TIMEOUT);
    if(status)
        e = NvOdmFFUOdmStatusToNvErr(status);
    else
        e = NvSuccess;

    return e;
}

static NvError NvOdmFFUOdmStatusToNvErr(NvOdmI2cStatus status)
{
    NvError e;

    switch (status)
    {
        case NvOdmI2cStatus_SlaveNotFound:
            e = NvError_I2cDeviceNotFound;
            break;
        case NvOdmI2cStatus_ReadFailed:
            e = NvError_I2cReadFailed;
            break;
        case NvOdmI2cStatus_WriteFailed:
            e = NvError_I2cWriteFailed;
            break;
        case NvOdmI2cStatus_ArbitrationFailed:
            e = NvError_I2cArbitrationFailed;
            break;
        case NvOdmI2cStatus_InternalError:
            e = NvError_I2cInternalError;
            break;
        case NvOdmI2cStatus_Timeout:
        default:
            e = NvError_Timeout;
            break;
    }
    return e;
}

NvError NvOdmFFUCheckSealed(NvOdmServicesI2cHandle hFGI2c, NvU8 *sealed)
{

    NvU8 buffer[3];
    NvError e = NvSuccess;

    *sealed = 0;

    buffer[0] = 0x00;  // Register (0x00)
    buffer[1] = 0x00;  // Data
    buffer[2] = 0x00;  // Data

    e = NvOdmFFUI2cWrite(hFGI2c, buffer, 3, NvOdmFFU_NORMAL_I2C_ADDR);
    if (e)
    {
        NvOdmOsDebugPrintf("NvOdmFFUCheckSealed : NvOdmFFUI2cWrite Failed\n");
        goto fail;
    }

    buffer[0] = 0x01;  // Register (0x01)
    buffer[1] = 0x00;  // Data
    buffer[2] = 0x00;  // Data

    e = NvOdmFFUI2cRead(hFGI2c, buffer, 2, NvOdmFFU_NORMAL_I2C_ADDR);

    if (e != NvSuccess)
    {
        NvOdmOsDebugPrintf("NvOdmFFUCheckSealed: NvOdmFFUI2cRead Failed %x", e);
        NvOdmOsDebugPrintf("Data:  %x  %x: ", buffer[0], buffer[1]);
        goto fail;
    }

    if (buffer[1] & 0x20)
    {
        *sealed = 1;
        NvOdmOsDebugPrintf("NvOdmFFUCheckSealed: Unsealed Key is Required\n");
    }
    if (buffer[1] & 0x40)
    {
        *sealed = 1;
        NvOdmOsDebugPrintf("NvOdmFFUCheckSealed: Full Access Unsealed Key"
                        " is Required\n");
    }

    return NvSuccess;
fail:
    return e;
}

NvError NvOdmFFUEnterRomMode(NvOdmServicesI2cHandle hFGI2c)
{

    NvU8 sealed = 0;
    NvError e = NvSuccess;
    NvU8 buffer[3];
    NvU32 mode = 0;

    e = NvOdmFFUCheckSealed(hFGI2c, &sealed);

    if (e)
    {
        NvOdmOsDebugPrintf("NvOdmFFUEnterRomMode : NvOdmFFUCheckSealed "
                "function returned with an eror %d\n", e);
        goto fail;
    }


    if (sealed)
    {
        NvOdmOsDebugPrintf("NvOdmFFUEnterRomMode : Device is Sealed\n");
        e = NvError_ResourceError;
        goto fail;
    }

    buffer[0] = 0x00;  // Register (0x00)
    buffer[1] = 0x00;  // Data
    buffer[2] = 0x0F;  // Data

    e = NvOdmFFUI2cWrite(hFGI2c, buffer, 3, NvOdmFFU_NORMAL_I2C_ADDR);
    if (e)
    {
        NvOdmOsDebugPrintf("NvOdmFFUEnterRomMode : NvOdmFFUI2cWrite Failed\n");
        goto fail;
    }

    NvOdmOsDebugPrintf("NvOdmFFUEnterRomMode : Sequence of Entering"
                   " Rom Mode is done\n");

    e = NvOdmFFUDeviceMode(hFGI2c, &mode);
    if (e)
    {
        NvOdmOsDebugPrintf("NvOdmFFUEnterRomMode :NvOdmFFUDeviceMode Failed\n");
        goto fail;
    }

    return NvSuccess;
fail:
    return e;
}

NvError NvOdmFFUDeviceMode(NvOdmServicesI2cHandle hFGI2c, NvU32 *pI2CAddr)
{
    NvError e = NvSuccess;
    NvU8 buffer[3];

    *pI2CAddr = NvOdmFFU_NORMAL_I2C_ADDR;

    buffer[0] = 0x00;  // Register (0x00)
    buffer[1] = 0x01;  // Data
    buffer[2] = 0x00;  // Data

    e = NvOdmFFUI2cWrite(hFGI2c, buffer, 3, NvOdmFFU_NORMAL_I2C_ADDR);
    if (e)
    {
        e = NvOdmFFUI2cWrite(hFGI2c, buffer, 3, NvOdmFFU_ROM_I2C_ADDR);
        if (e)
        {
            NvOdmOsDebugPrintf("NvOdmFFUDeviceMode : Device bq27510 is not"
                            " giving ACK\n");
            goto fail;
        }
        *pI2CAddr = NvOdmFFU_ROM_I2C_ADDR;
    }

    if (*pI2CAddr == NvOdmFFU_ROM_I2C_ADDR)
    {
        NvOdmOsDebugPrintf("NvOdmFFUDeviceMode : Device is in ROM mode\n");
    }
    else
    {
        NvOdmOsDebugPrintf("NvOdmFFUDeviceMode : Device is in Normal mode\n");

        buffer[0] = 0x00;  // Register (0x00)
        buffer[1] = 0x00;  // Data
        buffer[2] = 0x00;  // Data

        e = NvOdmFFUI2cRead(hFGI2c, buffer, 3, *pI2CAddr);
        if (e)
        {
            e = NvOdmFFUI2cRead(hFGI2c, buffer, 3, NvOdmFFU_ROM_I2C_ADDR);
            if (e)
            {
                NvOdmOsDebugPrintf("bq27510 is not responding "
                        "for both addr 0x16 and 0xAA\n");
                goto fail;
            }
            else
            {
                *pI2CAddr = NvOdmFFU_ROM_I2C_ADDR;
            }
        }
    }

    return NvSuccess;

fail:
    return e;
}

NvError NvOdmFFUExitRomMode(NvOdmServicesI2cHandle hFGI2c)
{
    NvError e = NvSuccess;
    NvU8 buffer[3];

    buffer[0] = 0x00;  // Register (0x00)
    buffer[1] = 0x0F;  // Data
    buffer[2] = 0x00;  // Data

    e = NvOdmFFUI2cWrite(hFGI2c, buffer, 2, NvOdmFFU_ROM_I2C_ADDR);
    if (e)
    {
        NvOdmOsDebugPrintf("NvOdmFFUExitRomMode : NvOdmFFUI2cWrite Failed "
                        "with error %d\n", e);
        goto fail;
    }

    buffer[0] = 0x64;  // Register (0x00)
    buffer[1] = 0x0F;  // Data
    buffer[2] = 0x00;  // Data

    e = NvOdmFFUI2cWrite(hFGI2c, buffer, 2, NvOdmFFU_ROM_I2C_ADDR);
    if (e)
    {
        NvOdmOsDebugPrintf("NvOdmFFUExitRomMode : NvOdmFFUI2cWrite Failed\n");
        goto fail;
    }

    buffer[0] = 0x65;  // Register (0x00)
    buffer[1] = 0x00;  // Data
    buffer[2] = 0x00;  // Data

    e = NvOdmFFUI2cWrite(hFGI2c, buffer, 2, NvOdmFFU_ROM_I2C_ADDR);
    if (e)
    {
        NvOdmOsDebugPrintf("NvOdmFFUExitRomMode : NvOdmFFUI2cWrite Failed\n");
        goto fail;
    }

    NvOdmOsDebugPrintf("NvOdmFFUExitRomMode : Exit sequence is completed\n");

    NvOdmOsSleepMS(NvOdmFFU_DELAY_4000MSEC);

    return NvSuccess;
fail:
    return e;
}

NvError
NvOdmFFUExecuteCmd(struct NvOdmFFUCommand *pHeadCmd,
    NvOdmServicesI2cHandle hFGI2c)
{
    struct NvOdmFFUCommand *pCurCmd = pHeadCmd;
    NvU8 *buff = NULL;
    NvU32 flashCnt = 0;
    NvError e = NvSuccess;
    NvU32 data_len = 0;

    NvOdmOsDebugPrintf("NvOdmFFUExecuteCmd : Start\n");

    buff = (NvU8 *)NvOdmOsAlloc(NvOdmFFU_LINE_DATALEN);
    if (buff == NULL)
    {
        NvOdmOsDebugPrintf("NvOdmFFUExecuteCmd: Memory Allcation Failed\n");
        e = NvError_InsufficientMemory;
        goto fail;
    }

    while (pCurCmd)
    {
        data_len = 0;

        switch (pCurCmd->cmd)
        {
            case NvOdmFFU_WRITE:
                buff[0] =  pCurCmd->register_address;
                NvOdmOsMemcpy(&buff[1], pCurCmd->data, pCurCmd->data_len);
                e = NvOdmFFUI2cWrite(hFGI2c, buff, (pCurCmd->data_len + 1),
                        NvOdmFFU_ROM_I2C_ADDR);
                if (e)
                {
                    NvOdmOsDebugPrintf("\nNvOdmFFUExecuteCmd :"
                        "NvOdmFFUI2cWrite Failed with error status =%d \n", e);
                    if (e == NvError_I2cDeviceNotFound)
                    {
                        NvOdmOsDebugPrintf("\nNvOdmFFUExecuteCmd : "
                        "NvError_I2cDeviceNotFound\n");
                    }
                    goto fail;
                }
                break;
            case NvOdmFFU_COMPARE:
                data_len = pCurCmd->data_len;
                buff[0] = pCurCmd->register_address;
                NvOdmOsMemset(&buff[1], 0, data_len);
                e = NvOdmFFUI2cRead(hFGI2c, buff, data_len + 1,
                                    NvOdmFFU_ROM_I2C_ADDR);
                if (e)
                {
                    NvOdmOsDebugPrintf("\nNvOdmFFUExecuteCmd :"
                                        "NvOdmFFUI2cRead Failed\n");
                    goto fail;
                }
                if (NvOdmOsMemcmp(pCurCmd->data, buff+1, data_len))
                {
                    // compare failed
                    if (flashCnt >= NvOdmFFU_FLASH_RETRY_CNT)
                    {
                        NvOdmOsDebugPrintf("NvOdmFFUExecuteCmd : "
                        "Device Flashing failed. Retry : %d", flashCnt);
                        e = NvError_ResourceError;
                        goto fail;
                    }
                    pCurCmd = pHeadCmd;
                    flashCnt++;
                    NvOdmOsDebugPrintf("NvOdmFFUExecuteCmd : Device "
                    "Flashing failed. Retrying... : %d", flashCnt);
                    continue;
                }
                break;

            case NvOdmFFU_DELAY:
                NvOdmOsSleepMS(pCurCmd->i2c_delay);
                break;

            default:
                NvOdmOsDebugPrintf("NvOdmFFUExecuteCmd : invalid command\n");
                e = NvError_BadValue;
                goto fail;
        }
        pCurCmd = pCurCmd->pNext;
    }    // While
    return NvSuccess;
fail:
    return e;
}

NvError NvOdmFFUFlashing(struct NvOdmFFUCommand *pHeadCmd)
{
    NvOdmServicesI2cHandle hFGI2c;
    NvError e = NvSuccess;
    NvU32 mode = 1;

    hFGI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, 4);
    if (!hFGI2c)
    {
        NvOdmOsDebugPrintf("\nNvOdmI2cOpen fail\n");
        e = NvError_ResourceError;
        goto err_nvodm;
    }

    e = NvOdmFFUDeviceMode(hFGI2c, &mode);
    if (e)
    {
        NvOdmOsDebugPrintf("\nNvOdmFFUDeviceMode fail\n");
        goto err;
    }

    if (mode == NvOdmFFU_NORMAL_I2C_ADDR)
    {
        // Device is in Normal mode
        e = NvOdmFFUEnterRomMode(hFGI2c);
        if (e)
        {
            NvOdmOsDebugPrintf("NvOdmFFUFlashing :"
                       " NvOdmFFUEnterRomMode failed\n");
            goto err;
        }
    }
    e = NvOdmFFUExecuteCmd(pHeadCmd, hFGI2c);
    if (e)
    {
        NvOdmOsDebugPrintf("NvOdmFFUFlashing : NvOdmFFUExecuteCmd failed\n");
    }
    e = NvOdmFFUDeviceMode(hFGI2c, &mode);
    if (e)
    {
        NvOdmOsDebugPrintf("\nNvOdmFFUDeviceMode fail\n");
        goto err;
    }
    if (mode == NvOdmFFU_ROM_I2C_ADDR)
    {
        e = NvOdmFFUExitRomMode(hFGI2c);
        if (e)
        {
            NvOdmOsDebugPrintf("NvOdmFFUFlashing :"
               " NvOdmFFUExitRomMode failed\n");
            goto err;
        }
    }

#ifdef NvOdmFFUDEBUG
    e = NvOdmFFUDeviceMode(hFGI2c, &mode);
    if (e)
    {
        NvOdmOsDebugPrintf("\nNvOdmFFUDeviceMode fail\n");
        goto err;
    }
#endif
    NvOdmI2cClose(hFGI2c);
    return NvSuccess;
err:
    NvOdmI2cClose(hFGI2c);
err_nvodm:
    return e;
}

NvError
NvOdmFFUMain(struct NvOdmFFUBuff *pHeadBuff1,
    struct NvOdmFFUBuff *pHeadBuff2)
{
    struct NvOdmFFUBuff *pTempBuff = NULL;
    struct NvOdmFFUCommand *pHeadCmd = NULL;
    NvU32 cnt = 0;
    NvU32 NoOfFiles = 1;
    NvError e = NvSuccess;

    pTempBuff = pHeadBuff1;

    cnt = 0;
    do
    {
        cnt++;

        // Buffer is passed to parsing process
        e = NvOdmFFUParse(pTempBuff, &pHeadCmd);
        if (e)
            goto cleanBuff;

        if (cnt < NoOfFiles)
            NvOdmOsDebugPrintf("\nFile1 : Parsing completed successfully\n");
        else if (NoOfFiles == 2)
            NvOdmOsDebugPrintf("\nFile2 : Parsing completed successfully\n");
        else
            NvOdmOsDebugPrintf("\nParsing completed successfully\n");
        /*
         * pHeadCmd represent the first node of the I2C cmd that has to be
         * executed for Firmware upgradation
         */
        e = NvOdmFFUFlashing(pHeadCmd);
        if (e)
        {
            NvOdmOsDebugPrintf("Failed in I2C execution for firmware"
                            " upgradation\n");
            goto cleanBuff;
        }

        // Firmware upgradation is successful
        if (cnt < NoOfFiles)
            NvOdmOsDebugPrintf("File1 : Firmware Upgradation is"
                           " done successfully\n\n");
        else if (NoOfFiles == 2)
            NvOdmOsDebugPrintf("File2 : Firmware Upgradation is"
                           " done successfully\n\n");
        else
            NvOdmOsDebugPrintf("Firmware Upgradation is done successfully\n\n");
        // Freeing the cmd nodes
        NvOdmFFURemoveCmdNode(&pHeadCmd);

        if (cnt < NoOfFiles)
            pTempBuff = pHeadBuff2;

    } while (cnt < NoOfFiles);

cleanBuff:
    // Freeing the buffer nodes
    NvOdmFFURemoveBuffNode(&pHeadBuff1);
    NvOdmFFURemoveBuffNode(&pHeadBuff2);
    return e;
}

