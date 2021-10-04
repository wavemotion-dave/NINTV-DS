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

#ifndef BACKTABRAM_H
#define BACKTABRAM_H

#include "RAM.h"

#define BACKTAB_SIZE     0xF0
#define BACKTAB_LOCATION 0x200

TYPEDEF_STRUCT_PACK(_BackTabRAMState
{
    UINT16       image[BACKTAB_SIZE];
    BOOL         dirtyBytes[BACKTAB_SIZE];
    BOOL         dirtyRAM;
    BOOL         colorAdvanceBitsDirty;
} BackTabRAMState;)

class BackTabRAM : public RAM
{
    public:
        BackTabRAM();
        void reset();

        UINT16 peek(UINT16 location);
        void poke(UINT16 location, UINT16 value);
        UINT16 peek_direct(UINT16 offset) { return image[offset]; }

        BOOL areColorAdvanceBitsDirty();
        BOOL isDirty(UINT16 location);
        BOOL isDirtyCache(UINT16 location);
        void markClean();
        void markCleanCache();
        
        void getState(BackTabRAMState *state);
        void setState(BackTabRAMState *state);

    private:
        UINT16       image[BACKTAB_SIZE];
        BOOL         dirtyBytes[BACKTAB_SIZE];
        BOOL         dirtyRAM;
        BOOL         colorAdvanceBitsDirty;
};

#endif
