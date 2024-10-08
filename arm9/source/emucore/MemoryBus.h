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

#ifndef MEMORYBUS_H
#define MEMORYBUS_H

#include <stdio.h>
#include <string.h>
#include "Memory.h"
#include "ROM.h"

#define MEM_DIV    4       // Divide by 16 which gives us 16 byte resolution on memory mapping - good enough and saves us a huge amount of RAM


/**
 * Emulates a 64K memory bus which may be composed of 8-bit or 16-bit memory units.
 *
 * @author Kyle Davis
 */
extern UINT16 r[8];

class MemoryBus
{
    public:
        MemoryBus();
        virtual ~MemoryBus();

        void reset();

        inline UINT16 peek(UINT16 location) {if (((UINT16 *) 0x06820000)[location] == 1) return readableMemorySpace[location>>MEM_DIV][0]->peek(location); else return peek_slow(location);}
        UINT16 peek_slow(UINT16 location);
        
        void poke(UINT16 location, UINT16 value);
        void poke_cheat(UINT16 location, UINT16 value);

        // ------------------------------------------------------
        // We use this to pre-fill the fast_memory[] buffer... 
        // ------------------------------------------------------
        UINT16 peek_slow_and_safe(UINT16 location) 
        {
           if (readableMemorySpace[location>>MEM_DIV])
           {
               if (readableMemorySpace[location>>MEM_DIV][0])
               {
                  return peek_slow(location);
               }
               else
               {
                  return 0xFFFF;
               }
           } else return 0xFFFF;
        }

        void addMemory(Memory* m);
        void removeMemory(Memory* m);
        void removeAll();
        UINT16 getMemCount() {return mappedMemoryCount;}

    private:
        Memory*     mappedMemories[MAX_MAPPED_MEMORIES];
        UINT16      mappedMemoryCount;
        Memory***   writeableMemorySpace;
        Memory***   readableMemorySpace;
        UINT16      *readableMemoryCounts;
        UINT8       *writeableMemoryCounts;
};

#endif
