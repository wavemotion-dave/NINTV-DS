// =====================================================================================
// Copyright (c) 2021-2024 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef TYPES_H
#define TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)    
    
//primitive types
typedef signed char             INT8;
typedef unsigned char           UINT8;
typedef signed short            INT16;
typedef unsigned short          UINT16;
typedef signed int              INT32;
typedef unsigned int            UINT32;
typedef signed long long int    INT64;  // C99 C++11
typedef unsigned long long int  UINT64; // C99 C++11
typedef char                    CHAR;

#if !defined(BOOL)
#if defined(__MACH__)
typedef signed char             BOOL;
#else
typedef int                     BOOL;
#endif
#endif
    
// Used universally...
extern INT32 debug[];

#if !defined(TRUE)
#define TRUE    (1)
#endif

#if !defined(FALSE)
#define FALSE   (0)
#endif

#if !defined(NULL)
#if defined(__cplusplus)
#define NULL (0)
#else
#define NULL ((void *)0)
#endif
#endif

#if !defined(__LITTLE_ENDIAN__) && defined(_WIN32)
#define __LITTLE_ENDIAN__       (1)
#endif

#if defined(__LITTLE_ENDIAN__)
#define FOURCHAR(x) SWAP32BIT((UINT32)(x))
#else
#define FOURCHAR(x) (x)
#endif

#if !defined(TYPEDEF_STRUCT_PACK)
#define TYPEDEF_STRUCT_PACK(_x_) typedef struct __attribute__((__packed__)) _x_
#endif

    
#include <string.h>

typedef unsigned char           UCHAR;
typedef unsigned short          USHORT;

#ifndef GUID
typedef struct
{
    unsigned int            data1;
    unsigned short          data2;
    unsigned short          data3;
    unsigned char           data4[8];
} GUID;
#endif

#ifndef ZeroMemory
#define ZeroMemory(ptr,size) memset(ptr, 0, size)
#endif

#ifndef strcmpi
#define strcmpi strcasecmp
#endif

#ifndef MAX_PATH
#define MAX_PATH 167
#endif

#ifdef __cplusplus
}
#endif

#endif //TYPES_H
