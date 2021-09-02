 
#include <stdio.h>
#include <string.h>
#include "ROMBanker.h"

ROMBanker::ROMBanker(ROM* r, UINT16 address, UINT16 tm, UINT16 t, UINT16 mm, UINT16 m)
: RAM(1, address, 0, 0xFFFF),
  rom(r),
  triggerMask(tm),
  trigger(t),
  matchMask(mm),
  match(m)
{}

void ROMBanker::reset()
{
    rom->SetEnabled(match == 0);
}

void ROMBanker::poke(UINT16, UINT16 value)
{
    if ((value & triggerMask) == trigger)
        rom->SetEnabled((value & matchMask) == match);
}
