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

#ifndef AY38914_REGISTERS_H
#define AY38914_REGISTERS_H

#include "RAM.h"

class AY38914;

class AY38914_Registers : public RAM
{
    friend class AY38914;

    public:
        void reset();
        void poke(UINT16 location, UINT16 value);
        UINT16 peek(UINT16 location);
        UINT16   psg_memory[0x0E];
        
    private:
        AY38914_Registers(UINT16 address);
        void init(AY38914* ay38914);

        AY38914* ay38914;
};

#endif
