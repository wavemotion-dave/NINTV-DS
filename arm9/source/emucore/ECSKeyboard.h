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

#ifndef ECSKEYBOARD_H
#define ECSKEYBOARD_H

#include "AY38914_InputOutput.h"
#include "InputConsumer.h"

// -------------------------------------------------------------
// For the ECS Keybaord (if present)
// -------------------------------------------------------------
extern UINT8 ecs_key_pressed;

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
        
        UINT16 getInputValue();
        void setOutputValue(UINT16 value);
        void setDirectionIO(UINT16 value);

    private:
        UINT16        directionIO;
        UINT16        rowsOrColumnsToScan;
        UINT16        rowInputValues[8];
};

#endif

