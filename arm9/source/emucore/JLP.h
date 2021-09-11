#ifndef JLP_H
#define JLP_H

#include <string.h>
#include "RAM.h"

#define RAM_JLP_SIZE    (0x2000-0x40-1)

class JLP : public RAM
{
    public:
        JLP();

        void reset();
        UINT16 peek(UINT16 location);
        void poke(UINT16 location, UINT16 value);

    private:
        UINT16  image[8192];
};

#endif
