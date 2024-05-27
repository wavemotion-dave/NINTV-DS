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

//#define GRAM_2K   // Enable this line if you want to experiment with a 2K GRAM build

#ifdef GRAM_2K

#define GRAM_SIZE           0x0800  // 2K of GRAM is non-standard but possible!
#define GRAM_READ_MASK      0xFFFF  // This is what produces the GRAM read aliases due to incomplete address decoding
#define GRAM_WRITE_MASK     0x3FFF  // This is what produces the GRAM write aliases due to incomplete address decoding
#define GRAM_MASK           0x07FF  // Allows indexing all 2K of GRAM
#define GRAM_COL_STACK_MASK 0x07F8  // Allows for indexing 256 tiles in Color Stack mode
#define GRAM_CARD_MOB_MASK  0xFF    // Allows for indexing 256 tiles for MOBs (technically should only be allowed in Color Stack but whatevs...)

#else // Normal 512b GRAM

#define GRAM_SIZE           0x0200  // 512 bytes of GRAM
#define GRAM_READ_MASK      0xF9FF  // This is what produces the GRAM read aliases due to incomplete address decoding
#define GRAM_WRITE_MASK     0x39FF  // This is what produces the GRAM write aliases due to incomplete address decoding
#define GRAM_MASK           0x01FF  // Allows indexing the 512 bytes of GRAM
#define GRAM_COL_STACK_MASK 0x01F8  // Allows for indexing 64 tiles in Color Stack mode
#define GRAM_CARD_MOB_MASK  0x3F    // Allows for indexing 64 tiles for MOBs

#endif // GRAM_2K

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
