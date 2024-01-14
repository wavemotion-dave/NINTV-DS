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
#include "AudioOutputLine.h"
#include "AudioMixer.h"

INT64 sampleBuffer[3]              __attribute__((section(".dtcm")));
INT32 commonClockCounter[3]        __attribute__((section(".dtcm")));
INT32 commonClocksPerSample[3]     __attribute__((section(".dtcm")));
INT16 previousSample[3]            __attribute__((section(".dtcm")));
INT16 currentSample[3]             __attribute__((section(".dtcm")));

void audio_output_line_reset(void)
{
    for (int i=0; i<3; i++)
    {
        sampleBuffer[i] = 0;
        previousSample[i] = 0;
        currentSample[i] = 0;
        commonClockCounter[i] = 0;
        commonClocksPerSample[i] = 0;
    }
}

// --------------------------------------------------------------------------------------
// These two functions used to be a single function - but it's called often enough that
// it's worth having a play sample 0/1 as we use 0 for the normal PSG and 1 for SP0256.
// --------------------------------------------------------------------------------------
ITCM_CODE void playSample0(INT16 sample) // Normal PSG
{
    sampleBuffer[0] += currentSample[0] * (INT64)commonClocksPerSample[0];
    commonClockCounter[0] += commonClocksPerSample[0];
    previousSample[0] = currentSample[0];
    currentSample[0] = sample;
}

ITCM_CODE void playSample1(INT16 sample) // ECS PSG or Intellivoice SP0256
{
    sampleBuffer[1] += currentSample[1] * (INT64)commonClocksPerSample[1];
    commonClockCounter[1] += commonClocksPerSample[1];
    previousSample[1] = currentSample[1];
    currentSample[1] = sample;
}


ITCM_CODE void playSample2(INT16 sample) // ECS PSG or Intellivoice SP0256
{
    sampleBuffer[2] += currentSample[2] * (INT64)commonClocksPerSample[2];
    commonClockCounter[2] += commonClocksPerSample[2];
    previousSample[2] = currentSample[2];
    currentSample[2] = sample;
}


