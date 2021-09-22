
#ifndef RAM_H
#define RAM_H

#include <string.h>
#include "Memory.h"

TYPEDEF_STRUCT_PACK( _RAMState
{
    INT8    enabled;
    UINT8   bitWidth;
    UINT16  size;
    UINT16  location;
    UINT16  readAddressMask;
    UINT16  writeAddressMask;
    UINT16  trimmer;
    UINT16  image[0x400];
} RAMState; )
    
    
class RAM : public Memory
{
    public:
        RAM(UINT16 size, UINT16 location, UINT16 readAddressMask, UINT16 writeAddressMask, UINT8 bitWidth=16);
        virtual ~RAM();

        virtual void reset();
        UINT8  getBitWidth();

        UINT16 getReadSize();
        UINT16 getReadAddress();
        UINT16 getReadAddressMask();
        virtual UINT16 peek(UINT16 location);

        UINT16 getWriteSize();
        UINT16 getWriteAddress();
        UINT16 getWriteAddressMask();
        virtual void poke(UINT16 location, UINT16 value);

        void getState(RAMState *state);
        void setState(RAMState *state);

        //enabled attributes
        void SetEnabled(BOOL b);
        BOOL IsEnabled() { return enabled; }

    protected:
        BOOL    enabled;
        UINT16  size;
        UINT16  location;
        UINT16  readAddressMask;
        UINT16  writeAddressMask;

    private:
        UINT8   bitWidth;
        UINT16  trimmer;
        UINT16* image;
};

#endif
