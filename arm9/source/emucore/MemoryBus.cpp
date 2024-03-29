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

#include <nds.h>
#include <stdio.h>
#include "MemoryBus.h"
#include "../nintv-ds.h"

UINT16 MAX_READ_OVERLAPPED_MEMORIES = 2;
UINT16 MAX_WRITE_OVERLAPPED_MEMORIES = 3;

// ----------------------------------------------------------------------------------------------
// We use this class and single object to fill all unused memory locations in the memory map.
// Returns 0xFFFF on all access as a real intellivision would with unused memory regions.
// ----------------------------------------------------------------------------------------------
class UnusedMemory : public Memory
{
public:
    UnusedMemory() {};
    virtual ~UnusedMemory() {}
    virtual void reset() {}
    UINT8  getByteWidth() {return 2;}
    UINT16 getReadSize() {return 2;}
    UINT16 getReadAddress() {return 0;}
    UINT16 getReadAddressMask() {return 0xFFFF;}
    inline virtual UINT16 peek(UINT16 location) {return 0xFFFF;}
    UINT16 getWriteSize() {return 2;}
    UINT16 getWriteAddress() {return 0;}
    UINT16 getWriteAddressMask() {return 0xFFFF;}
    virtual void poke(UINT16 location, UINT16 value) {}
    virtual void poke_cheat(UINT16 location, UINT16 value) {return;}
} MyUnusedMemory;


// -------------------------------------------------------------------------------
// This is a serious resource hog... it's multiple 16-bit 64k arrays take
// up a significant bit of our RAM... the max overlapped memories is what
// soaks up quite a bit of main RAM. We limit this for older DS hardware
// memories per address location which is sufficient provided we are only
// loading normal ROMs into a stock intellivision with, at most, an intellivoice
// or the JLP cart as the only peripherals... still, this is a strain on the
// older DS-LITE/PHAT.  The original BLISS core allowed 16 overlapping memory
// regions (to handle page flipping) which will fit into the DSi but is too
// large for the original DS-LITE/PHAT so for older hardware, we strip down
// to the bare essentials. For the DSi we can allocate more memory and provide
// the full 16 overlapped mapped memories.
// -------------------------------------------------------------------------------
UINT32 *overlappedMemoryPool = NULL;

MemoryBus::MemoryBus()
{
    // -------------------------------------------------------------------------------------
    // We swap in a larger memory model for the DSi to handle really complex page flipping
    // -------------------------------------------------------------------------------------
    if (isDSiMode())
    {
        MAX_READ_OVERLAPPED_MEMORIES       = 16;        // Good enough for any page-flipping game. This is massive!
        MAX_WRITE_OVERLAPPED_MEMORIES      = 17;        // Need one extra here to handle the GRAM mirrors up in odd places in ROM
    }
    else
    {
        MAX_READ_OVERLAPPED_MEMORIES       = 16;        // Good enough for almost all games except very large page-flipping games
        MAX_WRITE_OVERLAPPED_MEMORIES      = 17;        // Need one extra here to handle the GRAM mirrors up in odd places in ROM
    }

    UINT32 size = 1 << (sizeof(UINT16) << 3);
    UINT32 i;
    writeableMemoryCounts = new UINT8[size];
    memset(writeableMemoryCounts, 0, sizeof(UINT8) * size);
    writeableMemorySpace = new Memory**[size>>4];
    
    // ---------------------------------------------------------------------------------------------------------------------------
    // We do this rather than allocate piecemeal so we avoid malloc overhead and extra bytes padded (saves almost 500K on DS)
    // ---------------------------------------------------------------------------------------------------------------------------
    overlappedMemoryPool = new UINT32[(size*(MAX_READ_OVERLAPPED_MEMORIES+MAX_WRITE_OVERLAPPED_MEMORIES))>>4];
    UINT32 *memPoolPtr = (UINT32 *)overlappedMemoryPool;

    for (i = 0; i < size>>4; i++)
    {
        writeableMemorySpace[i] = (Memory **)memPoolPtr;
        for (int j=0; j<MAX_WRITE_OVERLAPPED_MEMORIES; j++)
        {
            memPoolPtr++;
            writeableMemorySpace[i][j] = &MyUnusedMemory;
        }
    }
    readableMemoryCounts = (UINT16 *) 0x06820000; // Use video memory ... slightly faster and saves main RAM
    memset(readableMemoryCounts, 0, sizeof(UINT16) * size);
    readableMemorySpace = new Memory**[size>>4];
    for (i = 0; i < size>>4; i++)
    {
        readableMemorySpace[i] = (Memory **)memPoolPtr;
        for (int j=0; j<MAX_READ_OVERLAPPED_MEMORIES; j++)
        {
            memPoolPtr++;
            readableMemorySpace[i][j] = &MyUnusedMemory;
        }
    }
    mappedMemoryCount = 0;
}

MemoryBus::~MemoryBus()
{
    delete[] writeableMemoryCounts;
    delete[] writeableMemorySpace;
    delete[] readableMemorySpace;
    delete[] overlappedMemoryPool;
}

void MemoryBus::reset()
{
    for (UINT8 i = 0; i < mappedMemoryCount; i++)
        mappedMemories[i]->reset();
}

void MemoryBus::addMemory(Memory* m)
{
    UINT8 bitCount = sizeof(UINT16)<<3;
    UINT8 bitShifts[sizeof(UINT16)<<3];
    UINT8 i;

    //get the important info
    UINT16 readSize = m->getReadSize();
    UINT16 readAddress = m->getReadAddress();
    UINT16 readAddressMask = m->getReadAddressMask();
    UINT16 writeSize = m->getWriteSize();
    UINT16 writeAddress = m->getWriteAddress();
    UINT16 writeAddressMask = m->getWriteAddressMask();

    if (mappedMemoryCount >= MAX_MAPPED_MEMORIES)
    {
        FatalError("GAME TOO COMPLEX - MAX MEMORIES");
        return;
    }

    //add all of the readable locations, if any
    if (readAddressMask != 0) {
        UINT8 zeroCount = 0;
        for (i = 0; i < bitCount; i++) {
            if (!(readAddressMask & (1<<i))) {
                bitShifts[zeroCount] = (i-zeroCount);
                zeroCount++;
            }
        }

        UINT8 combinationCount = (1<<zeroCount);
        for (i = 0; i < combinationCount; i++) {
            UINT16 orMask = 0;
            for (UINT8 j = 0; j < zeroCount; j++)
                orMask |= (i & (1<<j)) << bitShifts[j];
            UINT16 nextAddress = readAddress | orMask;
            UINT16 nextEnd = nextAddress + readSize - 1;

            for (UINT32 k = nextAddress; k <= nextEnd; k++) {
                UINT16 memCount = readableMemoryCounts[k];
                if (memCount >= MAX_READ_OVERLAPPED_MEMORIES)
                {
                    FatalError("ERROR MAX READABLE MEM OVERLAP");
                    return;
                }
                readableMemorySpace[k>>4][memCount] = m;
                readableMemoryCounts[k]++;
            }
        }
    }

    //add all of the writeable locations, if any
    if (writeAddressMask != 0) {
        UINT8 zeroCount = 0;
        for (i = 0; i < bitCount; i++) {
            if (!(writeAddressMask & (1<<i))) {
                bitShifts[zeroCount] = (i-zeroCount);
                zeroCount++;
            }
        }

        UINT8 combinationCount = (1<<zeroCount);
        for (i = 0; i < combinationCount; i++) {
            UINT16 orMask = 0;
            for (UINT8 j = 0; j < zeroCount; j++)
                orMask |= (i & (1<<j)) << bitShifts[j];
            UINT16 nextAddress = writeAddress | orMask;
            UINT16 nextEnd = nextAddress + writeSize - 1;

            for (UINT32 k = nextAddress; k <= nextEnd; k++) {
                UINT16 memCount = writeableMemoryCounts[k];
                if (memCount >= MAX_WRITE_OVERLAPPED_MEMORIES)
                {
                    FatalError("ERROR MAX WRITEABLE MEM OVERLAP");
                    return;
                }
                writeableMemorySpace[k>>4][memCount] = m;
                writeableMemoryCounts[k]++;
            }
        }
    }

    //add it to our list of memories
    mappedMemories[mappedMemoryCount] = m;
    mappedMemoryCount++;
}

void MemoryBus::removeMemory(Memory* m)
{
    UINT8 bitCount = sizeof(UINT16)<<3;
    UINT8 bitShifts[sizeof(UINT16)<<3];
    UINT32 i;

    //get the important info
    UINT16 readSize = m->getReadSize();
    UINT16 readAddress = m->getReadAddress();
    UINT16 readAddressMask = m->getReadAddressMask();
    UINT16 writeSize = m->getWriteSize();
    UINT16 writeAddress = m->getWriteAddress();
    UINT16 writeAddressMask = m->getWriteAddressMask();

    //add all of the readable locations, if any
    if (readAddressMask != 0) {
        UINT8 zeroCount = 0;
        for (i = 0; i < bitCount; i++) {
            if (!(readAddressMask & (1<<i))) {
                bitShifts[zeroCount] = (UINT8)(i-zeroCount);
                zeroCount++;
            }
        }

        UINT8 combinationCount = (1<<zeroCount);
        for (i = 0; i < combinationCount; i++) {
            UINT16 orMask = 0;
            for (UINT8 j = 0; j < zeroCount; j++)
                orMask |= (i & (1<<j)) << bitShifts[j];
            UINT16 nextAddress = readAddress | orMask;
            UINT16 nextEnd = nextAddress + readSize - 1;

            for (UINT32 k = nextAddress; k <= nextEnd; k++) 
            {
                UINT16 memCount = readableMemoryCounts[k];
                for (UINT16 n = 0; n < memCount; n++) 
                {
                    if (readableMemorySpace[k>>4][n] == m) 
                    {
                        for (INT32 l = n; l < (memCount-1); l++) 
                        {
                            readableMemorySpace[k>>4][l] = readableMemorySpace[k>>4][l+1];
                        }
                        readableMemorySpace[k>>4][memCount-1] = &MyUnusedMemory;
                        break;
                    }
                }
               readableMemoryCounts[k]--;
            }
        }
    }

    //add all of the writeable locations, if any
    if (writeAddressMask != 0) {
        UINT8 zeroCount = 0;
        for (i = 0; i < bitCount; i++) {
            if (!(writeAddressMask & (1<<i))) {
                bitShifts[zeroCount] = (UINT8)(i-zeroCount);
                zeroCount++;
            }
        }

        UINT8 combinationCount = (1<<zeroCount);
         for (i = 0; i < combinationCount; i++) {
            UINT16 orMask = 0;
            for (UINT8 j = 0; j < zeroCount; j++)
                orMask |= (i & (1<<j)) << bitShifts[j];
            UINT16 nextAddress = writeAddress | orMask;
            UINT16 nextEnd = nextAddress + writeSize - 1;

            for (UINT32 k = nextAddress; k <= nextEnd; k++) {
                UINT16 memCount = writeableMemoryCounts[k];
                for (UINT16 n = 0; n < memCount; n++) 
                {
                    if (writeableMemorySpace[k>>4][n] == m) 
                    {
                        for (INT32 l = n; l < (memCount-1); l++) 
                        {
                            writeableMemorySpace[k>>4][l] = writeableMemorySpace[k>>4][l+1];
                        }
                        writeableMemorySpace[k>>4][memCount-1] = &MyUnusedMemory;
                        break;
                    }
                }
                writeableMemoryCounts[k]--;
            }
        }
    }

    //remove it from our list of memories
    for (i = 0; i < mappedMemoryCount; i++) {
        if (mappedMemories[i] == m) {
            for (UINT32 j = i; j < (UINT32)(mappedMemoryCount-1); j++)
                mappedMemories[j] = mappedMemories[j+1];
            mappedMemoryCount--;
            return;
        }
    }
}

void MemoryBus::removeAll()
{
    while (mappedMemoryCount)
        removeMemory(mappedMemories[0]);
}

// ------------------------------------------------------------------------------------------------------
// This only needs to be called if we are in a region that might have multiple things mapped to it...
// Most of the PC ROM access will go through the normal peek() handler which is significantly faster...
// ------------------------------------------------------------------------------------------------------
ITCM_CODE UINT16 MemoryBus::peek_slow(UINT16 location)
{
    UINT16 numMemories = readableMemoryCounts[location];

    UINT16 value = 0xFFFF;
    for (UINT16 i = 0; i < numMemories; i++)
    {
        value &= readableMemorySpace[location>>4][i]->peek(location);
    }
    return value;
}

// ---------------------------------------------------------------------------------------
// Poke is less common than peek... so we're less concerned about optimization here.
// ---------------------------------------------------------------------------------------
ITCM_CODE void MemoryBus::poke(UINT16 location, UINT16 value)
{
    UINT8 numMemories = writeableMemoryCounts[location];

    for (UINT16 i = 0; i < numMemories; i++)
    {
        writeableMemorySpace[location>>4][i]->poke(location, value);
    }

    // For the lower RAM area... keep the "fast memory" updated
    if (!(location & 0xF800))
    {
        *((UINT16 *)(0x06860000 | (location<<1))) = value;
    }
}


// ---------------------------------------------------------------------------------------
// Poke Cheat Codes does not need any optimization - only happens once after ROM load.
// We allow poke to both readable and writable memory spaces - most of the time we are
// modifying a ROM location to provide some special cheat effect.  We don't need to
// update the "fast memory" as the cheats are applied post ROM load but pre "fast buffer".
// ---------------------------------------------------------------------------------------
void MemoryBus::poke_cheat(UINT16 location, UINT16 value)
{
    UINT8 numMemories = readableMemoryCounts[location];

    for (UINT16 i = 0; i < numMemories; i++)
    {
        readableMemorySpace[location>>4][i]->poke_cheat(location, value);
    }

    numMemories = writeableMemoryCounts[location];

    for (UINT16 i = 0; i < numMemories; i++)
    {
        writeableMemorySpace[location>>4][i]->poke_cheat(location, value);
    }
}
