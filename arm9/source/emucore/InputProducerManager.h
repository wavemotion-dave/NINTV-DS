
#ifndef INPUTPRODUCERMANAGER_H
#define INPUTPRODUCERMANAGER_H

// TODO: jeremiah sypult cross-platform
#if defined( _WIN32 )
#include <windows.h>
#include <dinput.h>
#endif

#include "InputProducer.h"
#include <vector>
using namespace std;

class InputProducerManager
{

public:
#if defined( _WIN32 )
    InputProducerManager(HWND wnd);
#endif
    virtual ~InputProducerManager();
#if defined( _WIN32 )
    IDirectInput8* getDevice();
#endif
    InputProducer* acquireInputProducer(GUID deviceGuid);
    INT32 getAcquiredInputProducerCount();
    InputProducer* getAcquiredInputProducer(INT32 i);
	
	void pollInputProducers();
	
	void releaseInputProducers();

private:
#if defined( _WIN32 )
    HWND wnd;
    IDirectInput8* directInput;
#endif
    vector<InputProducer*> devices;

};

#endif