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
 
#ifndef ROM_H
#define ROM_H

#include "Memory.h"
#include "types.h"

class ROM : public Memory
{

public:
    ROM(const CHAR* name, const CHAR* defaultFilename, UINT32 defaultFileOffset, UINT8 byteWidth, UINT16 size, UINT16 location, BOOL internal = FALSE);
    ROM(const CHAR* name, void* image, UINT8 byteWidth, UINT16 size, UINT16 location, UINT16 readAddressMask = 0xFFFF);
    virtual ~ROM();

    const CHAR* getName();
    const CHAR* getDefaultFileName();
    UINT32 getDefaultFileOffset();
    BOOL load(const CHAR* filename);
    BOOL load(const CHAR* filename, UINT32 offset);
    BOOL load(void* image);
	BOOL isLoaded() { return loaded; }
    BOOL isInternal() { return internal; }

    //enabled attributes
    void SetEnabled(BOOL b) {enabled = b;}
    BOOL IsEnabled() { return enabled; }

    //functions to implement the Memory interface
	virtual void reset() {}
    UINT8  getByteWidth();

    UINT16 getReadSize();
    UINT16 getReadAddress();
    UINT16 getReadAddressMask();
    
    inline UINT8 peek8(UINT16 location) 
    {   
        if (unlikely(!enabled)) return 0xFF;
        return ((UINT8*)image)[(location) - this->location];
    }
    
    // This is now optimized for 16-bit access... GROM and iVOICE ROM should always call the peek8() version above...
    inline virtual UINT16 peek(UINT16 location) 
    {   
        if (unlikely(!enabled)) return 0xFFFF;
        return ((UINT16*)image)[(location) - this->location];
    }

    UINT16 getWriteSize();
    UINT16 getWriteAddress();
    UINT16 getWriteAddressMask();
    virtual void poke(UINT16 location, UINT16 value);
    virtual void poke_cheat(UINT16 location, UINT16 value)
    {
        ((UINT16*)image)[(location) - this->location] = value;
    }

private:
    void Initialize(const CHAR* n, const CHAR* f, UINT32 o, UINT8 byteWidth, UINT16 size, UINT16 location, UINT16 readMask);

    CHAR*    name;
    CHAR*    filename;
    UINT32   fileoffset;

    UINT8*    image;
    UINT8    byteWidth;
    UINT8    multiByte;
    BOOL     enabled;
    UINT16   size;
    UINT16   location;
    UINT16   readAddressMask;
	BOOL	 loaded;
	BOOL	 internal;

};

#endif
