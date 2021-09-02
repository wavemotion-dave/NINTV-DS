
#ifndef INPUTCONSUMER_H
#define INPUTCONSUMER_H

#include "InputConsumerObject.h"
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

    //functions to get descriptive info about the input consumer
    virtual INT32 getInputConsumerObjectCount() = 0;
    virtual InputConsumerObject* getInputConsumerObject(INT32 i) = 0;

private:
    INT32 id;
};

#endif
