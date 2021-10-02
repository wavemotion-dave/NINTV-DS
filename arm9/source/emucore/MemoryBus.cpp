#include <stdio.h>
#include "MemoryBus.h"

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
} MyUnusedMemory;


MemoryBus::MemoryBus()
{
    UINT32 size = 1 << (sizeof(UINT16) << 3);
    UINT64 i;
    writeableMemoryCounts = new UINT8[size];
    memset(writeableMemoryCounts, 0, sizeof(UINT8) * size);
    writeableMemorySpace = new Memory**[size];
    for (i = 0; i < size; i++)
    {
        writeableMemorySpace[i] = new Memory*[MAX_OVERLAPPED_MEMORIES];
        for (int j=0; j<MAX_OVERLAPPED_MEMORIES; j++)
        {
            writeableMemorySpace[i][j] = &MyUnusedMemory;
        }
    }
    readableMemoryCounts = new UINT8[size];
    memset(readableMemoryCounts, 0, sizeof(UINT8) * size);
    readableMemorySpace = new Memory**[size];
    for (i = 0; i < size; i++)
    {
        readableMemorySpace[i] = new Memory*[MAX_OVERLAPPED_MEMORIES];
        for (int j=0; j<MAX_OVERLAPPED_MEMORIES; j++)
        {
            readableMemorySpace[i][j] = &MyUnusedMemory;
        }        
    }
    mappedMemoryCount = 0;
}

MemoryBus::~MemoryBus()
{
    UINT64 size = 1 << (sizeof(UINT16) << 3);
    UINT64 i;
    delete[] writeableMemoryCounts;
    for (i = 0; i < size; i++)
        delete[] writeableMemorySpace[i];
    delete[] writeableMemorySpace;
    delete[] readableMemoryCounts;
    for (i = 0; i < size; i++)
        delete[] readableMemorySpace[i];
    delete[] readableMemorySpace;
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

            for (UINT64 k = nextAddress; k <= nextEnd; k++) {
                UINT16 memCount = readableMemoryCounts[k];
                readableMemorySpace[k][memCount] = m;
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

            for (UINT64 k = nextAddress; k <= nextEnd; k++) {
                UINT16 memCount = writeableMemoryCounts[k];
                writeableMemorySpace[k][memCount] = m;
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

            for (UINT64 k = nextAddress; k <= nextEnd; k++) {
                UINT16 memCount = readableMemoryCounts[k];
                for (UINT16 n = 0; n < memCount; n++) {
                    if (readableMemorySpace[k][n] == m) {
                        for (INT32 l = n; l < (memCount-1); l++) {
                            readableMemorySpace[k][l] = readableMemorySpace[k][l+1];
                        }
                        readableMemorySpace[k][memCount-1] = &MyUnusedMemory;
                        readableMemoryCounts[k]--;
                        break;
                    }
                }
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

            for (UINT64 k = nextAddress; k <= nextEnd; k++) {
                UINT16 memCount = writeableMemoryCounts[k];
                for (UINT16 n = 0; n < memCount; n++) {
                    if (writeableMemorySpace[k][n] == m) {
                        for (INT32 l = n; l < (memCount-1); l++) {
                            writeableMemorySpace[k][l] = 
                                writeableMemorySpace[k][l+1];
                        }
                        writeableMemorySpace[k][memCount-1] = &MyUnusedMemory;
                        writeableMemoryCounts[k]--;
                        break;
                    }
                }
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

UINT16 MemoryBus::peek(UINT16 location)
{
    UINT8 numMemories = readableMemoryCounts[location];
    
    UINT16 value = 0xFFFF;
    for (UINT16 i = 0; i < numMemories; i++)
    {
        value &= readableMemorySpace[location][i]->peek(location);
    }
    return value;
}

void MemoryBus::poke(UINT16 location, UINT16 value)
{
    UINT8 numMemories = writeableMemoryCounts[location];

    for (UINT16 i = 0; i < numMemories; i++)
    {
        writeableMemorySpace[location][i]->poke(location, value);
    }
    
    if (location < 0x1000)
    {
        *((UINT16 *)0x06880000 + location) = value;
    }
}

