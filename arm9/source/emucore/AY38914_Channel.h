// =====================================================================================
// Copyright (c) 2021-2025 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef AY38914__CHANNEL_H
#define AY38914__CHANNEL_H

#include "types.h"

struct Channel_t
{
    INT32   period;
    INT32   periodValue;
    INT32   toneCounter;
    UINT8   volume;
    UINT8   tone;
    UINT8   envelope;
    UINT8   toneDisabled;
    UINT8   noiseDisabled;
    UINT8   isDirty;
};

#endif
