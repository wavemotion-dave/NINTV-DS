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

#include <stdio.h>
#include <string.h>
#include "ROMBanker.h"
#include "Emulator.h"

extern Emulator *currentEmu;

UINT16 gLastBankers[16] __attribute__((section(".dtcm"))) = {0};                  // We use this to know what the last enabled bank write value was for each 4K page. Needed when we restore a saved state.
UINT8  gBankerIsMappedHere[16][16] __attribute__((section(".dtcm"))) = {0};       // We use this matrix to know if a 4K page has banking logic. This lets us speed up page in/out processing and handle games where there is a non-paged bank overlapping with paged banks (which is OK so long as the paged banks are all flipped out)

ROMBanker::ROMBanker(ROM* r, UINT16 address, UINT16 tm, UINT16 t, UINT16 mm, UINT16 m)
: RAM(1, address, 0, 0xFFFF),
  rom(r),
  triggerMask(tm),
  trigger(t),
  matchMask(mm),
  match(m)
{
    // Keep track of ROM bankers
    gBankerIsMappedHere[address>>12][m] = 1;
    pokeCount = 0;
}

void ROMBanker::reset()
{
    rom->SetEnabled(match == 0);
    memset(gLastBankers, 0x00, sizeof(gLastBankers));
    pokeCount = 0;
}

// ----------------------------------------------------------------------------------------------
// From intvnut on AtariAge:
// Each 4K segment in memory can have up to 16 different 4K ROM pages mapped to it.
// After "reset", all switched segments much switch to page 0. To switch pages within
// a given 4K segment, write the value $xA5y to location $xFFF, where 'x' is the upper
// 4 bits of the range you want to switch, and 'y' is the page number to switch to.
// This will switch the ROM in segment $x000 - $xFFF to page 'y'.
// If there's no ROM mapped to that page within that segment, then to the Intellivision,
// it will look like there's no ROM there.
// ----------------------------------------------------------------------------------------------
ITCM_CODE void ROMBanker::poke(UINT16 address, UINT16 value)
{
    if ((value & triggerMask) == trigger)
    {
        bool bEnabled = ((value & matchMask) == match);
        if (bEnabled != rom->IsEnabled())
        {
            rom->SetEnabled(bEnabled);
            if (bEnabled)                                               // If we are enabling this bank, we can quickly refresh the fast_memory[]
            {
                UINT16 *fast_memory = (UINT16 *)0x06860000;
                UINT32 *ptr = (UINT32 *)rom->peek_image_address();
                
                UINT32 *dest = (UINT32 *) &fast_memory[(address&0xF000) + (rom->getReadAddress() & 0xFFF)];

                // Paddle Party has odd paging of a 2K bank at $4800 (virtually all other page-switching games use 4K banks)
                int blocks = (rom->getReadAddress() & 0x800) ? 128:256;
                for (int i=0; i<blocks; i++)
                {
                    // Partially unroll loop for a tiny speedup and 32-bits at a time for max speed
                    *dest++ = *ptr++;  *dest++ = *ptr++; *dest++ = *ptr++; *dest++ = *ptr++;
                    *dest++ = *ptr++;  *dest++ = *ptr++; *dest++ = *ptr++; *dest++ = *ptr++;
                }
            }
            else if (gBankerIsMappedHere[address>>12][value&0xF] == 0)   // Nothing is here... nothing will be swapped in... We need to check the main memory as there may be an unswapped bank in there...
            {
                UINT16 *fast_memory = (UINT16 *)0x06860000;
                for (int i=(address&0xF000)  + (rom->getReadAddress() & 0xFFF); i<=((address&0xF000)|((rom->getReadAddress() & 0x800) ? 0x7FF:0xFFF)); i++)
                {
                    fast_memory[i] = currentEmu->memoryBus.peek_slow(i);
                }
            }
        }

        gLastBankers[(address&0xF000)>>12] = value;
    }
}
