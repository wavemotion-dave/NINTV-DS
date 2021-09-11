#include <nds.h>
#include "GRAM.h"

#define GRAM_ADDRESS    0x3800
#define GRAM_READ_MASK  0xF9FF
#define GRAM_WRITE_MASK 0x39FF

UINT8     gram_image[GRAM_SIZE]         __attribute__((section(".dtcm")));
UINT8     dirtyCards[GRAM_SIZE>>3]      __attribute__((section(".dtcm")));
UINT8     dirtyRAM                      __attribute__((section(".dtcm")));

GRAM::GRAM()
: RAM(GRAM_SIZE, GRAM_ADDRESS, GRAM_READ_MASK, GRAM_WRITE_MASK, 16)
{}

void GRAM::reset()
{
    UINT16 i;
    dirtyRAM = TRUE;
    for (i = 0; i < GRAM_SIZE; i++)
        gram_image[i] = 0;

    for (i = 0; i < 0x40; i++)
        dirtyCards[i] = TRUE;
}

void GRAM::poke(UINT16 location, UINT16 value)
{
    location &= 0x01FF;
    //value &= 0xFF;

    gram_image[location] = (UINT8)value;
    dirtyCards[location>>3] = TRUE;
    dirtyRAM = TRUE;
}

void GRAM::markClean() {
    if (!dirtyRAM)
        return;

    for (UINT16 i = 0; i < 0x40; i++)
        dirtyCards[i] = FALSE;
    dirtyRAM = FALSE;
}

BOOL GRAM::isDirty() {
    return dirtyRAM;
}

BOOL GRAM::isCardDirty(UINT16 cardLocation) {
    return dirtyCards[cardLocation>>3];
}

