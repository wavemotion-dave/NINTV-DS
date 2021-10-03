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

#ifndef INPUTPRODUCER_H
#define INPUTPRODUCER_H

#include "types.h"

class InputProducer
{
public:
    InputProducer(GUID g) : guid(g) {}

    GUID getGuid() { return guid; }

    virtual const CHAR* getName() = 0;
// TODO: jeremiah sypult cross-platform
#if defined( _WIN32 )
	virtual IDirectInputDevice8* getDevice() = 0;
#endif
    virtual void poll() = 0;
	
    virtual INT32 getInputCount() = 0;

    virtual const CHAR* getInputName(INT32) = 0;

    virtual float getValue(INT32 enumeration) = 0;

    virtual BOOL isKeyboardDevice() = 0;

private:
    GUID guid;
};

#endif