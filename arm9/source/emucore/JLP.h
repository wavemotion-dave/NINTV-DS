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
#ifndef JLP_H
#define JLP_H

#include <string.h>
#include "RAM.h"

#define RAM_JLP_SIZE    (0x2000-0x40)

TYPEDEF_STRUCT_PACK( _JLPState
{
    UINT16     jlp_ram[RAM_JLP_SIZE];
} JLPState; )

class JLP : public RAM
{
    public:
        JLP();

        void reset();
        UINT16 peek(UINT16 location);
        void poke(UINT16 location, UINT16 value);
        
        void getState(JLPState *state);
        void setState(JLPState *state);
};

#endif
