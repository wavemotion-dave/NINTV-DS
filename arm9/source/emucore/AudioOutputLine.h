
#ifndef AUDIOOUTPUTLINE_H
#define AUDIOOUTPUTLINE_H

#include "types.h"

class AudioOutputLine
{

    friend class AudioMixer;

    public:
        void playSample(INT16 sample);

    private:
        AudioOutputLine();
        void reset();

        INT64 sampleBuffer;
		INT64 previousSample;
		INT64 currentSample;
		INT64 commonClockCounter;
		INT64 commonClocksPerSample;

};

#endif


