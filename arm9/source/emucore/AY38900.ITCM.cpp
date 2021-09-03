#include <nds.h>
//#include <crtdbg.h>
#include "AY38900.h"
#include "ProcessorBus.h"

#define MIN(v1, v2) (v1 < v2 ? v1 : v2)
#define MAX(v1, v2) (v1 > v2 ? v1 : v2)

#define MODE_VBLANK 0
#define MODE_START_ACTIVE_DISPLAY 1
#define MODE_IDLE_ACTIVE_DISPLAY 2
#define MODE_FETCH_ROW_0 3
#define MODE_RENDER_ROW_0 4
#define MODE_FETCH_ROW_1 5
#define MODE_RENDER_ROW_1 6
#define MODE_FETCH_ROW_2 7
#define MODE_RENDER_ROW_2 8
#define MODE_FETCH_ROW_3 9
#define MODE_RENDER_ROW_3 10
#define MODE_FETCH_ROW_4 11
#define MODE_RENDER_ROW_4 12
#define MODE_FETCH_ROW_5 13
#define MODE_RENDER_ROW_5 14
#define MODE_FETCH_ROW_6 15
#define MODE_RENDER_ROW_6 16
#define MODE_FETCH_ROW_7 17
#define MODE_RENDER_ROW_7 18
#define MODE_FETCH_ROW_8 19
#define MODE_RENDER_ROW_8 20
#define MODE_FETCH_ROW_9 21
#define MODE_RENDER_ROW_9 22
#define MODE_FETCH_ROW_10 23
#define MODE_RENDER_ROW_10 24
#define MODE_FETCH_ROW_11 25
#define MODE_RENDER_ROW_11 26
#define MODE_FETCH_ROW_12 27

const INT32 AY38900::TICK_LENGTH_SCANLINE             = 228;
const INT32 AY38900::TICK_LENGTH_FRAME                = 59736;
const INT32 AY38900::TICK_LENGTH_VBLANK               = 15164;
const INT32 AY38900::TICK_LENGTH_START_ACTIVE_DISPLAY = 112;
const INT32 AY38900::TICK_LENGTH_IDLE_ACTIVE_DISPLAY  = 456;
const INT32 AY38900::TICK_LENGTH_FETCH_ROW            = 456;
const INT32 AY38900::TICK_LENGTH_RENDER_ROW           = 3192;
const INT32 AY38900::LOCATION_BACKTAB    = 0x0200;
const INT32 AY38900::LOCATION_GROM       = 0x3000;
const INT32 AY38900::LOCATION_GRAM       = 0x3800;
const INT32 AY38900::FOREGROUND_BIT		 = 0x0010;

UINT16  mobBuffers[8][128] __attribute__((section(".dtcm")));
UINT8 stretch[16] __attribute__((section(".dtcm"))) = {0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F, 0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF};
UINT8 reverse[16] __attribute__((section(".dtcm"))) = {0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE, 0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF};

AY38900::AY38900(MemoryBus* mb, ROM* go, GRAM* ga)
	: Processor("AY-3-8900"),
      memoryBus(mb),
      grom(go),
      gram(ga),
	  backtab()
{
    registers.init(this);

    horizontalOffset = 0;
    verticalOffset   = 0;
    blockTop         = FALSE;
    blockLeft        = FALSE;
    mode             = 0;
}

void AY38900::resetProcessor()
{
    //switch to bus copy mode
    setGraphicsBusVisible(TRUE);

    //reset the mobs
    for (UINT8 i = 0; i < 8; i++)
        mobs[i].reset();

    //reset the state variables
    mode = -1;
    pinOut[AY38900_PIN_OUT_SR1]->isHigh = TRUE;
    pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;
    previousDisplayEnabled = TRUE;
    displayEnabled         = FALSE;
    colorStackMode         = FALSE;
    colorModeChanged       = TRUE;
    bordersChanged         = TRUE;
    colorStackChanged      = TRUE;
    offsetsChanged         = TRUE;

    //local register data
    borderColor = 0;
    blockLeft = blockTop = FALSE;
    horizontalOffset = verticalOffset = 0;
}

void AY38900::setGraphicsBusVisible(BOOL visible) {
    registers.SetEnabled(visible);
    gram->SetEnabled(visible);
    grom->SetEnabled(visible);
}

INT32 AY38900::tick(INT32 minimum) {
    INT32 totalTicks = 0;
    do {
        switch (mode) 
        {
        //start of vertical blank
        case MODE_VBLANK:
            //come out of bus isolation mode
            setGraphicsBusVisible(TRUE);
            if (previousDisplayEnabled)
                renderFrame();
            displayEnabled = FALSE;

            //start of vblank, so stop and go back to the main loop
            processorBus->stop();

            //release SR2, allowing the CPU to run
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;

            //kick the irq line
            pinOut[AY38900_PIN_OUT_SR1]->isHigh = FALSE;

            totalTicks += TICK_LENGTH_VBLANK;
            if (totalTicks >= minimum) {
                mode = MODE_START_ACTIVE_DISPLAY;
                break;
            }

        case MODE_START_ACTIVE_DISPLAY:
            pinOut[AY38900_PIN_OUT_SR1]->isHigh = TRUE;

            //if the display is not enabled, skip the rest of the modes
            if (!displayEnabled) {
                if (previousDisplayEnabled) {
                    //render a blank screen
                    for (int x = 0; x < 160*192; x++)
						pixelBuffer[x] = borderColor;
                }
                previousDisplayEnabled = FALSE;
                mode = MODE_VBLANK;
                totalTicks += (TICK_LENGTH_FRAME - TICK_LENGTH_VBLANK);
                break;
            }
            else {
                previousDisplayEnabled = TRUE;
                pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
                totalTicks += TICK_LENGTH_START_ACTIVE_DISPLAY;
                if (totalTicks >= minimum) {
                    mode = MODE_IDLE_ACTIVE_DISPLAY;
                    break;
                }
            }

        case MODE_IDLE_ACTIVE_DISPLAY:
            //switch to bus isolation mode, but only if the CPU has
            //acknowledged ~SR2 by asserting ~SST
            if (!pinIn[AY38900_PIN_IN_SST]->isHigh) {
                pinIn[AY38900_PIN_IN_SST]->isHigh = TRUE;
                setGraphicsBusVisible(FALSE);
            }

            //release SR2
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;

            totalTicks += TICK_LENGTH_IDLE_ACTIVE_DISPLAY +
                (2*verticalOffset*TICK_LENGTH_SCANLINE);
            if (totalTicks >= minimum) {
                mode = MODE_FETCH_ROW_0;
                break;
            }

        case MODE_FETCH_ROW_0:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
            //renderRow((mode-3)/2);
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_RENDER_ROW_0;
                break;
            }

        case MODE_RENDER_ROW_0:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;
            pinIn[AY38900_PIN_IN_SST]->isHigh = TRUE;
            totalTicks += TICK_LENGTH_RENDER_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_FETCH_ROW_1;
                break;
            }

        case MODE_FETCH_ROW_1:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
            //renderRow((mode-3)/2);
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_RENDER_ROW_1;
                break;
            }

        case MODE_RENDER_ROW_1:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;
            pinIn[AY38900_PIN_IN_SST]->isHigh = TRUE;
            totalTicks += TICK_LENGTH_RENDER_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_FETCH_ROW_2;
                break;
            }

        case MODE_FETCH_ROW_2:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
            //renderRow((mode-3)/2);
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_RENDER_ROW_2;
                break;
            }

        case MODE_RENDER_ROW_2:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;
            pinIn[AY38900_PIN_IN_SST]->isHigh = TRUE;
            totalTicks += TICK_LENGTH_RENDER_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_FETCH_ROW_3;
                break;
            }

        case MODE_FETCH_ROW_3:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
            //renderRow((mode-3)/2);
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_RENDER_ROW_3;
                break;
            }

        case MODE_RENDER_ROW_3:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;
            pinIn[AY38900_PIN_IN_SST]->isHigh = TRUE;
            totalTicks += TICK_LENGTH_RENDER_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_FETCH_ROW_4;
                break;
            }

        case MODE_FETCH_ROW_4:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
            //renderRow((mode-3)/2);
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_RENDER_ROW_4;
                break;
            }

        case MODE_RENDER_ROW_4:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;
            pinIn[AY38900_PIN_IN_SST]->isHigh = TRUE;
            totalTicks += TICK_LENGTH_RENDER_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_FETCH_ROW_5;
                break;
            }

        case MODE_FETCH_ROW_5:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
            //renderRow((mode-3)/2);
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_RENDER_ROW_5;
                break;
            }

        case MODE_RENDER_ROW_5:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;
            pinIn[AY38900_PIN_IN_SST]->isHigh = TRUE;
            totalTicks += TICK_LENGTH_RENDER_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_FETCH_ROW_6;
                break;
            }

        case MODE_FETCH_ROW_6:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
            //renderRow((mode-3)/2);
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_RENDER_ROW_6;
                break;
            }

        case MODE_RENDER_ROW_6:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;
            pinIn[AY38900_PIN_IN_SST]->isHigh = TRUE;
            totalTicks += TICK_LENGTH_RENDER_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_FETCH_ROW_7;
                break;
            }

        case MODE_FETCH_ROW_7:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
            //renderRow((mode-3)/2);
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_RENDER_ROW_7;
                break;
            }

        case MODE_RENDER_ROW_7:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;
            pinIn[AY38900_PIN_IN_SST]->isHigh = TRUE;
            totalTicks += TICK_LENGTH_RENDER_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_FETCH_ROW_8;
                break;
            }

        case MODE_FETCH_ROW_8:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
            //renderRow((mode-3)/2);
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_RENDER_ROW_8;
                break;
            }

        case MODE_RENDER_ROW_8:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;
            pinIn[AY38900_PIN_IN_SST]->isHigh = TRUE;
            totalTicks += TICK_LENGTH_RENDER_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_FETCH_ROW_9;
                break;
            }

        case MODE_FETCH_ROW_9:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
            //renderRow((mode-3)/2);
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_RENDER_ROW_9;
                break;
            }

        case MODE_RENDER_ROW_9:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;
            pinIn[AY38900_PIN_IN_SST]->isHigh = TRUE;
            totalTicks += TICK_LENGTH_RENDER_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_FETCH_ROW_10;
                break;
            }

        case MODE_FETCH_ROW_10:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
            //renderRow((mode-3)/2);
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_RENDER_ROW_10;
                break;
            }

        case MODE_RENDER_ROW_10:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;
            pinIn[AY38900_PIN_IN_SST]->isHigh = TRUE;
            totalTicks += TICK_LENGTH_RENDER_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_FETCH_ROW_11;
                break;
            }

        case MODE_FETCH_ROW_11:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
            //renderRow((mode-3)/2);
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                mode = MODE_RENDER_ROW_11;
                break;
            }

        case MODE_RENDER_ROW_11:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = TRUE;

            //this mode could be cut off in tick length if the vertical
            //offset is greater than 1
            if (verticalOffset == 0) {
                totalTicks += TICK_LENGTH_RENDER_ROW;
                if (totalTicks >= minimum) {
                    mode = MODE_FETCH_ROW_12;
                    break;
                }
            }
            else if (verticalOffset == 1) {
                totalTicks += TICK_LENGTH_RENDER_ROW - TICK_LENGTH_SCANLINE;
                mode = MODE_VBLANK;
                break;
            }
            else {
                totalTicks += (TICK_LENGTH_RENDER_ROW - TICK_LENGTH_SCANLINE
                        - (2 * (verticalOffset - 1) * TICK_LENGTH_SCANLINE));
                mode = MODE_VBLANK;
                break;
            }

        case MODE_FETCH_ROW_12:
        default:
            pinOut[AY38900_PIN_OUT_SR2]->isHigh = FALSE;
            totalTicks += TICK_LENGTH_SCANLINE;
            mode = MODE_VBLANK;
            break;
        }
    } while (totalTicks < minimum);

    return totalTicks;
}

void AY38900::setPixelBuffer(UINT8* pixelBuffer, UINT32 rowSize)
{
	AY38900::pixelBuffer = pixelBuffer;
	AY38900::pixelBufferRowSize = rowSize;
}

ITCM_CODE void AY38900::renderFrame()
{
    //render the next frame
    renderBackground();
    renderMOBs();
    for (int i = 0; i < 8; i++)
        mobs[i].collisionRegister = 0;
    determineMOBCollisions();
    markClean();
    renderBorders();
    copyBackgroundBufferToStagingArea();
    copyMOBsToStagingArea();
    for (int i = 0; i < 8; i++)
        memory[0x18+i] |= mobs[i].collisionRegister;
}

ITCM_CODE void AY38900::render()
{
	// the video bus handles the actual rendering.
}

ITCM_CODE void AY38900::markClean() {
    //everything has been rendered and is now clean
    offsetsChanged = FALSE;
    bordersChanged = FALSE;
    colorStackChanged = FALSE;
    colorModeChanged = FALSE;
    backtab.markClean();
    gram->markClean();
    for (int i = 0; i < 8; i++)
        mobs[i].markClean();
}

ITCM_CODE void AY38900::renderBorders()
{
    static int dampen_border_render = 0;
    
    //draw the top and bottom borders
    if (blockTop) {
        //move the image up 4 pixels and block the top and bottom 4 rows with the border
        for (UINT8 y = 0; y < 8; y++) {
            UINT8* buffer0 = ((UINT8*)pixelBuffer) + (y*pixelBufferRowSize);
            UINT8* buffer1 = buffer0 + (184*pixelBufferRowSize);
            for (UINT8 x = 0; x < 160; x++) {
                *buffer0++ = borderColor;
                *buffer1++ = borderColor;
            }
        }
    }
    else if (verticalOffset != 0) {
        //block the top rows of pixels depending on the amount of vertical offset
        UINT8 numRows = (UINT8)(verticalOffset<<1);
        for (UINT8 y = 0; y < numRows; y++) {
            UINT8* buffer0 = ((UINT8*)pixelBuffer) + (y*pixelBufferRowSize);
            for (UINT8 x = 0; x < 160; x++)
                *buffer0++ = borderColor;
        }
    }

    //draw the left and right borders
    if (blockLeft) {
        //move the image to the left 4 pixels and block the left and right 4 columns with the border
        for (UINT8 y = 0; y < 192; y++) {
            UINT8* buffer0 = ((UINT8*)pixelBuffer) + (y*pixelBufferRowSize);
            UINT8* buffer1 = buffer0 + 156;
            for (UINT8 x = 0; x < 4; x++) {
                *buffer0++ = borderColor;
                *buffer1++ = borderColor;
            }
        }
    }
    else if (horizontalOffset != 0) {
        //block the left columns of pixels depending on the amount of horizontal offset
        for (UINT8 y = 0; y < 192; y++) { 
            UINT8* buffer0 = ((UINT8*)pixelBuffer) + (y*pixelBufferRowSize);
            for (UINT8 x = 0; x < horizontalOffset; x++) {
                *buffer0++ = borderColor;
            }
        }
    }
}

ITCM_CODE void AY38900::renderMOBs()
{
    for (int i = 0; i < 8; i++)
    {
        //if the mob did not change shape and it's rendering from GROM (indicating that
        //the source of its rendering could not have changed), then this MOB does not need
        //to be re-rendered into its buffer
        if (!mobs[i].shapeChanged && mobs[i].isGrom)
            continue;

        //start at this memory location
        UINT16 firstMemoryLocation = (UINT16)(mobs[i].isGrom
                ? LOCATION_GROM + (mobs[i].cardNumber << 3)
                : LOCATION_GRAM + ((mobs[i].cardNumber & 0x3F) << 3));

        //end at this memory location
        UINT16 lastMemoryLocation = (UINT16)(firstMemoryLocation + 8);
        if (mobs[i].doubleYResolution)
            lastMemoryLocation += 8;

        //make the pixels this tall
        int pixelHeight = (mobs[i].quadHeight ? 4 : 1) * (mobs[i].doubleHeight ? 2 : 1);

        //start at the first line for regular vertical rendering or start at the last line
        //for vertically mirrored rendering
        int nextLine = 0;
        if (mobs[i].verticalMirror)
            nextLine = (pixelHeight * (mobs[i].doubleYResolution ? 15 : 7));
        for (UINT16 j = firstMemoryLocation; j < lastMemoryLocation; j++) {
            if (!mobs[i].shapeChanged && !gram->isCardDirty((UINT16)(j & 0x01FF))) {
                if (mobs[i].verticalMirror)
                    nextLine -= pixelHeight;
                else
                    nextLine += pixelHeight;
                continue;
            }

            //get the next line of pixels
            UINT16 nextData = (UINT16)(memoryBus->peek(j) & 0xFF);

            //reverse the pixels horizontally if necessary
            if (mobs[i].horizontalMirror)
                nextData = (UINT16)((reverse[nextData & 0x0F] << 4) | reverse[(nextData & 0xF0) >> 4]);

            //double them if necessary
            if (mobs[i].doubleWidth)
                nextData = (UINT16)((stretch[(nextData & 0xF0) >> 4] << 8) | stretch[nextData & 0x0F]);
            else
                nextData <<= 8;

            //lay down as many lines of pixels as necessary
            for (int k = 0; k < pixelHeight; k++)
                mobBuffers[i][nextLine++] = nextData;

            if (mobs[i].verticalMirror)
                nextLine -= (2*pixelHeight);
        }
    }
}

ITCM_CODE void AY38900::renderBackground()
{
    /*
    if (!backtab.isDirty() && !gram->isDirty() && !colorStackChanged && !colorModeChanged)
        return;
    */

    if (colorStackMode)
        renderColorStackMode();
    else
        renderForegroundBackgroundMode();
}

ITCM_CODE void AY38900::renderForegroundBackgroundMode()
{
    //iterate through all the cards in the backtab
    for (UINT8 i = 0; i < 240; i++) {
        //get the next card to render
        UINT16 nextCard = backtab.peek(LOCATION_BACKTAB+i);
        BOOL isGrom = (nextCard & 0x0800) == 0;
        UINT16 memoryLocation = nextCard & 0x01F8;

        //render this card only if this card has changed or if the card points to GRAM
        //and one of the eight bytes in gram that make up this card have changed
        if (colorModeChanged || backtab.isDirty(LOCATION_BACKTAB+i) || (!isGrom && gram->isCardDirty(memoryLocation))) {
            UINT8 fgcolor = (UINT8)((nextCard & 0x0007) | FOREGROUND_BIT);
            UINT8 bgcolor = (UINT8)(((nextCard & 0x2000) >> 11) | ((nextCard & 0x1600) >> 9));

            Memory* memory = (isGrom ? (Memory*)grom : (Memory*)gram);
            UINT16 address = memory->getReadAddress()+memoryLocation;
            UINT8 nextx = (i%20) * 8;
            UINT8 nexty = (i/20) * 8;
            for (UINT16 j = 0; j < 8; j++)
                renderLine((UINT8)memory->peek(address+j), nextx, nexty+j, fgcolor, bgcolor);
        }
    }
}

ITCM_CODE void AY38900::renderColorStackMode()
{
    UINT8 csPtr = 0;
    //if there are any dirty color advance bits in the backtab, or if
    //the color stack or the color mode has changed, the whole scene
    //must be rendered
    BOOL renderAll = backtab.areColorAdvanceBitsDirty() ||
        colorStackChanged || colorModeChanged;

    UINT8 nextx = 0;
    UINT8 nexty = 0;
    //iterate through all the cards in the backtab
    for (UINT8 h = 0; h < 240; h++) 
    {
        UINT16 nextCard = backtab.peek(LOCATION_BACKTAB+h);

        //colored squares mode
        if ((nextCard & 0x1800) == 0x1000) {
            if (renderAll || backtab.isDirty(LOCATION_BACKTAB+h)) {
                UINT8 csColor = (UINT8)memory[0x28 + csPtr];
                UINT8 color0 = (UINT8)(nextCard & 0x0007);
                UINT8 color1 = (UINT8)((nextCard & 0x0038) >> 3);
                UINT8 color2 = (UINT8)((nextCard & 0x01C0) >> 6);
                UINT8 color3 = (UINT8)(((nextCard & 0x2000) >> 11) |
                    ((nextCard & 0x0600) >> 9));
                renderColoredSquares(nextx, nexty,
                    (color0 == 7 ? csColor : (UINT8)(color0 | FOREGROUND_BIT)),
                    (color1 == 7 ? csColor : (UINT8)(color1 | FOREGROUND_BIT)),
                    (color2 == 7 ? csColor : (UINT8)(color2 | FOREGROUND_BIT)),
                    (color3 == 7 ? csColor : (UINT8)(color3 | FOREGROUND_BIT)));
            }
        }
        //color stack mode
        else {
            //advance the color pointer, if necessary
            if ((nextCard & 0x2000) != 0)
                csPtr = (UINT8)((csPtr+1) & 0x03);

            BOOL isGrom = (nextCard & 0x0800) == 0;
            UINT16 memoryLocation = (isGrom ? (nextCard & 0x07F8)
                : (nextCard & 0x01F8));

            if (renderAll || backtab.isDirty(LOCATION_BACKTAB+h) ||
                (!isGrom && gram->isCardDirty(memoryLocation))) {
                UINT8 fgcolor = (UINT8)(((nextCard & 0x1000) >> 9) |
                    (nextCard & 0x0007) | FOREGROUND_BIT);
                UINT8 bgcolor = (UINT8)memory[0x28 + csPtr];
                Memory* memory = (isGrom ? (Memory*)grom : (Memory*)gram);
                UINT16 address = memory->getReadAddress()+memoryLocation;
                for (UINT16 j = 0; j < 8; j++)
                    renderLine((UINT8)memory->peek(address+j), nextx, nexty+j, fgcolor, bgcolor);
            }
        }
        nextx += 8;
        if (nextx == 160) {
            nextx = 0;
            nexty += 8;
        }
    }
}

ITCM_CODE void AY38900::copyBackgroundBufferToStagingArea()
{
    int sourceWidthX = blockLeft ? 152 : (160 - horizontalOffset);
    int sourceHeightY = blockTop ? 88 : (96 - verticalOffset);

    int nextSourcePixel = (blockLeft ? (8 - horizontalOffset) : 0) +
	((blockTop ? (8 - verticalOffset) : 0) * 160);

    for (int y = 0; y < sourceHeightY; y++) 
    {
		UINT8* nextPixelStore0 = (UINT8*)pixelBuffer;
		nextPixelStore0 += (y*pixelBufferRowSize*4)>>1;
		if (blockTop) nextPixelStore0 += (pixelBufferRowSize*4)<<1;
		if (blockLeft) nextPixelStore0 += 4;
		UINT8* nextPixelStore1 = nextPixelStore0 + pixelBufferRowSize;
        for (int x = 0; x < sourceWidthX; x++) {
			UINT8 nextColor = backgroundBuffer[nextSourcePixel+x];
			*nextPixelStore0++ = nextColor;
			*nextPixelStore1++ = nextColor;
        }
        nextSourcePixel += 160;
    }
}

//copy the offscreen mob buffers to the staging area
ITCM_CODE void AY38900::copyMOBsToStagingArea()
{
    for (INT8 i = 7; i >= 0; i--) {
        if (mobs[i].xLocation == 0 || (!mobs[i].flagCollisions && !mobs[i].isVisible))
            continue;

        BOOL borderCollision = FALSE;
        BOOL foregroundCollision = FALSE;

        MOBRect* r = mobs[i].getBounds();
        UINT8 mobPixelHeight = (UINT8)(r->height << 1);
        UINT8 fgcolor = (UINT8)mobs[i].foregroundColor;

        INT16 leftX = (INT16)(r->x + horizontalOffset);
        INT16 nextY = (INT16)((r->y + verticalOffset) << 1);
        for (UINT8 y = 0; y < mobPixelHeight; y++) 
        {
            UINT16 zzz = (r->x+0)+ ((r->y+(y/2))*160);
            for (UINT8 x = 0; x < r->width; x++) 
            {
                //if this mob pixel is not on, then our life has no meaning
                if ((mobBuffers[i][y] & (0x8000 >> x)) == 0)
                    continue;

                //if the next pixel location is on the border, then we
                //have a border collision and we can ignore painting it
                int nextX = leftX + x;
                if (nextX < (blockLeft ? 8 : 0) || nextX > 158 ||
                        nextY < (blockTop ? 16 : 0) || nextY > 191) {
                    borderCollision = TRUE;
                    continue;
                }

                //check for foreground collision
                UINT8 currentPixel = backgroundBuffer[zzz+x];
                if ((currentPixel & FOREGROUND_BIT) != 0) 
                {
                    foregroundCollision = TRUE;
                    if (mobs[i].behindForeground)
                        continue;
                }
                if (mobs[i].isVisible) 
                {
					UINT8* nextPixel = (UINT8*)pixelBuffer;
					nextPixel += leftX - (blockLeft ? 4 : 0) + x;
					nextPixel += (nextY - (blockTop ? 8 : 0)) * (pixelBufferRowSize);
					*nextPixel = fgcolor | (currentPixel & FOREGROUND_BIT);
                }
            }
            nextY++;
        }

        //update the collision bits
        if (mobs[i].flagCollisions) {
            if (foregroundCollision)
                mobs[i].collisionRegister |= 0x0100;
            if (borderCollision)
                mobs[i].collisionRegister |= 0x0200;
        }
    }
}

ITCM_CODE void AY38900::renderLine(UINT8 nextbyte, int x, int y, UINT8 fgcolor, UINT8 bgcolor)
{
    UINT8* nextTargetPixel = backgroundBuffer + x + (y*160);
    *nextTargetPixel++ = (nextbyte & 0x80) != 0 ? fgcolor : bgcolor;
    *nextTargetPixel++ = (nextbyte & 0x40) != 0 ? fgcolor : bgcolor;
    *nextTargetPixel++ = (nextbyte & 0x20) != 0 ? fgcolor : bgcolor;
    *nextTargetPixel++ = (nextbyte & 0x10) != 0 ? fgcolor : bgcolor;
    
    *nextTargetPixel++ = (nextbyte & 0x08) != 0 ? fgcolor : bgcolor;
    *nextTargetPixel++ = (nextbyte & 0x04) != 0 ? fgcolor : bgcolor;
    *nextTargetPixel++ = (nextbyte & 0x02) != 0 ? fgcolor : bgcolor;
    *nextTargetPixel++ = (nextbyte & 0x01) != 0 ? fgcolor : bgcolor;
}

ITCM_CODE void AY38900::renderColoredSquares(int x, int y, UINT8 color0, UINT8 color1,
    UINT8 color2, UINT8 color3) {
    int topLeftPixel = x + (y*160);
    int topRightPixel = topLeftPixel+4;
    int bottomLeftPixel = topLeftPixel+640;
    int bottomRightPixel = bottomLeftPixel+4;

    for (UINT8 w = 0; w < 4; w++) {
        for (UINT8 i = 0; i < 4; i++) {
            backgroundBuffer[topLeftPixel++] = color0;
            backgroundBuffer[topRightPixel++] = color1;
            backgroundBuffer[bottomLeftPixel++] = color2;
            backgroundBuffer[bottomRightPixel++] = color3;
        }
        topLeftPixel += 156;
        topRightPixel += 156;
        bottomLeftPixel += 156;
        bottomRightPixel += 156;
    }
}

ITCM_CODE void AY38900::determineMOBCollisions()
{
    for (int i = 0; i < 7; i++) {
        if (mobs[i].xLocation == 0 || !mobs[i].flagCollisions)
            continue;

        /*
        //check MOB on foreground collisions
        if (mobCollidesWithForeground(i))
            mobs[i].collisionRegister |= 0x0100;

        //check MOB on border collisions
        if (mobCollidesWithBorder(i))
            mobs[i].collisionRegister |= 0x0200;
        */

        //check MOB on MOB collisions
        for (int j = i+1; j < 8; j++) 
        {
            if (mobs[j].xLocation == 0 || !mobs[j].flagCollisions)
                continue;

            if (mobsCollide(i, j)) {
                mobs[i].collisionRegister |= (1 << j);
                mobs[j].collisionRegister |= (1 << i);
            }
        }
    }
}

BOOL AY38900::mobCollidesWithBorder(int mobNum)
{
    MOBRect* r = mobs[mobNum].getBounds();
    UINT8 mobPixelHeight = (UINT8)(r->height<<1);

    /*
    if (r->x > (blockLeft ? 8 : 0) && r->x+r->width <= 191 &&
            r->y > (blockTop ? 8 : 0) && r->y+r->height <= 158)
        return FALSE;

    for (UINT8 i = 0; i < r->height; i++) {
        if (mobBuffers[mobNum][i<<1] == 0 || mobBuffers[mobNum][(i<<1)+1] == 0)
            continue;

        if (r->y+i < (blockLeft ? 8 : 0) || r->y+r->height+i > 158)
            return TRUE;

        //if (r->x && border
    }
    */

    UINT16 leftRightBorder = 0;
    //check if could possibly touch the left border
    if (r->x < (blockLeft ? 8 : 0)) {
        leftRightBorder = (UINT16)((blockLeft ? 0xFFFF : 0xFF00) << mobs[mobNum].xLocation);
    }
    //check if could possibly touch the right border
    else if (r->x+r->width > 158) {
        leftRightBorder = 0xFFFF;
        if (r->x < 158)
            leftRightBorder >>= r->x-158;
    }

    //check if touching the left or right border
    if (leftRightBorder) {
        for (INT32 i = 0; i < mobPixelHeight; i++) {
            if ((mobBuffers[mobNum][i] & leftRightBorder) != 0)
                return TRUE;
        }
    }

    //check if touching the top border
    UINT8 overlappingStart = 0;
    UINT8 overlappingHeight = 0;
    if (r->y < (blockTop ? 8 : 0)) {
        overlappingHeight = mobPixelHeight;
        if (r->y+r->height > (blockTop ? 8 : 0))
            overlappingHeight = (UINT8)(mobPixelHeight - (2*(r->y+r->height-(blockTop ? 8 : 0))));
    }
    //check if touching the bottom border
    else if (r->y+r->height > 191) {
        if (r->y < 191)
            overlappingStart = (UINT8)(2*(191-r->y));
        overlappingHeight = mobPixelHeight - overlappingStart;
    }

    if (overlappingHeight) {
        for (UINT8 i = overlappingStart; i < overlappingHeight; i++) {
            if (mobBuffers[mobNum][i] != 0)
                return TRUE;
        }
    }

    return FALSE;
}

BOOL AY38900::mobsCollide(int mobNum0, int mobNum1)
{
    MOBRect* r0 = mobs[mobNum0].getBounds();
    MOBRect* r1 = mobs[mobNum1].getBounds();
    if (!r0->intersects(r1))
        return FALSE;

    //determine the overlapping horizontal area
    int startingX = MAX(r0->x, r1->x);
    int offsetXr0 = startingX - r0->x;
    int offsetXr1 = startingX - r1->x;

    //determine the overlapping vertical area
    int startingY = MAX(r0->y, r1->y);
    int offsetYr0 = (startingY - r0->y) * 2;
    int offsetYr1 = (startingY - r1->y) * 2;
    int overlappingHeight = (MIN(r0->y + r0->height, r1->y + r1->height) - startingY) * 2;

    //iterate over the intersecting bits to see if any touch
    for (int y = 0; y < overlappingHeight; y++) {
        if (((mobBuffers[mobNum0][offsetYr0 + y] << offsetXr0) & (mobBuffers[mobNum1][offsetYr1 + y] << offsetXr1)) != 0)
            return TRUE;
    }

    return FALSE;
}

AY38900State AY38900::getState()
{
	AY38900State state = {0};
#if 0    
	this->registers.getMemory(state.registers, 0, this->registers.getMemoryByteSize());
	state.backtab = this->backtab.getState();

	state.inVBlank = this->inVBlank;
	state.mode = this->mode;
	state.previousDisplayEnabled = this->previousDisplayEnabled;
	state.displayEnabled = this->displayEnabled;
	state.colorStackMode = this->colorStackMode;

	state.borderColor = this->borderColor;
	state.blockLeft = this->blockLeft;
	state.blockTop = this->blockTop;
	state.horizontalOffset = this->horizontalOffset;
	state.verticalOffset = this->verticalOffset;

	for (int i = 0; i < 8; i++) {
		state.mobs[i] = this->mobs[i].getState();
	}
#endif
	return state;
}

void AY38900::setState(AY38900State state)
{
#if 0
    this->registers.setMemory(state.registers, 0, this->registers.getMemoryByteSize());
	this->backtab.setState(state.backtab, state.backtab.image);

	this->inVBlank = state.inVBlank;
	this->mode = state.mode;
	this->previousDisplayEnabled = state.previousDisplayEnabled;
	this->displayEnabled = state.displayEnabled;
	this->colorStackMode = state.colorStackMode;

	this->borderColor = state.borderColor;
	this->blockLeft = state.blockLeft;
	this->blockTop = state.blockTop;
	this->horizontalOffset = state.horizontalOffset;
	this->verticalOffset = state.verticalOffset;

	for (int i = 0; i < 8; i++) {
		mobs[i].setState(state.mobs[i]);
	}

	this->colorModeChanged = TRUE;
	this->bordersChanged = TRUE;
	this->colorStackChanged = TRUE;
	this->offsetsChanged = TRUE;
	this->imageBufferChanged = TRUE;
#endif    
}

/*
void AY38900::renderRow(int rowNum) {
    UINT8 backTabPtr = (UINT8)(0x200+(rowNum*20));
    if (colorStackMode) {
        UINT8 csPtr = 0;
        UINT8 nextx = 0;
        UINT8 nexty = 0;
        for (UINT8 h = 0; h < 20; h++) {
            UINT8 nextCard = (UINT8)backtab.peek(backTabPtr);
            backTabPtr++;

            if ((nextCard & 0x1800) == 0x1000) {
                //colored squares mode
                UINT8 csColor = (UINT8)registers.memory[0x28 + csPtr];
                UINT8 color0 = (UINT8)(nextCard & 0x0007);
                UINT8 color1 = (UINT8)((nextCard & 0x0038) >> 3);
                UINT8 color2 = (UINT8)((nextCard & 0x01C0) >> 6);
                UINT8 color3 = (UINT8)(((nextCard & 0x2000) >> 11) |
                    ((nextCard & 0x0600) >> 9));
                renderColoredSquares(nextx, nexty,
                    (color0 == 7 ? csColor : (UINT8)(color0 | FOREGROUND_BIT)),
                    (color1 == 7 ? csColor : (UINT8)(color1 | FOREGROUND_BIT)),
                    (color2 == 7 ? csColor : (UINT8)(color2 | FOREGROUND_BIT)),
                    (color3 == 7 ? csColor : (UINT8)(color3 | FOREGROUND_BIT)));
            }
            else {
                //color stack mode
                //advance the color pointer, if necessary
                if ((nextCard & 0x2000) != 0)
                    csPtr = (UINT8)((csPtr+1) & 0x03);

                BOOL isGrom = (nextCard & 0x0800) == 0;
                UINT8 memoryLocation = (UINT8)(isGrom ? (nextCard & 0x07F8)
                    : (nextCard & 0x01F8));

                UINT8 fgcolor = (UINT8)(((nextCard & 0x1000) >> 9) |
                    (nextCard & 0x0007) | FOREGROUND_BIT);
                UINT8 bgcolor = (UINT8)registers.memory[0x28 + csPtr];
                UINT16* memory = (isGrom ? grom->image : gram->image);
                for (int j = 0; j < 8; j++)
                    renderLine((UINT8)memory[memoryLocation+j], nextx, nexty+j, fgcolor, bgcolor);
            }
            nextx += 8;
            if (nextx == 160) {
                nextx = 0;
                nexty += 8;
            }
        }
    }
    else {
        UINT8 nextx = 0;
        UINT8 nexty = 0;
        for (UINT8 i = 0; i < 240; i++) {
            UINT8 nextCard = (UINT8)backtab.peek((UINT8)(0x200+i));
            BOOL isGrom = (nextCard & 0x0800) == 0;
            BOOL renderAll = backtab.isDirty((UINT8)(0x200+i)) || colorModeChanged;
            UINT8 memoryLocation = (UINT8)(nextCard & 0x01F8);

            if (renderAll || (!isGrom && gram->isCardDirty(memoryLocation))) {
                UINT8 fgcolor = (UINT8)((nextCard & 0x0007) | FOREGROUND_BIT);
                UINT8 bgcolor = (UINT8)(((nextCard & 0x2000) >> 11) |
                    ((nextCard & 0x1600) >> 9));

                UINT16* memory = (isGrom ? grom->image : gram->image);
                for (int j = 0; j < 8; j++)
                    renderLine((UINT8)memory[memoryLocation+j], nextx, nexty+j, fgcolor, bgcolor);
            }
            nextx += 8;
            if (nextx == 160) {
                nextx = 0;
                nexty += 8;
            }
        }
    }
}
*/
