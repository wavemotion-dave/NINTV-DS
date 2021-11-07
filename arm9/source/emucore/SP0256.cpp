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
#include "SP0256.h"

INT32 repeat                    __attribute__((section(".dtcm")));
INT32 period                    __attribute__((section(".dtcm")));
INT32 periodCounter             __attribute__((section(".dtcm")));
INT32 amplitude                 __attribute__((section(".dtcm")));
INT8  b[6]                      __attribute__((section(".dtcm")));
INT8  f[6]                      __attribute__((section(".dtcm")));
INT32 y[6][2]                   __attribute__((section(".dtcm")));
INT8  periodInterpolation       __attribute__((section(".dtcm")));
INT8  amplitudeInterpolation    __attribute__((section(".dtcm")));


INT32 bitsLeft                  __attribute__((section(".dtcm")));
INT32 currentBits               __attribute__((section(".dtcm")));
UINT16 pc                       __attribute__((section(".dtcm")));
UINT16 stack                    __attribute__((section(".dtcm")));
INT32 mode                      __attribute__((section(".dtcm")));
INT32 repeatPrefix              __attribute__((section(".dtcm")));

UINT16 fifoBytes[64] __attribute__((section(".dtcm")));

UINT8 sp_idle __attribute__((section(".dtcm"))) = 1;

UINT16 bitMasks[16] __attribute__((section(".dtcm"))) = {
        0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF,
        0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

INT16 qtbl[256] __attribute__((section(".dtcm"))) = {
    0,      511,    510,    509,    508,    507,    506,    505,
    504,    503,    502,    501,    500,    499,    498,    497,
    496,    495,    494,    493,    492,    491,    490,    489,
    488,    487,    486,    485,    484,    483,    482,    481,
    479,    477,    475,    473,    471,    469,    467,    465,
    463,    461,    459,    457,    455,    453,    451,    449,
    447,    445,    443,    441,    439,    437,    435,    433,
    431,    429,    427,    425,    421,    417,    413,    409,
    405,    401,    397,    393,    389,    385,    381,    377,
    373,    369,    365,    361,    357,    353,    349,    345,
    341,    337,    333,    329,    325,    321,    317,    313,
    309,    305,    301,    297,    289,    281,    273,    265,
    257,    249,    241,    233,    225,    217,    209,    201,
    193,    185,    177,    169,    161,    153,    145,    137,
    129,    121,    113,    105,    97,     89,     81,     73,
    65,     57,     49,     41,     33,     25,     12,     9,
    0,     -9,     -12,    -25,    -33,    -41,    -49,    -57,
   -65,    -73,    -81,    -89,    -97,    -105,   -113,   -121,
   -129,   -137,   -145,   -153,   -161,   -169,   -177,   -185,
   -193,   -201,   -209,   -217,   -225,   -233,   -241,   -249,
   -257,   -265,   -273,   -281,   -289,   -297,   -301,   -305,
   -309,   -313,   -317,   -321,   -325,   -329,   -333,   -337,
   -341,   -345,   -349,   -353,   -357,   -361,   -365,   -369,
   -373,   -377,   -381,   -385,   -389,   -393,   -397,   -401,
   -405,   -409,   -413,   -417,   -421,   -425,   -427,   -429,
   -431,   -433,   -435,   -437,   -439,   -441,   -443,   -445,
   -447,   -449,   -451,   -453,   -455,   -457,   -459,   -461,
   -463,   -465,   -467,   -469,   -471,   -473,   -475,   -477,
   -479,   -481,   -482,   -483,   -484,   -485,   -486,   -487,
   -488,   -489,   -490,   -491,   -492,   -493,   -494,   -495,
   -496,   -497,   -498,   -499,   -500,   -501,   -502,   -503,
   -504,   -505,   -506,   -507,   -508,   -509,   -510,   -511
};

SP0256::SP0256()
    : Processor("SP0256"),
      ivoiceROM("Intellivoice ROM", "ivoice.bin", 0, 1, 0x800, 0x1000, TRUE)
{
    registers.init(this);
}

SP0256::~SP0256()
{
 sp_idle = 1;   
}

INT32 SP0256::getClockSpeed() {
    return 10000;
}

INT32 SP0256::getClocksPerSample() {
    return 1;
}

void SP0256::resetProcessor()
{
    currentBits = 0;
    bitsLeft = 0;

    pc = 0;
    page = 1;
    stack = 0;
    mode = 0;
    repeatPrefix = 0;
    command = 0;
    lrqHigh = TRUE;
    idle = TRUE;
    fifoHead = 0;
    fifoSize = 0;
    speaking = FALSE;
    sp_idle = 1;

    amplitude = 0;
    period = 0;
    periodCounter = 0x1;
    random = 1;
    for (INT32 i = 0; i < 6; i++) {
        y[i][0] = 0;
        y[i][1] = 0;
    }
}

UINT8 cleared = false;
ITCM_CODE INT32 SP0256::tick(INT32 minimum)
{
    if (idle) 
    {
        if (!cleared)
        {
            cleared = true;
            for (int i = 0; i < minimum; i++)
                playSample1(0);
        }
        else // Effective delay 1 tick...
        {
            sp_idle = 1;
        }
        return minimum;
    } else sp_idle=0;

    cleared = false;
    INT32 totalTicks = 0;
    do {
        if (!speaking) {
            speaking = TRUE;
            lrqHigh = TRUE;
            pc = 0x1000 | (command << 1);
            bitsLeft = 0;
            command = 0;
        }

        //if the speaking filters are empty, fill 'em up
        while (!idle && repeat == 0) {
            INT32 repeatBefore = repeatPrefix;
            decode();
            if (repeatBefore != 0)
                repeatPrefix = 0;
        }

        INT32 sample = 0;
        if (period == 0) {
            if (periodCounter == 0) {
                periodCounter = 64;
                repeat--;
                for (UINT8 j = 0; j < 6; j++)
                    y[j][0] = y[j][1] = 0;
            }
            else
                periodCounter--;

            sample = ((amplitude & 0x1F) << ((amplitude & 0xE0) >> 5));
            BOOL noise = ((random & 1) != 0);
            random = (random >> 1) ^ (noise ? 0x14000 : 0);
            if (!noise)
                sample = -sample;
        }
        else {
            if (periodCounter == 0) {
                periodCounter = period;
                repeat--;
                sample = ((amplitude & 0x1F) << ((amplitude & 0xE0) >> 5));
                for (INT32 j = 0; j < 6; j++)
                    y[j][0] = y[j][1] = 0;

            }
            else
                periodCounter--;
        }

        period = ((period | 0x10000) + periodInterpolation) & 0xFFFF;
        amplitude = ((amplitude | 0x10000) + amplitudeInterpolation) & 0xFFFF;

        for (INT32 i = 0; i < 6; i++) {
            sample += ((qtbl[0x80+b[i]]*y[i][1]) >> 9);
            sample += ((qtbl[0x80+f[i]]*y[i][0]) >> 8);
            y[i][1] = y[i][0];
            y[i][0] = sample;
        }

        //clamp the sample to a 12-bit range
        if (sample > 2047) sample = 2047;
        else if (sample < -2048) sample = -2048;

        playSample1((INT16)(sample << 4));

        totalTicks++;

    } while (totalTicks < minimum);
    return totalTicks;
}

INT8 SP0256::readDelta(INT32 numBits) {
    INT32 value = readBits(numBits);
    if ((value & (1 << (numBits - 1))) != 0)
        value |= -1 << numBits;
    return (INT8)value;
}

ITCM_CODE INT32 SP0256::readBitsReverse(INT32 numBits) 
{
    while (bitsLeft < numBits) {
        if (pc < 0x1800) {
            currentBits |= (ivoiceROM.peek8((UINT16)pc) << bitsLeft);
            bitsLeft += 8;
            pc++;
        }
        else if (pc == 0x1800 && fifoSize > 0) {
            currentBits |= (fifoBytes[fifoHead] << bitsLeft);
            fifoHead = (fifoHead+1) & 0x3F;
            fifoSize--;
            bitsLeft += 10;
        }
        else {
            //error, read outside of bounds
            currentBits |= (0x03FF << bitsLeft);
            bitsLeft += 10;
            pc++;
        }

    }

    INT32 output = currentBits & bitMasks[numBits-1];
    output = flipEndian(output, numBits);
    currentBits = currentBits >> numBits;
    bitsLeft -= numBits;
    return output;
}

ITCM_CODE INT32 SP0256::readBits(INT32 numBits) 
{
    while (bitsLeft < numBits) {
        if (pc < 0x1800) {
            currentBits |= (ivoiceROM.peek8((UINT16)pc) << bitsLeft);
            bitsLeft += 8;
            pc++;
        }
        else if (pc == 0x1800 && fifoSize > 0) {
            currentBits |= (fifoBytes[fifoHead] << bitsLeft);
            fifoHead = (fifoHead+1) & 0x3F;
            fifoSize--;
            bitsLeft += 10;
        }
        else {
            //error, read outside of bounds
            currentBits |= (0x03FF << bitsLeft);
            bitsLeft += 10;
            pc++;
        }

    }

    INT32 output = currentBits & bitMasks[numBits-1];
    currentBits = currentBits >> numBits;
    bitsLeft -= numBits;
    return output;
}

void SP0256::RTS() {
    if (stack == 0) {
        if (!lrqHigh) {
            pc = 0x1000 | (command << 1);
            bitsLeft = 0;
            command = 0;
            lrqHigh = TRUE;
        }
        else {
            speaking = FALSE;
            idle = TRUE;
        }
    }
    else {
        pc = stack;
        stack = 0;
        bitsLeft = 0;
    }
}

void SP0256::SETPAGE(INT32 immed4) {
    this->page = flipEndian(immed4, 4);
}

void SP0256::LOADALL(INT32 immed4) {
    repeat = (repeatPrefix << 4) | immed4;
    if (repeat == 0)
        return;

    amplitude = readBits(8);
    period = readBits(8);
    //periodCounter = (period == 0 ? 0x100 : period);
    b[0] = (INT8)readBits(8);
    f[0] = (INT8)readBits(8);
    b[1] = (INT8)readBits(8);
    f[1] = (INT8)readBits(8);
    b[2] = (INT8)readBits(8);
    f[2] = (INT8)readBits(8);
    b[3] = (INT8)readBits(8);
    f[3] = (INT8)readBits(8);
    b[4] = (INT8)readBits(8);
    f[4] = (INT8)readBits(8);
    b[5] = (INT8)readBits(8);
    f[5] = (INT8)readBits(8);
    if ((mode & 0x01) == 0) {
        amplitudeInterpolation = 0;
        periodInterpolation = 0;
    }
    else {
        amplitudeInterpolation = (INT8)readBits(8);
        periodInterpolation = (INT8)readBits(8);
    }
}

void SP0256::LOAD_2(INT32 immed4) {
    repeat = (repeatPrefix << 4) | immed4;
    if (repeat == 0)
        return;

    amplitude = (readBits(6) << 2) | (amplitude & 0x03);
    period = readBits(8);
    //periodCounter = (period == 0 ? 0x100 : period);
    switch (mode) {
        case 0x0:
            b[0] = (INT8)((readBits(3) << 4) | (b[0] & 0x0F));
            f[0] = (INT8)((readBits(5) << 3) | (f[0] & 0x07));
            b[1] = (INT8)((readBits(3) << 4) | (b[1] & 0x0F));
            f[1] = (INT8)((readBits(5) << 3) | (f[1] & 0x07));
            b[2] = (INT8)((readBits(3) << 4) | (b[2] & 0x0F));
            f[2] = (INT8)((readBits(5) << 3) | (f[2] & 0x07));
            b[3] = (INT8)((readBits(4) << 3) | (b[3] & 0x07));
            f[3] = (INT8)((readBits(6) << 2) | (f[3] & 0x03));
            b[4] = (INT8)((readBits(7) << 1) | (b[4] & 0x01));
            f[4] = (INT8)((readBits(6) << 2) | (f[4] & 0x03));
            b[5] = 0;
            f[5] = 0;
            break;
        case 0x1:
            b[0] = (INT8)((readBits(3) << 4) | (b[0] & 0x0F));
            f[0] = (INT8)((readBits(5) << 3) | (f[0] & 0x07));
            b[1] = (INT8)((readBits(3) << 4) | (b[1] & 0x0F));
            f[1] = (INT8)((readBits(5) << 3) | (f[1] & 0x07));
            b[2] = (INT8)((readBits(3) << 4) | (b[2] & 0x0F));
            f[2] = (INT8)((readBits(5) << 3) | (f[2] & 0x07));
            b[3] = (INT8)((readBits(4) << 3) | (b[3] & 0x07));
            f[3] = (INT8)((readBits(6) << 2) | (f[3] & 0x03));
            b[4] = (INT8)((readBits(7) << 1) | (b[4] & 0x01));
            f[4] = (INT8)((readBits(6) << 2) | (f[4] & 0x03));
            b[5] = (INT8)readBits(8);
            f[5] = (INT8)readBits(8);
            break;
        case 0x2:
            b[0] = (INT8)((readBits(6) << 1) | (b[0] & 0x01));
            f[0] = (INT8)((readBits(6) << 2) | (f[0] & 0x03));
            b[1] = (INT8)((readBits(6) << 1) | (b[1] & 0x01));
            f[1] = (INT8)((readBits(6) << 2) | (f[1] & 0x03));
            b[2] = (INT8)((readBits(6) << 1) | (b[2] & 0x01));
            f[2] = (INT8)((readBits(6) << 2) | (f[2] & 0x03));
            b[3] = (INT8)((readBits(6) << 1) | (b[3] & 0x01));
            f[3] = (INT8)((readBits(7) << 1) | (f[3] & 0x01));
            b[4] = (INT8)readBits(8);
            f[4] = (INT8)readBits(8);
            b[5] = 0;
            f[5] = 0;
            break;
        case 0x3:
            b[0] = (INT8)((readBits(6) << 1) | (b[0] & 0x01));
            f[0] = (INT8)((readBits(6) << 2) | (f[0] & 0x03));
            b[1] = (INT8)((readBits(6) << 1) | (b[1] & 0x01));
            f[1] = (INT8)((readBits(6) << 2) | (f[1] & 0x03));
            b[2] = (INT8)((readBits(6) << 1) | (b[2] & 0x01));
            f[2] = (INT8)((readBits(6) << 2) | (f[2] & 0x03));
            b[3] = (INT8)((readBits(6) << 1) | (b[3] & 0x01));
            f[3] = (INT8)((readBits(7) << 1) | (f[3] & 0x01));
            b[4] = (INT8)readBits(8);
            f[4] = (INT8)readBits(8);
            b[5] = (INT8)readBits(8);
            f[5] = (INT8)readBits(8);
            break;
    }

    amplitudeInterpolation = (INT8)
            ((amplitudeInterpolation & 0xE0) | (readBits(5)));
    periodInterpolation = (INT8)
            ((periodInterpolation & 0xE0) | (readBits(5)));
}

void SP0256::SETMSB_3(INT32 immed4) {
    repeat = (repeatPrefix << 4) | immed4;
    if (repeat == 0)
        return;

    amplitude = (readBits(6) << 2) | (amplitude & 0x03);
    if (mode == 0x00 || mode == 0x02) {
        b[5] = 0;
        f[5] = 0;
    }

    switch (mode) {
        case 0x0:
        case 0x1:
            f[0] = (INT8)((readBits(5) << 3) | (f[0] & 0x07));
            f[1] = (INT8)((readBits(5) << 3) | (f[1] & 0x07));
            f[2] = (INT8)((readBits(5) << 3) | (f[2] & 0x07));
            break;
        case 0x2:
        case 0x3:
            f[0] = (INT8)((readBits(6) << 2) | (f[0] & 0x03));
            f[1] = (INT8)((readBits(6) << 2) | (f[1] & 0x03));
            f[2] = (INT8)((readBits(6) << 2) | (f[2] & 0x03));
            break;
    }

    amplitudeInterpolation = (INT8)
            ((amplitudeInterpolation & 0xE0) | (readBits(5)));
    periodInterpolation = (INT8)
            ((periodInterpolation & 0xE0) | (readBits(5)));
}

void SP0256::LOAD_4(INT32 immed4) {
    repeat = (repeatPrefix << 4) | immed4;
    if (repeat == 0)
        return;

    amplitude = (readBits(6) << 2) | (amplitude & 0x03);
    period = readBits(8);
    //periodCounter = (period == 0 ? 0x100 : period);
    b[0] = 0;
    f[0] = 0;
    b[1] = 0;
    f[1] = 0;
    b[2] = 0;
    f[2] = 0;
    switch (mode) {
        case 0x0:
            b[3] = (INT8)((readBits(4) << 3) | (b[3] & 0x07));
            f[3] = (INT8)((readBits(6) << 2) | (f[3] & 0x03));
            b[4] = (INT8)((readBits(7) << 1) | (b[4] & 0x01));
            f[4] = (INT8)((readBits(6) << 2) | (f[4] & 0x03));
            b[5] = 0;
            f[5] = 0;
            break;
        case 0x1:
            b[3] = (INT8)((readBits(4) << 3) | (b[3] & 0x07));
            f[3] = (INT8)((readBits(6) << 2) | (f[3] & 0x03));
            b[4] = (INT8)((readBits(7) << 1) | (b[4] & 0x01));
            f[4] = (INT8)((readBits(6) << 2) | (f[4] & 0x03));
            b[5] = (INT8)readBits(8);
            f[5] = (INT8)readBits(8);
            break;
        case 0x2:
            b[3] = (INT8)((readBits(6) << 1) | (b[3] & 0x01));
            f[3] = (INT8)((readBits(7) << 1) | (f[3] & 0x01));
            b[4] = (INT8)readBits(8);
            f[4] = (INT8)readBits(8);
            b[5] = 0;
            f[5] = 0;
            break;
        case 0x3:
            b[3] = (INT8)((readBits(6) << 1) | (b[3] & 0x01));
            f[3] = (INT8)((readBits(7) << 1) | (f[3] & 0x01));
            b[4] = (INT8)readBits(8);
            f[4] = (INT8)readBits(8);
            b[5] = (INT8)readBits(8);
            f[5] = (INT8)readBits(8);
            break;
    }

    amplitudeInterpolation = 0;
    periodInterpolation = 0;
}

void SP0256::SETMSB_5(INT32 immed4) {
    repeat = (repeatPrefix << 4) | immed4;
    if (repeat == 0)
        return;

    amplitude = (readBits(6) << 2) | (amplitude & 0x03);
    period = readBits(8);
    //periodCounter = (period == 0 ? 0x100 : period);
    if (mode == 0x00 || mode == 0x02) {
        b[5] = 0;
        f[5] = 0;
    }

    switch (mode) {
        case 0x0:
        case 0x1:
            f[0] = (INT8)((readBits(5) << 3) | (f[0] & 0x07));
            f[1] = (INT8)((readBits(5) << 3) | (f[1] & 0x07));
            f[2] = (INT8)((readBits(5) << 3) | (f[2] & 0x07));
            break;
        case 0x2:
        case 0x3:
            f[0] = (INT8)((readBits(6) << 2) | (f[0] & 0x03));
            f[1] = (INT8)((readBits(6) << 2) | (f[1] & 0x03));
            f[2] = (INT8)((readBits(6) << 2) | (f[2] & 0x03));
            break;
    }
}

void SP0256::SETMSB_6(INT32 immed4) {
    repeat = (repeatPrefix << 4) | immed4;
    if (repeat == 0)
        return;

    amplitude = (readBits(6) << 2) | (amplitude & 0x03);
    if (mode == 0x00 || mode == 0x02) {
        b[5] = 0;
        f[5] = 0;
    }

    switch (mode) {
        case 0x0:
            f[3] = (INT8)((readBits(6) << 2) | (f[3] & 0x03));
            f[4] = (INT8)((readBits(6) << 2) | (f[4] & 0x03));
            b[5] = 0;
            f[5] = 0;
            break;
        case 0x1:
            f[3] = (INT8)((readBits(6) << 2) | (f[3] & 0x03));
            f[4] = (INT8)((readBits(6) << 2) | (f[4] & 0x03));
            f[5] = (INT8)readBits(8);
            break;
        case 0x2:
            f[3] = (INT8)((readBits(7) << 1) | (f[3] & 0x01));
            f[4] = (INT8)readBits(8);
            b[5] = 0;
            f[5] = 0;
            break;
        case 0x3:
            f[3] = (INT8)((readBits(7) << 1) | (f[3] & 0x01));
            f[4] = (INT8)readBits(8);
            f[5] = (INT8)readBits(8);
            break;
    }
}

void SP0256::JMP(INT32 immed4) {
    pc = (page << 12) | (flipEndian(immed4, 4) << 8) | readBitsReverse(8);
    bitsLeft = 0;
}

void SP0256::SETMODE(INT32 immed4) {
    immed4 = flipEndian(immed4, 4);
    mode = immed4 & 0x3;
    repeatPrefix = (immed4 & 0xC) >> 2;
}

void SP0256::DELTA_9(INT32 immed4) {
    repeat = (repeatPrefix << 4) | immed4;
    if (repeat == 0)
        return;

    amplitude = (amplitude + 0x10000 + (readDelta(4) << 2))
            & 0xFFFF;
    period = (period + 0x10000 + readDelta(5)) & 0xFFFF;
    //periodCounter = (period == 0 ? 0x100 : period);
    switch (mode) {
        case 0x0:
            b[0] += readDelta(3) << 4;
            f[0] += readDelta(3) << 3;
            b[1] += readDelta(3) << 4;
            f[1] += readDelta(3) << 3;
            b[2] += readDelta(3) << 4;
            f[2] += readDelta(3) << 3;
            b[3] += readDelta(3) << 3;
            f[3] += readDelta(4) << 2;
            b[4] += readDelta(4) << 1;
            f[4] += readDelta(4) << 2;
            break;
        case 0x1:
            b[0] += readDelta(3) << 4;
            f[0] += readDelta(3) << 3;
            b[1] += readDelta(3) << 4;
            f[1] += readDelta(3) << 3;
            b[2] += readDelta(3) << 4;
            f[2] += readDelta(3) << 3;
            b[3] += readDelta(3) << 3;
            f[3] += readDelta(4) << 2;
            b[4] += readDelta(4) << 1;
            f[4] += readDelta(4) << 2;
            b[5] += readDelta(5) << 0;
            f[5] += readDelta(5) << 0;
            break;
        case 0x2:
            b[0] += readDelta(4) << 2;
            f[0] += readDelta(4) << 0;
            b[1] += readDelta(4) << 1;
            f[1] += readDelta(4) << 2;
            b[2] += readDelta(4) << 1;
            f[2] += readDelta(4) << 2;
            b[3] += readDelta(4) << 1;
            f[3] += readDelta(5) << 2;
            b[4] += readDelta(5) << 1;
            f[4] += readDelta(5) << 1;
            break;
        case 0x3:
            b[0] += readDelta(4) << 2;
            f[0] += readDelta(4) << 0;
            b[1] += readDelta(4) << 1;
            f[1] += readDelta(4) << 2;
            b[2] += readDelta(4) << 1;
            f[2] += readDelta(4) << 2;
            b[3] += readDelta(4) << 1;
            f[3] += readDelta(5) << 2;
            b[4] += readDelta(5) << 1;
            f[4] += readDelta(5) << 1;
            b[5] += readDelta(5) << 0;
            f[5] += readDelta(5) << 0;
            break;
    }
}

void SP0256::SETMSB_A(INT32 immed4) {
    repeat = (repeatPrefix << 4) | immed4;
    if (repeat == 0)
        return;

    amplitude = (readBits(6) << 2) | (amplitude & 0x03);
    switch (mode) {
        case 0x0:
        case 0x1:
            f[0] = (INT8)((readBits(5) << 3) | (f[0] & 0x07));
            f[1] = (INT8)((readBits(5) << 3) | (f[1] & 0x07));
            f[2] = (INT8)((readBits(5) << 3) | (f[2] & 0x07));
            break;
        case 0x2:
        case 0x3:
            f[0] = (INT8)((readBits(6) << 2) | (f[0] & 0x03));
            f[1] = (INT8)((readBits(6) << 2) | (f[1] & 0x03));
            f[2] = (INT8)((readBits(6) << 2) | (f[2] & 0x03));
            break;
    }
}

void SP0256::JSR(INT32 immed4) {
    INT32 newpc = (page << 12) | (flipEndian(immed4, 4) << 8) | readBitsReverse(8);
    stack = pc;
    pc = newpc;
    bitsLeft = 0;
}

void SP0256::LOAD_C(INT32 immed4) {
    repeat = (repeatPrefix << 4) | immed4;
    if (repeat == 0)
        return;

    amplitude = (readBits(6) << 2) | (amplitude & 0x03);
    period = readBits(8);
    //periodCounter = (period == 0 ? 0x100 : period);
    switch (mode) {
        case 0x0:
            b[0] = (INT8)((readBits(3) << 4) | (b[0] & 0x0F));
            f[0] = (INT8)((readBits(5) << 3) | (f[0] & 0x07));
            b[1] = (INT8)((readBits(3) << 4) | (b[1] & 0x0F));
            f[1] = (INT8)((readBits(5) << 3) | (f[1] & 0x07));
            b[2] = (INT8)((readBits(3) << 4) | (b[2] & 0x0F));
            f[2] = (INT8)((readBits(5) << 3) | (f[2] & 0x07));
            b[3] = (INT8)((readBits(4) << 3) | (b[3] & 0x07));
            f[3] = (INT8)((readBits(6) << 2) | (f[3] & 0x03));
            b[4] = (INT8)((readBits(7) << 1) | (b[4] & 0x01));
            f[4] = (INT8)((readBits(6) << 2) | (f[4] & 0x03));
            b[5] = 0;
            f[5] = 0;
            break;
        case 0x1:
            b[0] = (INT8)((readBits(3) << 4) | (b[0] & 0x0F));
            f[0] = (INT8)((readBits(5) << 3) | (f[0] & 0x07));
            b[1] = (INT8)((readBits(3) << 4) | (b[1] & 0x0F));
            f[1] = (INT8)((readBits(5) << 3) | (f[1] & 0x07));
            b[2] = (INT8)((readBits(3) << 4) | (b[2] & 0x0F));
            f[2] = (INT8)((readBits(5) << 3) | (f[2] & 0x07));
            b[3] = (INT8)((readBits(4) << 3) | (b[3] & 0x07));
            f[3] = (INT8)((readBits(6) << 2) | (f[3] & 0x03));
            b[4] = (INT8)((readBits(7) << 1) | (b[4] & 0x01));
            f[4] = (INT8)((readBits(6) << 2) | (f[4] & 0x03));
            b[5] = (INT8)readBits(8);
            f[5] = (INT8)readBits(8);
            break;
        case 0x2:
            b[0] = (INT8)((readBits(6) << 1) | (b[0] & 0x01));
            f[0] = (INT8)((readBits(6) << 2) | (f[0] & 0x03));
            b[1] = (INT8)((readBits(6) << 1) | (b[1] & 0x01));
            f[1] = (INT8)((readBits(6) << 2) | (f[1] & 0x03));
            b[2] = (INT8)((readBits(6) << 1) | (b[2] & 0x01));
            f[2] = (INT8)((readBits(6) << 2) | (f[2] & 0x03));
            b[3] = (INT8)((readBits(6) << 1) | (b[3] & 0x01));
            f[3] = (INT8)((readBits(7) << 1) | (f[3] & 0x01));
            b[4] = (INT8)readBits(8);
            f[4] = (INT8)readBits(8);
            b[5] = 0;
            f[5] = 0;
            break;
        case 0x3:
            b[0] = (INT8)((readBits(6) << 1) | (b[0] & 0x01));
            f[0] = (INT8)((readBits(6) << 2) | (f[0] & 0x03));
            b[1] = (INT8)((readBits(6) << 1) | (b[1] & 0x01));
            f[1] = (INT8)((readBits(6) << 2) | (f[1] & 0x03));
            b[2] = (INT8)((readBits(6) << 1) | (b[2] & 0x01));
            f[2] = (INT8)((readBits(6) << 2) | (f[2] & 0x03));
            b[3] = (INT8)((readBits(6) << 1) | (b[3] & 0x01));
            f[3] = (INT8)((readBits(7) << 1) | (f[3] & 0x01));
            b[4] = (INT8)readBits(8);
            f[4] = (INT8)readBits(8);
            b[5] = (INT8)readBits(8);
            f[5] = (INT8)readBits(8);
            break;
    }

    amplitudeInterpolation = 0;
    periodInterpolation = 0;
}

void SP0256::DELTA_D(INT32 immed4) {
    repeat = (repeatPrefix << 4) | immed4;
    if (repeat == 0)
        return;

    amplitude = (amplitude + 0x10000 + (readDelta(4) << 2))
            & 0xFFFF;
    period = (period + 0x10000 + readDelta(5)) & 0xFFFF;
    //periodCounter = (period == 0 ? 0x100 : period);
    switch (mode) {
        case 0x0:
            b[3] += readDelta(3) << 3;
            f[3] += readDelta(4) << 2;
            b[4] += readDelta(4) << 1;
            f[4] += readDelta(4) << 2;
            break;
        case 0x1:
            b[3] += readDelta(3) << 3;
            f[3] += readDelta(4) << 2;
            b[4] += readDelta(4) << 1;
            f[4] += readDelta(4) << 2;
            b[5] += readDelta(5) << 0;
            f[5] += readDelta(5) << 0;
            break;
        case 0x2:
            b[3] += readDelta(4) << 1;
            f[3] += readDelta(5) << 1;
            b[4] += readDelta(5) << 0;
            f[4] += readDelta(5) << 0;
            break;
        case 0x3:
            b[3] += readDelta(4) << 1;
            f[3] += readDelta(5) << 1;
            b[4] += readDelta(5) << 0;
            f[4] += readDelta(5) << 0;
            b[5] += readDelta(5) << 0;
            f[5] += readDelta(5) << 0;
            break;
    }
}

void SP0256::LOAD_E(INT32 immed4) {
    repeat = (repeatPrefix << 4) | immed4;
    if (repeat == 0)
        return;

    amplitude = (readBits(6) << 2) | (amplitude & 0x03);
    period = readBits(8);
    //periodCounter = (period == 0 ? 0x100 : period);
}

void SP0256::PAUSE(INT32 immed4) {
    repeat = (repeatPrefix << 4) | immed4;
    if (repeat == 0)
        return;

    //clear everything
    amplitude = 0;
    period = 0;
    for (INT32 j = 0; j < 6; j++) {
        b[j] = 0;
        f[j] = 0;
    }
    amplitudeInterpolation = 0;
    periodInterpolation = 0;
}

ITCM_CODE void SP0256::decode() {
    INT32 immed4 = readBits(4);
    INT32 nextInstruction = readBitsReverse(4);
    switch (nextInstruction) {
        case 0x0:
            if (immed4 == 0)
                RTS();
            else
                SETPAGE(immed4);
            break;
        case 0x8:
            SETMODE(immed4);
            break;
        case 0x4:
            LOAD_4(immed4);
            break;
        case 0xC:
            LOAD_C(immed4);
            break;
        case 0x2:
            LOAD_2(immed4);
            break;
        case 0xA:
            SETMSB_A(immed4);
            break;
        case 0x6:
            SETMSB_6(immed4);
            break;
        case 0xE:
            LOAD_E(immed4);
            break;
        case 0x1:
            LOADALL(immed4);
            break;
        case 0x9:
            DELTA_9(immed4);
            break;
        case 0x5:
            SETMSB_5(immed4);
            break;
        case 0xD:
            DELTA_D(immed4);
            break;
        case 0x3:
            SETMSB_3(immed4);
            break;
        case 0xB:
            JSR(immed4);
            break;
        case 0x7:
            JMP(immed4);
            break;
        case 0xF:
            PAUSE(immed4);
            break;
    }
}

ITCM_CODE INT32 SP0256::flipEndian(INT32 value, INT32 bits) {
    INT32 output = 0;
    INT32 bitMask = 1;
    for (INT32 i = 0; i < bits; i++) {
        INT32 offset = (bits-1)-(i<<1);
        if (offset > 0)
            output |= (value & bitMask) << offset;
        else
            output |= (value & bitMask) >> -offset;
        bitMask = bitMask << 1;
    }
    return output;
}

void SP0256::getState(SP0256State *state)
{
    state->bitsLeft = bitsLeft;
    state->currentBits = currentBits;
    state->pc = pc;
    state->stack = stack;
    state->mode = mode;
    state->repeatPrefix = repeatPrefix;
    state->page = page;
    state->command = command;
    state->repeat = repeat;
    state->period = period;
    state->periodCounter = periodCounter;
    state->amplitude = amplitude;
    state->random = random;
    state->fifoHead = fifoHead;
    state->fifoSize = fifoSize;
    for (int i=0; i<64; i++) state->fifoBytes[i] = fifoBytes[i];
    for (int i=0; i<6; i++)
    {
        state->y[i][0] = y[i][0];
        state->y[i][1] = y[i][1];
        state->b[i] = b[i];
        state->f[i] = f[i];
    }
    state->periodInterpolation = periodInterpolation;
    state->amplitudeInterpolation = amplitudeInterpolation;
    state->idle = idle;
    state->lrqHigh = lrqHigh;
    state->speaking = speaking;
}

void SP0256::setState(SP0256State *state)
{
    bitsLeft = state->bitsLeft;
    currentBits = state->currentBits;
    pc = state->pc;
    stack = state->stack;
    mode = state->mode;
    repeatPrefix = state->repeatPrefix;
    page = state->page;
    command = state->command;
    repeat = state->repeat;
    period = state->period;
    periodCounter = state->periodCounter;
    amplitude = state->amplitude;
    random = state->random;
    fifoHead = state->fifoHead;
    fifoSize = state->fifoSize;
    for (int i=0; i<64; i++) fifoBytes[i] = state->fifoBytes[i];
    for (int i=0; i<6; i++)
    {
        y[i][0] = state->y[i][0];
        y[i][1] = state->y[i][1];
        b[i] = state->b[i];
        f[i] = state->f[i];
    }
    periodInterpolation = state->periodInterpolation;
    amplitudeInterpolation = state->amplitudeInterpolation;
    idle = state->idle;
    lrqHigh = state->lrqHigh;
    speaking = state->speaking;    
}
