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
#include "GRAM.h"


UINT8     gram_image[GRAM_SIZE]         __attribute__((section(".dtcm")));
UINT8     dirtyCards[GRAM_SIZE>>3]      __attribute__((section(".dtcm")));
UINT8     dirtyRAM                      __attribute__((section(".dtcm")));

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
}

ITCM_CODE void GRAM::poke(UINT16 location, UINT16 value)
{
    if (!enabled) return;
    location &= GRAM_MASK;
    
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
