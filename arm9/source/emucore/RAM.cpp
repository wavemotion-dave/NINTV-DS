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
#include "../nintv-ds.h"
#include "Intellivision.h"
#include "RAM.h"
#include "GRAM.h"
#include "JLP.h"

// 16-bit bus so even 8-bit RAM chunks are declared as 16-bit arrays
UINT16 inty_16bit_ram[RAM16BIT_SIZE]     __attribute__((section(".dtcm"))) = {0};
UINT16 inty_8bit_ram[RAM8BIT_SIZE]       __attribute__((section(".dtcm"))) = {0};
UINT16 fast_ram16[0x400]                 __attribute__((section(".dtcm"))) = {0};
UINT16 fast_ram16_idx = 0;
UINT16 slow_ram16_idx = 0;
UINT16 slow_ram8_idx = 0;

UINT16 *slow_ram8  = (UINT16*)0x06890000;   // 6K of 8-bit RAM here (utilizing 12K of VRAM) - used for carts that define extra 8-bit RAM like Chess and Land Battle. We allow this to expand into the 2K section below.
UINT16 *ecs_ram8   = (UINT16*)0x06893000;   // 2K of 8-bit ECS RAM (utilizing 4K of VRAM) for the ECS computer peripheral.  We do allow the 6K RAM to expand into this area... so non-ECS games can get 8K of 8-bit ram allocated.
UINT16 *jlp_ram    = (UINT16*)0x06894000;   // 8K Words (16K Bytes) of JLP ram into the semi-fast VRAM buffer
UINT16 *slow_ram16 = (UINT16*)0x06898000;   // 16K Words (32K Bytes) of slower RAM for the one-off carts that need it... mostly demos and such...

RAM::RAM(UINT16 size, UINT16 location, UINT16 readAddressMask, UINT16 writeAddressMask, UINT8 bitWidth)
: enabled(TRUE)
{
    this->size = size;
    this->location = location;
    this->readAddressMask = readAddressMask;
    this->writeAddressMask = writeAddressMask;
    this->bitWidth = bitWidth;
    this->trimmer = (UINT16)((1 << bitWidth) - 1);
    if ((location == JLP_RAM_ADDRESS) && (size == JLP_RAM_SIZE))
    {
        image = (UINT16*)jlp_ram;       // Special 8K words of 16-bit RAM
        for (UINT16 i=0; i<JLP_RAM_SIZE; i++) jlp_ram[i] = 0x00;
    }
    else if ((location == GRAM_ADDRESS) && (size == GRAM_SIZE))
    {
        image = (UINT16*)gram_image;    // Special fixed fast 8-bit RAM
        for (UINT16 i=0; i<GRAM_SIZE; i++) gram_image[i] = 0x00;
    }
    else if ((location == RAM16BIT_LOCATION) && (size == RAM16BIT_SIZE))
    {
        image = (UINT16*)inty_16bit_ram; // Internal 16-bit Intellivision RAM
        for (UINT16 i=0; i<RAM16BIT_SIZE; i++) inty_16bit_ram[i] = 0x00;
    }
    else if ((location == RAM8BIT_LOCATION) && (size == RAM8BIT_SIZE))
    {
        image = (UINT16*)inty_8bit_ram;  // Internal 8-bit Intellivision RAM
        for (UINT16 i=0; i<RAM8BIT_SIZE; i++) inty_8bit_ram[i] = 0x00;
    }
    else if ((bitWidth == 8) && (location == ECS_RAM_LOCATION) && (size == ECS_RAM_SIZE))
    {
        image = (UINT16*)ecs_ram8;      // Use this dedicated area for ECS 8-bit RAM
        for (UINT16 i=0; i<ECS_RAM_SIZE; i++) ecs_ram8[i] = 0x00;
    }
    else if (bitWidth == 8) // Is this 8-bit RAM being requested?
    {
        image = (UINT16*)&slow_ram8[slow_ram8_idx];   // Slow RAM for one-off carts that declare a lot of extra RAM (mostly demos)
        slow_ram8_idx += size;
        if (slow_ram8_idx > 0x2000) FatalError("OUT OF SLOW 8-BIT MEMORY");
        else for (UINT16 i=0; i<size; i++) image[i] = 0x00;
    }
    else if (size >= 0x100) // And for any game needing 256 bytes/words or more of extra RAM we use the slow ram16 buffer
    {
        image = (UINT16*)&slow_ram16[slow_ram16_idx];   // Slow RAM for one-off carts that declare a lot of extra RAM (mostly demos)
        slow_ram16_idx += size;
        if (slow_ram16_idx > 0x4000) FatalError("OUT OF SLOW 16-BIT MEMORY");
        else for (UINT16 i=0; i<size; i++) image[i] = 0x00;
    }
    else
    {
        image = &fast_ram16[fast_ram16_idx]; // Otherwise it's a small pool being requested... "allocate" it from the internal fast buffer... should never run out since this will just be internal Intellivision RAM
        fast_ram16_idx += size;
        if (fast_ram16_idx > sizeof(fast_ram16)) FatalError("OUT OF FAST MEMORY");
        else for (UINT16 i=0; i<size; i++) image[i] = 0x00;
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

ITCM_CODE void RAM::poke(UINT16 location, UINT16 value)
{
    image[(location&writeAddressMask)-this->location] = (value & trimmer);
}

void RAM::poke_cheat(UINT16 location, UINT16 value)
{
    return;
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

