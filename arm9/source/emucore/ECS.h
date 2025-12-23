// =====================================================================================
// Copyright (c) 2021-2025 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef ECS_H
#define ECS_H

#include "Peripheral.h"
#include "ECSKeyboard.h"
#include "AudioOutputLine.h"
#include "RAM.h"
#include "ROM.h"
#include "ROMBanker.h"
#include "types.h"
#include "AY38914.h"

#define ECS_RAM_SIZE        0x0800
#define ECS_RAM_LOCATION    0x4000

TYPEDEF_STRUCT_PACK( _ECSState
{
    AY38914State psg2State;
} ECSState; )

class Intellivision;

class ECS : public Peripheral
{
    friend class Intellivision;

    public:
        ECS();
        
        void getState(ECSState *state);
        void setState(ECSState *state);
        bool isBank0Enabled() {return bank0.IsEnabled();}
        bool isBank1Enabled() {return bank1.IsEnabled();}
        bool isBank2Enabled() {return bank2.IsEnabled();}

    private:
        ECSKeyboard         keyboard;

    public:
        ROM       bank0;
        ROM       bank1;
        ROM       bank2;
        RAM       ecsRAM;
        RAM       uartRAM;
        AY38914   psg2;
        ROMBanker banker0;
        ROMBanker banker1;
        ROMBanker banker2;
};

#endif
