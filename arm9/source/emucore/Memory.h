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

#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

#define MAX_MAPPED_MEMORIES  144
#define MAX_PERIPHERALS      4
#define MAX_COMPONENTS       4
#define MAX_ROMS             128

extern UINT16 MAX_OVERLAPPED_MEMORIES;
extern UINT32 MAX_ROM_FILE_SIZE;

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
        virtual void poke_cheat(UINT16 location, UINT16 value) = 0;
};

#endif
