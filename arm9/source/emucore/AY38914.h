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

#ifndef AY38914_H
#define AY38914_H

#include "AudioProducer.h"
#include "AY38914_Registers.h"
#include "AY38914_Channel.h"
#include "AY38914_InputOutput.h"
#include "AudioOutputLine.h"
#include "Processor.h"
#include "types.h"

class Intellivision;

extern INT32 clockDivisor;
extern INT32 clocksPerSample;

TYPEDEF_STRUCT_PACK( _AY38914State
{
    UINT16 registers[0x0E];
    struct Channel_t channel0;
    struct Channel_t channel1;
    struct Channel_t channel2;
    INT32 clockDivisor;
    INT32 cachedTotalOutput;
    INT32 envelopePeriod;
    INT32 envelopePeriodValue;
    INT32 envelopeCounter;
    INT32 envelopeVolume;
    INT32 noisePeriod;
    INT32 noisePeriodValue;
    INT32 noiseCounter;
    INT32 random;
    UINT8 cachedTotalOutputIsDirty;
    UINT8 envelopeIdle;
    UINT8 envelopeHold;
    UINT8 envelopeAltr;
    UINT8 envelopeAtak;
    UINT8 envelopeCont;
    UINT8 noiseIdle;
    UINT8 noise;
} AY38914State; )


/**
 * The AY-3-8914 chip in the Intellivision, also known as the Programmable
 * Sound Generator (PSG).
 *
 * @author Kyle Davis
 */
class AY38914 : public Processor, public AudioProducer
{
    friend class AY38914_Registers;

    public:
        AY38914(UINT16 location, AY38914_InputOutput* io0,
                AY38914_InputOutput* io1);
        void resetProcessor();
        INT32 getClockSpeed();
        INT32 getClocksPerSample();
        INT32 getSampleRate() { return getClockSpeed(); }
        INT32 tick(INT32);

        void getState(AY38914State *state);
        void setState(AY38914State *state);

        void setClockDivisor(INT32 clockDivisor);
        INT32 getClockDivisor();

        AY38914_Registers registers;

        struct Channel_t channel0;
        struct Channel_t channel1;
        struct Channel_t channel2;

        //cached total output sample
        UINT8 cachedTotalOutputIsDirty;

        //envelope data
        UINT8 envelopeIdle;
        INT32 envelopePeriod;
        INT32 envelopePeriodValue;
        INT32 envelopeVolume;
        UINT8 envelopeHold;
        UINT8 envelopeAltr;
        UINT8 envelopeAtak;
        UINT8 envelopeCont;
        INT32 envelopeCounter;

        //noise data
        UINT8 noiseIdle;
        INT32 noisePeriod;
        INT32 noisePeriodValue;
        INT32 noiseCounter;

        //data for random number generator, used for white noise accuracy
        INT32 my_random;
        UINT8 noise;

    private:
        AY38914_InputOutput*   psgIO0;
        AY38914_InputOutput*   psgIO1;
        UINT16  location;
};

#endif
