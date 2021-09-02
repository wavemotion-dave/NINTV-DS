
#ifndef INPUTCONSUMERBUS_H
#define INPUTCONSUMERBUS_H

#include "InputConsumer.h"

const INT32 MAX_INPUT_CONSUMERS = 10;

class InputConsumerBus
{

    public:
        InputConsumerBus();
        void reset();
        void evaluateInputs();

        void addInputConsumer(InputConsumer* ic);
        void removeInputConsumer(InputConsumer* ic);
        void removeAll();

    private:
        InputConsumer*  inputConsumers[MAX_INPUT_CONSUMERS];
        UINT32          inputConsumerCount;

};

#endif
