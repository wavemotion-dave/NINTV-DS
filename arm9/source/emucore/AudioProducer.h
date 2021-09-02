
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
