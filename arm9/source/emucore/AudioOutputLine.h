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
		INT64 commonClockCounter;
		INT64 commonClocksPerSample;
		INT16 previousSample;
		INT16 currentSample;
};

#endif


