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

#ifndef PALETTE_H
#define PALETTE_H

#include "types.h"

typedef struct _PALETTE
{
    UINT16 m_iNumEntries;
    UINT32 m_iEntries[256];

} PALETTE;

#endif
