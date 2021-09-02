 
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

};

#endif
