// =====================================================================================
// Copyright (c) 2021-2023 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================
 
#ifndef ROMBANKER_H
#define ROMBANKER_H

#include "RAM.h"
#include "ROM.h"
#include "types.h"

class ROMBanker : public RAM
{
public:
    ROMBanker(ROM* rom, UINT16 address, UINT16 triggerMask, UINT16 trigger, UINT16 matchMask, UINT16 match);

    virtual void reset();
    virtual void poke(UINT16 location, UINT16 value);

private:
    ROM*     rom;
    UINT16   triggerMask;
    UINT16   trigger;
    UINT16   matchMask;
    UINT16   match;
    UINT16   pokeCount;
};

extern UINT16 gLastBankers[16];

#endif
