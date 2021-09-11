
#ifndef GROM_H
#define GROM_H

#include "Memory.h"
#include "ROM.h"

#define GROM_SIZE       0x0800
#define GROM_ADDRESS    0x3000

class GROM : public ROM
{
    friend class AY38900;

    public:
        GROM();

        void reset();
        virtual UINT16 peek(UINT16 location) {return ROM::peek8(location);}     // GROM is 8-bits wide so force the call to get 8-bit value only

    private:
        BOOL         visible;
};

#endif
