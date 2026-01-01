// =====================================================================================
// Copyright (c) 2021-2026 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#include "ECSKeyboard.h"
#include "HandController.h"

UINT8 ecs_key_pressed = 0;
UINT8 ecs_shift_key   = 0;
UINT8 ecs_ctrl_key    = 0;

ECSKeyboard::ECSKeyboard(INT32 id)
: InputConsumer(id),
  directionIO(0),
  rowsOrColumnsToScan(0)
{
    memset(rowInputValues, 0, sizeof(rowInputValues));
}

ECSKeyboard::~ECSKeyboard()
{
}

void ECSKeyboard::resetInputConsumer()
{
    rowsOrColumnsToScan = 0;
    memset(rowInputValues, 0, sizeof(rowInputValues));
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

// The music synth is much simpler... the lowest key on the synthesizer
// is left arrow (row 0, lowest bit). The highest key on the synth is
// row 6, lowest bit. This gives a total of 49 keys. Our emulation only
// handles a subset of these notes in the middle of the keyboard and
// we map the virtual synth overlay to the corresponding ECS keyboard
// key for simplicity. There isn't much software that uses the music
// synth - the only commercial game was Melody Blaster.
  
void ECSKeyboard::evaluateInputs()
{
    extern UINT8 bUseECS;
    
    memset(rowInputValues, 0, sizeof(rowInputValues));
    
    // If a key on the ECS Keyboard was pressed...
    switch (ecs_key_pressed)
    {
        case (1) : rowInputValues[5] |= 0x10; break;    // '1'
        case (2) : rowInputValues[4] |= 0x20; break;    // '2'
        case (3) : rowInputValues[4] |= 0x10; break;    // '3'
        case (4) : rowInputValues[3] |= 0x20; break;    // '4'
        case (5) : rowInputValues[3] |= 0x10; break;    // '5'
        case (6) : rowInputValues[2] |= 0x20; break;    // '6'    
        case (7) : rowInputValues[2] |= 0x10; break;    // '7'
        case (8) : rowInputValues[1] |= 0x20; break;    // '8'
        case (9) : rowInputValues[1] |= 0x10; break;    // '9'
        case (10): rowInputValues[0] |= 0x20; break;    // '0'
        case (11): rowInputValues[5] |= 0x80; break;    // 'A'
        case (12): rowInputValues[2] |= 0x02; break;    // 'B'            
        case (13): rowInputValues[3] |= 0x02; break;    // 'C'
        case (14): rowInputValues[4] |= 0x80; break;    // 'D'
        case (15): rowInputValues[4] |= 0x40; break;    // 'E'
        case (16): rowInputValues[3] |= 0x04; break;    // 'F'
        case (17): rowInputValues[3] |= 0x80; break;    // 'G'            
        case (18): rowInputValues[2] |= 0x04; break;    // 'H'
        case (19): rowInputValues[1] |= 0x08; break;    // 'I'
        case (20): rowInputValues[2] |= 0x80; break;    // 'J'
        case (21): rowInputValues[1] |= 0x04; break;    // 'K'
        case (22): rowInputValues[1] |= 0x80; break;    // 'L'
        case (23): rowInputValues[1] |= 0x02; break;    // 'M'
        case (24): rowInputValues[2] |= 0x01; break;    // 'N'
        case (25): rowInputValues[1] |= 0x40; break;    // 'O'
        case (26): rowInputValues[0] |= 0x08; break;    // 'P'
        case (27): rowInputValues[5] |= 0x08; break;    // 'Q'
        case (28): rowInputValues[3] |= 0x08; break;    // 'R'
        case (29): rowInputValues[4] |= 0x04; break;    // 'S'
        case (30): rowInputValues[3] |= 0x40; break;    // 'T'
        case (31): rowInputValues[2] |= 0x40; break;    // 'U'
        case (32): rowInputValues[3] |= 0x01; break;    // 'V'
        case (33): rowInputValues[4] |= 0x08; break;    // 'W'
        case (34): rowInputValues[4] |= 0x01; break;    // 'X'
        case (35): rowInputValues[2] |= 0x08; break;    // 'Y'
        case (36): rowInputValues[4] |= 0x02; break;    // 'Z'
        case (37): rowInputValues[0] |= 0x01; break;    // LEFT
        case (38): rowInputValues[5] |= 0x02; break;    // DOWN
        case (39): rowInputValues[5] |= 0x04; break;    // UP 
        case (40): rowInputValues[5] |= 0x20; break;    // RIGHT
        case (41): rowInputValues[5] |= 0x01; break;    // SPACE
        case (42): rowInputValues[0] |= 0x40; break;    // RETURN (CR)
        case (43): rowInputValues[0] |= 0x10; break;    // ESC key
        case (44): rowInputValues[0] |= 0x04; break;    // Semicolon
        case (45): rowInputValues[1] |= 0x01; break;    // Comma
        case (46): rowInputValues[0] |= 0x02; break;    // Period
    }
    
    // Shift and Control are special modifier keys...
    if (ecs_ctrl_key)  rowInputValues[5] |= 0x40;   // Ctrl key
    if (ecs_shift_key) rowInputValues[6] |= 0x80;   // Shift key        
}


// -------------------------------------------------------------------------------------------------
// There are two ways to read the keyboard from software... it can be read in normal key scanning
// fashion (row/column... write on 0xE and read on 0xF) or it can be 'transposed' which is done
// as column/row (write on 0xF and read on 0xE). Sometimes clever software will do both kinds of
// reads to try and eliminate key ghosting... we handle both flavors below to ensure compatibility.
// -------------------------------------------------------------------------------------------------
UINT16 ECSKeyboard::getInputValue(UINT8 port)
{
    UINT16 inputValue = 0;
    
    if (port == 0x0F)
    {
        if (directionIO & 0x80) return rowsOrColumnsToScan; // Reading the output port... just return the last value written
        if ((directionIO & 0xC0) == 0x40)  // Normal key scanning row/column (Write on 0xE and read on 0xF)
        {
            UINT16 rowMask = 1;
            for (UINT16 row = 0; row < 8; row++)  
            {
                if ((rowsOrColumnsToScan & rowMask) == 0) 
                {
                    inputValue |= rowInputValues[row];
                }
                rowMask = (UINT16)(rowMask << 1);
            }
        }
        // else just fall through and return no input below...
    }
    else
    if (port == 0x0E)
    {
        if (directionIO & 0x40) return rowsOrColumnsToScan; // Reading the output port... just return the last value written
        if ((directionIO & 0xC0) == 0x80)  // Transposed key scanning column/row (Write on 0xF and read on 0xE)
        {
            UINT16 colMask = 1;
            for (UINT16 col = 0; col < 8; col++)  
            {
                if ((rowsOrColumnsToScan & colMask) == 0) 
                {
                    for (UINT16 i=0; i<8; i++)
                    {
                        if (rowInputValues[i] & (1<<col)) inputValue |= (1<<i);
                    }
                }
                colMask = (UINT16)(colMask << 1);
            }
        }
        // else just fall through and return no input below...
    }
    
    return (UINT16)(0xFF ^ inputValue);
}

void ECSKeyboard::setOutputValue(UINT16 value) 
{
    this->rowsOrColumnsToScan = value & 0xFF;
}

void ECSKeyboard::setDirectionIO(UINT16 value)
{
    this->directionIO = value & 0xFF;
}
