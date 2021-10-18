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
#ifndef __DEBUGGER_H
#define __DEBUGGER_H

#include <nds.h>
#include "types.h"
#include "AY38900.h"

//#define DEBUG_ENABLE

extern UINT32 debug_frames;
extern UINT32 debug_opcodes;
extern AY38900 *debug_stic;

extern void show_debug_overlay(void);
extern void debugger(void);
extern UINT8 debugger_input(int tx, int ty);

#endif
