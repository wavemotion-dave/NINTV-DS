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

#ifndef AUDIOPRODUCER_H
#define AUDIOPRODUCER_H

#include "AudioOutputLine.h"

/**
 * This interface is implemented by any piece of hardware that produces audio.
 */
class AudioProducer
{

    friend class AudioMixer;

public:
    AudioProducer() : audioOutputLine(NULL) {}

    virtual INT32 getClockSpeed() = 0;
	virtual INT32 getClocksPerSample() = 0;

    protected:
        AudioOutputLine* audioOutputLine;

};

#endif
