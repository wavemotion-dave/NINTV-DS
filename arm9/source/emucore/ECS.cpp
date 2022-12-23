#include "ECS.h"

ECS::ECS()
: Peripheral("Electronic Computer System", "ECS"),
  keyboard(2),
  ramBank(ECS_RAM_SIZE, 0x4000, 0xFFFF, 0xFFFF, 8),
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
      
    AddRAM(&ramBank);      

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