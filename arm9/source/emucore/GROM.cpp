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

#include "GROM.h"

GROM::GROM(const char *name, const char *fileName)
: ROM(name, fileName, 0, 1, GROM_SIZE, GROM_ADDRESS)
{}

void GROM::reset()
{
    visible = TRUE;
}




