// =====================================================================================
// Copyright (c) 2021-2023 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef __DEBUGGER_H
#define __DEBUGGER_H

//#define DEBUG_ENABLE

#include <nds.h>
#include "types.h"
#include "AY38900.h"
#include "AY38914.h"

#define DBG_PRESS_PLAY   0
#define DBG_PRESS_STOP   1
#define DBG_PRESS_STEP   2
#define DBG_PRESS_FRAME  3
#define DBG_PRESS_META   254
#define DBG_PRESS_NONE   255

extern UINT32 debug_frames;
extern UINT32 debug_opcodes;
extern AY38900 *debug_stic;
extern AY38914 *debug_psg;
extern AY38914 *debug_psg2;

extern INT32 debug[];

extern void show_debug_overlay(void);
extern void debugger(void);
extern UINT8 debugger_input(UINT16 tx, UINT16 ty);
extern int getMemUsed(void);

#endif
