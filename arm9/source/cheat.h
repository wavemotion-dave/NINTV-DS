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

#ifndef __CHEAT_H
#define __CHEAT_H

#include <nds.h>
#include "types.h"

#define MAX_CHEATS_PER_GAME     16
#define MAX_POKES_PER_CHEAT     50

struct Cheat_t
{
    char    name[30];   // Name of this particular cheat (e.g. "Infinite Lives" or "Invincibility")
    u8      enabled;    // Is this particular cheat enabled?
    struct
    {
        u16  addr;      // Poke Address
        u16  value;     // Value to Poke
    } pokes[MAX_POKES_PER_CHEAT];
};

extern struct Cheat_t myCheats[MAX_CHEATS_PER_GAME];

void LoadCheats(void);
void CheatMenu(void);

#endif
