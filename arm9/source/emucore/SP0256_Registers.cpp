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

#include <nds.h>
#include "SP0256.h"
#include "SP0256_Registers.h"

SP0256_Registers::SP0256_Registers()
: RAM(2, 0x0080, 0xFFFF, 0xFFFF)
{}

void SP0256_Registers::init(SP0256* ms)
{
    this->ms = ms;
}

ITCM_CODE void SP0256_Registers::poke(UINT16 location, UINT16 value)
{
    switch(location) {
        //a poke of any value into $80 means that the SP0256 should
        //start speaking
        case 0x0080:
            if (ms->lrqHigh) {
                ms->lrqHigh = FALSE;

                ms->command = value & 0xFF;

                if (!ms->speaking)
                    ms->idle = FALSE;
            }
            break;
        //$81 will reset the SP0256 or push an 8-bit value into the queue
        case 0x0081:
            if (value & 0x0400) {
                ms->resetProcessor();
            }
            else if (ms->fifoSize < FIFO_MAX_SIZE) {
                fifoBytes[(ms->fifoHead+ms->fifoSize) & 0x3F] = value;
                ms->fifoSize++;
            }
            break;
    }
}

ITCM_CODE UINT16 SP0256_Registers::peek(UINT16 location) {
    switch(location) {
        case 0x0080:
            return (ms->lrqHigh ? 0x8000 : 0);
        case 0x0081:
        default:
            return (ms->fifoSize == FIFO_MAX_SIZE ? 0x8000 : 0);
    }
}

