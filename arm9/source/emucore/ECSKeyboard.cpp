
#include "ECSKeyboard.h"


ECSKeyboard::ECSKeyboard(INT32 id)
: InputConsumer(id),
  rowsToScan(0)
{
    memset(rowInputValues, 0, sizeof(rowInputValues));
}

ECSKeyboard::~ECSKeyboard()
{
}

void ECSKeyboard::resetInputConsumer()
{
    rowsToScan = 0;
    for (UINT16 i = 0; i < 8; i++)
        rowInputValues[i] = 0;
}

// $00FE |                         $00FF bits
// bits  | 0       1       2       3       4       5       6       7
// ------+-----------------------------------------------------------------
//   0   | left,   period, scolon, p,      esc,    0,      enter,  [n/a]
//   1   | comma,  m,      k,      i,      9,      8,      o,      l
//   2   | n,      b,      h,      y,      7,      6,      u,      j
//   3   | v,      c,      f,      r,      5,      4,      t,      g
//   4   | x,      z,      s,      w,      3,      2,      e,      d
//   5   | space,  down,   up,     q,      1,      right,  ctl,    a
//   6   | [n/a],  [n/a],  [n/a],  [n/a],  [n/a],  [n/a],  [n/a],  shift
  
  
void ECSKeyboard::evaluateInputs()
{
    extern UINT8 bUseECS;
    extern int ds_key_input[3][16];   // Set to '1' if pressed... 0 if released
    
    if (ds_key_input[0][0])  rowInputValues[5] |= 0x10; else rowInputValues[5] &= ~0x10;    // '1'
    if (ds_key_input[0][1])  rowInputValues[4] |= 0x20; else rowInputValues[4] &= ~0x20;    // '2'
    if (ds_key_input[0][2])  rowInputValues[4] |= 0x10; else rowInputValues[4] &= ~0x10;    // '3'
    if (ds_key_input[0][3])  rowInputValues[3] |= 0x20; else rowInputValues[3] &= ~0x20;    // '4'
    if (ds_key_input[0][4])  rowInputValues[3] |= 0x10; else rowInputValues[3] &= ~0x10;    // '5'
    if (bUseECS == 2)   // Special for Mind Strike which needs START to run... sigh...
    {
        if (ds_key_input[0][5])  rowInputValues[4] |= 0x04; else rowInputValues[4] &= ~0x04;    // 'S'
        if (ds_key_input[0][6])  rowInputValues[3] |= 0x40; else rowInputValues[3] &= ~0x40;    // 'T'    
        if (ds_key_input[0][7])  rowInputValues[5] |= 0x80; else rowInputValues[5] &= ~0x80;    // 'A'
        if (ds_key_input[0][8])  rowInputValues[3] |= 0x08; else rowInputValues[3] &= ~0x08;    // 'R'
    }
    else
    {
        if (ds_key_input[0][5])  rowInputValues[2] |= 0x20; else rowInputValues[2] &= ~0x20;    // '6'    
        if (ds_key_input[0][6])  rowInputValues[2] |= 0x10; else rowInputValues[2] &= ~0x10;    // '7'
        if (ds_key_input[0][7])  rowInputValues[1] |= 0x20; else rowInputValues[1] &= ~0x20;    // '8'
        if (ds_key_input[0][8])  rowInputValues[1] |= 0x10; else rowInputValues[1] &= ~0x10;    // '9'
    }
    
    if (ds_key_input[0][9])  rowInputValues[5] |= 0x01; else rowInputValues[5] &= ~0x01;    // SP
    if (ds_key_input[0][10]) rowInputValues[2] |= 0x01; else rowInputValues[2] &= ~0x01;    // 'N'
    if (ds_key_input[0][11]) rowInputValues[0] |= 0x40; else rowInputValues[0] &= ~0x40;    // CR
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
