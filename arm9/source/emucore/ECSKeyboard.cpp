
#include "ECSKeyboard.h"

const INT32 ECSKeyboard::sortedObjectIndices[NUM_ECS_OBJECTS] = {
    24, 29, 34, 28, 33, 27, 32, 26, 31, 25, 30, // ESCAPE, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0
    23, 22, 40, 21, 39, 20, 38, 19, 37, 18,     // Q, W, E, R, T, Y, U, I, O, P
    46, 16, 45, 15, 44, 14, 43, 13, 42, 12, 36, // A, S, D, F, G, H, J, K, L, SEMICOLON, ENTER
    10,  4,  9,  3,  8,  2,  7,  1,  6,         // Z, X, C, V, B, N, M, COMMA, PERIOD
    41, 47,  5,                                 // CTRL, SHIFT, SPACE, 
    17, 11,  0,  35,                            // UP, DOWN, LEFT, RIGHT
};

ECSKeyboard::ECSKeyboard(INT32 id)
: InputConsumer(id),
  rowsToScan(0)
{
    memset(rowInputValues, 0, sizeof(rowInputValues));
}

ECSKeyboard::~ECSKeyboard()
{
}

INT32 ECSKeyboard::getInputConsumerObjectCount()
{
    return 0;
}

InputConsumerObject* ECSKeyboard::getInputConsumerObject(INT32 i)
{
    return NULL;
}

void ECSKeyboard::resetInputConsumer()
{
    rowsToScan = 0;
    for (UINT16 i = 0; i < 8; i++)
        rowInputValues[i] = 0;
}

void ECSKeyboard::evaluateInputs()
{
    for (UINT16 row = 0; row < 6; row++) 
    {
        rowInputValues[row] = 0;
        if (inputConsumerObjects[row]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x01;
        if (inputConsumerObjects[row+6]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x02;
        if (inputConsumerObjects[row+12]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x04;
        if (inputConsumerObjects[row+18]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x08;
        if (inputConsumerObjects[row+24]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x10;
        if (inputConsumerObjects[row+30]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x20;
        if (inputConsumerObjects[row+36]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x40;
        if (inputConsumerObjects[row+42]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x80;
    }
    rowInputValues[6] = (inputConsumerObjects[47]->getInputValue() == 1.0f ? 0x80 : 0);
}

UINT16 ECSKeyboard::getInputValue()
{
    UINT16 inputValue = 0;
    UINT16 rowMask = 1;
    for (UINT16 row = 0; row < 8; row++)  {
        if ((rowsToScan & rowMask) != 0) {
            rowMask = (UINT16)(rowMask << 1);
            continue;
        }
        inputValue |= rowInputValues[row];

        rowMask = (UINT16)(rowMask << 1);
    }

    return (UINT16)(0xFF ^ inputValue);
}

void ECSKeyboard::setOutputValue(UINT16 value) {
    this->rowsToScan = value;
}
