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
#include "BackTabRAM.h"
#include "../config.h"

// ---------------------------------------------------------------------------
// We access the backtab ram often enough that it's worth putting these into
// the .DTCM fast memory... each array takes up 240 bytes.
// ---------------------------------------------------------------------------
UINT16  bt_image[BACKTAB_SIZE]             __attribute__((section(".dtcm")));
UINT16  bt_imageLatched[BACKTAB_SIZE]      __attribute__((section(".dtcm")));
UINT8   dirtyBytes[BACKTAB_SIZE]           __attribute__((section(".dtcm")));

BackTabRAM::BackTabRAM()
: RAM(BACKTAB_SIZE, BACKTAB_LOCATION, 0xFFFF, 0xFFFF)
{}

void BackTabRAM::reset()
{
    dirtyRAM = TRUE;
    colorAdvanceBitsDirty = TRUE;
    for (UINT16 i = 0; i < BACKTAB_SIZE; i++) {
        bt_image[i] = 0;
        dirtyBytes[i] = TRUE;
        bt_imageLatched[i] = 0;
        dirtyBytesLatched[i] = TRUE;
    }
}

// -------------------------------------------------------------------------------------
// We check to see if the location already has the same value - if so we don't process
// which would set the dirty byte and such... this check gets us some additional speed!
// -------------------------------------------------------------------------------------
ITCM_CODE void BackTabRAM::poke(UINT16 location, UINT16 value)
{
    location -= BACKTAB_LOCATION;

    if (bt_image[location] == value)
        return;

    if ((bt_image[location] & 0x2000) != (value & 0x2000))
        colorAdvanceBitsDirty = TRUE;

    bt_image[location] = value;
    dirtyBytes[location] = TRUE;
}

void BackTabRAM::poke_cheat(UINT16 location, UINT16 value)
{
    return;
}


ITCM_CODE void BackTabRAM::markClean() 
{
    for (UINT16 i = 0; i < BACKTAB_SIZE; i++)
        dirtyBytes[i] = FALSE;
    colorAdvanceBitsDirty = FALSE;
}


ITCM_CODE void BackTabRAM::LatchRow(UINT8 row)
{
    for (int i=(row*20); i<((row+1)*20); i++) 
    {
        bt_imageLatched[i] = bt_image[i];
        dirtyBytesLatched[i] = dirtyBytes[i];
    }
}

ITCM_CODE void BackTabRAM::markCleanLatched() 
{
    for (UINT16 i = 0; i < BACKTAB_SIZE; i++)
        dirtyBytesLatched[i] = FALSE;
    colorAdvanceBitsDirty = FALSE;
}


// -----------------------------------------------------------------------------------
// For saving and restoring the state, we only save either the normal backtab RAM or
// the latched backtab RAM since they are mutually exclusive depending on the game.
// -----------------------------------------------------------------------------------
void BackTabRAM::getState(BackTabRAMState *state)
{
    for (int i=0; i<BACKTAB_SIZE; i++) state->image[i] = bt_image[i];
    for (int i=0; i<BACKTAB_SIZE; i++) state->dirtyBytes[i] = dirtyBytes[i];
    
    state->dirtyRAM = dirtyRAM;
    state->colorAdvanceBitsDirty = colorAdvanceBitsDirty;
}

void BackTabRAM::setState(BackTabRAMState *state)
{
    for (int i=0; i<BACKTAB_SIZE; i++)  bt_image[i]   = bt_imageLatched[i]   = state->image[i];
    for (int i=0; i<BACKTAB_SIZE; i++)  dirtyBytes[i] = dirtyBytesLatched[i] = state->dirtyBytes[i];
    
    // Just force the redraw...
    dirtyRAM = TRUE;
    colorAdvanceBitsDirty = TRUE;
}

