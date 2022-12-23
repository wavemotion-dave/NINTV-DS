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

UINT16 amplitudes16Bit[16] __attribute__((section(".dtcm"))) = 
{
    0x003C, 0x0055, 0x0079, 0x00AB, 0x00F1, 0x0155, 0x01E3, 0x02AA,
    0x03C5, 0x0555, 0x078B, 0x0AAB, 0x0F16, 0x1555, 0x1E2B, 0x2AAA
};

// ----------------------------------------------------------------
// These are common no matter whether we have 1 PSG or 2 (for ECS)
// ----------------------------------------------------------------
INT32 clockDivisor   __attribute__((section(".dtcm")));
INT32 clocksPerSample  __attribute__((section(".dtcm")));

extern UINT8 bUseIVoice;


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
    this->location = location;
    this->psgIO0 = io0;
    this->psgIO1 = io1;
    clockDivisor = 8; 
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

void AY38914::setClockDivisor(INT32 clockDivisor) 
{
    clockDivisor = clockDivisor;
}

ITCM_CODE INT32 AY38914::getClockDivisor() {
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
    do 
    {
        //iterate the envelope generator
        envelopeCounter -= clockDivisor;
        while (envelopeCounter <= 0)
        {
            envelopeCounter += envelopePeriodValue;
            if (!envelopeIdle) 
            {
                envelopeVolume += (envelopeAtak ? 1 : -1);
                if (envelopeVolume > 15 || envelopeVolume < 0) 
                {
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

        //iterate the noise generator
        noiseCounter -= clockDivisor;
        while (noiseCounter <= 0)
        {
            noiseCounter += noisePeriodValue;
            if (!noiseIdle) 
            {
                my_random = (my_random >> 1) ^ (noise ? 0x14000 : 0);
                noise = (my_random & 1);
            }
        }

        //iterate the tone generator for channel 0
        channel0.toneCounter -= clockDivisor;
        while (channel0.toneCounter <= 0)
        {
            channel0.toneCounter += channel0.periodValue;
            channel0.tone = !channel0.tone;
        }

        //iterate the tone generator for channel 1
        channel1.toneCounter -= clockDivisor;
        while (channel1.toneCounter <= 0)
        {
            channel1.toneCounter += channel1.periodValue;
            channel1.tone = !channel1.tone;
        }

        //iterate the tone generator for channel 2
        channel2.toneCounter -= clockDivisor;
        while (channel2.toneCounter <= 0)
        {
            channel2.toneCounter += channel2.periodValue;
            channel2.tone = !channel2.tone;
        }

        cachedTotalOutput  = amplitudes16Bit[(((channel0.toneDisabled | channel0.tone) & (channel0.noiseDisabled | noise)) ? (channel0.envelope ? envelopeVolume : channel0.volume) : 0)];
        cachedTotalOutput += amplitudes16Bit[(((channel1.toneDisabled | channel1.tone) & (channel1.noiseDisabled | noise)) ? (channel1.envelope ? envelopeVolume : channel1.volume) : 0)];
        cachedTotalOutput += amplitudes16Bit[(((channel2.toneDisabled | channel2.tone) & (channel2.noiseDisabled | noise)) ? (channel2.envelope ? envelopeVolume : channel2.volume) : 0)];

        // Now place the sample onto the audio output line...
        if (this->location == 0xF0)
        {
            if (bUseIVoice) playSample2(cachedTotalOutput);     // This is the ECS PSG on channel 2 (Intellivoice on Channel 1)
            else playSample1(cachedTotalOutput);                // This is the ECS PSG on channel 1 (no Intellivoice)
        }
        else
            playSample0(cachedTotalOutput);     // This is the normal Intellivision console PSG

        totalTicks += clocksPerSample;

    } while (totalTicks < minimum);

    //every tick here always uses some multiple of 4 CPU cycles
    //or 16 NTSC cycles
    return totalTicks;
}


void AY38914::getState(AY38914State *state)
{
    memcpy(state->registers, registers.psg_memory, 0x0E * sizeof(UINT16));
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
    memcpy(registers.psg_memory, state->registers, 0x0E * sizeof(UINT16));
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
    clocksPerSample = clockDivisor<<4;
}

