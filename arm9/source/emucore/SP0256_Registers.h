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

#ifndef MICROSEQUENCER_REGISTERS_H
#define MICROSEQUENCER_REGISTERS_H

#include "types.h"
#include "RAM.h"

class SP0256;

class SP0256_Registers : public RAM
{

    friend class SP0256;

    public:
        void reset() {}

        void poke(UINT16 location, UINT16 value);
        UINT16 peek(UINT16 location);

    private:
        SP0256_Registers();
        void init(SP0256* ms);
        SP0256*  ms;

};

#endif
