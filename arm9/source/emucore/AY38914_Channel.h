
#ifndef AY38914__CHANNEL_H
#define AY38914__CHANNEL_H

#include "types.h"

struct Channel_t
{
    INT32   period;
    INT32   periodValue;
    INT32   volume;
    INT32   toneCounter;
    BOOL    tone;
    BOOL    envelope;
    BOOL    toneDisabled;
    BOOL    noiseDisabled;
    BOOL    isDirty;
    INT32   cachedSample;
};

extern struct Channel_t channel0;
extern struct Channel_t channel1;
extern struct Channel_t channel2;

#endif
