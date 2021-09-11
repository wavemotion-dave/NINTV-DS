
#ifndef GRAM_H
#define GRAM_H

#include "RAM.h"

#define GRAM_SIZE       0x0200

extern UINT8     gram_image[GRAM_SIZE];
extern UINT8     dirtyCards[GRAM_SIZE>>3];
extern UINT8     dirtyRAM;

class GRAM : public RAM
{
    friend class AY38900;

    public:
        GRAM();

        void reset();
        inline UINT16 peek(UINT16 location) {return gram_image[location & 0x01FF];}
        void poke(UINT16 location, UINT16 value);

        void markClean();
        BOOL isDirty();
        BOOL isCardDirty(UINT16 cardLocation);
       
    private:
};

#endif
