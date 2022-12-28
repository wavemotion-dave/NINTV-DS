// =====================================================================================
// Copyright (c) 2021-2023 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef RAM_H
#define RAM_H

#include <string.h>
#include "Memory.h"

extern UINT16 fast_ram16[];
extern UINT16 *slow_ram16;
extern UINT16 *ecs_ram8;
extern UINT16 *jlp_ram;
extern UINT16 fast_ram16_idx;
extern UINT16 slow_ram16_idx;

TYPEDEF_STRUCT_PACK( _RAMState
{
    INT8    enabled;
    UINT8   bitWidth;
    UINT16  size;
    UINT16  location;
    UINT16  readAddressMask;
    UINT16  writeAddressMask;
    UINT16  trimmer;
    UINT16  image[0x300];
} RAMState; )
    
    
TYPEDEF_STRUCT_PACK( _SlowRAMState
{
    UINT16     image[0x4000];
} SlowRAMState; )
    
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
        virtual UINT16 peek(UINT16 location) {return image[(location&readAddressMask) - this->location];}

        UINT16 getWriteSize();
        UINT16 getWriteAddress();
        UINT16 getWriteAddressMask();
        virtual void poke(UINT16 location, UINT16 value);
        virtual void poke_cheat(UINT16 location, UINT16 value);

        void getState(RAMState *state);
        void setState(RAMState *state);

        //enabled attributes
        void SetEnabled(BOOL b);
        BOOL IsEnabled() { return enabled; }

    protected:
        UINT8   enabled;
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
