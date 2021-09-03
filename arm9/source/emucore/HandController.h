
#ifndef HANDCONTROLLER_H
#define HANDCONTROLLER_H

#define NUM_HAND_CONTROLLER_OBJECTS 23

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

        //functions to get descriptive info about the input consumer
        INT32 getInputConsumerObjectCount();
        InputConsumerObject* getInputConsumerObject(INT32 i);

        void setOutputValue(UINT16 value);
        UINT16 getInputValue();

    private:
        const CHAR* name;
        UINT16 controllerID;

        InputConsumerObject* inputConsumerObjects[NUM_HAND_CONTROLLER_OBJECTS];
        UINT16            inputValue;

        static const UINT16 BUTTON_OUTPUT_VALUES[15];
        static const UINT16 DIRECTION_OUTPUT_VALUES[16];

};

// jeremiah sypult
enum
{
	CONTROLLER_DISC_DOWN = 0x01,
	CONTROLLER_DISC_RIGHT = 0x02,
	CONTROLLER_DISC_UP = 0x04,
	CONTROLLER_DISC_LEFT = 0x08,
	CONTROLLER_DISC_WIDE = 0x10,
	CONTROLLER_DISC_UP_LEFT = 0x1C,
	CONTROLLER_DISC_UP_RIGHT = 0x16,
	CONTROLLER_DISC_DOWN_LEFT = 0x19,
	CONTROLLER_DISC_DOWN_RIGHT = 0x13,

	CONTROLLER_KEYPAD_ONE = 0x81,
	CONTROLLER_KEYPAD_TWO = 0x41,
	CONTROLLER_KEYPAD_THREE = 0x21,
	CONTROLLER_KEYPAD_FOUR = 0x82,
	CONTROLLER_KEYPAD_FIVE = 0x42,
	CONTROLLER_KEYPAD_SIX = 0x22,
	CONTROLLER_KEYPAD_SEVEN = 0x84,
	CONTROLLER_KEYPAD_EIGHT = 0x44,
	CONTROLLER_KEYPAD_NINE = 0x24,
	CONTROLLER_KEYPAD_CLEAR = 0x88,
	CONTROLLER_KEYPAD_ZERO = 0x48,
	CONTROLLER_KEYPAD_ENTER = 0x28,

	CONTROLLER_ACTION_TOP = 0xA0,
	CONTROLLER_ACTION_BOTTOM_LEFT = 0x60,
	CONTROLLER_ACTION_BOTTOM_RIGHT = 0xC0
};

#endif
