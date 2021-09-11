 
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
    void SetEnabled(BOOL b);
    BOOL IsEnabled() { return enabled; }

    //functions to implement the Memory interface
	virtual void reset() {}
    UINT8  getByteWidth();

    UINT16 getReadSize();
    UINT16 getReadAddress();
    UINT16 getReadAddressMask();
    
    inline UINT8 peek8(UINT16 location) 
    {   
        return ((UINT8*)image)[(location) - this->location];
    }
    
    // This is now optimized for 16-bit access... GROM and iVOICE ROM should always call the peek8() version above...
    inline virtual UINT16 peek(UINT16 location) 
    {   
        return ((UINT16*)image)[(location) - this->location];
    }

    UINT16 getWriteSize();
    UINT16 getWriteAddress();
    UINT16 getWriteAddressMask();
    virtual void poke(UINT16 location, UINT16 value);

private:
    void Initialize(const CHAR* n, const CHAR* f, UINT32 o, UINT8 byteWidth, UINT16 size, UINT16 location, UINT16 readMask);

    UINT16 (ROM::*peekFunc)(UINT16); 
    UINT16 peek1(UINT16 location);
    UINT16 peek2(UINT16 location);
    UINT16 peek4(UINT16 location);
    UINT16 peekN(UINT16 location);

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
