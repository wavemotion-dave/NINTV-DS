// =====================================================================================
// Copyright (c) 2021-2026 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef __DS_LOADGAME_H
#define __DS_LOADGAME_H

#include <nds.h>
#include "types.h"

#define MAX_ROMS    512

typedef struct FICtoLoad 
{
    char   filename[MAX_PATH];
    UINT8  directory;
    UINT8  favorite;
} FICA_INTV;

extern BOOL LoadCart(const CHAR* filename);
extern BOOL LoadPeripheralRoms(Peripheral* peripheral);
extern unsigned int dsWaitForRom(char *chosen_filename);
extern UINT8 load_options;

#define LOAD_NORMALLY           0x01        // Use the normal .cfg or .rom to load the game
#define LOAD_WITH_STOCK_INTY    0x80        // Do not load any periperhals - just the stock Intellivision as God created
#define LOAD_WITH_JLP           0x81        // Load with stock Inty + JLP
#define LOAD_WITH_ECS           0x82        // Load with stock Inty + ECS
#define LOAD_WITH_IVOICE        0x84        // Load with stock Inty + IVOICE
#define LOAD_WITH_TUTORVISION   0x88        // Load in Tutorvision Mode (wbexec, expanded GRAM, expanded system RAM)
// The above bits can be ORed so you can load with combinations (e.g. JLP+ECS or ECS+IVOICE)

#endif
