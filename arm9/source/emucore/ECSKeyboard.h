
#ifndef ECSKEYBOARD_H
#define ECSKEYBOARD_H

#define NUM_ECS_OBJECTS 48

#include "AY38914_InputOutput.h"
#include "InputConsumer.h"

class EmulationDirector;

class ECSKeyboard : public AY38914_InputOutput, public InputConsumer
{

    friend class EmulationDirector;

    public:
        ECSKeyboard(INT32 id);
        virtual ~ECSKeyboard();

        const CHAR* getName() { return "ECS Keyboard"; }
	    
        void resetInputConsumer();

        /**
         * Poll the controller.  This function is invoked by the
         * InputConsumerBus just after the Emulator indicates it has entered
         * vertical blank.
         */
        void evaluateInputs();

        //functions to get descriptive info about the input consumer
        INT32 getInputConsumerObjectCount();
        InputConsumerObject* getInputConsumerObject(INT32 i);

        UINT16 getInputValue();
        void setOutputValue(UINT16 value);

    private:
        InputConsumerObject* inputConsumerObjects[NUM_ECS_OBJECTS];
        UINT16        rowsToScan;
        UINT16        rowInputValues[8];

        static const INT32         sortedObjectIndices[NUM_ECS_OBJECTS];

};

#endif

