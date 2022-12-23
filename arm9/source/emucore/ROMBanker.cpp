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
 
#include <stdio.h>
#include <string.h>
#include "ROMBanker.h"

extern void PatchFastMemory(UINT16 address);

extern INT32 debug[];

ROMBanker::ROMBanker(ROM* r, UINT16 address, UINT16 tm, UINT16 t, UINT16 mm, UINT16 m)
: RAM(1, address, 0, 0xFFFF),
  rom(r),
  triggerMask(tm),
  trigger(t),
  matchMask(mm),
  match(m)
{
}

void ROMBanker::reset()
{
    rom->SetEnabled(match == 0);
    PatchFastMemory(trigger);
}

void ROMBanker::poke(UINT16 address, UINT16 value)
{
    if ((value & triggerMask) == trigger)
    {
        bool bEnabled = ((value & matchMask) == match);
        if (bEnabled != rom->IsEnabled())
        {
            debug[2]++;
            rom->SetEnabled(bEnabled);
            PatchFastMemory(address);
        }
    }
}
