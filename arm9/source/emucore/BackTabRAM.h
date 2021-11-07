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

extern UINT16  bt_image[BACKTAB_SIZE];
extern UINT16  bt_imageLatched[BACKTAB_SIZE];

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

        inline UINT16 peek(UINT16 location) {return bt_image[location-BACKTAB_LOCATION];}
        void poke(UINT16 location, UINT16 value);
        inline UINT16 peek_direct(UINT16 offset) { return bt_image[offset]; }

        inline BOOL areColorAdvanceBitsDirty() {return colorAdvanceBitsDirty;}
        inline BOOL isDirty(UINT16 location) {return dirtyBytes[location-BACKTAB_LOCATION];}
        inline BOOL isDirtyDirect(UINT16 location) {return dirtyBytes[location];}
        void markClean();

        void LatchRow(UINT8 row);
        void markCleanLatched();
        inline BOOL isDirtyLatched(UINT16 location) { return dirtyBytesLatched[location];}
        inline UINT16 peek_latched(UINT16 offset) { return bt_imageLatched[offset]; }
        
        void getState(BackTabRAMState *state);
        void setState(BackTabRAMState *state);

    private:
        UINT8        dirtyBytes[BACKTAB_SIZE];
        UINT8        dirtyRAM;
        UINT8        colorAdvanceBitsDirty;
        UINT8        dirtyBytesLatched[BACKTAB_SIZE];
};

#endif
