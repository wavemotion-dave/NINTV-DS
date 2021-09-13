#include <stdio.h>
#include <string.h>
#include "ROM.h"
#include "CRC32.h"

ROM::ROM(const CHAR* n, const CHAR* f, UINT32 o, UINT8 byteWidth, UINT16 size, UINT16 location, BOOL i)
: enabled(TRUE),
  loaded(FALSE),
  internal(i)
{
    Initialize(n, f, o, byteWidth, size, location, 0xFFFF);
}

ROM::ROM(const CHAR* n, void* image, UINT8 byteWidth, UINT16 size, UINT16 location, UINT16 readAddressMask)
: enabled(TRUE),
  loaded(TRUE),
  internal(FALSE)
{
    Initialize(n, "", 0, byteWidth, size, location, readAddressMask);
    memcpy(this->image, image, size*byteWidth);
}

void ROM::Initialize(const CHAR* n, const CHAR* f, UINT32 o, UINT8 byteWidth, UINT16 size, UINT16 location, UINT16 readMask)
{
    name = new CHAR[strlen(n)+1];
    strcpy(name, n);
    filename = new CHAR[strlen(f)+1];
    strcpy(filename, f);
    fileoffset = o;
	this->byteWidth = byteWidth;
    this->multiByte = (byteWidth > 1) ? 1:0;
    this->size = size;
    this->location = location;
    this->readAddressMask = readMask;
    this->image = new UINT8[size*byteWidth];
}

ROM::~ROM()
{
    if (name)
        delete[] name;
    if (filename)
        delete[] filename;
    if (image)
        delete[] image;
}

BOOL ROM::load(const char* filename)
{
    return load(filename, 0);
}

BOOL ROM::load(const char* filename, UINT32 offset)
{
	FILE* f = fopen(filename, "rb");
    if (!f)
        return FALSE;

    fseek(f, offset, SEEK_SET);
    int byteCount = size*byteWidth;
	int totalSize = 0;
	while (totalSize < byteCount) {
        for (UINT8 k = 0; k < byteWidth; k++)
		    ((UINT8*)image)[totalSize+(byteWidth-k-1)] = (UINT8)fgetc(f);
		totalSize += byteWidth;
	}
	fclose(f);
	loaded = TRUE;
    
    return TRUE;
}

BOOL ROM::load(void* buffer)
{
    UINT8* byteImage = (UINT8*)buffer;
    int byteCount = size*byteWidth;
	int totalSize = 0;
	while (totalSize < byteCount) {
        for (UINT8 k = 0; k < byteWidth; k++)
		    ((UINT8*)image)[totalSize+(byteWidth-k-1)] = byteImage[totalSize+k];
		totalSize += byteWidth;
	}

	loaded = TRUE;
    return TRUE;
}

void ROM::SetEnabled(BOOL b)
{
    enabled = b;
}

const CHAR* ROM::getName()
{
    return name;
}

const CHAR* ROM::getDefaultFileName()
{
    return filename;
}

UINT32 ROM::getDefaultFileOffset()
{
    return fileoffset;
}

UINT16 ROM::getReadSize()
{
    return size;
}

UINT16 ROM::getReadAddress() {
    return location;
}

UINT16 ROM::getReadAddressMask()
{
    return readAddressMask;
}

UINT16 ROM::getWriteSize()
{
    return 0;
}

UINT16 ROM::getWriteAddress() {
    return 0;
}

UINT16 ROM::getWriteAddressMask()
{
    return 0;
}

UINT8 ROM::getByteWidth()
{
    return byteWidth;
}

void ROM::poke(UINT16, UINT16)
{
    //can't change ROM
}

