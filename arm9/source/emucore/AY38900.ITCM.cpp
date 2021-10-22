// =====================================================================================
// Copyright (c) 2021 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, it's source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================
#include <nds.h>
#include "AY38900.h"
#include "ProcessorBus.h"
#include "../ds_tools.h"
#include "../config.h"

#define MIN(v1, v2) (v1 < v2 ? v1 : v2)
#define MAX(v1, v2) (v1 > v2 ? v1 : v2)

#define MODE_VBLANK                 0
#define MODE_START_ACTIVE_DISPLAY   1
#define MODE_IDLE_ACTIVE_DISPLAY    2
#define MODE_FETCH_ROW_0            3
#define MODE_RENDER_ROW_0           4
#define MODE_FETCH_ROW_1            5
#define MODE_RENDER_ROW_1           6
#define MODE_FETCH_ROW_2            7
#define MODE_RENDER_ROW_2           8
#define MODE_FETCH_ROW_3            9
#define MODE_RENDER_ROW_3           10
#define MODE_FETCH_ROW_4            11
#define MODE_RENDER_ROW_4           12
#define MODE_FETCH_ROW_5            13
#define MODE_RENDER_ROW_5           14
#define MODE_FETCH_ROW_6            15
#define MODE_RENDER_ROW_6           16
#define MODE_FETCH_ROW_7            17
#define MODE_RENDER_ROW_7           18
#define MODE_FETCH_ROW_8            19
#define MODE_RENDER_ROW_8           20
#define MODE_FETCH_ROW_9            21
#define MODE_RENDER_ROW_9           22
#define MODE_FETCH_ROW_10           23
#define MODE_RENDER_ROW_10          24
#define MODE_FETCH_ROW_11           25
#define MODE_RENDER_ROW_11          26
#define MODE_FETCH_ROW_12           27

UINT16 fudge_timing = 0;

#define TICK_LENGTH_SCANLINE             228
#define TICK_LENGTH_FRAME                (59736+fudge_timing)
#define TICK_LENGTH_VBLANK               (15164+fudge_timing)
#define TICK_LENGTH_START_ACTIVE_DISPLAY 112
#define TICK_LENGTH_IDLE_ACTIVE_DISPLAY  456
#define TICK_LENGTH_FETCH_ROW            456
#define TICK_LENGTH_RENDER_ROW           3192
#define LOCATION_BACKTAB                 0x0200
#define LOCATION_GROM                    0x3000
#define LOCATION_GRAM                    0x3800
#define FOREGROUND_BIT                   0x0010

UINT16 global_frames        __attribute__((section(".dtcm"))) = 0;
UINT16  mobBuffers[8][128]  __attribute__((section(".dtcm")));
UINT8 bLatched              __attribute__((section(".dtcm"))) = false;

UINT32 __attribute__ ((aligned (4))) __attribute__((section(".dtcm"))) color_repeat_table[]  = {
        0x00000000,  0x01010101,  0x02020202,  0x03030303,  0x04040404,  0x05050505,  0x06060606,  0x07070707,  
        0x08080808,  0x09090909,  0x0A0A0A0A,  0x0B0B0B0B,  0x0C0C0C0C,  0x0D0D0D0D,  0x0E0E0E0E,  0x0F0F0F0F,  
        0x10101010,  0x11111111,  0x12121212,  0x13131313,  0x14141414,  0x15151515,  0x16161616,  0x17171717,  
        0x18181818,  0x19191919,  0x1A1A1A1A,  0x1B1B1B1B,  0x1C1C1C1C,  0x1D1D1D1D,  0x1E1E1E1E,  0x1F1F1F1F
};

UINT8  __attribute__ ((aligned (4))) reverse[256] =
{
    0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
    0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
    0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
    0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
    0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
    0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
    0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
    0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
    0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
    0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
    0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
    0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
    0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
    0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
    0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
    0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF
};

UINT16  __attribute__ ((aligned (4))) stretch[256] =
{
    0x0000,0x0003,0x000C,0x000F,0x0030,0x0033,0x003C,0x003F,0x00C0,0x00C3,0x00CC,0x00CF,0x00F0,0x00F3,0x00FC,0x00FF,
    0x0300,0x0303,0x030C,0x030F,0x0330,0x0333,0x033C,0x033F,0x03C0,0x03C3,0x03CC,0x03CF,0x03F0,0x03F3,0x03FC,0x03FF,
    0x0C00,0x0C03,0x0C0C,0x0C0F,0x0C30,0x0C33,0x0C3C,0x0C3F,0x0CC0,0x0CC3,0x0CCC,0x0CCF,0x0CF0,0x0CF3,0x0CFC,0x0CFF,
    0x0F00,0x0F03,0x0F0C,0x0F0F,0x0F30,0x0F33,0x0F3C,0x0F3F,0x0FC0,0x0FC3,0x0FCC,0x0FCF,0x0FF0,0x0FF3,0x0FFC,0x0FFF,
    0x3000,0x3003,0x300C,0x300F,0x3030,0x3033,0x303C,0x303F,0x30C0,0x30C3,0x30CC,0x30CF,0x30F0,0x30F3,0x30FC,0x30FF,
    0x3300,0x3303,0x330C,0x330F,0x3330,0x3333,0x333C,0x333F,0x33C0,0x33C3,0x33CC,0x33CF,0x33F0,0x33F3,0x33FC,0x33FF,
    0x3C00,0x3C03,0x3C0C,0x3C0F,0x3C30,0x3C33,0x3C3C,0x3C3F,0x3CC0,0x3CC3,0x3CCC,0x3CCF,0x3CF0,0x3CF3,0x3CFC,0x3CFF,
    0x3F00,0x3F03,0x3F0C,0x3F0F,0x3F30,0x3F33,0x3F3C,0x3F3F,0x3FC0,0x3FC3,0x3FCC,0x3FCF,0x3FF0,0x3FF3,0x3FFC,0x3FFF,
    0xC000,0xC003,0xC00C,0xC00F,0xC030,0xC033,0xC03C,0xC03F,0xC0C0,0xC0C3,0xC0CC,0xC0CF,0xC0F0,0xC0F3,0xC0FC,0xC0FF,
    0xC300,0xC303,0xC30C,0xC30F,0xC330,0xC333,0xC33C,0xC33F,0xC3C0,0xC3C3,0xC3CC,0xC3CF,0xC3F0,0xC3F3,0xC3FC,0xC3FF,
    0xCC00,0xCC03,0xCC0C,0xCC0F,0xCC30,0xCC33,0xCC3C,0xCC3F,0xCCC0,0xCCC3,0xCCCC,0xCCCF,0xCCF0,0xCCF3,0xCCFC,0xCCFF,
    0xCF00,0xCF03,0xCF0C,0xCF0F,0xCF30,0xCF33,0xCF3C,0xCF3F,0xCFC0,0xCFC3,0xCFCC,0xCFCF,0xCFF0,0xCFF3,0xCFFC,0xCFFF,
    0xF000,0xF003,0xF00C,0xF00F,0xF030,0xF033,0xF03C,0xF03F,0xF0C0,0xF0C3,0xF0CC,0xF0CF,0xF0F0,0xF0F3,0xF0FC,0xF0FF,
    0xF300,0xF303,0xF30C,0xF30F,0xF330,0xF333,0xF33C,0xF33F,0xF3C0,0xF3C3,0xF3CC,0xF3CF,0xF3F0,0xF3F3,0xF3FC,0xF3FF,
    0xFC00,0xFC03,0xFC0C,0xFC0F,0xFC30,0xFC33,0xFC3C,0xFC3F,0xFCC0,0xFCC3,0xFCCC,0xFCCF,0xFCF0,0xFCF3,0xFCFC,0xFCFF,
    0xFF00,0xFF03,0xFF0C,0xFF0F,0xFF30,0xFF33,0xFF3C,0xFF3F,0xFFC0,0xFFC3,0xFFCC,0xFFCF,0xFFF0,0xFFF3,0xFFFC,0xFFFF
};


AY38900::AY38900(MemoryBus* mb, GROM* go, GRAM* ga)
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
    
    global_frames = 0;
}

ITCM_CODE void AY38900::setGraphicsBusVisible(BOOL visible) {
    registers.SetEnabled(visible);
    gram->SetEnabled(visible);
    grom->SetEnabled(visible);
}


ITCM_CODE INT32 AY38900::tick(INT32 minimum) {
    INT32 totalTicks = 0;
    do {
        switch (mode) 
        {
        //start of vertical blank
        case MODE_VBLANK:
            //come out of bus isolation mode
            setGraphicsBusVisible(TRUE);
            if (previousDisplayEnabled)
            {
                renderFrame();
            }
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
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                if (bLatched) backtab.LatchRow(0);
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
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                if (bLatched) backtab.LatchRow(1);
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
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                if (bLatched) backtab.LatchRow(2);
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
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                if (bLatched) backtab.LatchRow(3);
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
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                if (bLatched) backtab.LatchRow(4);
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
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                if (bLatched) backtab.LatchRow(5);
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
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                if (bLatched) backtab.LatchRow(6);
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
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                if (bLatched) backtab.LatchRow(7);
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
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                if (bLatched) backtab.LatchRow(8);
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
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                if (bLatched) backtab.LatchRow(9);
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
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                if (bLatched) backtab.LatchRow(10);
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
            totalTicks += TICK_LENGTH_FETCH_ROW;
            if (totalTicks >= minimum) {
                if (bLatched) backtab.LatchRow(11);
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

#define PIXEL_BUFFER_ROW_SIZE  160

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
    
    // -------------------------------------------------------------------------------------
    // If we are skipping frames, we can skip rendering the pixels to the staging area...
    // -------------------------------------------------------------------------------------
    if (myConfig.frame_skip_opt)
    {
        if (!((global_frames & 1)  == (myConfig.frame_skip_opt==1 ? 1:0)))        // Skip ODD or EVEN Frames as configured
        {
            renderBorders();
            copyBackgroundBufferToStagingArea();
        }
    }
    else
    {
        renderBorders();
        copyBackgroundBufferToStagingArea();
    }
    copyMOBsToStagingArea();
    
    for (int i = 0; i < 8; i++)
        memory[0x18+i] |= mobs[i].collisionRegister;
}

ITCM_CODE void AY38900::render()
{
    // the video bus handles the actual rendering.
}

ITCM_CODE void AY38900::markClean() 
{
    //everything has been rendered and is now clean
    offsetsChanged = FALSE;
    bordersChanged = FALSE;
    colorStackChanged = FALSE;
    colorModeChanged = FALSE;
    if (bLatched)
        backtab.markCleanLatched();
    else
        backtab.markClean();
    gram->markClean();
    for (int i = 0; i < 8; i++)
        mobs[i].markClean();
}

ITCM_CODE void AY38900::renderBorders()
{
    //draw the top and bottom borders
    if (blockTop) {
        //move the image up 4 pixels and block the top and bottom 4 rows with the border
        UINT32 borderColor32 = color_repeat_table[borderColor];
        
        for (UINT8 y = 0; y < 8; y++) 
        {
            UINT32* buffer0 = ((UINT32*)pixelBuffer) + (y*PIXEL_BUFFER_ROW_SIZE/4);
            UINT32* buffer1 = buffer0 + (184*PIXEL_BUFFER_ROW_SIZE/4);
            for (UINT8 x = 0; x < 160/4; x++) {
                *buffer0++ = borderColor32;
                *buffer1++ = borderColor32;
            }
        }
    }
    else if (verticalOffset != 0) {
        //block the top rows of pixels depending on the amount of vertical offset
        UINT8 numRows = (UINT8)(verticalOffset<<1);
        UINT32 borderColor32 = color_repeat_table[borderColor];
        for (UINT8 y = 0; y < numRows; y++) 
        {
            UINT32* buffer0 = ((UINT32*)pixelBuffer) + (y*PIXEL_BUFFER_ROW_SIZE/4);
            for (UINT8 x = 0; x < 160/4; x++)
                *buffer0++ = borderColor32;
        }
    }

    //draw the left and right borders
    if (blockLeft) {
        //move the image to the left 4 pixels and block the left and right 4 columns with the border
         UINT32 borderColor32 = color_repeat_table[borderColor];
        for (UINT8 y = 0; y < 192; y++) {
            UINT32* buffer0 = ((UINT32*)pixelBuffer) + (y*PIXEL_BUFFER_ROW_SIZE/4);
            UINT32* buffer1 = buffer0 + (156/4);
            *buffer0++ = borderColor32;
            *buffer1++ = borderColor32;
        }
    }
    else if (horizontalOffset != 0) {
        //block the left columns of pixels depending on the amount of horizontal offset
        for (UINT8 y = 0; y < 192; y++) { 
            UINT8* buffer0 = ((UINT8*)pixelBuffer) + (y*PIXEL_BUFFER_ROW_SIZE);
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
        UINT16 card = mobs[i].cardNumber;
        if (mobs[i].doubleYResolution) card &= 0xFE;
        UINT16 firstMemoryLocation = (UINT16)(mobs[i].isGrom ? LOCATION_GROM + (card << 3) : ((card & 0x3F) << 3));

        //end at this memory location
        UINT16 lastMemoryLocation = (UINT16)(firstMemoryLocation + 8);
        if (mobs[i].doubleYResolution)
        {
            lastMemoryLocation += 8;
        }

        //make the pixels this tall
        int pixelHeight = (mobs[i].quadHeight ? 4 : 1) * (mobs[i].doubleHeight ? 2 : 1);

        //start at the first line for regular vertical rendering or start at the last line
        //for vertically mirrored rendering
        int nextLine = 0;
        if (mobs[i].verticalMirror)
            nextLine = (pixelHeight * (mobs[i].doubleYResolution ? 15 : 7));
        for (UINT16 j = firstMemoryLocation; j < lastMemoryLocation; j++) 
        {
            UINT16 nextData;

            //get the next line of pixels
            if (mobs[i].isGrom)
                nextData = (UINT16)(memoryBus->peek(j));
            else
                nextData = (UINT16)(gram_image[j]);

            //reverse the pixels horizontally if necessary
            if (mobs[i].horizontalMirror)
                nextData = (UINT16)reverse[nextData];

            //double them if necessary
            if (mobs[i].doubleWidth)
                nextData = (UINT16)stretch[nextData];
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
    if (colorStackMode)
    {
        if (bLatched)
            renderColorStackModeLatched();
        else
            renderColorStackMode();
    }
    else
    {
        if (bLatched)
            renderForegroundBackgroundModeLatched();
        else
            renderForegroundBackgroundMode();
    }
}


ITCM_CODE void AY38900::renderForegroundBackgroundMode()
{
    //iterate through all the cards in the backtab
    for (UINT8 i = 0; i < 240; i++) 
    {
        //get the next card to render
        UINT16 nextCard = backtab.peek_direct(i);
        BOOL isGrom = (nextCard & 0x0800) == 0;
        UINT16 memoryLocation = nextCard & 0x01F8;

        //render this card only if this card has changed or if the card points to GRAM
        //and one of the eight bytes in gram that make up this card have changed
        if (colorModeChanged || backtab.isDirtyDirect(i) || (!isGrom && gram->isCardDirty(memoryLocation))) 
        {
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

ITCM_CODE void AY38900::renderForegroundBackgroundModeLatched()
{
    //iterate through all the cards in the backtab
    for (UINT8 i = 0; i < 240; i++) 
    {
        //get the next card to render
        UINT16 nextCard = backtab.peek_latched(i);
        BOOL isGrom = (nextCard & 0x0800) == 0;
        UINT16 memoryLocation = nextCard & 0x01F8;

        //render this card only if this card has changed or if the card points to GRAM
        //and one of the eight bytes in gram that make up this card have changed
        if (colorModeChanged || backtab.isDirtyLatched(i) || (!isGrom && gram->isCardDirty(memoryLocation))) 
        {
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
        UINT16 nextCard = backtab.peek_direct(h);

        //colored squares mode
        if ((nextCard & 0x1800) == 0x1000) 
        {
            if (renderAll || backtab.isDirtyDirect(h)) 
            {
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
        else 
        {
            //advance the color pointer, if necessary
            if ((nextCard & 0x2000) != 0)
                csPtr = (UINT8)((csPtr+1) & 0x03);

            BOOL isGrom = (nextCard & 0x0800) == 0;
            UINT16 memoryLocation = (isGrom ? (nextCard & 0x07F8)
                : (nextCard & 0x01F8));

            if (renderAll || backtab.isDirtyDirect(h) ||
                (!isGrom && gram->isCardDirty(memoryLocation))) 
            {
                UINT8 fgcolor = (UINT8)(((nextCard & 0x1000) >> 9) | (nextCard & 0x0007) | FOREGROUND_BIT);
                UINT8 bgcolor = (UINT8)memory[0x28 + csPtr];
                if (isGrom)
                {
                    Memory* memory = (Memory*)grom;
                    UINT16 address = memory->getReadAddress()+memoryLocation;
                    for (UINT16 j = 0; j < 8; j++)
                        renderLine((UINT8)memory->peek(address+j), nextx, nexty+j, fgcolor, bgcolor);
                }
                else
                {
                    for (UINT16 j = 0; j < 8; j++)
                        renderLine(gram_image[memoryLocation+j], nextx, nexty+j, fgcolor, bgcolor);
                }
            }
        }
        nextx += 8;
        if (nextx == 160) {
            nextx = 0;
            nexty += 8;
        }
    }
}

ITCM_CODE void AY38900::renderColorStackModeLatched()
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
        UINT16 nextCard = backtab.peek_latched(h); //backtab.peek_direct(h);

        //colored squares mode
        if ((nextCard & 0x1800) == 0x1000) 
        {
            if (renderAll || backtab.isDirtyDirect(h)) 
            {
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
        else 
        {
            //advance the color pointer, if necessary
            if ((nextCard & 0x2000) != 0)
                csPtr = (UINT8)((csPtr+1) & 0x03);

            BOOL isGrom = (nextCard & 0x0800) == 0;
            UINT16 memoryLocation = (isGrom ? (nextCard & 0x07F8)
                : (nextCard & 0x01F8));

            if (renderAll || backtab.isDirtyDirect(h) || (!isGrom && gram->isCardDirty(memoryLocation))) 
            {
                UINT8 fgcolor = (UINT8)(((nextCard & 0x1000) >> 9) |
                    (nextCard & 0x0007) | FOREGROUND_BIT);
                UINT8 bgcolor = (UINT8)memory[0x28 + csPtr];
                
                if (isGrom)
                {
                    Memory* memory = (Memory*)grom;
                    UINT16 address = memory->getReadAddress()+memoryLocation;
                    for (UINT16 j = 0; j < 8; j++)
                        renderLine((UINT8)memory->peek(address+j), nextx, nexty+j, fgcolor, bgcolor);
                }
                else
                {
                    for (UINT16 j = 0; j < 8; j++)
                        renderLine(gram_image[memoryLocation+j], nextx, nexty+j, fgcolor, bgcolor);
                }
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
    if (blockLeft || blockTop || horizontalOffset || verticalOffset)
    {
        // No offsets - this is a reasonably fast 32-bit render
        if (horizontalOffset == 0 && verticalOffset == 0)
        {
            int sourceWidthX = blockLeft ? 152 : 160;
            int sourceHeightY = blockTop ? 88 : 96;
            int nextSourcePixel = (blockLeft ? (8) : 0) + ((blockTop ? (8) : 0) * 160);

            for (int y = 0; y < sourceHeightY; y++) 
            {
                UINT8* nextPixelStore0 = (UINT8*)pixelBuffer;
                nextPixelStore0 += (y*PIXEL_BUFFER_ROW_SIZE*2);
                if (blockTop) nextPixelStore0 += (PIXEL_BUFFER_ROW_SIZE*8);
                if (blockLeft) nextPixelStore0 += 4;
                UINT8* nextPixelStore1 = nextPixelStore0 + PIXEL_BUFFER_ROW_SIZE;
                
                UINT32* np0 = (UINT32 *)nextPixelStore0;
                UINT32* np1 = (UINT32 *)nextPixelStore1;
                UINT32* backColor = (UINT32*)(&backgroundBuffer[nextSourcePixel]);
                for (int x = 0; x < sourceWidthX/4; x++) 
                {
                    *np0++ = *backColor;
                    *np1++ = *backColor++;
                }
                nextSourcePixel += 160;
            }
        }
        else    // This is the worst case... offsets!
        {
            short int sourceWidthX = blockLeft ? 152 : (160-horizontalOffset);
            short int sourceHeightY = blockTop ? 88 : (96-verticalOffset);
            short int myHoriz = (blockLeft ? (8 - horizontalOffset) : 0);
            short int myVert  = (blockTop  ? (8 - verticalOffset)   : 0);
            short int nextSourcePixel = myHoriz + (myVert * 160);           
            
            for (int y = 0; y < sourceHeightY; y++) 
            {
                UINT8* nextPixelStore0 = (UINT8*)pixelBuffer;
                nextPixelStore0 += (y*PIXEL_BUFFER_ROW_SIZE*2);

                if (blockTop) nextPixelStore0 += (PIXEL_BUFFER_ROW_SIZE*8);
                else if (verticalOffset) nextPixelStore0 += (PIXEL_BUFFER_ROW_SIZE*(verticalOffset*2));
                
                if (blockLeft) nextPixelStore0 += 4;
                else if (horizontalOffset) nextPixelStore0 += horizontalOffset;
                
                UINT8* nextPixelStore1 = nextPixelStore0 + PIXEL_BUFFER_ROW_SIZE;
                for (int x = 0; x < sourceWidthX; x++) 
                {
                    UINT8 nextColor = backgroundBuffer[nextSourcePixel+x];
                    *nextPixelStore0++ = nextColor;
                    *nextPixelStore1++ = nextColor;
                }
                nextSourcePixel += 160;
            }
        }
    }
    else // No borders and no offsets...
    {
        // This is the fastest of them all... no offsets and no borders so everything will be a multiple of 4...
        UINT32* backColor = (UINT32*)(&backgroundBuffer[0]);
        for (int y = 0; y < 96; y++) 
        {
            UINT32* nextPixelStore0 = (UINT32*)(&pixelBuffer[y*PIXEL_BUFFER_ROW_SIZE*2]);
            UINT32* nextPixelStore1 = nextPixelStore0 + (PIXEL_BUFFER_ROW_SIZE/4);
            for (int x = 0; x < PIXEL_BUFFER_ROW_SIZE/4; x++) 
            {
                *nextPixelStore0++ = *backColor;
                *nextPixelStore1++ = *backColor++;
            }
        }
    }
}

//copy the offscreen mob buffers to the staging area
ITCM_CODE void AY38900::copyMOBsToStagingArea()
{
    for (INT8 i = 7; i >= 0; i--) 
    {
        if (mobs[i].xLocation == 0 || mobs[i].xLocation > 167 || (!mobs[i].flagCollisions && !mobs[i].isVisible))
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
            UINT16 idx = (r->x+0)+ ((r->y+(y/2))*160);
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
                UINT8 currentPixel = backgroundBuffer[idx+x];
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
                    nextPixel += (nextY - (blockTop ? 8 : 0)) * (PIXEL_BUFFER_ROW_SIZE);
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
    if (nextbyte == 0x00)
    {
        UINT32* nextTargetPixel = (UINT32*)(backgroundBuffer + x + (y*160));
        UINT32 bgColor32 = color_repeat_table[bgcolor];
        *nextTargetPixel++ = bgColor32;
        *nextTargetPixel = bgColor32;
    }
    else
    {
        UINT8* nextTargetPixel = backgroundBuffer + x + (y*160);
        *nextTargetPixel++ = (nextbyte & 0x80) ? fgcolor : bgcolor;
        *nextTargetPixel++ = (nextbyte & 0x40) ? fgcolor : bgcolor;
        *nextTargetPixel++ = (nextbyte & 0x20) ? fgcolor : bgcolor;
        *nextTargetPixel++ = (nextbyte & 0x10) ? fgcolor : bgcolor;

        *nextTargetPixel++ = (nextbyte & 0x08) ? fgcolor : bgcolor;
        *nextTargetPixel++ = (nextbyte & 0x04) ? fgcolor : bgcolor;
        *nextTargetPixel++ = (nextbyte & 0x02) ? fgcolor : bgcolor;
        *nextTargetPixel = (nextbyte & 0x01)   ? fgcolor : bgcolor;
    }
}

ITCM_CODE void AY38900::renderColoredSquares(int x, int y, UINT8 color0, UINT8 color1,
    UINT8 color2, UINT8 color3) 
{
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
    for (int i = 0; i < 7; i++) 
    {
        if (mobs[i].xLocation == 0 || !mobs[i].flagCollisions || mobs[i].xLocation > 167)
            continue;

        //check MOB on MOB collisions
        for (int j = i+1; j < 8; j++) 
        {
            if (mobs[j].xLocation == 0 || !mobs[j].flagCollisions  || mobs[j].xLocation > 167)
                continue;

            if (mobsCollide(i, j)) {
                mobs[i].collisionRegister |= (1 << j);
                mobs[j].collisionRegister |= (1 << i);
            }
        }
    }
}

ITCM_CODE BOOL AY38900::mobCollidesWithBorder(int mobNum)
{
    MOBRect* r = mobs[mobNum].getBounds();
    UINT8 mobPixelHeight = (UINT8)(r->height<<1);

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

ITCM_CODE BOOL AY38900::mobsCollide(int mobNum0, int mobNum1)
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


void AY38900::getState(AY38900State *state)
{
    extern UINT16 memory[0x40];
    memcpy(state->registers, memory, 0x40*sizeof(UINT16));
    this->backtab.getState(&state->backtab);

    state->inVBlank = this->inVBlank;
    state->mode = this->mode;
    state->previousDisplayEnabled = this->previousDisplayEnabled;
    state->displayEnabled = this->displayEnabled;
    state->colorStackMode = this->colorStackMode;

    state->borderColor = this->borderColor;
    state->blockLeft = this->blockLeft;
    state->blockTop = this->blockTop;
    state->horizontalOffset = this->horizontalOffset;
    state->verticalOffset = this->verticalOffset;

    for (int i = 0; i < 8; i++) 
    {
        this->mobs[i].getState(&state->mobs[i]);
    }
}

void AY38900::setState(AY38900State *state)
{
    extern UINT16 memory[0x40];
    memcpy(memory, state->registers, 0x40*sizeof(UINT16));
    this->backtab.setState(&state->backtab);

    this->inVBlank = state->inVBlank;
    this->mode = state->mode;
    this->previousDisplayEnabled = state->previousDisplayEnabled;
    this->displayEnabled = state->displayEnabled;
    this->colorStackMode = state->colorStackMode;

    this->borderColor = state->borderColor;
    this->blockLeft = state->blockLeft;
    this->blockTop = state->blockTop;
    this->horizontalOffset = state->horizontalOffset;
    this->verticalOffset = state->verticalOffset;

    for (int i = 0; i < 8; i++) 
    {
        this->mobs[i].setState(&state->mobs[i]);
    }
    
    this->colorModeChanged = TRUE;
    this->bordersChanged = TRUE;
    this->colorStackChanged = TRUE;
    this->offsetsChanged = TRUE;
    this->imageBufferChanged = TRUE;
}
