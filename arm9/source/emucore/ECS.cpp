
#include <iostream>
#include "ECS.h"

ECS::ECS()
: Peripheral("Electronic Computer System", "ECS"),
  keyboard(2),
  ramBank(ECS_RAM_SIZE, 0x4000, 8),
  uart(4, 0xE0, 8),
  psg2(0x00F0, &keyboard, &keyboard),
  bank0("ECS ROM #1", "ecs.bin", 0, 2, 0x1000, 0x2000),
  bank0Banker(&bank0, 0x2FFF, 0xFFF0, 0x2A50, 0x000F, 1),
  bank1("ECS ROM #2", "ecs.bin", 8192, 2, 0x1000, 0x7000),
  bank1Banker(&bank1, 0x7FFF, 0xFFF0, 0x7A50, 0x000F, 0),
  bank2("ECS ROM #3", "ecs.bin", 16384, 2, 0x1000, 0xE000),
  bank2Banker(&bank2, 0xEFFF, 0xFFF0, 0xEA50, 0x000F, 1)
{
    AddROM(&bank0);
    AddRAM(&bank0Banker);
    AddROM(&bank1);
    AddRAM(&bank1Banker);
    AddROM(&bank2);
    AddRAM(&bank2Banker);

    AddRAM(&ramBank);
    AddRAM(&uart);

    AddProcessor(&psg2);
    AddAudioProducer(&psg2);
    AddRAM(&psg2.registers);

    AddInputConsumer(&keyboard);
}

ECSState ECS::getState()
{
    ECSState state = {0};

    state.ramState = ramBank.getState(state.ramImage);
    state.uartState = uart.getState(NULL);
    state.psg2State = psg2.getState();

    return state;
}

void ECS::setState(ECSState state)
{
    ramBank.setState(state.ramState, state.ramImage);
    uart.setState(state.uartState, NULL);
    psg2.setState(state.psg2State);
}
