
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
    for (UINT32 i = 0; i < inputConsumerCount; i++)
        inputConsumers[i]->evaluateInputs();
}
