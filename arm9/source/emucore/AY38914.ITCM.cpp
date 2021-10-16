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
#include "AY38914.h"
#include "AudioMixer.h"

INT32 amplitudes16Bit[16] __attribute__((section(".dtcm"))) = 
{
    0x003C, 0x0055, 0x0079, 0x00AB, 0x00F1, 0x0155, 0x01E3, 0x02AA,
    0x03C5, 0x0555, 0x078B, 0x0AAB, 0x0F16, 0x1555, 0x1E2B, 0x2AAA
};

struct Channel_t channel0 __attribute__((section(".dtcm")));
struct Channel_t channel1 __attribute__((section(".dtcm")));
struct Channel_t channel2 __attribute__((section(".dtcm")));

INT32 clockDivisor __attribute__((section(".dtcm")));

//cached total output sample
UINT8 cachedTotalOutputIsDirty __attribute__((section(".dtcm")));
INT32 cachedTotalOutput __attribute__((section(".dtcm")));

//envelope data
UINT8 envelopeIdle __attribute__((section(".dtcm")));
INT32 envelopePeriod __attribute__((section(".dtcm")));
INT32 envelopePeriodValue __attribute__((section(".dtcm")));
INT32 envelopeVolume __attribute__((section(".dtcm")));
UINT8 envelopeHold __attribute__((section(".dtcm")));
UINT8 envelopeAltr __attribute__((section(".dtcm")));
UINT8 envelopeAtak __attribute__((section(".dtcm")));
UINT8 envelopeCont __attribute__((section(".dtcm")));
INT32 envelopeCounter __attribute__((section(".dtcm")));         

//noise data
UINT8 noiseIdle __attribute__((section(".dtcm")));
INT32 noisePeriod __attribute__((section(".dtcm")));
INT32 noisePeriodValue __attribute__((section(".dtcm")));
INT32 noiseCounter __attribute__((section(".dtcm")));        

//data for random number generator, used for white noise accuracy
INT32 my_random __attribute__((section(".dtcm")));
UINT8 noise __attribute__((section(".dtcm")));



/**
 * The AY-3-8914 chip in the Intellivision, also know as the Programmable
 * Sound Generator (PSG).
 *
 * @author Kyle Davis
 */
AY38914::AY38914(UINT16 location, AY38914_InputOutput* io0,
        AY38914_InputOutput* io1)
    : Processor("AY-3-8914"),
      registers(location)
{
    this->psgIO0 = io0;
    this->psgIO1 = io1;
    clockDivisor = 8; //TODO: was 1... trying to speed thigns up by lowering sound quality...
    registers.init(this);
}

INT32 AY38914::getClockSpeed() {
    return 3579545;
}

INT32 AY38914::getClocksPerSample() {
    return clockDivisor<<4;
}

void AY38914::resetProcessor()
{
    //reset the state variables
    noisePeriod = 0;
    noisePeriodValue = 0x0040;
    noiseCounter = noisePeriodValue;
    my_random = 1;
    noise = TRUE;
    noiseIdle = TRUE;
    envelopeIdle = TRUE;
    envelopePeriod = 0;
    envelopePeriodValue = 0x20000;
    envelopeCounter = envelopePeriodValue;
    envelopeVolume = 0;
    envelopeHold = FALSE;
    envelopeAltr = FALSE;
    envelopeAtak = FALSE;
    envelopeCont = FALSE;
    cachedTotalOutput = 0;
    cachedTotalOutputIsDirty = TRUE;
    
    memset(&channel0, 0x00, sizeof(channel0));
    memset(&channel1, 0x00, sizeof(channel1));
    memset(&channel2, 0x00, sizeof(channel2));
    
    channel0.periodValue = 0x1000;
    channel0.toneCounter = 0x1000;
    channel0.isDirty = TRUE;

    channel1.periodValue = 0x1000;
    channel1.toneCounter = 0x1000;
    channel1.isDirty = TRUE;
    
    channel2.periodValue = 0x1000;
    channel2.toneCounter = 0x1000;
    channel2.isDirty = TRUE;
}

void AY38914::setClockDivisor(INT32 clockDivisor) {
    clockDivisor = clockDivisor;
}

INT32 AY38914::getClockDivisor() {
    return clockDivisor;
}

/**
 * Tick the AY38914.  This method has been profiled over and over again
 * in an attempt to tweak it for optimal performance.  Please be careful
 * with any modifications that may adversely affect performance.
 *
 * @return the number of ticks used by the AY38914, always returns 4.
 */
ITCM_CODE INT32 AY38914::tick(INT32 minimum)
{
	INT32 totalTicks = 0;
	do {
    //iterate the envelope generator
    envelopeCounter -= clockDivisor;
    if (envelopeCounter <= 0) 
    {
        do {
            envelopeCounter += envelopePeriodValue;
            if (!envelopeIdle) {
                envelopeVolume += (envelopeAtak ? 1 : -1);
                if (envelopeVolume > 15 || envelopeVolume < 0) {
                    if (!envelopeCont) {
                        envelopeVolume = 0;
                        envelopeIdle = TRUE;
                    }
                    else if (envelopeHold) {
                        envelopeVolume = (envelopeAtak == envelopeAltr ? 0 : 15);
                        envelopeIdle = TRUE;
                    }
                    else {
                        envelopeAtak = (envelopeAtak != envelopeAltr);
                        envelopeVolume = (envelopeAtak ? 0 : 15);
                    }
                }
            }
        }
        while (envelopeCounter <= 0);
    }

    //iterate the noise generator
    noiseCounter -= clockDivisor;
    if (noiseCounter <= 0) 
    {
        do {
            noiseCounter += noisePeriodValue;
            if (!noiseIdle) {
                my_random = (my_random >> 1) ^ (noise ? 0x14000 : 0);
                noise = (my_random & 1);
            }
        }
        while (noiseCounter <= 0);
    }
        
    //iterate the tone generator for channel 0
    channel0.toneCounter -= clockDivisor;
    if (channel0.toneCounter <= 0) 
    {
        do {
            channel0.toneCounter += channel0.periodValue;
            channel0.tone = !channel0.tone;
        }
        while (channel0.toneCounter <= 0);
    }

    //iterate the tone generator for channel 1
    channel1.toneCounter -= clockDivisor;
    if (channel1.toneCounter <= 0) 
    {
        do {
            channel1.toneCounter += channel1.periodValue;
            channel1.tone = !channel1.tone;
        }
        while (channel1.toneCounter <= 0);
    }

    //iterate the tone generator for channel 2
    channel2.toneCounter -= clockDivisor;
    if (channel2.toneCounter <= 0) 
    {
        do {
            channel2.toneCounter += channel2.periodValue;
            channel2.tone = !channel2.tone;
        }
        while (channel2.toneCounter <= 0);
    }
        
    cachedTotalOutput  = amplitudes16Bit[(((channel0.toneDisabled | channel0.tone) & (channel0.noiseDisabled | noise)) ? (channel0.envelope ? envelopeVolume : channel0.volume) : 0)];
    cachedTotalOutput += amplitudes16Bit[(((channel1.toneDisabled | channel1.tone) & (channel1.noiseDisabled | noise)) ? (channel1.envelope ? envelopeVolume : channel1.volume) : 0)];
    cachedTotalOutput += amplitudes16Bit[(((channel2.toneDisabled | channel2.tone) & (channel2.noiseDisabled | noise)) ? (channel2.envelope ? envelopeVolume : channel2.volume) : 0)];

    // Now place the sample onto the audio output line...
    audioOutputLine->playSample((INT16)cachedTotalOutput);

	totalTicks += (clockDivisor<<4);

	} while (totalTicks < minimum);

    //every tick here always uses some multiple of 4 CPU cycles
    //or 16 NTSC cycles
	return totalTicks;
}


void AY38914::getState(AY38914State *state)
{
    memcpy(state->registers, registers.memory, 0x0E * sizeof(UINT16));
    memcpy(&state->channel0, &channel0, sizeof(struct Channel_t));
    memcpy(&state->channel1, &channel1, sizeof(struct Channel_t));
    memcpy(&state->channel2, &channel2, sizeof(struct Channel_t));

    state->clockDivisor              = clockDivisor;
    state->cachedTotalOutput         = cachedTotalOutput;
    state->envelopePeriod            = envelopePeriod;
    state->envelopePeriodValue       = envelopePeriodValue;
    state->envelopeCounter           = envelopeCounter;
    state->envelopeVolume            = envelopeVolume;
    state->noisePeriod               = noisePeriod;
    state->noisePeriodValue          = noisePeriodValue;
    state->noiseCounter              = noiseCounter;
    state->random                    = my_random;
    state->cachedTotalOutputIsDirty  = cachedTotalOutputIsDirty;
    state->envelopeIdle              = envelopeIdle;
    state->envelopeHold              = envelopeHold;
    state->envelopeAltr              = envelopeAltr;
    state->envelopeAtak              = envelopeAtak;
    state->envelopeCont              = envelopeCont;
    state->noiseIdle                 = noiseIdle;
    state->noise                     = noise;
}

void AY38914::setState(AY38914State *state)
{
    memcpy(registers.memory, state->registers, 0x0E * sizeof(UINT16));
    memcpy(&channel0, &state->channel0, sizeof(struct Channel_t));
    memcpy(&channel1, &state->channel1, sizeof(struct Channel_t));
    memcpy(&channel2, &state->channel2, sizeof(struct Channel_t));

    clockDivisor = state->clockDivisor;
    cachedTotalOutput = state->cachedTotalOutput;
    envelopePeriod = state->envelopePeriod;
    envelopePeriodValue = state->envelopePeriodValue;
    envelopeCounter = state->envelopeCounter;
    envelopeVolume = state->envelopeVolume;
    noisePeriod = state->noisePeriod;
    noisePeriodValue = state->noisePeriodValue;
    noiseCounter = state->noiseCounter;
    my_random = state->random;
    cachedTotalOutputIsDirty = state->cachedTotalOutputIsDirty;
    envelopeIdle = state->envelopeIdle;
    envelopeHold = state->envelopeHold;
    envelopeAltr = state->envelopeAltr;
    envelopeAtak = state->envelopeAtak;
    envelopeCont = state->envelopeCont;
    noiseIdle = state->noiseIdle;
    noise = state->noise;
}

