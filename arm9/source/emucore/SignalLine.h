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

#ifndef SIGNALLINE_H
#define SIGNALLINE_H

#include "types.h"

class Processor;

class SignalLine
{
    public:
        SignalLine() 
            { SignalLine(NULL, 0, NULL, 0); }

        SignalLine(Processor* pop, UINT8 pon, Processor* pip, UINT8 pin)
            : pinOutProcessor(pop),
              pinOutNum(pon),
              pinInProcessor(pip),
              pinInNum(pin),
              isHigh(FALSE) { }

        Processor* pinOutProcessor;
        UINT8      pinOutNum;
        Processor* pinInProcessor;
        UINT8      pinInNum;
        BOOL       isHigh;

};

#endif
