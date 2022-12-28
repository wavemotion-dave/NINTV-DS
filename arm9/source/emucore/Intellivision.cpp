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

#include <nds.h>
#include <stdio.h>
#include <time.h>

#include "Intellivision.h"
#include "../debugger.h"
#include "../savestate.h"

/**
 * Initializes all of the basic hardware included in the Intellivision
 * Master Component as well as the Intellivoice peripheral.
 * This method is called only once when the Intellivision is constructed.
 */
Intellivision::Intellivision()
    : Emulator("Intellivision"),
      player1Controller(0, "Hand Controller #1"),
      player2Controller(1, "Hand Controller #2"),
      psg(0x01F0, &player1Controller, &player2Controller),
      RAM8bit(RAM8BIT_SIZE, RAM8BIT_LOCATION, 0xFFFF, 0xFFFF, 8),
      RAM16bit(RAM16BIT_SIZE, RAM16BIT_LOCATION, 0xFFFF, 0xFFFF, 16),
      execROM("Executive ROM", "exec.bin", 0, 2, 0x1000, 0x1000),
      grom(),
      gram(),
      cpu(&memoryBus, 0x1000, 0x1004),
      stic(&memoryBus, &grom, &gram)
{
    // define the video pixel dimensions
    videoWidth = 160;
    videoHeight = 192;

    //add the player one hand controller
    AddInputConsumer(&player1Controller);

    //add the player two hand controller
    AddInputConsumer(&player2Controller);

    //add the 8-bit RAM
    AddRAM(&RAM8bit);

    //add the 16-bit RAM
    AddRAM(&RAM16bit);

    //add the executive ROM
    AddROM(&execROM);

    //add the GROM
    AddROM(&grom);

    //add the GRAM
    AddRAM(&gram);

    //add the backtab ram
    AddRAM(&stic.backtab);

    //add the CPU
    AddProcessor(&cpu);

    //add the STIC
    AddProcessor(&stic);
    AddVideoProducer(&stic);

    //add the STIC registers
    AddRAM(&stic.registers);

#ifdef DEBUG_ENABLE
    debug_stic = &stic;
#endif

    //add the PSG
    AddProcessor(&psg);
    AddAudioProducer(&psg);

    //add the PSG registers
    AddRAM(&psg.registers);

    //add the ECS
    AddPeripheral(&ecs);
          
    //add the Intellivoice
    AddPeripheral(&intellivoice);
}


BOOL Intellivision::SaveState(struct _stateStruct *saveState)
{
    BOOL didSave = TRUE;

    cpu.getState(&saveState->cpuState);
    stic.getState(&saveState->sticState);
    psg.getState(&saveState->psgState);
    RAM8bit.getState(&saveState->RAM8bitState);
    RAM16bit.getState(&saveState->RAM16bitState);
    gram.getState(&saveState->MyGRAMState);
    intellivoice.getState(&saveState->ivoiceState);
    ecs.getState(&saveState->ecsState);
    
    return didSave;
}

BOOL Intellivision::LoadState(struct _stateStruct *saveState)
{
    BOOL didLoadState = TRUE;

    cpu.setState(&saveState->cpuState);
    stic.setState(&saveState->sticState);
    psg.setState(&saveState->psgState);
    RAM8bit.setState(&saveState->RAM8bitState);
    RAM16bit.setState(&saveState->RAM16bitState);
    gram.setState(&saveState->MyGRAMState);
    intellivoice.setState(&saveState->ivoiceState);
    ecs.getState(&saveState->ecsState);
    
    return didLoadState;
}

