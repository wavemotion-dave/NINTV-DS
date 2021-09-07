#include <nds.h>
#include "RAM.h"

UINT16 fast_ram[4096] __attribute__((section(".dtcm")));
UINT16 fast_ram_idx = 0;

RAM::RAM(UINT16 size, UINT16 location)
: enabled(TRUE)
{
    this->size = size;
    this->location = location;
    this->readAddressMask = 0xFFFF;
    this->writeAddressMask = 0xFFFF;
    this->bitWidth = sizeof(UINT16)<<3;
    this->trimmer = (UINT16)((1 << (sizeof(UINT16) << 3)) - 1);
    image = &fast_ram[fast_ram_idx];
    fast_ram_idx += size;
}

RAM::RAM(UINT16 size, UINT16 location, UINT8 bitWidth)
: enabled(TRUE)
{
    this->size = size;
    this->location = location;
    this->readAddressMask = 0xFFFF;
    this->writeAddressMask = 0xFFFF;
    this->bitWidth = bitWidth;
    this->trimmer = (UINT16)((1 << bitWidth) - 1);
    image = &fast_ram[fast_ram_idx];
    fast_ram_idx += size;
}

RAM::RAM(UINT16 size, UINT16 location, UINT16 readAddressMask, UINT16 writeAddressMask)
: enabled(TRUE)
{
    this->size = size;
    this->location = location;
    this->readAddressMask = readAddressMask;
    this->writeAddressMask = writeAddressMask;
    this->bitWidth = sizeof(UINT16)<<3;
    this->trimmer = (UINT16)((1 << bitWidth) - 1);
    image = &fast_ram[fast_ram_idx];
    fast_ram_idx += size;
}

RAM::RAM(UINT16 size, UINT16 location, UINT16 readAddressMask, UINT16 writeAddressMask, UINT8 bitWidth)
: enabled(TRUE)
{
    this->size = size;
    this->location = location;
    this->readAddressMask = readAddressMask;
    this->writeAddressMask = writeAddressMask;
    this->bitWidth = bitWidth;
    this->trimmer = (UINT16)((1 << bitWidth) - 1);
    image = &fast_ram[fast_ram_idx];
    fast_ram_idx += size;
}

RAM::~RAM()
{
    
}

void RAM::reset()
{
    enabled = TRUE;
    for (UINT16 i = 0; i < size; i++)
        image[i] = 0;
}

void RAM::SetEnabled(BOOL b)
{
    enabled = b;
}

UINT8 RAM::getBitWidth()
{
    return bitWidth;
}

UINT16 RAM::getReadSize()
{
    return size;
}

UINT16 RAM::getReadAddress()
{
    return location;
}

UINT16 RAM::getReadAddressMask()
{
    return readAddressMask;
}

UINT16 RAM::getWriteSize()
{
    return size;
}

UINT16 RAM::getWriteAddress()
{
    return location;
}

UINT16 RAM::getWriteAddressMask()
{
    return writeAddressMask;
}

UINT16 RAM::peek(UINT16 location)
{
    return image[(location&readAddressMask)-this->location];
}

void RAM::poke(UINT16 location, UINT16 value)
{
    image[(location&writeAddressMask)-this->location] = (value & trimmer);
}

RAMState RAM::getState(UINT16* image)
{
	RAMState state = {0};

	state.enabled = this->enabled;
	state.size = this->size;
	state.location = this->location;
	state.readAddressMask = this->readAddressMask;
	state.writeAddressMask = this->writeAddressMask;
	state.bitWidth = this->bitWidth;
	state.trimmer = this->trimmer;

	if (image != NULL) {
		this->getImage(image, 0, this->getImageByteSize());
	}

	return state;
}

void RAM::setState(RAMState state, UINT16* image)
{
	this->enabled = state.enabled;
	this->size = state.size;
	this->location = state.location;
	this->readAddressMask = state.readAddressMask;
	this->writeAddressMask = state.writeAddressMask;
	this->bitWidth = state.bitWidth;
	this->trimmer = state.trimmer;

	if (image != NULL) {
		this->setImage(image, 0, this->getImageByteSize());
	}
}
