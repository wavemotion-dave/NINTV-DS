#include <stdio.h>
#include <string.h>
#include "InputProducerManager.h"

#if defined( _WIN32 )
InputProducerManager::InputProducerManager(HWND w)
: wnd(w)
{
    DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, NULL);
}
#endif

InputProducerManager::~InputProducerManager()
{
#if defined( _WIN32 )
    directInput->Release();
#endif
}

#if defined( _WIN32 )
IDirectInput8* InputProducerManager::getDevice()
{
    return directInput;
}
#endif

InputProducer* InputProducerManager::acquireInputProducer(GUID deviceGuid)
{
    return NULL;
}

INT32 InputProducerManager::getAcquiredInputProducerCount()
{
    return (INT32)devices.size();
}

InputProducer* InputProducerManager::getAcquiredInputProducer(INT32 i)
{
    return devices[i];
}

void InputProducerManager::pollInputProducers()
{
    for (vector<InputProducer*>::iterator it = devices.begin(); it < devices.end(); it++)
        (*it)->poll();
}

void InputProducerManager::releaseInputProducers()
{
    for (vector<InputProducer*>::iterator it = devices.begin(); it < devices.end(); it++) {
#if defined( _WIN32 )
        IDirectInputDevice8* nextDevice = (*it)->getDevice();
        nextDevice->Unacquire();
        nextDevice->Release();
        delete (*it);
#endif
    }
    devices.clear();
}

