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

#include "ECS.h"
#include "../debugger.h"

extern UINT8 ecs_ram[];

ECS::ECS()
: Peripheral("Electronic Computer System", "ECS"),
  keyboard(2),
  ecsRAM(ECS_RAM_SIZE, ECS_RAM_LOCATION, 0xFFFF, 0xFFFF, 8),
  uartRAM(4, 0x00E0, 0xFFFF, 0xFFFF, 8),
  psg2(0x00F0, &keyboard, &keyboard),
  bank0("ECS ROM #1", "ecs.bin", 0,     2, 0x1000, 0x2000),
  banker0(&bank0, 0x2FFF, 0xFFF0, 0x2A50, 0x000F, 1),
  bank1("ECS ROM #2", "ecs.bin", 8192,  2, 0x1000, 0x7000),
  banker1(&bank1, 0x7FFF, 0xFFF0, 0x7A50, 0x000F, 0),
  bank2("ECS ROM #3", "ecs.bin", 16384, 2, 0x1000, 0xE000),
  banker2(&bank2, 0xEFFF, 0xFFF0, 0xEA50, 0x000F, 1)
{
    bank0.SetEnabled(false);
    AddROM(&bank0);
    AddRAM(&banker0);
      
    bank1.SetEnabled(true);
    AddROM(&bank1);
    AddRAM(&banker1);
      
    bank2.SetEnabled(false);
    AddROM(&bank2);
    AddRAM(&banker2);
      
    AddRAM(&ecsRAM);
    AddRAM(&uartRAM);

#ifdef DEBUG_ENABLE
    debug_psg2  = &psg2;
#endif
      
    AddProcessor(&psg2);
    AddAudioProducer(&psg2);
    AddRAM(&psg2.registers);
      
    AddInputConsumer(&keyboard);
}

void ECS::getState(ECSState *state)
{
    psg2.getState(&state->psg2State);
}

void ECS::setState(ECSState *state)
{
    psg2.setState(&state->psg2State);
 }
