
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

TYPEDEF_STRUCT_PACK( _AY38914State
{
    UINT16 registers[0x0E];
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
    INT8  cachedTotalOutputIsDirty;
    INT8  envelopeIdle;
    INT8  envelopeHold;
    INT8  envelopeAltr;
    INT8  envelopeAtak;
    INT8  envelopeCont;
    INT8  noiseIdle;
    INT8  noise;
} AY38914State; )
    
    
//divisor for slowing down the clock speed for the AY38914
extern  INT32 clockDivisor;

//cached total output sample
extern  BOOL  cachedTotalOutputIsDirty;
extern  INT32 cachedTotalOutput;

//envelope data
extern  BOOL  envelopeIdle;
extern  INT32 envelopePeriod;
extern  INT32 envelopePeriodValue;
extern  INT32 envelopeVolume;
extern  BOOL  envelopeHold;
extern  BOOL  envelopeAltr;
extern  BOOL  envelopeAtak;
extern  BOOL  envelopeCont;
extern  INT32 envelopeCounter;        

//noise data
extern  BOOL  noiseIdle;
extern  INT32 noisePeriod;
extern  INT32 noisePeriodValue;
extern  INT32 noiseCounter;        

//data for random number generator, used for white noise accuracy
extern  INT32 my_random;
extern  BOOL  noise;


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

        void setClockDivisor(INT32 clockDivisor);
        INT32 getClockDivisor();

        AY38914State getState();
        void setState(AY38914State state);

        //registers
        AY38914_Registers      registers;

    private:
        AY38914_InputOutput*   psgIO0;
        AY38914_InputOutput*   psgIO1;
};

#endif
