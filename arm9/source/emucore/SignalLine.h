
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
