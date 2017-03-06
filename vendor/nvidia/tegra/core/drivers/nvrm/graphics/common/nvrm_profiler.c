/*
 * Copyright (c) 2008 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvrm_profiler.h"
#include "nvrm_moduleloader_private.h"


NvU32 GetSymbolTable(NvU8 *image, char **strtable, char **symtable, NvU32 loadOffset);

/** 
  * This function rips the symbol table and string table from the given
  * ELF file and stores it for later analysis. This function is called
  * once per task (kernel is considered a task as well)
  *
  * @param image - ELF file data
  * @param strtable - Variable in which to rip the string table
  * @param symtable - Variable in which to rip the symbol table
  * @param loadOffset - Loadoffset at which the task is loaded
  * @return Returns the number of symbols present in the symbol table
  */
NvU32 GetSymbolTable(NvU8 *image, char **strtable, char **symtable, NvU32 loadOffset)
{

    NvU32 i = 0, j;    
    char *stringTable;
    Elf32_Ehdr *elfH;
    Elf32_Shdr *shPBase;
    NvU32 size;    
    char *name;            
    NvU32 strtabOffset = 0, symtabOffset = 0, numEntries = 0, offset, strtabSize = 0, numSymbols = 0;    
    Elf32_Sym symtab;    
    char *tempPtr;

    // Search for specified Elf headers
    elfH = (Elf32_Ehdr *) (void *) image;
    shPBase = (Elf32_Shdr *) (void *) (image+elfH->e_shoff);
    stringTable = (char*)&image[shPBase[elfH->e_shstrndx].sh_offset];
    
    size = 0;
    for (i = 0; i < elfH->e_shnum; i++)
    {
        Elf32_Shdr *shP = &shPBase[i];
        name = stringTable+shP->sh_name;
        size = shP->sh_size;

        if (!NvOsStrcmp(name, ".strtab"))
        {
            strtabOffset = shP->sh_offset;
            *strtable = NvOsAlloc(size);
            strtabSize = size;
        }
        if (!NvOsStrcmp(name, ".symtab"))
        {
            symtabOffset = shP->sh_offset;
            numEntries = size / 16;
            *symtable = NvOsAlloc(size);
        }
    }        
       
    if (strtabSize == 0 || numEntries == 0)
    {
        NvOsDebugPrintf("ProfilingError: Symbol Table not found. Ensure that all binaries compiled in debug mode.\n");
        NV_ASSERT(0);
    }

    tempPtr = *symtable;        
    NvOsMemcpy(*strtable, &image[strtabOffset], strtabSize);        

    for (j=0;j<numEntries;j++)    
    {        
        offset = symtabOffset + j * sizeof(symtab);                    
        NvOsMemcpy(&symtab, &image[offset], sizeof(symtab));    
        
        //To save space, we only store symbols that have non-zero size.
        if (symtab.st_size > 0)
        {                            
            numSymbols++;

            symtab.st_value += loadOffset; //relocate symtab

            NvOsMemcpy(tempPtr, &symtab, sizeof(symtab));   
            tempPtr += sizeof(symtab);
        }            
    }

    return numSymbols;
}

NvError NvRmProfilerStart(NvOsFileHandle elfSourceHandle, NvU32 loadOffset)
{
    return NvSuccess;
}

void NvRmProfilerStop( void )
{
    return;
}

