
#include "BackTabRAM.h"

BackTabRAM::BackTabRAM()
: RAM(BACKTAB_SIZE, BACKTAB_LOCATION, 0xFFFF, 0xFFFF)
{}

void BackTabRAM::reset()
{
    dirtyRAM = TRUE;
    colorAdvanceBitsDirty = TRUE;
    for (UINT16 i = 0; i < BACKTAB_SIZE; i++) {
        image[i] = 0;
        dirtyBytes[i] = TRUE;
    }
}

UINT16 BackTabRAM::peek(UINT16 location)
{
    return image[location-BACKTAB_LOCATION];
}

void BackTabRAM::poke(UINT16 location, UINT16 value)
{
    value &= 0xFFFF;
    location -= BACKTAB_LOCATION;

    if (image[location] == value)
        return;

    if ((image[location] & 0x2000) != (value & 0x2000))
        colorAdvanceBitsDirty = TRUE;

    image[location] = value;
    dirtyBytes[location] = TRUE;
    dirtyRAM = TRUE;
}

void BackTabRAM::markClean() {
    if (!dirtyRAM)
        return;

    for (UINT16 i = 0; i < BACKTAB_SIZE; i++)
        dirtyBytes[i] = FALSE;
    dirtyRAM = FALSE;
    colorAdvanceBitsDirty = FALSE;
}

BOOL BackTabRAM::areColorAdvanceBitsDirty() {
    return colorAdvanceBitsDirty;
}

BOOL BackTabRAM::isDirty() {
    return dirtyRAM;
}

BOOL BackTabRAM::isDirty(UINT16 location) {
    return dirtyBytes[location-BACKTAB_LOCATION];
}

BackTabRAMState BackTabRAM::getState()
{
	BackTabRAMState state = {0};

	state.RAMState = RAM::getState(state.image);
	this->getImage(state.image, 0, this->getImageByteSize());

	return state;
}

void BackTabRAM::setState(BackTabRAMState state, UINT16* image)
{
	memset(this->dirtyBytes, TRUE, sizeof(this->dirtyBytes));
	RAM::setState(state.RAMState, NULL);
	this->setImage(state.image, 0, this->getImageByteSize());

	this->dirtyRAM = TRUE;
	this->colorAdvanceBitsDirty = TRUE;
}
