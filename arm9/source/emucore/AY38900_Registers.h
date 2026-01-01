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

#ifndef AY38900_REGISTERS_H
#define AY38900_REGISTERS_H

#include "RAM.h"

class AY38900;

extern UINT16   stic_memory[0x40];

class AY38900_Registers : public RAM
{
    friend class AY38900;

    public:
        void reset();

        void poke(UINT16 location, UINT16 value);
        UINT16 peek(UINT16 location);
        void poke_cheat(UINT16 location, UINT16 value);

    private:
        AY38900_Registers();
        void init(AY38900* ay38900);

        AY38900* ay38900;
};

#endif
