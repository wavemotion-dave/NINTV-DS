
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
