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

#include "BackTabRAM.h"

BackTabRAM::BackTabRAM()
: RAM(BACKTAB_SIZE, BACKTAB_LOCATION, 0xFFFF, 0xFFFF)
{}

void BackTabRAM::reset()
{
    dirtyRAM = TRUE;
    colorAdvanceBitsDirty = TRUE;
    for (UINT16 i = 0; i < BACKTAB_SIZE; i++) {
        image[i] = 0;
        dirtyBytes[i] = TRUE;
    }
}

UINT16 BackTabRAM::peek(UINT16 location)
{
    return image[location-BACKTAB_LOCATION];
}

void BackTabRAM::poke(UINT16 location, UINT16 value)
{
    location -= BACKTAB_LOCATION;

    if (image[location] == value)
        return;

    if ((image[location] & 0x2000) != (value & 0x2000))
        colorAdvanceBitsDirty = TRUE;

    image[location] = value;
    dirtyBytes[location] = TRUE;
}

void BackTabRAM::markClean() {
    for (UINT16 i = 0; i < BACKTAB_SIZE; i++)
        dirtyBytes[i] = FALSE;
    colorAdvanceBitsDirty = FALSE;
}

BOOL BackTabRAM::areColorAdvanceBitsDirty() {
    return colorAdvanceBitsDirty;
}


BOOL BackTabRAM::isDirty(UINT16 location) {
    return dirtyBytes[location-BACKTAB_LOCATION];
}

BOOL BackTabRAM::isDirtyCache(UINT16 location) 
{
    bool ret = dirtyBytes[location];
    dirtyBytes[location] = FALSE;
    return ret;
}

void BackTabRAM::markCleanCache() 
{
    colorAdvanceBitsDirty = FALSE;
}


void BackTabRAM::getState(BackTabRAMState *state)
{
    for (int i=0; i<BACKTAB_SIZE; i++) state->image[i] = image[i];
    for (int i=0; i<BACKTAB_SIZE; i++) state->dirtyBytes[i] = dirtyBytes[i];
    state->dirtyRAM = dirtyRAM;
    state->colorAdvanceBitsDirty = colorAdvanceBitsDirty;
}

void BackTabRAM::setState(BackTabRAMState *state)
{
    for (int i=0; i<BACKTAB_SIZE; i++)  image[i] = state->image[i];
    for (int i=0; i<BACKTAB_SIZE; i++)  dirtyBytes[i] = state->dirtyBytes[i];
    dirtyRAM = state->dirtyRAM;
    colorAdvanceBitsDirty = state->colorAdvanceBitsDirty;
}

