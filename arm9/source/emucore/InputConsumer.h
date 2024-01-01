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

#ifndef INPUTCONSUMER_H
#define INPUTCONSUMER_H
#include "types.h"

/**
 * A InputConsumer is a gaming input device, such as the hand controllers for
 * the Intellivision and Atari 5200 or the joysticks and paddles for the
 * Atari2600.
 */
class InputConsumer
{

friend class InputConsumerBus;

public:
    InputConsumer(INT32 i) : id(i) {}

    INT32 getId() { return id; }

    virtual const CHAR* getName() = 0;

    virtual void resetInputConsumer() = 0;

    /**
        * Poll the controller.  This function is invoked by the
        * InputConsumerBus just after the Emulator indicates it has entered
        * vertical blank.
        */
    virtual void evaluateInputs() = 0;

private:
    INT32 id;
};

#endif
