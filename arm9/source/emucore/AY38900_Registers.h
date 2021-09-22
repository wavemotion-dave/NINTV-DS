
#ifndef AY38900_REGISTERS_H
#define AY38900_REGISTERS_H

#include "RAM.h"

class AY38900;

extern UINT16   memory[0x40];
class AY38900_Registers : public RAM
{
    friend class AY38900;

    public:
        void reset();

        void poke(UINT16 location, UINT16 value);
        UINT16 peek(UINT16 location);

    private:
        AY38900_Registers();
        void init(AY38900* ay38900);

        AY38900* ay38900;
};

#endif
