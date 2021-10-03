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

#include "CP1610.h"

SignalLine nullPin;

Processor::Processor(const char* nm)
    : name(nm)
{
    for (UINT8 i = 0; i < MAX_PINS; i++) {
        pinOut[i] = &nullPin;
        pinIn[i] = &nullPin;
    }
}

Processor::~Processor()
{
    for (UINT8 i = 0; i < MAX_PINS; i++) {
        if (pinOut[i] != &nullPin)
            delete pinOut[i];
    }
}

void Processor::connectPinOut(UINT8 pinOutNum, Processor* targetProcessor,
        UINT8 targetPinInNum)
{
    if (pinOut[pinOutNum] != &nullPin)
        disconnectPinOut(pinOutNum);

    if (targetProcessor->pinIn[targetPinInNum] != &nullPin) {
        targetProcessor->pinIn[targetPinInNum]->pinOutProcessor->disconnectPinOut(
                targetProcessor->pinIn[targetPinInNum]->pinOutNum);
    }

    pinOut[pinOutNum] = new SignalLine(this, pinOutNum, targetProcessor,
            targetPinInNum);

    targetProcessor->pinIn[targetPinInNum] = pinOut[pinOutNum];
}

void Processor::disconnectPinOut(UINT8 pinOutNum)
{
    if (pinOut[pinOutNum] == &nullPin)
        return;

    SignalLine* s = pinOut[pinOutNum];
    s->pinInProcessor->pinIn[s->pinInNum] = &nullPin;
    pinOut[pinOutNum] = &nullPin;
    delete s;
}
