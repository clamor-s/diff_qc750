/*
 * Copyright 2009 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

/*
 * aos_posix.c : implementation of POSIX system calls for AOS, potentially
 * semi-hosted on an RVICE.  Needed to create AOS executable images which
 * link against ARM's newlib implementation of libc.
 *
 * For background information, see
 * www.codesourcery.com/sgppp/lite/arm/portal/doc4131/libc.pdf
 *
 */


#include <errno.h>
#undef errno
extern int errno;

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "nvcommon.h"
#include "nvos.h"
#include "nvassert.h"

//Give the stdlib calls a small
//heap to play with.
#define SBRK_HEAP_SIZE 8192
char sbrk_heap[SBRK_HEAP_SIZE];

/* standard stub implementation of sbrk and environ */

char *__env[1] = { 0 };
char **environ = __env;

caddr_t _sbrk(int incr)
{
    static char *heap_end;
    char *prev_heap_end;

    if (heap_end==0)
        heap_end = sbrk_heap;

    prev_heap_end = heap_end;
    heap_end += incr;
    
    if ((unsigned int)heap_end > ((unsigned int)sbrk_heap + SBRK_HEAP_SIZE))
    {
        heap_end -= incr;
        prev_heap_end = (char*)-1;
        errno = ENOMEM;
    }

    return (caddr_t)prev_heap_end;
}

/* File operations (open/lseek/read/write/close)
 * Perform semihosting operations here.
 */
NV_NAKED
int _open(const char *name, int flags, int mode)
{
    asm volatile(
    "stmfd sp!, {r1-r3, lr}\n"
    "stm sp, {r0, r2}\n"
    "bl strlen\n"  
    "mov r1, sp\n"
    "str r0, [sp,#8]\n"
    "mov r0, #1\n"
    "svc 0x123456\n"
    "ldmfd sp!, {r2, r3, r12, pc}"
    ::: "r1", "r2", "r3", "r12"
    );
}

NV_NAKED
int _close(int fd)
{
    asm volatile(
    "stmfd sp!, {r3, lr}\n"
    "mov r1, sp\n"
    "str r0, [sp,#0]\n"
    "mov r0, #2\n"
    "svc 0x123456\n"
    "ldmfd sp!, {r12, pc}"
    ::: "r1", "r3", "r12"    
    );
}

int _fstat(int fd, struct stat *st)
{
    return -1;
}

NV_NAKED
ssize_t _read(int fd, void *ptr, size_t len)
{
    asm volatile(
    "stmfd sp!, {r1-r4, lr}\n"
    "stmfd sp!, {r0}\n"
    "stm sp, {r0-r3}\n"
    "mov r1, sp\n"
    "mov r0, #6\n"    
    "svc 0x123456\n"
    "sub r0, r2, r0\n"
    "add sp, sp, #0x10\n"
    "ldmfd sp!, {r4, pc}"
    ::: "r2", "r3", "r4"
    );
}

NV_NAKED
ssize_t _write(int fd, const void *buf, size_t nbyte)
{
    asm volatile(
    "stmfd sp!, {r1-r4, lr}\n"
    "stmfd sp!, {r0}\n"
    "stm sp, {r0-r2}\n"
    "mov r1, sp\n"
    "mov r0, #5\n"    
    "svc 0x123456\n"
    "sub r0, r2, r0\n"
    "add sp, sp, #0x10\n"
    "ldmfd sp!, {r4, pc}"
    ::: "r2", "r3", "r4"
    );    
}

NV_NAKED
off_t _seekpos(int fd, long pos)
{
    asm volatile(
    "stmfd sp!, {r2-r4, lr}\n"
    "stm sp, {r0-r1}\n"
    "mov r1, sp\n"
    "mov r0, #0xa\n"    
    "svc 0x123456\n"
    "ldmfd sp!, {r2-r4, pc}"
    ::: "r1"
    );
}

NV_NAKED
off_t _flen(int fd)
{
    asm volatile(
    "stmfd sp!, {r3, lr}\n"
    "mov r1, sp\n"
    "str r0, [sp, #0]\n"
    "mov r0, #0xc\n"    
    "svc 0x123456\n"
    "ldmfd sp!, {r12, pc}"
    ::: "r1", "r3", "r12"
    );
}

off_t _lseek(int fd, off_t ptr, int dir)
{
    off_t cur_pos;
    
    switch (dir)
    {
        case SEEK_SET:
            cur_pos = _seekpos(fd, ptr);
        break;
        case SEEK_CUR:
            cur_pos = 0;
        break;
        case SEEK_END:
            cur_pos = _flen(fd);
            cur_pos = _seekpos(fd, cur_pos+ptr);
    }
    
    return cur_pos;
}

/* not supported on AOS */
int _stat(char *file, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}

/* stubs for single-process, stdout-only terminal */
int _getpid(void)
{
    return 1;
}

int _isatty(int file)
{
    return 1;
}

void _exit(int status)
{
    NV_ASSERT("Exit" && status!=status);
}

int _times(struct tms *buf)
{
    return -1;
}

/* AOS doesn't support fork / exec / kill / wait / link / unlink */
int _fork(void)
{
    errno = EAGAIN;
    return -1;
}

int _execve(char *name, char **argv, char **env)
{
    errno = ENOMEM;
    return -1;
}

int _kill(int pid, int sig)
{
    errno = EINVAL;
    return -1;
}

int _wait(int *status)
{
    errno = ECHILD;
    return -1;
}

int _link(char *old, char *new)
{
    errno = EMLINK;
    return -1;
}

int _unlink(char *name)
{
    errno = ENOENT;
    return -1;
}
