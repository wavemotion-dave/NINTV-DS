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

#include "AudioOutputLine.h"
#include "AudioMixer.h"

AudioOutputLine::AudioOutputLine()
  : sampleBuffer(0),
	previousSample(0),
	currentSample(0),
	commonClockCounter(0),
	commonClocksPerSample(0)
{}

void AudioOutputLine::reset()
{
    sampleBuffer = 0;
	previousSample = 0;
	currentSample = 0;
	commonClockCounter = 0;
	commonClocksPerSample = 0;
}

void AudioOutputLine::playSample(INT16 sample)
{
    sampleBuffer += currentSample * commonClocksPerSample;
    commonClockCounter += commonClocksPerSample;
	previousSample = currentSample;
	currentSample = sample;
}



