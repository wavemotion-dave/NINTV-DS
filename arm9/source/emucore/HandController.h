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

#ifndef HANDCONTROLLER_H
#define HANDCONTROLLER_H

#include "AY38914_InputOutput.h"
#include "InputConsumer.h"

class HandController : public AY38914_InputOutput, public InputConsumer
{

    public:
        HandController(INT32 id, const CHAR* n);
        virtual ~HandController();

        const CHAR* getName() { return name; }
	    
        void resetInputConsumer();

        /**
         * Poll the controller.  This function is invoked by the
         * InputConsumerBus just after the Emulator indicates it has entered
         * vertical blank.
         */
        void evaluateInputs();

        void setOutputValue(UINT16 value);
        UINT16 getInputValue();

    private:
        const CHAR* name;
        UINT16 controllerID;

        UINT16            inputValue;

        static const UINT16 BUTTON_OUTPUT_VALUES[15];
        static const UINT16 DIRECTION_OUTPUT_VALUES[16];

};

#endif
