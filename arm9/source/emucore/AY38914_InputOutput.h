// =====================================================================================
// Copyright (c) 2021-2024 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef AY38914INPUTOUTPUT_H
#define AY38914INPUTOUTPUT_H

#include "types.h"

class AY38914_InputOutput
{
    public:
        virtual UINT16 getInputValue(UINT8) = 0;
        virtual void setOutputValue(UINT16 value) = 0;
        virtual void setDirectionIO(UINT16 value) = 0;
};

#endif
