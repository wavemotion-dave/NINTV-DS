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

#ifndef INPUTCONSUMEROBJECT_H
#define INPUTCONSUMEROBJECT_H

#include "InputProducer.h"
#include "types.h"

#define MAX_BINDINGS 10

class InputConsumerObject
{
    friend class InputConsumer;

public:
	InputConsumerObject(INT32 id, const CHAR* name, GUID defaultDeviceGuid, INT32 defaultObjectID);
    virtual ~InputConsumerObject();

    INT32 getId() { return id; }

    const CHAR* getName() { return name; }
	
    GUID getDefaultDeviceGuid() { return defaultDeviceGuid; }
	
    INT32 getDefaultEnum() { return defaultObjectID; }
    
    INT32 getBindingCount() { return bindingCount; }
	
    INT32 getSubBindingCount(INT32 i) { return subBindingCounts[i]; }

    InputProducer* getSubBindingProducer(INT32 i, INT32 j) { return producerBindings[i][j]; }

    INT32 getSubBindingEnum(INT32 i, INT32 j) { return objectIDBindings[i][j]; }

    void addBinding(InputProducer** producer, INT32* objectid, INT32 count);

    void clearBindings();
	
	float getInputValue();

private:
    INT32 id;
  	const CHAR* name;
    GUID defaultDeviceGuid;
    INT32  defaultObjectID;
	
    InputProducer** producerBindings[MAX_BINDINGS];
    INT32*          objectIDBindings[MAX_BINDINGS];
    INT32           subBindingCounts[MAX_BINDINGS];
    INT32           bindingCount;
	
};

#endif