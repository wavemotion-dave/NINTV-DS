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
#include "CP1610.h"
#include "AY38900.h"
#include "AY38914.h"

extern void savestate_entry(void);

struct _stateStruct
{
    UINT16              frames;
    UINT16              emu_frames;
    CP1610State         cpuState;
    AY38900State        sticState;
    AY38914State        psgState;
    RAMState            RAM8bitState;
    RAMState            RAM16bitState;
    GRAMState           MyGRAMState; 
    IntellivoiceState   ivoiceState;
    RAMState            ExtraCartRamState;
};


#endif
