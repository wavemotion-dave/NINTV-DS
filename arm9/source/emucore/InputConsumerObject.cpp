#include <stdio.h>
#include <string.h>

// TODO: jeremiah sypult cross-platform
#if defined( _WIN32 )
#include <dinput.h>
#endif

#include "InputConsumerObject.h"

InputConsumerObject::InputConsumerObject(INT32 i, const CHAR* n, GUID ddg, INT32 doid)
: id(i),
  name(n),
  defaultDeviceGuid(ddg),
  defaultObjectID(doid),
  bindingCount(0)
{
    memset(producerBindings, 0, sizeof(producerBindings));
    memset(objectIDBindings, 0, sizeof(objectIDBindings));
    memset(subBindingCounts, 0, sizeof(subBindingCounts));
}

InputConsumerObject::~InputConsumerObject()
{
    clearBindings();
}

void InputConsumerObject::addBinding(InputProducer** producers, INT32* objectids, INT32 count)
{
    if (bindingCount >= MAX_BINDINGS)
        return;

    producerBindings[bindingCount] = new InputProducer*[count];
    memcpy(producerBindings[bindingCount], producers, sizeof(InputProducer*)*count);
    objectIDBindings[bindingCount] = new INT32[count];
    memcpy(objectIDBindings[bindingCount], objectids, sizeof(INT32)*count);
    subBindingCounts[bindingCount] = count;
    bindingCount++;
}

void InputConsumerObject::clearBindings()
{
    for (INT32 i = 0; i < bindingCount; i++) {
        delete[] producerBindings[i];
        delete[] objectIDBindings[i];
    }
    memset(producerBindings, 0, sizeof(producerBindings));
    memset(objectIDBindings, 0, sizeof(objectIDBindings));
    memset(subBindingCounts, 0, sizeof(subBindingCounts));
    bindingCount = 0;
}

float InputConsumerObject::getInputValue()
{
    float value = 0.0f;
    for (INT32 i = 0; i < bindingCount; i++) {
        float nextValue = 1.0f;
        for (INT32 j = 0; j < subBindingCounts[i]; j++) {
            float v = producerBindings[i][j]->getValue(objectIDBindings[i][j]);
            nextValue = (nextValue < v ? nextValue : v);
        }
        value = (value != 0 && value > nextValue ? value : nextValue);
    }
    return value;
}
