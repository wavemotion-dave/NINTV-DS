// =====================================================================================
// Copyright (c) 2021 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, it's source code and associated 
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

extern int debug1, debug2;
    
//primitive types
typedef signed char				INT8;
typedef unsigned char			UINT8;
typedef signed short			INT16;
typedef unsigned short			UINT16;
typedef signed int				INT32;
typedef unsigned int			UINT32;

// There is no ANSI standard for 64 bit integers :(
#ifdef _MSC_VER
typedef __int64					INT64;  // MS VC++
typedef unsigned __int64		UINT64;  // MS VC++
#else
typedef signed long long int	INT64;  // C99 C++11
typedef unsigned long long int	UINT64;	// C99 C++11
#endif

typedef char					CHAR;

#if !defined(BOOL)
#if defined(__MACH__)
typedef signed char				BOOL;
#else
typedef int						BOOL;
#endif
#endif

#if !defined(TRUE)
#define TRUE	(1)
#endif

#if !defined(FALSE)
#define FALSE	(0)
#endif

#if !defined(NULL)
#if defined(__cplusplus)
#define NULL (0)
#else
#define NULL ((void *)0)
#endif
#endif

#define SWAP32BIT(swap)			((((swap) << 24) & 0xFF000000) | \
								 (((swap) <<  8) & 0x00FF0000) | \
								 (((swap) >>  8) & 0x0000FF00) | \
								 (((swap) >> 24) & 0x000000FF))

#if !defined(__LITTLE_ENDIAN__) && defined(_WIN32)
#define __LITTLE_ENDIAN__		(1)
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

typedef unsigned char			UCHAR;
typedef unsigned short			USHORT;

#ifndef GUID
typedef struct
{
	unsigned int			data1;
	unsigned short			data2;
	unsigned short			data3;
	unsigned char			data4[8];
} GUID;
#endif

#ifndef ZeroMemory
#define ZeroMemory(ptr,size) memset(ptr, 0, size)
#endif

#ifndef strcmpi
#define strcmpi strcasecmp
#endif

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

// total hack for non-windows
#ifndef GUID_SysKeyboard
#define GUID_SysKeyboard	(GUID){0}
#define DIK_NUMPAD7			((INT32)0x81)
#define DIK_NUMPAD8			((INT32)0x41)
#define DIK_NUMPAD9			((INT32)0x21)
#define DIK_NUMPAD4			((INT32)0x82)
#define DIK_NUMPAD5			((INT32)0x42)
#define DIK_NUMPAD6			((INT32)0x22)
#define DIK_NUMPAD1			((INT32)0x84)
#define DIK_NUMPAD2			((INT32)0x44)
#define DIK_NUMPAD3			((INT32)0x24)
#define DIK_NUMPADPERIOD	((INT32)0x88)
#define DIK_NUMPAD0			((INT32)0x48)
#define DIK_NUMPADENTER		((INT32)0x28)
#define DIK_SPACE			((INT32)0xA0)
#define DIK_LALT			((INT32)0x00)
#define DIK_LCONTROL		((INT32)0x60)
#define DIK_RCONTROL		((INT32)0xC0)
#define DIK_UP				((INT32)0x04)
#define DIK_W				((INT32)0x16)
#define DIK_RIGHT			((INT32)0x02)
#define DIK_S				((INT32)0x13)
#define DIK_DOWN			((INT32)0x01)
#define DIK_A				((INT32)0x19)
#define DIK_LEFT			((INT32)0x08)
#define DIK_Q				((INT32)0x1C)

#define DIK_COMMA			((INT32)0x0)
#define DIK_N				((INT32)0x0)
#define DIK_V				((INT32)0x0)
#define DIK_X				((INT32)0x0)
#define DIK_PERIOD			((INT32)0x0)
#define DIK_M				((INT32)0x0)
#define DIK_B				((INT32)0x0)
#define DIK_C				((INT32)0x0)
#define DIK_Z				((INT32)0x0)
#define DIK_SEMICOLON		((INT32)0x0)
#define DIK_K				((INT32)0x0)
#define DIK_H				((INT32)0x0)
#define DIK_F				((INT32)0x0)
#define DIK_P				((INT32)0x0)
#define DIK_I				((INT32)0x0)
#define DIK_Y				((INT32)0x0)
#define DIK_R				((INT32)0x0)
#define DIK_GRAVE			((INT32)0x0)
#define DIK_TAB				((INT32)0x0)
#define DIK_RETURN			((INT32)0x0)
#define DIK_O				((INT32)0x0)
#define DIK_U				((INT32)0x0)
#define DIK_T				((INT32)0x0)
#define DIK_E				((INT32)0x0)
#define DIK_L				((INT32)0x0)
#define DIK_J				((INT32)0x0)
#define DIK_G				((INT32)0x0)
#define DIK_D				((INT32)0x0)
#define DIK_LSHIFT			((INT32)0x0)

#define DIK_F2				((INT32)0x0)

#define DI__WTFX			((INT32)0xFA)
#define DI__WTFY			((INT32)0xFB)
#endif

#ifdef __cplusplus
}
#endif

#endif //TYPES_H

