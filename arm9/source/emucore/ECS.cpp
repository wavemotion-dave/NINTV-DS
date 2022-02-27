#include "ECS.h"

ECS::ECS()
: Peripheral("Electronic Computer System", "ECS"),
  keyboard(2),
  ramBank(ECS_RAM_SIZE, 0x4000, 0xFFFF, 0xFFFF, 8),
  psg2(0x00F0, &keyboard, &keyboard),
  bank0("ECS ROM #1", "ecs.bin", 0, 2, 0x1000, 0x2000),
  bank1("ECS ROM #2", "ecs.bin", 8192, 2, 0x1000, 0x7000),
  bank2("ECS ROM #3", "ecs.bin", 16384, 2, 0x1000, 0xE000)
{
    AddROM(&bank0);
    AddROM(&bank1);
    AddROM(&bank2);
    AddRAM(&ramBank);

    AddProcessor(&psg2);
    AddAudioProducer(&psg2);
    AddRAM(&psg2.registers);

    AddInputConsumer(&keyboard);
}

