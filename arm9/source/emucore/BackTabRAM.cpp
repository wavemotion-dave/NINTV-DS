// =====================================================================================
// Copyright (c) 2021-2026 Dave Bernazzani (wavemotion-dave)
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
UINT8   dirtyBytesLatched[BACKTAB_SIZE]    __attribute__((section(".dtcm")));
UINT8   colorAdvanceBitsDirty              __attribute__((section(".dtcm")));
UINT8   bRowAltered[BACKTAB_SIZE/20]       __attribute__((section(".dtcm")));

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
        bRowAltered[i/20] = 0;
    }
}

// -------------------------------------------------------------------------------------
// We check to see if the location already has the same value - if so we don't process
// which would set the dirty byte and such... this check gets us some additional speed!
// -------------------------------------------------------------------------------------
ITCM_CODE void BackTabRAM::poke(UINT16 location, UINT16 value)
{
    if (bt_image[location & (BACKTAB_LOCATION-1)] == value)
        return;

    UINT16 loc = location & (BACKTAB_LOCATION-1);

    if ((bt_image[loc] & 0x2000) != (value & 0x2000))
        colorAdvanceBitsDirty = TRUE;

    bt_image[loc] = value;
    dirtyBytes[loc] = TRUE;
    bRowAltered[loc/20] = TRUE;
}

void BackTabRAM::poke_cheat(UINT16 location, UINT16 value)
{
    return;
}


ITCM_CODE void BackTabRAM::markClean()
{
    UINT32 *dest = (UINT32*)dirtyBytes;

    // Unroll loop for a tiny speedup
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;

    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;

    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;

    colorAdvanceBitsDirty = FALSE;
}


ITCM_CODE void BackTabRAM::LatchRow(UINT8 row)
{
    // ----------------------------------------------------------------------------
    // Check if any one of the cards in this row has been changed since the last
    // latch and if so, latch the entire row. Some games depend on this - Stampede
    // and Masters of the Universe will show graphical glitches if not latched.
    // ----------------------------------------------------------------------------
    if (bRowAltered[row])
    {
        UINT32 *source = (UINT32*)&bt_image[(row*20)];
        UINT32 *dest = (UINT32*)&bt_imageLatched[(row*20)];

        // Unroll loop for a tiny speedup
        *dest++ = *source++; *dest++ = *source++; *dest++ = *source++; *dest++ = *source++; *dest++ = *source++;
        *dest++ = *source++; *dest++ = *source++; *dest++ = *source++; *dest++ = *source++; *dest++ = *source++;

        memcpy(&dirtyBytesLatched[(row*20)], &dirtyBytes[row*20], 20);
        
        bRowAltered[row] = FALSE; // No need to re-latch this row until one of the cards changes via poke()
    }
}


ITCM_CODE void BackTabRAM::markCleanLatched()
{
    UINT32 *dest = (UINT32*)dirtyBytesLatched;

    // Unroll loop for a tiny speedup
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;

    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;

    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;
    *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0; *dest++ = 0;

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

