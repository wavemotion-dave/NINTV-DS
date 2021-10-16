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

#include "HandController.h"
#include "Intellivision.h"

const UINT16 HandController::BUTTON_OUTPUT_VALUES[15] = {
    0x81, //OUTPUT_KEYPAD_ONE
    0x41, //OUTPUT_KEYPAD_TWO
    0x21, //OUTPUT_KEYPAD_THREE
    0x82, //OUTPUT_KEYPAD_FOUR
    0x42, //OUTPUT_KEYPAD_FIVE
    0x22, //OUTPUT_KEYPAD_SIX
    0x84, //OUTPUT_KEYPAD_SEVEN
    0x44, //OUTPUT_KEYPAD_EIGHT
    0x24, //OUTPUT_KEYPAD_NINE
    0x88, //OUTPUT_KEYPAD_CLEAR
    0x48, //OUTPUT_KEYPAD_ZERO
    0x28, //OUTPUT_KEYPAD_ENTER
    0xA0, //OUTPUT_ACTION_BUTTON_TOP
    0x60, //OUTPUT_ACTION_BUTTON_BOTTOM_LEFT
    0xC0  //OUTPUT_ACTION_BUTTON_BOTTOM_RIGHT
};

const UINT16 HandController::DIRECTION_OUTPUT_VALUES[16] = {
    0x04, //OUTPUT_DISC_NORTH
    0x14, //OUTPUT_DISC_NORTH_NORTH_EAST
    0x16, //OUTPUT_DISC_NORTH_EAST
    0x06, //OUTPUT_DISC_EAST_NORTH_EAST
    0x02, //OUTPUT_DISC_EAST
    0x12, //OUTPUT_DISC_EAST_SOUTH_EAST
    0x13, //OUTPUT_DISC_SOUTH_EAST
    0x03, //OUTPUT_DISC_SOUTH_SOUTH_EAST
    0x01, //OUTPUT_DISC_SOUTH
    0x11, //OUTPUT_DISC_SOUTH_SOUTH_WEST
    0x19, //OUTPUT_DISC_SOUTH_WEST
    0x09, //OUTPUT_DISC_WEST_SOUTH_WEST
    0x08, //OUTPUT_DISC_WEST
    0x18, //OUTPUT_DISC_WEST_NORTH_WEST
    0x1C, //OUTPUT_DISC_NORTH_WEST
    0x0C  //OUTPUT_DISC_NORTH_NORTH_WEST
};

HandController::HandController(INT32 id, const CHAR* n)
: InputConsumer(id),
  name(n)
{
      controllerID = id;
}

HandController::~HandController()
{
}

int ds_key_input[3][16] = {0};   // Set to '1' if pressed... 0 if released
int ds_disc_input[3][16] = {0};  // Set to '1' if pressed... 0 if released.

void HandController::evaluateInputs()
{
    inputValue = 0;
    bool keypad_active = false;

    // Check the keypad and side-buttons first...
    for (UINT16 i = 0; i < 15; i++) 
    {
        if (ds_key_input[controllerID][i]) 
        {
            if (i <= 11) keypad_active=true;
            inputValue |= BUTTON_OUTPUT_VALUES[i];
        }
    }

    // Keypad has priority over disc...
    if (!keypad_active)
    {
        for (UINT16 i = 0; i < 16; i++) 
        {
            if (ds_disc_input[controllerID][i]) inputValue |= DIRECTION_OUTPUT_VALUES[i];
        }
    }

    inputValue = (UINT16)(0xFF ^ inputValue);
}

void HandController::resetInputConsumer()
{
    inputValue = 0xFF;
}

void HandController::setOutputValue(UINT16)
{}

UINT16 HandController::getInputValue()
{
    return inputValue;
}

