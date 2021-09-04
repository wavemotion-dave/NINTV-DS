#include <nds.h>
#include "AY38914.h"
#include "AudioMixer.h"

INT32 amplitudes16Bit[16] __attribute__((section(".dtcm"))) = 
{
    0x003C, 0x0055, 0x0079, 0x00AB, 0x00F1, 0x0155, 0x01E3, 0x02AA,
    0x03C5, 0x0455, 0x058B, 0x08AB, 0x0B16, 0x0D55, 0x0F2B, 0x1AAA
    //0x03C5, 0x0555, 0x078B, 0x0AAB, 0x0F16, 0x1555, 0x1E2B, 0x2AAA
};

struct Channel_t channel0 __attribute__((section(".dtcm")));
struct Channel_t channel1 __attribute__((section(".dtcm")));
struct Channel_t channel2 __attribute__((section(".dtcm")));

  INT32 clockDivisor __attribute__((section(".dtcm")));

  //cached total output sample
  BOOL  cachedTotalOutputIsDirty __attribute__((section(".dtcm")));
  INT32 cachedTotalOutput __attribute__((section(".dtcm")));

  //envelope data
  BOOL  envelopeIdle __attribute__((section(".dtcm")));
  INT32 envelopePeriod __attribute__((section(".dtcm")));
  INT32 envelopePeriodValue __attribute__((section(".dtcm")));
  INT32 envelopeVolume __attribute__((section(".dtcm")));
  BOOL  envelopeHold __attribute__((section(".dtcm")));
  BOOL  envelopeAltr __attribute__((section(".dtcm")));
  BOOL  envelopeAtak __attribute__((section(".dtcm")));
  BOOL  envelopeCont __attribute__((section(".dtcm")));
  INT32 envelopeCounter __attribute__((section(".dtcm")));         

  //noise data
  BOOL  noiseIdle __attribute__((section(".dtcm")));
  INT32 noisePeriod __attribute__((section(".dtcm")));
  INT32 noisePeriodValue __attribute__((section(".dtcm")));
  INT32 noiseCounter __attribute__((section(".dtcm")));        

  //data for random number generator, used for white noise accuracy
  INT32 my_random __attribute__((section(".dtcm")));
  BOOL  noise __attribute__((section(".dtcm")));



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
    if (noiseCounter <= 0) {
        BOOL oldNoise = noise;
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

AY38914State AY38914::getState()
{
	AY38914State state = {0};
#if 0
	this->registers.getMemory(state.registers, 0, this->registers.getMemoryByteSize());

	state.clockDivisor = this->clockDivisor;

	state.channel0 = this->channel0.getState();
	state.channel1 = this->channel1.getState();
	state.channel2 = this->channel2.getState();

	state.cachedTotalOutputIsDirty = this->cachedTotalOutputIsDirty;
	state.cachedTotalOutput = this->cachedTotalOutput;

	state.envelopeIdle = this->envelopeIdle;
	state.envelopePeriod = this->envelopePeriod;
	state.envelopePeriodValue = this->envelopePeriodValue;
	state.envelopeCounter = this->envelopeCounter;
	state.envelopeVolume = this->envelopeVolume;
	state.envelopeHold = this->envelopeHold;
	state.envelopeAltr = this->envelopeAltr;
	state.envelopeAtak = this->envelopeAtak;
	state.envelopeCont = this->envelopeCont;

	state.noiseIdle = this->noiseIdle;
	state.noisePeriod = this->noisePeriod;
	state.noisePeriodValue = this->noisePeriodValue;
	state.noiseCounter = this->noiseCounter;

	state.my_random = this->my_random;
	state.noise = this->noise;
#endif
	return state;
}

void AY38914::setState(AY38914State state)
{
#if 0    
	this->registers.setMemory(state.registers, 0, this->registers.getMemoryByteSize());

	this->clockDivisor = state.clockDivisor;

	this->channel0.setState(state.channel0);
	this->channel1.setState(state.channel1);
	this->channel2.setState(state.channel2);

	this->cachedTotalOutputIsDirty = state.cachedTotalOutputIsDirty;
	this->cachedTotalOutput = state.cachedTotalOutput;

	this->envelopeIdle = state.envelopeIdle;
	this->envelopePeriod = state.envelopePeriod;
	this->envelopePeriodValue = state.envelopePeriodValue;
	this->envelopeCounter = state.envelopeCounter;
	this->envelopeVolume = state.envelopeVolume;
	this->envelopeHold = state.envelopeHold;
	this->envelopeAltr = state.envelopeAltr;
	this->envelopeAtak = state.envelopeAtak;
	this->envelopeCont = state.envelopeCont;

	this->noiseIdle = state.noiseIdle;
	this->noisePeriod = state.noisePeriod;
	this->noisePeriodValue = state.noisePeriodValue;
	this->noiseCounter = state.noiseCounter;

	this->my_random = state.my_random;
	this->noise = state.noise;
#endif    
}
