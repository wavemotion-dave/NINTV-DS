#include <nds.h>
#include <stdio.h>
#include <string.h>
#include "AudioMixer.h"
#include "AudioOutputLine.h"

UINT16 audio_mixer_buffer[1500] __attribute__((section(".dtcm")));

extern UINT64 lcm(UINT64, UINT64);

AudioMixer::AudioMixer()
  : Processor("Audio Mixer"),
    audioProducerCount(0),
    commonClocksPerTick(0),
    sampleBuffer(NULL),
    sampleBufferSize(0),
    sampleCount(0),
    sampleSize(0)
{
	memset(&audioProducers, 0, sizeof(audioProducers));
}

AudioMixer::~AudioMixer()
{
    for (UINT32 i = 0; i < audioProducerCount; i++)
        delete audioProducers[i]->audioOutputLine;
}

void AudioMixer::addAudioProducer(AudioProducer* p)
{
    audioProducers[audioProducerCount] = p;
    audioProducers[audioProducerCount]->audioOutputLine =
            new AudioOutputLine();
    audioProducerCount++;
}

void AudioMixer::removeAudioProducer(AudioProducer* p)
{
    for (UINT32 i = 0; i < audioProducerCount; i++) {
        if (audioProducers[i] == p) {
            delete p->audioOutputLine;
            for (UINT32 j = i; j < (audioProducerCount-1); j++)
                audioProducers[j] = audioProducers[j+1];
            audioProducerCount--;
            return;
        }
    }
}

void AudioMixer::removeAll()
{
    while (audioProducerCount)
        removeAudioProducer(audioProducers[0]);
}

void AudioMixer::resetProcessor()
{
    //reset instance data
    commonClocksPerTick = 0;
    sampleCount = 0;

	if (sampleBuffer) {
        memset(sampleBuffer, 0, sampleBufferSize);
	}

    //iterate through my audio output lines to determine the common output clock
    UINT64 totalClockSpeed = getClockSpeed();
    for (UINT32 i = 0; i < audioProducerCount; i++) {
        audioProducers[i]->audioOutputLine->reset();
        totalClockSpeed = lcm(totalClockSpeed, ((UINT64)audioProducers[i]->getClockSpeed()));
    }

    //iterate again to determine the clock factor of each
    commonClocksPerTick = totalClockSpeed / getClockSpeed();
    for (UINT32 i = 0; i < audioProducerCount; i++) {
        audioProducers[i]->audioOutputLine->commonClocksPerSample = (totalClockSpeed / audioProducers[i]->getClockSpeed())
				* audioProducers[i]->getClocksPerSample();
    }
}

void AudioMixer::init(UINT32 sampleRate)
{
	// TODO: assert if sampleRate/clockSpeed is 0

	AudioMixer::release();

	clockSpeed = sampleRate;
	sampleSize = ( clockSpeed / 60.0 );
	sampleBufferSize = sampleSize * sizeof(INT16);
	sampleBuffer = (INT16*) audio_mixer_buffer;

	if (sampleBuffer) {
		memset(sampleBuffer, 0, sampleBufferSize);
	}
}

void AudioMixer::release()
{
    if (sampleBuffer) {
        sampleBufferSize = 0;
        sampleSize = 0;
        sampleCount = 0;
    }
}

INT32 AudioMixer::getClockSpeed()
{
	return clockSpeed;
}

ITCM_CODE INT32 AudioMixer::tick(INT32 minimum)
{
    extern int sp_idle;
    UINT8 apc = (sp_idle ? 1:audioProducerCount);
    debug1=apc;
    for (int totalTicks = 0; totalTicks < minimum; totalTicks++) 
    {
        //mix and flush the sample buffers
        INT64 totalSample = 0;
        for (UINT32 i = 0; i < apc; i++) 
        {
            AudioOutputLine* nextLine = audioProducers[i]->audioOutputLine;

            INT64 missingClocks = (this->commonClocksPerTick - nextLine->commonClockCounter);
            INT64 sampleToUse = (missingClocks < 0 ? nextLine->previousSample : nextLine->currentSample);

            //account for when audio producers idle by adding enough samples to each producer's buffer
			//to fill the time since last sample calculation
            INT64 missingSampleCount = (missingClocks / nextLine->commonClocksPerSample);
            if (missingSampleCount != 0) {
			    nextLine->sampleBuffer += missingSampleCount * sampleToUse * nextLine->commonClocksPerSample;
                nextLine->commonClockCounter += missingSampleCount * nextLine->commonClocksPerSample;
                missingClocks -= missingSampleCount * nextLine->commonClocksPerSample;
            }
			INT64 partialSample = sampleToUse * missingClocks;

            //calculate the sample for this line
            totalSample += (INT16)((nextLine->sampleBuffer + partialSample) / commonClocksPerTick);

            //clear the sample buffer for this line
            nextLine->sampleBuffer = -partialSample;
            nextLine->commonClockCounter = -missingClocks;
        }

        if (apc > 1) 
        {
            totalSample = totalSample / apc;
        }

        sampleBuffer[sampleCount++] = totalSample;
        
        if (sampleCount == sampleSize) {
            flushAudio();
        }
    }

    return minimum;
}

void AudioMixer::flushAudio()
{
    //the platform subclass must copy the sampleBuffer to the device
    //before calling here (which discards the contents of sampleBuffer)
    sampleCount = 0;
}
