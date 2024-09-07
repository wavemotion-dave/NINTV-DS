// =====================================================================================
// Copyright (c) 2021-2024 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef GRAM_H
#define GRAM_H

#include "RAM.h"

#define GRAM_ADDRESS        0x3800  // GRAM base address (mirros handled with READ/WRITE masks below)

#define GRAM_SIZE           0x0800  // Max of 2K GRAM even though most of the time we will only use the normal 512 byte version
#define GRAM_READ_MASK      0xFFFF  // To allow for a full 2K GRAM even though the actual GRAM MASK might restrict
#define GRAM_WRITE_MASK     0x3FFF  // To allow for a full 2K GRAM even though the actual GRAM MASK might restrict 

extern UINT16 GRAM_MASK;
extern UINT16 GRAM_COL_STACK_MASK;
extern UINT16 GRAM_CARD_MOB_MASK;

extern UINT8     gram_image[GRAM_SIZE];
extern UINT8     dirtyCards[GRAM_SIZE>>3];
extern UINT8     dirtyRAM;

TYPEDEF_STRUCT_PACK( _GRAMState
{
    UINT8     gram_image[GRAM_SIZE];
    UINT8     dirtyCards[GRAM_SIZE>>3];
    UINT8     dirtyRAM;
} GRAMState; )

    
class GRAM : public RAM
{
    friend class AY38900;

    public:
        GRAM();

        void reset();
        inline UINT16 peek(UINT16 location) {return gram_image[location & GRAM_MASK];}
        void poke(UINT16 location, UINT16 value);

        void markClean();
        inline BOOL isDirty() {return dirtyRAM;}
        inline BOOL isCardDirty(UINT16 cardLocation) {return dirtyCards[cardLocation>>3];}

        void getState(GRAMState *state);
        void setState(GRAMState *state);
        
    private:
};

#endif
