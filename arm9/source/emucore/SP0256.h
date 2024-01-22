// =====================================================================================
// Copyright (c) 2021-2024 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef MICROSEQUENCER_H
#define MICROSEQUENCER_H

#include "AudioProducer.h"
#include "SP0256_Registers.h"
#include "types.h"
#include "Processor.h"
#include "ROM.h"
#include "AudioOutputLine.h"

#define FIFO_LOCATION 0x1800
#define FIFO_MAX_SIZE 64

TYPEDEF_STRUCT_PACK( _SP0256State
{
    INT32 bitsLeft;
    INT32 currentBits;
    INT32 pc;
    INT32 stack;
    INT32 mode;
    INT32 repeatPrefix;
    INT32 page;
    INT32 command;
    INT32 repeat;
    INT32 period;
    INT32 periodCounter;
    INT32 amplitude;
    INT32 random;
    INT32 fifoHead;
    INT32 fifoSize;
    INT32 fifoBytes[64];
    INT32 y[6][2];
    INT8  b[6];
    INT8  f[6];
    INT8  periodInterpolation;
    INT8  amplitudeInterpolation;
    INT8  idle;
    INT8  lrqHigh;
    INT8  speaking;
} SP0256State; )

//registers
extern INT32 repeat;
extern INT32 period;
extern INT32 periodCounter;
extern INT32 amplitude;
extern INT8  b[6];
extern INT8  f[6];
extern INT32 y[6][2];
extern INT8  periodInterpolation;
extern INT8  amplitudeInterpolation;

extern  INT32 bitsLeft;
extern  INT32 currentBits;

//registers
extern  UINT16 pc;
extern  UINT16 stack;
extern  UINT8  mode;
extern  INT32 repeatPrefix;
extern  INT32 command;

extern UINT16 fifoBytes[64];


class SP0256 : public Processor, public AudioProducer
{
    friend class SP0256_Registers;

    public:
        SP0256();
        ~SP0256();
        void resetProcessor();
        INT32 getClockSpeed();
        INT32 getClocksPerSample();
        INT32 getSampleRate() { return getClockSpeed(); }
        INT32 tick(INT32);
        inline BOOL isIdle() { return idle; }

        SP0256_Registers registers;
        ROM        ivoiceROM;
        
        void getState(SP0256State *state);
        void setState(SP0256State *state);

    private:
        INT8 readDelta(INT32 numBits);
        INT32 readBits(INT32 numBits);
        INT32 readBitsReverse(INT32 numBits);
        void RTS();
        void SETPAGE(INT32 immed4);
        void LOADALL(INT32 immed4);
        void LOAD_2(INT32 immed4);
        void SETMSB_3(INT32 immed4);
        void LOAD_4(INT32 immed4);
        void SETMSB_5(INT32 immed4);
        void SETMSB_6(INT32 immed4);
        void JMP(INT32 immed4);
        void SETMODE(INT32 immed4);
        void DELTA_9(INT32 immed4);
        void SETMSB_A(INT32 immed4);
        void JSR(INT32 immed4);
        void LOAD_C(INT32 immed4);
        void DELTA_D(INT32 immed4);
        void LOAD_E(INT32 immed4);
        void PAUSE(INT32 immed4);
        void decode();
        static INT32 flipEndian(INT32 value, INT32 bits);
        
        UINT8   idle;
        UINT8   lrqHigh;
        UINT8   speaking;
        INT16   fifoHead;
        INT16   fifoSize;
        INT32   command;
        INT32   random;
        INT32   page;
};

#endif
