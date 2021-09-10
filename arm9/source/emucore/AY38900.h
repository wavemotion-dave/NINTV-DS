
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

#define AY38900_PIN_IN_SST 0
#define AY38900_PIN_OUT_SR1 0
#define AY38900_PIN_OUT_SR2 1

TYPEDEF_STRUCT_PACK( _AY38900State
{
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
	AY38900(MemoryBus* mb, ROM* go, GRAM* ga);

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

	AY38900State getState();
	void setState(AY38900State state);

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
	//void renderRow(INT32 rowNum);

	const static INT32 TICK_LENGTH_SCANLINE;
    const static INT32 TICK_LENGTH_FRAME;
    const static INT32 TICK_LENGTH_VBLANK;
    const static INT32 TICK_LENGTH_START_ACTIVE_DISPLAY;
    const static INT32 TICK_LENGTH_IDLE_ACTIVE_DISPLAY;
    const static INT32 TICK_LENGTH_FETCH_ROW;
    const static INT32 TICK_LENGTH_RENDER_ROW;
    const static INT32 LOCATION_BACKTAB;
    const static INT32 LOCATION_GROM;
    const static INT32 LOCATION_GRAM;
    const static INT32 LOCATION_COLORSTACK;
    const static INT32 FOREGROUND_BIT;

    MemoryBus*      memoryBus;

    MOB             mobs[8];
    UINT8           backgroundBuffer[160*96];

    UINT8*          pixelBuffer;
    UINT32          pixelBufferRowSize;

    //memory listeners, for optimizations
    ROM*            grom;
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

/*
#ifndef AY38900_H
#define AY38900_H

#include "AY38900_Registers.h"
#include "BackTabRAM.h"
#include "GRAM.h"
#include "GROM.h"
#include "MOB.h"
#include "VideoProducer.h"
#include "core/types.h"
#include "core/cpu/Processor.h"
#include "core/cpu/SignalLine.h"
#include "core/memory/MemoryBus.h"
#include "services/VideoOutputDevice.h"

#define AY38900_PIN_IN_SST 0

#define AY38900_PIN_OUT_SR1 0
#define AY38900_PIN_OUT_SR2 1

class AY38900 : public Processor, public VideoProducer
{

    friend class AY38900_Registers;

    public:
        AY38900(MemoryBus* memoryBus);
        void setGROMImage(UINT16* gromImage);
        BOOL inVerticalBlank();

        //Processor functions
        void initProcessor();
        void releaseProcessor();
		void setVideoOutputDevice(IDirect3DDevice9* vod);
        INT32 getClockSpeed();
        INT32 tick();

        //VideoProducer functions
        void render();

        //registers
        AY38900_Registers registers;

        static const INT32 TICK_LENGTH_SCANLINE;
        static const INT32 TICK_LENGTH_FRAME;
        static const INT32 TICK_LENGTH_VBLANK;
        static const INT32 TICK_LENGTH_START_ACTIVE_DISPLAY;
        static const INT32 TICK_LENGTH_IDLE_ACTIVE_DISPLAY;
        static const INT32 TICK_LENGTH_FETCH_ROW;
        static const INT32 TICK_LENGTH_RENDER_ROW;
        static const INT32 LOCATION_BACKTAB;
        static const INT32 LOCATION_GROM;
        static const INT32 LOCATION_GRAM;
        static const INT32 LOCATION_COLORSTACK;
        static const INT32 FOREGROUND_BIT;

    private:
        void setGraphicsBusVisible(BOOL visible);
        void renderFrame();
        BOOL somethingChanged();
        void markClean();
        void renderBorders();
        void renderMOBs();
        void renderBackground();
        void copyBackgroundBufferToStagingArea();
        void renderForegroundBackgroundMode();
        void renderColorStackMode();
        void copyMOBsToStagingArea();
        void renderLine(UINT8 nextUINT8, INT32 x, INT32 y,
                UINT8 fgcolor, UINT8 bgcolor);
        void renderColoredSquares(INT32 x, INT32 y, UINT8 color0, UINT8 color1,
                UINT8 color2, UINT8 color3);
        void renderMessage();
        void renderCharacter(char c, INT32 x, INT32 y);
        void determineMOBCollisions();
        BOOL mobsCollide(INT32 mobNum0, INT32 mobNum1);

        BOOL                    mobBuffers[8][16][128];
        MOB                     mobs[8];
        UINT8                   backgroundBuffer[160*96];
        IDirect3DDevice9*       videoOutputDevice;
        IDirect3DTexture9*      combinedTexture;
        IDirect3DVertexBuffer9* vertexBuffer;
        D3DLOCKED_RECT          combinedBufferLock;

        MemoryBus* memoryBus;

        //memory listeners, for optimizations
        BackTabRAM        backtab;
        GROM              grom;
        GRAM              gram;

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

        //palette
        const static UINT32 palette[32];

};

#endif
*/
