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
#ifndef __MANUAL_H
#define __MANUAL_H

#include <nds.h>
#include "types.h"

// ----------------------------------------------------
// This is roughly 16k of text... more than enough...
// ----------------------------------------------------
#define MAX_MAN_COLS    32
#define MAX_MAN_ROWS    512
#define ROWS_PER_PAGE   18

extern void dsShowManual(void);

#endif
