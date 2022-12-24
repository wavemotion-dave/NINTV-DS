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
#include <nds.h> 

#include <stdio.h>
#include <string.h>
#include "ROMBanker.h"
#include "Emulator.h"

extern Emulator *currentEmu;

UINT16 gLastBankers[16] = {0};

ROMBanker::ROMBanker(ROM* r, UINT16 address, UINT16 tm, UINT16 t, UINT16 mm, UINT16 m)
: RAM(1, address, 0, 0xFFFF),
  rom(r),
  triggerMask(tm),
  trigger(t),
  matchMask(mm),
  match(m)
{
    pokeCount = 0;
}

void ROMBanker::reset()
{
    rom->SetEnabled(match == 0);
    memset(gLastBankers, 0x00, sizeof(gLastBankers));
    pokeCount = 0;    
}

ITCM_CODE void ROMBanker::poke(UINT16 address, UINT16 value)
{
    if ((value & triggerMask) == trigger)
    {
        bool bEnabled = ((value & matchMask) == match);
        if (bEnabled != rom->IsEnabled())
        {
            rom->SetEnabled(bEnabled);
            
            // -------------------------------------------------------------
            // We allow for the first few pokes to set things up. Most games
            // don't need this but Onion does and it's still unclear why...
            // -------------------------------------------------------------
            if (pokeCount >= 3)
            {
                if (bEnabled)
                {
                    UINT16 *fast_memory = (UINT16 *)0x06880000;
                    for (int i=(address&0xF000); i<=((address&0xF000)|0xFFF); i++)
                    {
                        fast_memory[i] = rom->peek_fast(i&0xFFF);
                    }
                }
            }
            else
            {
                pokeCount++;
                // ------------------------------------------------------------------
                // We must now patch up the fast memory ROM with the new swap in/out
                // This is the 'slow' way to ensure we get the bus peeks right...
                // ------------------------------------------------------------------
                UINT16 *fast_memory = (UINT16 *)0x06880000;
                for (int i=(address&0xF000); i<=((address&0xF000)|0xFFF); i++)
                {
                    fast_memory[i] = (bEnabled ? rom->peek_fast(i&0xFFF) : currentEmu->memoryBus.peek_slow(i));
                }
            }
        }
        gLastBankers[(address&0xF000)>>12] = value;
    }
}
