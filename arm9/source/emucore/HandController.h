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

#ifndef HANDCONTROLLER_H
#define HANDCONTROLLER_H

#include "AY38914_InputOutput.h"
#include "InputConsumer.h"

extern UINT8 ds_key_input[2][16];   // Set to '1' if pressed... 0 if released
extern UINT8 ds_disc_input[2][16];  // Set to '1' if pressed... 0 if released.

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
        inline UINT16 getInputValue(UINT8) { return inputValue; }
        void setDirectionIO(UINT16 value);

    private:
        const CHAR* name;
        UINT16 controllerID;

        UINT16            inputValue;

        static const UINT8 BUTTON_OUTPUT_VALUES[15];
        static const UINT8 DIRECTION_OUTPUT_VALUES[16];
};

#endif
