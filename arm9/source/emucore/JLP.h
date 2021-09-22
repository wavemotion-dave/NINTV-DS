#ifndef JLP_H
#define JLP_H

#include <string.h>
#include "RAM.h"

#define RAM_JLP_SIZE    (0x2000-0x40)

TYPEDEF_STRUCT_PACK( _JLPState
{
    UINT8     jlp_ram[RAM_JLP_SIZE];
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
