
#ifndef AY38900_H
#define AY38900_H

#include "Processor.h"
#include "MemoryBus.h"
#include "ROM.h"
#include "VideoProducer.h"
#include "AY38900_Registers.h"
#include "MOB.h"
#include "BackTabRAM.h"
#include "GRAM.h"
#include "GROM.h"

#define AY38900_PIN_IN_SST 0
#define AY38900_PIN_OUT_SR1 0
#define AY38900_PIN_OUT_SR2 1

TYPEDEF_STRUCT_PACK( _AY38900State
{
    BackTabRAMState backtab;
    MOBState        mobs[8];
    INT32           horizontalOffset;
    INT32           verticalOffset;
    INT32           mode;
    UINT16          registers[0x40];
    INT8            inVBlank;
    INT8            previousDisplayEnabled;
    INT8            displayEnabled;
    INT8            colorStackMode;
    UINT8           borderColor;
    INT8            blockLeft;
    INT8            blockTop;
    UINT8           _pad[1];
} AY38900State; )


class AY38900 : public Processor, public VideoProducer
{

    friend class AY38900_Registers;

public:
	AY38900(MemoryBus* mb, GROM* go, GRAM* ga);

    /**
     * Implemented from the Processor interface.
     * Returns the clock speed of the AY-3-8900, currently hardcoded to the NTSC clock
     * rate of 3.579545 Mhz.
     */
	INT32 getClockSpeed() { return 3579545; }

    /**
     * Implemented from the Processor interface.
     */
	void resetProcessor();
    
  	void getState(AY38900State *state);
	void setState(AY38900State *state);

    /**
     * Implemented from the Processor interface.
     */
    INT32 tick(INT32);

    /**
     * Implemented from the VideoProducer interface.
     */
	void setPixelBuffer(UINT8* pixelBuffer, UINT32 rowSize);

    /**
     * Implemented from the VideoProducer interface.
     */
    void render();

    //registers
    AY38900_Registers registers;
    BackTabRAM         backtab;

private:
	void setGraphicsBusVisible(BOOL visible);
	void renderFrame();
	BOOL somethingChanged();
	void markClean();
	void renderBorders();
	void renderMOBs();
	void renderBackground();
	void renderForegroundBackgroundMode();
	void renderColorStackMode();
	void copyBackgroundBufferToStagingArea();
	void copyMOBsToStagingArea();
	void renderLine(UINT8 nextByte, INT32 x, INT32 y, UINT8 fgcolor, UINT8 bgcolor);
	void renderColoredSquares(INT32 x, INT32 y, UINT8 color0, UINT8 color1, UINT8 color2, UINT8 color3);
	void determineMOBCollisions();
	BOOL mobsCollide(INT32 mobNum0, INT32 mobNum1);
    BOOL mobCollidesWithBorder(int mobNum);
    BOOL mobCollidesWithForeground(int mobNum);

    MemoryBus*      memoryBus;

    MOB             mobs[8];
    UINT8           backgroundBuffer[160*96];

    UINT8*          pixelBuffer;
    UINT32          pixelBufferRowSize;

    //memory listeners, for optimizations
    GROM*           grom;
    GRAM*           gram;

    //state info
    BOOL            inVBlank;
    INT32           mode;
    BOOL            previousDisplayEnabled;
    BOOL            displayEnabled;
    BOOL            colorStackMode;
    BOOL            colorModeChanged;
    BOOL            bordersChanged;
    BOOL            colorStackChanged;
    BOOL            offsetsChanged;
    BOOL            imageBufferChanged;

    //register info
    UINT8   borderColor;
    BOOL    blockLeft;
    BOOL    blockTop;
    INT32   horizontalOffset;
    INT32   verticalOffset;
};

#endif
