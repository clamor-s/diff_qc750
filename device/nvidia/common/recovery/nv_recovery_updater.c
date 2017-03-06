/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "edify/expr.h"
#include "minzip/Zip.h"
#include "updater/updater.h"

#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE (1024)

static int GetDevicePath(char *mount_path, char *device_path)
{

    if (NULL == mount_path || NULL == device_path)
        return -1;

    FILE* fstab = fopen("/etc/recovery.fstab", "r");
    if (fstab == NULL)
        return -1;

    char *buffer;
    int i;
    unsigned int found = 0;
    buffer = malloc(BUFFER_SIZE);
    if (buffer == NULL)
    {
        fclose(fstab);
        return -1;
    }
    while (fgets(buffer, BUFFER_SIZE-1, fstab))
    {
        for (i = 0; buffer[i] && isspace(buffer[i]); ++i);
        if (buffer[i] == '\0' || buffer[i] == '#')
            continue;

        char* original = strdup(buffer);
        char* mount_point = strtok(buffer+i, " \t\n");

        if (mount_point && (0 == strcmp(mount_point, mount_path)))
        {
            char* fs_type = strtok(NULL, " \t\n");
            char* device = strtok(NULL, " \t\n");
            strcpy(device_path, device);
            found = 1;
        }
        free(original);
        if (found == 1)
        {
            free(buffer);
            fclose(fstab);
            return 0;
        }
    }
    free(buffer);
    fclose(fstab);
    return -1;
}

void updateRebootFlag(){
	char* path ="/dev/block/platform/sdhci-tegra.3/by-name/UDC";
	int f = open(path, O_RDWR);

	fprintf(stderr, "writing updateRebootFlag3 f=0x%x\n",f);
	
	if(f <=0){
		return;
	}

	lseek(f, 2048, SEEK_SET);

	char* command ="ota";
	int readb =write(f,command, strlen(command));
	if(readb!=strlen(command)){
		fprintf(stderr, "write fail!!readb=%d\n",readb);
		close(f);
		return;
	}
	close(f);
}

/*  copies the blob file to staging partition */
Value* NvCopyBlobToUSP(const char* name, State* state,
                           int argc, Expr* argv[])
{

    if (argc != 2)
    {
        return ErrorAbort(state, "%s() expects 2 args, but got %d",
                          name, argc);
    }
    unsigned int success = 0;

    char* zip_path;
    char* dest_path;
    char device_path[128];
    if (ReadArgs(state, argv, 2, &zip_path, &dest_path) < 0)
        return NULL;

    ZipArchive* za = ((UpdaterInfo*)(state->cookie))->package_zip;
    const ZipEntry* entry = mzFindZipEntry(za, zip_path);
    if (entry == NULL)
    {
        fprintf(stderr, "%s: no %s in package\n", name, zip_path);
        goto fail;
    }

    /*  this function will return the device path for the corresponding input path */
    if (GetDevicePath(dest_path, device_path) != 0)
    {
        fprintf(stderr, "%s: couldn't get the device path\n", name);
        return NULL;
    }

    FILE* f = fopen(device_path, "wb");
    if (f == NULL)
    {
        fprintf(stderr, "%s: can't open %s for write: %s\n",
                name, device_path, strerror(errno));
        goto fail;
    }
    success = mzExtractZipEntryToFile(za, entry, fileno(f));
    fclose(f);

    //updateRebootFlag();

fail:
    free(zip_path);
    free(dest_path);
    return StringValue(strdup(success ? "t" : ""));
}

void Register_libnvrecoveryupdater()
{
    RegisterFunction("nv_copy_blob_file",NvCopyBlobToUSP);
}

