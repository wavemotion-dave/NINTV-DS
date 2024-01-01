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

#include "stdio.h"
#include "InputConsumerBus.h"

InputConsumerBus::InputConsumerBus()
{
    inputConsumerCount = 0;
}

void InputConsumerBus::addInputConsumer(InputConsumer* p)
{
    inputConsumers[inputConsumerCount] = p;
    inputConsumerCount++;
}

void InputConsumerBus::removeInputConsumer(InputConsumer* p)
{
    for (UINT32 i = 0; i < inputConsumerCount; i++) {
        if (inputConsumers[i] == p) {
            for (UINT32 j = i; j < (inputConsumerCount-1); j++)
                inputConsumers[j] = inputConsumers[j+1];
            inputConsumerCount--;
            return;
        }
    }
}

void InputConsumerBus::removeAll()
{
    while (inputConsumerCount)
        removeInputConsumer(inputConsumers[0]);
}

void InputConsumerBus::reset()
{
    for (UINT32 i = 0; i < inputConsumerCount; i++)
        inputConsumers[i]->resetInputConsumer();
}

void InputConsumerBus::evaluateInputs()
{
    //tell each of the input consumers that they may now pull their data from
    //the input device
    for (UINT8 i = 0; i < inputConsumerCount; i++)
        inputConsumers[i]->evaluateInputs();
}
