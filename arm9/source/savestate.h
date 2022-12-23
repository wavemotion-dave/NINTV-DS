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
#ifndef __SAVESTATE_H
#define __SAVESTATE_H

#include <nds.h>
#include "types.h"
#include "Emulator.h"
#include "HandController.h"
#include "Intellivoice.h"
#include "Emulator.h"
#include "MemoryBus.h"
#include "RAM.h"
#include "ROM.h"
#include "GROM.h"
#include "JLP.h"
#include "ECS.h"
#include "CP1610.h"
#include "AY38900.h"
#include "AY38914.h"

extern void savestate_entry(void);

// ---------------------------------------------------------------------------------
// This represents a single game save slot... it's our whole Intellivision world!
// ---------------------------------------------------------------------------------
struct _stateStruct
{
    UINT16              global_frames;
    UINT16              emu_frames;
    CP1610State         cpuState;
    AY38900State        sticState;
    AY38914State        psgState;
    RAMState            RAM8bitState;
    RAMState            RAM16bitState;
    GRAMState           MyGRAMState; 
    IntellivoiceState   ivoiceState;
    ECSState            ecsState;
    UINT16              lastBankers[16];
    UINT16              extraRAM[0x800];
};


#endif
