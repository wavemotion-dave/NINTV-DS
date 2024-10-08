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

#ifndef INTELLIVISION_H
#define INTELLIVISION_H

#include "Emulator.h"
#include "HandController.h"
#include "Intellivoice.h"
#include "ECS.h"
#include "Emulator.h"
#include "MemoryBus.h"
#include "RAM.h"
#include "ROM.h"
#include "GROM.h"
#include "JLP.h"
#include "CP1610.h"
#include "AY38900.h"
#include "AY38914.h"
#include "../savestate.h"

#define RAM8BIT_SIZE        0x00F0
#define RAM16BIT_SIZE       0x0160
#define RAM8BIT_LOCATION    0x0100
#define RAM16BIT_LOCATION   0x0200

class Intellivision : public Emulator
{
    public:
        Intellivision();
        void SetMachineType(void);
        BOOL SaveState(struct _stateStruct *saveState);
        BOOL LoadState(struct _stateStruct *saveState);

    private:
        //core processors
        CP1610      cpu;
        AY38900     stic;
        AY38914     psg;
    
        //core memories
        RAM         RAM8bit;
        RAM         RAM16bit;
        ROM         execROM;
        ROM         tutorROM;
        GROM        grom;
        GROM        tutorGrom;
        GRAM        gram;

        //hand controllers
        HandController    player1Controller;
        HandController    player2Controller;

        //the ECS peripheral
        ECS               ecs;
        
        //the Intellivoice peripheral
        Intellivoice      intellivoice;
};

#endif
