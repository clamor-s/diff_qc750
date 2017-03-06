#ifndef __SGDEFS_H
#define __SGDEFS_H

#if !defined(__HGPUBLICDEFS_H)
#   include "hgdefs.h"
#endif

#define SG_VERSION_MAJOR 7
#define SG_VERSION_MINOR 0
#define SG_VERSION_PATCH 4

HG_EXTERN_C const char* sgGetVersion(void);

#endif /* __SGDEFS_H */
