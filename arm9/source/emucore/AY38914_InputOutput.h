
#ifndef AY38914INPUTOUTPUT_H
#define AY38914INPUTOUTPUT_H

#include "types.h"

class AY38914_InputOutput
{

    public:
        virtual UINT16 getInputValue() = 0;
        virtual void setOutputValue(UINT16 value) = 0;

};

#endif
