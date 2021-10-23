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

#ifndef MEMORYBUS_H
#define MEMORYBUS_H

#include <stdio.h>
#include <string.h>
#include "Memory.h"
#include "ROM.h"

#define MAX_MAPPED_MEMORIES     15
#define MAX_OVERLAPPED_MEMORIES 3

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

        inline UINT16 peek(UINT16 location) {if (readableMemoryCounts[location] == 1) return readableMemorySpace[location][0]->peek(location); else return peek_slow(location);}
        UINT16 peek_slow(UINT16 location);
        
        // Since PC fetched memory should be static and 16-bits... we pre-load into fast_memory[] for blazingly (relatively!) fast access...
        //inline UINT16 peek_pc(void) {return fast_memory[r[7]];}        
        inline UINT16 peek_pc(void) {return *((UINT16 *)0x06880000 + r[7]);}        
        
        void poke(UINT16 location, UINT16 value);

        UINT16 peek_slow_and_safe(UINT16 location) 
        {
           if (readableMemorySpace[location])
           {
               if (readableMemorySpace[location][0])
                  return readableMemorySpace[location][0]->peek(location);
               else
                  return 0xFFFF;
           } else return 0xFFFF;
        }
        
        void poke_slow_and_safe(UINT16 location, UINT16 value) 
        {
           if (writeableMemorySpace[location])
           {
               if (writeableMemorySpace[location][0])
                  writeableMemorySpace[location][0]->poke(location,value);
           }           
        }

        void addMemory(Memory* m);
        void removeMemory(Memory* m);
        void removeAll();

    private:
        Memory*     mappedMemories[MAX_MAPPED_MEMORIES];
        UINT16      mappedMemoryCount;
        UINT8*      writeableMemoryCounts;
        Memory***   writeableMemorySpace;
        UINT8*      readableMemoryCounts;
        Memory***   readableMemorySpace;
};

#endif
