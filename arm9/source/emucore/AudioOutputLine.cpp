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
#include <nds.h>
#include "AudioOutputLine.h"
#include "AudioMixer.h"

INT64 sampleBuffer[2]              __attribute__((section(".dtcm")));
INT64 commonClockCounter[2]        __attribute__((section(".dtcm")));
INT64 commonClocksPerSample[2]     __attribute__((section(".dtcm")));
INT16 previousSample[2]            __attribute__((section(".dtcm")));
INT16 currentSample[2]             __attribute__((section(".dtcm")));

void audio_output_line_reset(void)
{
    for (int i=0; i<2; i++)
    {
        sampleBuffer[i] = 0;
        previousSample[i] = 0;
        currentSample[i] = 0;
        commonClockCounter[i] = 0;
        commonClocksPerSample[i] = 0;
    }
}

ITCM_CODE void playSample0(INT16 sample)
{
    sampleBuffer[0] += currentSample[0] * commonClocksPerSample[0];
    commonClockCounter[0] += commonClocksPerSample[0];
    previousSample[0] = currentSample[0];
    currentSample[0] = sample;
}

ITCM_CODE void playSample1(INT16 sample)
{
    sampleBuffer[1] += currentSample[1] * commonClocksPerSample[1];
    commonClockCounter[1] += commonClocksPerSample[1];
	previousSample[1] = currentSample[1];
	currentSample[1] = sample;
}



