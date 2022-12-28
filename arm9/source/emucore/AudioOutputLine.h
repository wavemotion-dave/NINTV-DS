// =====================================================================================
// Copyright (c) 2021-2023 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef AUDIOOUTPUTLINE_H
#define AUDIOOUTPUTLINE_H

#include "types.h"

extern INT64 sampleBuffer[3];
extern INT32 commonClockCounter[3];
extern INT64 commonClocksPerSample[3];
extern INT16 previousSample[3];
extern INT16 currentSample[3];

extern void playSample0(INT16 sample);  // For the normal PSG
extern void playSample1(INT16 sample);  // For the Intellivoice
extern void playSample2(INT16 sample);  // For the ECS extra PSG
extern void audio_output_line_reset();

#endif
