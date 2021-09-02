
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



