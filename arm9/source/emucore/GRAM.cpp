#include <nds.h>
#include "GRAM.h"

#define GRAM_ADDRESS    0x3800
#define GRAM_READ_MASK  0xF9FF
#define GRAM_WRITE_MASK 0x39FF

UINT8     gram_image[GRAM_SIZE]         __attribute__((section(".dtcm")));
UINT8     dirtyCards[GRAM_SIZE>>3]      __attribute__((section(".dtcm")));
UINT8     dirtyRAM                      __attribute__((section(".dtcm")));

GRAM::GRAM()
: RAM(GRAM_SIZE, GRAM_ADDRESS, GRAM_READ_MASK, GRAM_WRITE_MASK)
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

RAMState GRAM::getState(UINT16* image)
{
    RAMState state = {0};
#if 0
	state = RAM::getState(NULL);

	if (image != NULL) {
		this->getImage(image, 0, this->getImageByteSize());
	}
#endif
	return state;
}

void GRAM::setState(RAMState state, UINT16* image)
{
#if 0
    RAM::setState(state, NULL);

	if (image != NULL) {
		this->setImage(image, 0, this->getImageByteSize());
	}

	memset(this->dirtyCards, TRUE, sizeof(this->dirtyCards));
	this->dirtyRAM = TRUE;
#endif    
}
