
#ifndef AUDIOMIXER_H
#define AUDIOMIXER_H

#include "AudioProducer.h"
#include "types.h"
#include "Processor.h"

#define MAX_AUDIO_PRODUCERS 10

template<typename A, typename W> class EmulatorTmpl;

class AudioMixer : public Processor
{

    friend class ProcessorBus;
    friend class AudioOutputLine;

    public:
        AudioMixer();
        virtual ~AudioMixer();

        inline INT16 clipSample(INT64 sample) {
            return sample > 32767 ? 32767 : sample < -32768 ? -32768 : (INT16)sample;
        }

        virtual void resetProcessor();
        INT32 getClockSpeed();
        INT32 tick(INT32 minimum);
        virtual void flushAudio();

        //only to be called by the Emulator
        virtual void init(UINT32 sampleRate);
        virtual void release();

        void addAudioProducer(AudioProducer*);
        void removeAudioProducer(AudioProducer*);
        void removeAll();

    protected:
        //output info
        INT32 clockSpeed;

        AudioProducer*     audioProducers[MAX_AUDIO_PRODUCERS];
        UINT32             audioProducerCount;

        INT64 commonClocksPerTick;
        INT16* sampleBuffer;
        UINT32 sampleBufferSize;
        UINT32 sampleCount;
        UINT32 sampleSize;
};

#endif
