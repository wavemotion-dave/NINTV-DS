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
#include "GRAM.h"
#include "../config.h"

UINT8     gram_image[GRAM_SIZE]         __attribute__((section(".dtcm")));
UINT8     dirtyCards[GRAM_SIZE>>3]      __attribute__((section(".dtcm")));
UINT8     dirtyRAM                      __attribute__((section(".dtcm")));

// --------------------------------------------------------------------------------------------------------
// These are not defines so that we can adjust based on whether the 2K GRAM (aka Tutorvision mode) is
// enabled. This is 1-2% slower emulation but provides for the ability to have an upgraded GRAM emulation.
// --------------------------------------------------------------------------------------------------------
UINT16 GRAM_MASK            __attribute__((section(".dtcm"))) = 0x01FF;  // Allows indexing the 512 or 2K bytes of GRAM
UINT16 GRAM_COL_STACK_MASK  __attribute__((section(".dtcm"))) = 0x01F8;  // Allows for indexing 64 / 256 tiles in Color Stack mode
UINT16 GRAM_CARD_MOB_MASK   __attribute__((section(".dtcm"))) =   0x3F;  // Allows for indexing 64 / 256 tiles for MOBs in Color Stack mode

GRAM::GRAM()
: RAM(GRAM_SIZE, GRAM_ADDRESS, GRAM_READ_MASK, GRAM_WRITE_MASK, 8)
{}

void GRAM::reset()
{
    UINT16 i;
    dirtyRAM = TRUE;
    for (i = 0; i < GRAM_SIZE; i++)
        gram_image[i] = 0;

    for (i = 0; i < (GRAM_SIZE>>3); i++)
        dirtyCards[i] = TRUE;
        
    if (myConfig.gramSize)
    {
        GRAM_MASK            = 0x07FF;
        GRAM_COL_STACK_MASK  = 0x07F8;
        GRAM_CARD_MOB_MASK   =   0xFF;
    }
    else
    {
        GRAM_MASK            = 0x01FF;
        GRAM_COL_STACK_MASK  = 0x01F8;
        GRAM_CARD_MOB_MASK   =   0x3F;
    }
}

ITCM_CODE void GRAM::poke(UINT16 location, UINT16 value)
{
    if (!enabled) return;
    location &= GRAM_MASK;
    
    if (gram_image[location] == (UINT8)value) return;  // Don't set dirty bits below if not changed
    gram_image[location] = (UINT8)value;
    dirtyCards[location>>3] = TRUE;
    dirtyRAM = TRUE;
}

ITCM_CODE void GRAM::markClean() {
    if (!dirtyRAM)
        return;

    memset(dirtyCards, 0x00, sizeof(dirtyCards));
    
    dirtyRAM = FALSE;
}

void GRAM::getState(GRAMState *state)
{
    for (int i=0; i<GRAM_SIZE; i++)         state->gram_image[i] = gram_image[i];
    for (int i=0; i<(GRAM_SIZE>>3); i++)    state->dirtyCards[i] = dirtyCards[i];
    state->dirtyRAM = dirtyRAM;
}

void GRAM::setState(GRAMState *state)
{
    for (int i=0; i<GRAM_SIZE; i++)         gram_image[i] = state->gram_image[i];
    for (int i=0; i<(GRAM_SIZE>>3); i++)    dirtyCards[i] = state->dirtyCards[i];
    dirtyRAM = TRUE; // Force a redraw to be safe
}
