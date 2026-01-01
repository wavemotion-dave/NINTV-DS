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
    //$81 will reset the SP0256 or push an 16-bit value into the queue
    if (location & 1)
    {
        if (value & 0x0400) {
            ms->resetProcessor();
        }
        else if (ms->fifoSize < FIFO_MAX_SIZE) {
            fifoBytes[(ms->fifoHead+ms->fifoSize) & 0x3F] = value & 0x3FF;  // Write DECLE into FIFO
            ms->fifoSize++;
        } // else drop the write...
    }
    else //Any poke of any value into $80 means that the SP0256 should start speaking
    {
        if (ms->lrqHigh) // If we're already busy, drop this write...
        {
            ms->lrqHigh = FALSE;

            ms->command = value & 0xFF;

            if (!ms->speaking)
                ms->idle = FALSE;
        }
    }
}

ITCM_CODE UINT16 SP0256_Registers::peek(UINT16 location) 
{
    if (location & 1) return (ms->fifoSize == FIFO_MAX_SIZE ? 0x8000 : 0);
    return (ms->lrqHigh ? 0x8000 : 0);
}

