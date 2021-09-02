
#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

class Memory
{

    public:
        virtual ~Memory() {}

        virtual void reset() = 0;
        virtual UINT16 getReadSize() = 0;
        virtual UINT16 getReadAddress() = 0;
        virtual UINT16 getReadAddressMask() = 0;
        virtual UINT16 peek(UINT16 location) = 0;

        virtual UINT16 getWriteSize() = 0;
        virtual UINT16 getWriteAddress() = 0;
        virtual UINT16 getWriteAddressMask() = 0;
        virtual void poke(UINT16 location, UINT16 value) = 0;

};

#endif
