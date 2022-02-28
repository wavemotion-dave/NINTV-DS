
#include "ECSKeyboard.h"

UINT8 ecs_key_pressed = 0;

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
    
    if (ecs_key_pressed == -1)  // No mini-keyboard so just use dual-purpose the main keypad
    {
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
    else
    {
        // If a key on the ECS Mini-Keyboard was pressed...
//        if (ecs_key_pressed > 0)
        {
            if (ecs_key_pressed == 1)  rowInputValues[5] |= 0x10; else rowInputValues[5] &= ~0x10;    // '1'
            if (ecs_key_pressed == 2)  rowInputValues[4] |= 0x20; else rowInputValues[4] &= ~0x20;    // '2'
            if (ecs_key_pressed == 3)  rowInputValues[4] |= 0x10; else rowInputValues[4] &= ~0x10;    // '3'
            if (ecs_key_pressed == 4)  rowInputValues[3] |= 0x20; else rowInputValues[3] &= ~0x20;    // '4'
            if (ecs_key_pressed == 5)  rowInputValues[3] |= 0x10; else rowInputValues[3] &= ~0x10;    // '5'
            if (ecs_key_pressed == 6)  rowInputValues[2] |= 0x20; else rowInputValues[2] &= ~0x20;    // '6'    
            if (ecs_key_pressed == 7)  rowInputValues[2] |= 0x10; else rowInputValues[2] &= ~0x10;    // '7'
            if (ecs_key_pressed == 8)  rowInputValues[1] |= 0x20; else rowInputValues[1] &= ~0x20;    // '8'
            if (ecs_key_pressed == 9)  rowInputValues[1] |= 0x10; else rowInputValues[1] &= ~0x10;    // '9'
            if (ecs_key_pressed == 10) rowInputValues[0] |= 0x20; else rowInputValues[0] &= ~0x20;    // '0'
            if (ecs_key_pressed == 11) rowInputValues[5] |= 0x80; else rowInputValues[5] &= ~0x80;    // 'A'
            if (ecs_key_pressed == 12) rowInputValues[2] |= 0x02; else rowInputValues[2] &= ~0x02;    // 'B'            
            if (ecs_key_pressed == 13) rowInputValues[3] |= 0x02; else rowInputValues[3] &= ~0x02;    // 'C'
            if (ecs_key_pressed == 14) rowInputValues[4] |= 0x80; else rowInputValues[4] &= ~0x80;    // 'D'
            if (ecs_key_pressed == 15) rowInputValues[4] |= 0x40; else rowInputValues[4] &= ~0x40;    // 'E'
            if (ecs_key_pressed == 16) rowInputValues[3] |= 0x04; else rowInputValues[3] &= ~0x04;    // 'F'
            if (ecs_key_pressed == 17) rowInputValues[3] |= 0x80; else rowInputValues[3] &= ~0x80;    // 'G'            
            if (ecs_key_pressed == 18) rowInputValues[2] |= 0x04; else rowInputValues[2] &= ~0x04;    // 'H'
            if (ecs_key_pressed == 19) rowInputValues[1] |= 0x08; else rowInputValues[1] &= ~0x08;    // 'I'
            if (ecs_key_pressed == 20) rowInputValues[2] |= 0x80; else rowInputValues[2] &= ~0x80;    // 'J'
            if (ecs_key_pressed == 21) rowInputValues[1] |= 0x04; else rowInputValues[1] &= ~0x04;    // 'K'
            if (ecs_key_pressed == 22) rowInputValues[1] |= 0x80; else rowInputValues[1] &= ~0x80;    // 'L'
            if (ecs_key_pressed == 23) rowInputValues[1] |= 0x02; else rowInputValues[1] &= ~0x02;    // 'M'
            if (ecs_key_pressed == 24) rowInputValues[2] |= 0x01; else rowInputValues[2] &= ~0x01;    // 'N'
            if (ecs_key_pressed == 25) rowInputValues[1] |= 0x40; else rowInputValues[1] &= ~0x40;    // 'O'
            if (ecs_key_pressed == 26) rowInputValues[0] |= 0x08; else rowInputValues[0] &= ~0x08;    // 'P'
            if (ecs_key_pressed == 27) rowInputValues[5] |= 0x08; else rowInputValues[5] &= ~0x08;    // 'Q'
            if (ecs_key_pressed == 28) rowInputValues[3] |= 0x08; else rowInputValues[3] &= ~0x08;    // 'R'
            if (ecs_key_pressed == 29) rowInputValues[4] |= 0x04; else rowInputValues[4] &= ~0x04;    // 'S'
            if (ecs_key_pressed == 30) rowInputValues[3] |= 0x40; else rowInputValues[3] &= ~0x40;    // 'T'
            if (ecs_key_pressed == 31) rowInputValues[2] |= 0x40; else rowInputValues[2] &= ~0x40;    // 'U'
            if (ecs_key_pressed == 32) rowInputValues[3] |= 0x01; else rowInputValues[3] &= ~0x01;    // 'V'
            if (ecs_key_pressed == 33) rowInputValues[4] |= 0x08; else rowInputValues[4] &= ~0x08;    // 'W'
            if (ecs_key_pressed == 34) rowInputValues[4] |= 0x01; else rowInputValues[4] &= ~0x01;    // 'X'
            if (ecs_key_pressed == 35) rowInputValues[2] |= 0x08; else rowInputValues[2] &= ~0x08;    // 'Y'
            if (ecs_key_pressed == 36) rowInputValues[4] |= 0x02; else rowInputValues[4] &= ~0x02;    // 'Z'
            if (ecs_key_pressed == 37) rowInputValues[0] |= 0x01; else rowInputValues[0] &= ~0x01;    // LEFT
            if (ecs_key_pressed == 38) rowInputValues[5] |= 0x02; else rowInputValues[5] &= ~0x02;    // DOWN
            if (ecs_key_pressed == 39) rowInputValues[5] |= 0x04; else rowInputValues[5] &= ~0x04;    // UP 
            if (ecs_key_pressed == 40) rowInputValues[5] |= 0x20; else rowInputValues[5] &= ~0x20;    // RIGHT
            if (ecs_key_pressed == 41) rowInputValues[5] |= 0x01; else rowInputValues[5] &= ~0x01;    // SPACE
            if (ecs_key_pressed == 42) rowInputValues[0] |= 0x40; else rowInputValues[0] &= ~0x40;    // RETURN (CR)
        }
    }
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
