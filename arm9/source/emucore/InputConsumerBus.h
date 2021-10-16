// =====================================================================================
// Copyright (c) 2021 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, it's source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef INPUTCONSUMERBUS_H
#define INPUTCONSUMERBUS_H

#include "InputConsumer.h"

#define MAX_INPUT_CONSUMERS  4

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
        UINT8          inputConsumerCount;
};

#endif
