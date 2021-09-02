
#ifndef INPUTPRODUCER_H
#define INPUTPRODUCER_H

// TODO: jeremiah sypult cross-platform
#if defined( _WIN32 )
#include <dinput.h>
#endif

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