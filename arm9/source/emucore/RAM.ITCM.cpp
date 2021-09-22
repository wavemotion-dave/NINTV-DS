#include <nds.h>
#include "RAM.h"
#include "GRAM.h"
#include "JLP.h"

UINT16 fast_ram[4096] __attribute__((section(".dtcm")));
UINT16 fast_ram_idx = 0;
UINT16 jlp_ram[8192] = {0};

RAM::RAM(UINT16 size, UINT16 location, UINT16 readAddressMask, UINT16 writeAddressMask, UINT8 bitWidth)
: enabled(TRUE)
{
    this->size = size;
    this->location = location;
    this->readAddressMask = readAddressMask;
    this->writeAddressMask = writeAddressMask;
    this->bitWidth = bitWidth;
    this->trimmer = (UINT16)((1 << bitWidth) - 1);
    if (size == RAM_JLP_SIZE)
    {
        image = (UINT16*)jlp_ram;       // Special 8K words of 16-bit RAM
    }
    else if (location == GRAM_ADDRESS)
    {
        image = (UINT16*)gram_image;    // Special fixed fast 8-bit RAM
    }
    else
    {
        image = &fast_ram[fast_ram_idx]; // Otherwise "allocate" it from the internal fast buffer... should never run out as most games don't use much extra RAM
        fast_ram_idx += size;
    }
}

RAM::~RAM()
{
    
}

void RAM::reset()
{
    enabled = TRUE;
    for (UINT16 i = 0; i < size; i++)
        image[i] = 0;
}

void RAM::SetEnabled(BOOL b)
{
    enabled = b;
}

UINT8 RAM::getBitWidth()
{
    return bitWidth;
}

UINT16 RAM::getReadSize()
{
    return size;
}

UINT16 RAM::getReadAddress()
{
    return location;
}

UINT16 RAM::getReadAddressMask()
{
    return readAddressMask;
}

UINT16 RAM::getWriteSize()
{
    return size;
}

UINT16 RAM::getWriteAddress()
{
    return location;
}

UINT16 RAM::getWriteAddressMask()
{
    return writeAddressMask;
}

UINT16 RAM::peek(UINT16 location)
{
    return image[(location&readAddressMask) - this->location];
}

void RAM::poke(UINT16 location, UINT16 value)
{
    image[(location&writeAddressMask)-this->location] = (value & trimmer);
}


void RAM::getState(RAMState *state)
{
    state->enabled = enabled;
    state->bitWidth = bitWidth;
    state->size = size;
    state->location = location;
    state->readAddressMask = readAddressMask;
    state->writeAddressMask = writeAddressMask;
    state->trimmer = trimmer;
    for (int i=0; i<size; i++) state->image[i] = image[i];
}

void RAM::setState(RAMState *state)
{
    enabled = state->enabled;
    bitWidth = state->bitWidth;
    size = state->size;
    location = state->location;
    readAddressMask = state->readAddressMask;
    writeAddressMask = state->writeAddressMask;
    trimmer = state->trimmer;
    for (int i=0; i<size; i++) image[i] = state->image[i];
}

