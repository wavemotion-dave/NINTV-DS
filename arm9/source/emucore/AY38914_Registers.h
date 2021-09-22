
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
        UINT16   memory[0x0E];
        
    private:
        AY38914_Registers(UINT16 address);
        void init(AY38914* ay38914);

        AY38914* ay38914;
};

#endif
