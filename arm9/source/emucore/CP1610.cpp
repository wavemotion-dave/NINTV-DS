// =====================================================================================
// Copyright (c) 2021-2025 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#include <nds.h>
#include <string.h>
#include "CP1610.h"
#include "types.h"
#include "../debugger.h"

// ------------------------------------------------------------------------------
// We store the "fast buffer" out in video RAM which is faster than main RAM if 
// there is a cache "miss". Experimentally, this buys us about 10% speed up.
// ------------------------------------------------------------------------------
#define PEEK_FAST(x) *((UINT16 *)(0x06860000 | ((x)<<1)))

//the eight registers available in the CP1610
UINT16 r[8] __attribute__((section(".dtcm")));

//the six flags available in the CP1610
UINT8 S __attribute__((section(".dtcm")));
UINT8 Z __attribute__((section(".dtcm")));
UINT8 O __attribute__((section(".dtcm")));
UINT8 C __attribute__((section(".dtcm")));
UINT8 I __attribute__((section(".dtcm")));
UINT8 D __attribute__((section(".dtcm")));

//indicates whether the last executed instruction is interruptible
UINT8 interruptible __attribute__((section(".dtcm")));

UINT8 bCP1610_PIN_IN_BUSRQ   __attribute__((section(".dtcm")));
UINT8 bCP1610_PIN_IN_INTRM   __attribute__((section(".dtcm"))); 
UINT8 bCP1610_PIN_OUT_BUSAK  __attribute__((section(".dtcm")));
UINT8 bHandleInterrupts      __attribute__((section(".dtcm"))) = 0;

INT8 ext __attribute__((section(".dtcm")));

CP1610::CP1610(MemoryBus* m, UINT16 resetAddress,
        UINT16 interruptAddress)
    : Processor("CP1610"),
      memoryBus(m),
      resetAddress(resetAddress),
      interruptAddress(interruptAddress)
{
}

// The clock speed is hard-coded to the NTSC frequency...
INT32 CP1610::getClockSpeed() {
    return NTSC_FREQUENCY;
}

void CP1610::resetProcessor()
{
    //the four external lines
    ext = 0;

    bCP1610_PIN_OUT_BUSAK = TRUE;
    interruptible = FALSE;
    S = C = O = Z = I = D = FALSE;
    for (INT32 i = 0; i < 7; i++)
        r[i] = 0;
    r[7] = resetAddress;
    bHandleInterrupts = (!bCP1610_PIN_IN_BUSRQ || (I && !bCP1610_PIN_IN_INTRM));
}

/**
 * This method ticks the CPU and returns the number of cycles that were
 * used up, indicating to the main emulation loop when the CPU will need
 * to be ticked again.
 */
INT32 CP1610::tick(INT32 minimum)
{
    UINT32 usedCycles = 0;
    
    // ---------------------------------------------------------------------
    // Small speedup so we don't have to shift usedCycles on every loop..
    // ---------------------------------------------------------------------
    int min_shifted = (minimum >> 2);
    if (minimum & 0x03)  min_shifted++;
    
    do {
        // This chunk of code doesn't happen very often (less than 10%) so we use the unlikely() gcc switch to help optmize
        if (unlikely(bHandleInterrupts))
        {
            if (!bCP1610_PIN_IN_BUSRQ || (I && !bCP1610_PIN_IN_INTRM))
            {
                if ((!bCP1610_PIN_IN_BUSRQ))
                {
                    if (interruptible) 
                    {
                        bCP1610_PIN_OUT_BUSAK = bCP1610_PIN_IN_BUSRQ;
                        return MAX((usedCycles<<2), minimum);
                    }
                }
                else // if ((I && !bCP1610_PIN_IN_INTRM))
                {
                    if (interruptible) 
                    {
                        bCP1610_PIN_IN_INTRM = TRUE;
                        bHandleInterrupts = (!bCP1610_PIN_IN_BUSRQ || (I && !bCP1610_PIN_IN_INTRM));
                        interruptible = false;
                        memoryBus->poke(r[6], r[7]);
                        r[6]++;
                        r[7] = interruptAddress;
                        usedCycles += 7;
                        if ((usedCycles << 2) >= minimum)
                            return (usedCycles<<2);
                    }
                }
            }
        }

        //do the next instruction
        UINT16 op = *((UINT16 *)(0x06860000 | (r[7]<<1)));
#ifdef DEBUG_ENABLE        
        debug_opcodes++;
        debugger();
#endif
        // Handle the instruction 
        switch (op) {
            case 0x0000:
                usedCycles += HLT();
                break;            
            case 0x0001:
                usedCycles += SDBD();
                break;
            case 0x0002:
                usedCycles += EIS();
                break;
            case 0x0003:
                usedCycles += DIS();
                break;
            case 0x0004:
                {
                    int read = PEEK_FAST((UINT16)(r[7] + 1));
                    int reg = ((read & 0x0300) >> 8);
                    int interrupt = (read & 0x0003);
                    UINT16 target = (UINT16)((read & 0x00FC) << 8);
                    read = PEEK_FAST((UINT16)(r[7] + 2));
                    target |= (UINT16)(read & 0x03FF);
                    if (reg == 3) {
                        if (interrupt == 0)
                            usedCycles += J(target);
                        else if (interrupt == 1)
                            usedCycles += JE(target);
                        else if (interrupt == 2)
                            usedCycles += JD(target);
                        else
                            usedCycles += HLT(); //invalid opcode
                    }
                    else {
                        if (interrupt == 0)
                            usedCycles += JSR((UINT16)(reg + 4), target);
                        else if (interrupt == 1)
                            usedCycles += JSRE((UINT16)(reg + 4), target);
                        else if (interrupt == 2)
                            usedCycles += JSRD((UINT16)(reg + 4), target);
                        else
                            usedCycles += HLT(); //invalid opcode
                    }
                }
                break;
            case 0x0005:
                usedCycles += TCI();
                break;
            case 0x0006:
                usedCycles += CLRC();
                break;
            case 0x0007:
                usedCycles += SETC();
                break;
            case 0x0008:
                usedCycles += INCR(0);
                break;
            case 0x0009:
                usedCycles += INCR(1);
                break;
            case 0x000A:
                usedCycles += INCR(2);
                break;
            case 0x000B:
                usedCycles += INCR(3);
                break;
            case 0x000C:
                usedCycles += INCR(4);
                break;
            case 0x000D:
                usedCycles += INCR(5);
                break;
            case 0x000E:
                usedCycles += INCR(6);
                break;
            case 0x000F:
                usedCycles += INCR(7);
                break;
            case 0x0010:
                usedCycles += DECR(0);
                break;
            case 0x0011:
                usedCycles += DECR(1);
                break;
            case 0x0012:
                usedCycles += DECR(2);
                break;
            case 0x0013:
                usedCycles += DECR(3);
                break;
            case 0x0014:
                usedCycles += DECR(4);
                break;
            case 0x0015:
                usedCycles += DECR(5);
                break;
            case 0x0016:
                usedCycles += DECR(6);
                break;
            case 0x0017:
                usedCycles += DECR(7);
                break;
            case 0x0018:
                usedCycles += COMR(0);
                break;
            case 0x0019:
                usedCycles += COMR(1);
                break;
            case 0x001A:
                usedCycles += COMR(2);
                break;
            case 0x001B:
                usedCycles += COMR(3);
                break;
            case 0x001C:
                usedCycles += COMR(4);
                break;
            case 0x001D:
                usedCycles += COMR(5);
                break;
            case 0x001E:
                usedCycles += COMR(6);
                break;
            case 0x001F:
                usedCycles += COMR(7);
                break;
            case 0x0020:
                usedCycles += NEGR(0);
                break;
            case 0x0021:
                usedCycles += NEGR(1);
                break;
            case 0x0022:
                usedCycles += NEGR(2);
                break;
            case 0x0023:
                usedCycles += NEGR(3);
                break;
            case 0x0024:
                usedCycles += NEGR(4);
                break;
            case 0x0025:
                usedCycles += NEGR(5);
                break;
            case 0x0026:
                usedCycles += NEGR(6);
                break;
            case 0x0027:
                usedCycles += NEGR(7);
                break;
            case 0x0028:
                usedCycles += ADCR(0);
                break;
            case 0x0029:
                usedCycles += ADCR(1);
                break;
            case 0x002A:
                usedCycles += ADCR(2);
                break;
            case 0x002B:
                usedCycles += ADCR(3);
                break;
            case 0x002C:
                usedCycles += ADCR(4);
                break;
            case 0x002D:
                usedCycles += ADCR(5);
                break;
            case 0x002E:
                usedCycles += ADCR(6);
                break;
            case 0x002F:
                usedCycles += ADCR(7);
                break;
            case 0x0030:
                usedCycles += GSWD(0);
                break;
            case 0x0031:
                usedCycles += GSWD(1);
                break;
            case 0x0032:
                usedCycles += GSWD(2);
                break;
            case 0x0033:
                usedCycles += GSWD(3);
                break;
            case 0x0034:
                usedCycles += NOP(0);
                break;
            case 0x0035:
                usedCycles += NOP(1);
                break;
            case 0x0036:
                usedCycles += SIN(0);
                break;
            case 0x0037:
                usedCycles += SIN(1);
                break;
            case 0x0038:
                usedCycles += RSWD(0);
                break;
            case 0x0039:
                usedCycles += RSWD(1);
                break;
            case 0x003A:
                usedCycles += RSWD(2);
                break;
            case 0x003B:
                usedCycles += RSWD(3);
                break;
            case 0x003C:
                usedCycles += RSWD(4);
                break;
            case 0x003D:
                usedCycles += RSWD(5);
                break;
            case 0x003E:
                usedCycles += RSWD(6);
                break;
            case 0x003F:
                usedCycles += RSWD(7);
                break;
            case 0x0040:
                usedCycles += SWAP_1(0);
                break;
            case 0x0041:
                usedCycles += SWAP_1(1);
                break;
            case 0x0042:
                usedCycles += SWAP_1(2);
                break;
            case 0x0043:
                usedCycles += SWAP_1(3);
                break;
            case 0x0044:
                usedCycles += SWAP_2(0);
                break;
            case 0x0045:
                usedCycles += SWAP_2(1);
                break;
            case 0x0046:
                usedCycles += SWAP_2(2);
                break;
            case 0x0047:
                usedCycles += SWAP_2(3);
                break;
            case 0x0048:
                usedCycles += SLL_1(0);
                break;
            case 0x0049:
                usedCycles += SLL_1(1);
                break;
            case 0x004A:
                usedCycles += SLL_1(2);
                break;
            case 0x004B:
                usedCycles += SLL_1(3);
                break;
            case 0x004C:
                usedCycles += SLL_2(0);
                break;
            case 0x004D:
                usedCycles += SLL_2(1);
                break;
            case 0x004E:
                usedCycles += SLL_2(2);
                break;
            case 0x004F:
                usedCycles += SLL_2(3);
                break;
            case 0x0050:
                usedCycles += RLC_1(0);
                break;
            case 0x0051:
                usedCycles += RLC_1(1);
                break;
            case 0x0052:
                usedCycles += RLC_1(2);
                break;
            case 0x0053:
                usedCycles += RLC_1(3);
                break;
            case 0x0054:
                usedCycles += RLC_2(0);
                break;
            case 0x0055:
                usedCycles += RLC_2(1);
                break;
            case 0x0056:
                usedCycles += RLC_2(2);
                break;
            case 0x0057:
                usedCycles += RLC_2(3);
                break;
            case 0x0058:
                usedCycles += SLLC_1(0);
                break;
            case 0x0059:
                usedCycles += SLLC_1(1);
                break;
            case 0x005A:
                usedCycles += SLLC_1(2);
                break;
            case 0x005B:
                usedCycles += SLLC_1(3);
                break;
            case 0x005C:
                usedCycles += SLLC_2(0);
                break;
            case 0x005D:
                usedCycles += SLLC_2(1);
                break;
            case 0x005E:
                usedCycles += SLLC_2(2);
                break;
            case 0x005F:
                usedCycles += SLLC_2(3);
                break;
            case 0x0060:
                usedCycles += SLR_1(0);
                break;
            case 0x0061:
                usedCycles += SLR_1(1);
                break;
            case 0x0062:
                usedCycles += SLR_1(2);
                break;
            case 0x0063:
                usedCycles += SLR_1(3);
                break;
            case 0x0064:
                usedCycles += SLR_2(0);
                break;
            case 0x0065:
                usedCycles += SLR_2(1);
                break;
            case 0x0066:
                usedCycles += SLR_2(2);
                break;
            case 0x0067:
                usedCycles += SLR_2(3);
                break;
            case 0x0068:
                usedCycles += SAR_1(0);
                break;
            case 0x0069:
                usedCycles += SAR_1(1);
                break;
            case 0x006A:
                usedCycles += SAR_1(2);
                break;
            case 0x006B:
                usedCycles += SAR_1(3);
                break;
            case 0x006C:
                usedCycles += SAR_2(0);
                break;
            case 0x006D:
                usedCycles += SAR_2(1);
                break;
            case 0x006E:
                usedCycles += SAR_2(2);
                break;
            case 0x006F:
                usedCycles += SAR_2(3);
                break;
            case 0x0070:
                usedCycles += RRC_1(0);
                break;
            case 0x0071:
                usedCycles += RRC_1(1);
                break;
            case 0x0072:
                usedCycles += RRC_1(2);
                break;
            case 0x0073:
                usedCycles += RRC_1(3);
                break;
            case 0x0074:
                usedCycles += RRC_2(0);
                break;
            case 0x0075:
                usedCycles += RRC_2(1);
                break;
            case 0x0076:
                usedCycles += RRC_2(2);
                break;
            case 0x0077:
                usedCycles += RRC_2(3);
                break;
            case 0x0078:
                usedCycles += SARC_1(0);
                break;
            case 0x0079:
                usedCycles += SARC_1(1);
                break;
            case 0x007A:
                usedCycles += SARC_1(2);
                break;
            case 0x007B:
                usedCycles += SARC_1(3);
                break;
            case 0x007C:
                usedCycles += SARC_2(0);
                break;
            case 0x007D:
                usedCycles += SARC_2(1);
                break;
            case 0x007E:
                usedCycles += SARC_2(2);
                break;
            case 0x007F:
                usedCycles += SARC_2(3);
                break;
            case 0x0080:
                usedCycles += MOVR(0, 0);
                break;
            case 0x0081:
                usedCycles += MOVR(0, 1);
                break;
            case 0x0082:
                usedCycles += MOVR(0, 2);
                break;
            case 0x0083:
                usedCycles += MOVR(0, 3);
                break;
            case 0x0084:
                usedCycles += MOVR(0, 4);
                break;
            case 0x0085:
                usedCycles += MOVR(0, 5);
                break;
            case 0x0086:
                usedCycles += MOVR(0, 6);
                break;
            case 0x0087:
                usedCycles += MOVR(0, 7);
                break;
            case 0x0088:
                usedCycles += MOVR(1, 0);
                break;
            case 0x0089:
                usedCycles += MOVR(1, 1);
                break;
            case 0x008A:
                usedCycles += MOVR(1, 2);
                break;
            case 0x008B:
                usedCycles += MOVR(1, 3);
                break;
            case 0x008C:
                usedCycles += MOVR(1, 4);
                break;
            case 0x008D:
                usedCycles += MOVR(1, 5);
                break;
            case 0x008E:
                usedCycles += MOVR(1, 6);
                break;
            case 0x008F:
                usedCycles += MOVR(1, 7);
                break;
            case 0x0090:
                usedCycles += MOVR(2, 0);
                break;
            case 0x0091:
                usedCycles += MOVR(2, 1);
                break;
            case 0x0092:
                usedCycles += MOVR(2, 2);
                break;
            case 0x0093:
                usedCycles += MOVR(2, 3);
                break;
            case 0x0094:
                usedCycles += MOVR(2, 4);
                break;
            case 0x0095:
                usedCycles += MOVR(2, 5);
                break;
            case 0x0096:
                usedCycles += MOVR(2, 6);
                break;
            case 0x0097:
                usedCycles += MOVR(2, 7);
                break;
            case 0x0098:
                usedCycles += MOVR(3, 0);
                break;
            case 0x0099:
                usedCycles += MOVR(3, 1);
                break;
            case 0x009A:
                usedCycles += MOVR(3, 2);
                break;
            case 0x009B:
                usedCycles += MOVR(3, 3);
                break;
            case 0x009C:
                usedCycles += MOVR(3, 4);
                break;
            case 0x009D:
                usedCycles += MOVR(3, 5);
                break;
            case 0x009E:
                usedCycles += MOVR(3, 6);
                break;
            case 0x009F:
                usedCycles += MOVR(3, 7);
                break;
            case 0x00A0:
                usedCycles += MOVR(4, 0);
                break;
            case 0x00A1:
                usedCycles += MOVR(4, 1);
                break;
            case 0x00A2:
                usedCycles += MOVR(4, 2);
                break;
            case 0x00A3:
                usedCycles += MOVR(4, 3);
                break;
            case 0x00A4:
                usedCycles += MOVR(4, 4);
                break;
            case 0x00A5:
                usedCycles += MOVR(4, 5);
                break;
            case 0x00A6:
                usedCycles += MOVR(4, 6);
                break;
            case 0x00A7:
                usedCycles += MOVR(4, 7);
                break;
            case 0x00A8:
                usedCycles += MOVR(5, 0);
                break;
            case 0x00A9:
                usedCycles += MOVR(5, 1);
                break;
            case 0x00AA:
                usedCycles += MOVR(5, 2);
                break;
            case 0x00AB:
                usedCycles += MOVR(5, 3);
                break;
            case 0x00AC:
                usedCycles += MOVR(5, 4);
                break;
            case 0x00AD:
                usedCycles += MOVR(5, 5);
                break;
            case 0x00AE:
                usedCycles += MOVR(5, 6);
                break;
            case 0x00AF:
                usedCycles += MOVR(5, 7);
                break;
            case 0x00B0:
                usedCycles += MOVR(6, 0);
                break;
            case 0x00B1:
                usedCycles += MOVR(6, 1);
                break;
            case 0x00B2:
                usedCycles += MOVR(6, 2);
                break;
            case 0x00B3:
                usedCycles += MOVR(6, 3);
                break;
            case 0x00B4:
                usedCycles += MOVR(6, 4);
                break;
            case 0x00B5:
                usedCycles += MOVR(6, 5);
                break;
            case 0x00B6:
                usedCycles += MOVR(6, 6);
                break;
            case 0x00B7:
                usedCycles += MOVR(6, 7);
                break;
            case 0x00B8:
                usedCycles += MOVR(7, 0);
                break;
            case 0x00B9:
                usedCycles += MOVR(7, 1);
                break;
            case 0x00BA:
                usedCycles += MOVR(7, 2);
                break;
            case 0x00BB:
                usedCycles += MOVR(7, 3);
                break;
            case 0x00BC:
                usedCycles += MOVR(7, 4);
                break;
            case 0x00BD:
                usedCycles += MOVR(7, 5);
                break;
            case 0x00BE:
                usedCycles += MOVR(7, 6);
                break;
            case 0x00BF:
                usedCycles += MOVR(7, 7);
                break;
            case 0x00C0:
                usedCycles += ADDR(0, 0);
                break;
            case 0x00C1:
                usedCycles += ADDR(0, 1);
                break;
            case 0x00C2:
                usedCycles += ADDR(0, 2);
                break;
            case 0x00C3:
                usedCycles += ADDR(0, 3);
                break;
            case 0x00C4:
                usedCycles += ADDR(0, 4);
                break;
            case 0x00C5:
                usedCycles += ADDR(0, 5);
                break;
            case 0x00C6:
                usedCycles += ADDR(0, 6);
                break;
            case 0x00C7:
                usedCycles += ADDR(0, 7);
                break;
            case 0x00C8:
                usedCycles += ADDR(1, 0);
                break;
            case 0x00C9:
                usedCycles += ADDR(1, 1);
                break;
            case 0x00CA:
                usedCycles += ADDR(1, 2);
                break;
            case 0x00CB:
                usedCycles += ADDR(1, 3);
                break;
            case 0x00CC:
                usedCycles += ADDR(1, 4);
                break;
            case 0x00CD:
                usedCycles += ADDR(1, 5);
                break;
            case 0x00CE:
                usedCycles += ADDR(1, 6);
                break;
            case 0x00CF:
                usedCycles += ADDR(1, 7);
                break;
            case 0x00D0:
                usedCycles += ADDR(2, 0);
                break;
            case 0x00D1:
                usedCycles += ADDR(2, 1);
                break;
            case 0x00D2:
                usedCycles += ADDR(2, 2);
                break;
            case 0x00D3:
                usedCycles += ADDR(2, 3);
                break;
            case 0x00D4:
                usedCycles += ADDR(2, 4);
                break;
            case 0x00D5:
                usedCycles += ADDR(2, 5);
                break;
            case 0x00D6:
                usedCycles += ADDR(2, 6);
                break;
            case 0x00D7:
                usedCycles += ADDR(2, 7);
                break;
            case 0x00D8:
                usedCycles += ADDR(3, 0);
                break;
            case 0x00D9:
                usedCycles += ADDR(3, 1);
                break;
            case 0x00DA:
                usedCycles += ADDR(3, 2);
                break;
            case 0x00DB:
                usedCycles += ADDR(3, 3);
                break;
            case 0x00DC:
                usedCycles += ADDR(3, 4);
                break;
            case 0x00DD:
                usedCycles += ADDR(3, 5);
                break;
            case 0x00DE:
                usedCycles += ADDR(3, 6);
                break;
            case 0x00DF:
                usedCycles += ADDR(3, 7);
                break;
            case 0x00E0:
                usedCycles += ADDR(4, 0);
                break;
            case 0x00E1:
                usedCycles += ADDR(4, 1);
                break;
            case 0x00E2:
                usedCycles += ADDR(4, 2);
                break;
            case 0x00E3:
                usedCycles += ADDR(4, 3);
                break;
            case 0x00E4:
                usedCycles += ADDR(4, 4);
                break;
            case 0x00E5:
                usedCycles += ADDR(4, 5);
                break;
            case 0x00E6:
                usedCycles += ADDR(4, 6);
                break;
            case 0x00E7:
                usedCycles += ADDR(4, 7);
                break;
            case 0x00E8:
                usedCycles += ADDR(5, 0);
                break;
            case 0x00E9:
                usedCycles += ADDR(5, 1);
                break;
            case 0x00EA:
                usedCycles += ADDR(5, 2);
                break;
            case 0x00EB:
                usedCycles += ADDR(5, 3);
                break;
            case 0x00EC:
                usedCycles += ADDR(5, 4);
                break;
            case 0x00ED:
                usedCycles += ADDR(5, 5);
                break;
            case 0x00EE:
                usedCycles += ADDR(5, 6);
                break;
            case 0x00EF:
                usedCycles += ADDR(5, 7);
                break;
            case 0x00F0:
                usedCycles += ADDR(6, 0);
                break;
            case 0x00F1:
                usedCycles += ADDR(6, 1);
                break;
            case 0x00F2:
                usedCycles += ADDR(6, 2);
                break;
            case 0x00F3:
                usedCycles += ADDR(6, 3);
                break;
            case 0x00F4:
                usedCycles += ADDR(6, 4);
                break;
            case 0x00F5:
                usedCycles += ADDR(6, 5);
                break;
            case 0x00F6:
                usedCycles += ADDR(6, 6);
                break;
            case 0x00F7:
                usedCycles += ADDR(6, 7);
                break;
            case 0x00F8:
                usedCycles += ADDR(7, 0);
                break;
            case 0x00F9:
                usedCycles += ADDR(7, 1);
                break;
            case 0x00FA:
                usedCycles += ADDR(7, 2);
                break;
            case 0x00FB:
                usedCycles += ADDR(7, 3);
                break;
            case 0x00FC:
                usedCycles += ADDR(7, 4);
                break;
            case 0x00FD:
                usedCycles += ADDR(7, 5);
                break;
            case 0x00FE:
                usedCycles += ADDR(7, 6);
                break;
            case 0x00FF:
                usedCycles += ADDR(7, 7);
                break;
            case 0x0100:
                usedCycles += SUBR(0, 0);
                break;
            case 0x0101:
                usedCycles += SUBR(0, 1);
                break;
            case 0x0102:
                usedCycles += SUBR(0, 2);
                break;
            case 0x0103:
                usedCycles += SUBR(0, 3);
                break;
            case 0x0104:
                usedCycles += SUBR(0, 4);
                break;
            case 0x0105:
                usedCycles += SUBR(0, 5);
                break;
            case 0x0106:
                usedCycles += SUBR(0, 6);
                break;
            case 0x0107:
                usedCycles += SUBR(0, 7);
                break;
            case 0x0108:
                usedCycles += SUBR(1, 0);
                break;
            case 0x0109:
                usedCycles += SUBR(1, 1);
                break;
            case 0x010A:
                usedCycles += SUBR(1, 2);
                break;
            case 0x010B:
                usedCycles += SUBR(1, 3);
                break;
            case 0x010C:
                usedCycles += SUBR(1, 4);
                break;
            case 0x010D:
                usedCycles += SUBR(1, 5);
                break;
            case 0x010E:
                usedCycles += SUBR(1, 6);
                break;
            case 0x010F:
                usedCycles += SUBR(1, 7);
                break;
            case 0x0110:
                usedCycles += SUBR(2, 0);
                break;
            case 0x0111:
                usedCycles += SUBR(2, 1);
                break;
            case 0x0112:
                usedCycles += SUBR(2, 2);
                break;
            case 0x0113:
                usedCycles += SUBR(2, 3);
                break;
            case 0x0114:
                usedCycles += SUBR(2, 4);
                break;
            case 0x0115:
                usedCycles += SUBR(2, 5);
                break;
            case 0x0116:
                usedCycles += SUBR(2, 6);
                break;
            case 0x0117:
                usedCycles += SUBR(2, 7);
                break;
            case 0x0118:
                usedCycles += SUBR(3, 0);
                break;
            case 0x0119:
                usedCycles += SUBR(3, 1);
                break;
            case 0x011A:
                usedCycles += SUBR(3, 2);
                break;
            case 0x011B:
                usedCycles += SUBR(3, 3);
                break;
            case 0x011C:
                usedCycles += SUBR(3, 4);
                break;
            case 0x011D:
                usedCycles += SUBR(3, 5);
                break;
            case 0x011E:
                usedCycles += SUBR(3, 6);
                break;
            case 0x011F:
                usedCycles += SUBR(3, 7);
                break;
            case 0x0120:
                usedCycles += SUBR(4, 0);
                break;
            case 0x0121:
                usedCycles += SUBR(4, 1);
                break;
            case 0x0122:
                usedCycles += SUBR(4, 2);
                break;
            case 0x0123:
                usedCycles += SUBR(4, 3);
                break;
            case 0x0124:
                usedCycles += SUBR(4, 4);
                break;
            case 0x0125:
                usedCycles += SUBR(4, 5);
                break;
            case 0x0126:
                usedCycles += SUBR(4, 6);
                break;
            case 0x0127:
                usedCycles += SUBR(4, 7);
                break;
            case 0x0128:
                usedCycles += SUBR(5, 0);
                break;
            case 0x0129:
                usedCycles += SUBR(5, 1);
                break;
            case 0x012A:
                usedCycles += SUBR(5, 2);
                break;
            case 0x012B:
                usedCycles += SUBR(5, 3);
                break;
            case 0x012C:
                usedCycles += SUBR(5, 4);
                break;
            case 0x012D:
                usedCycles += SUBR(5, 5);
                break;
            case 0x012E:
                usedCycles += SUBR(5, 6);
                break;
            case 0x012F:
                usedCycles += SUBR(5, 7);
                break;
            case 0x0130:
                usedCycles += SUBR(6, 0);
                break;
            case 0x0131:
                usedCycles += SUBR(6, 1);
                break;
            case 0x0132:
                usedCycles += SUBR(6, 2);
                break;
            case 0x0133:
                usedCycles += SUBR(6, 3);
                break;
            case 0x0134:
                usedCycles += SUBR(6, 4);
                break;
            case 0x0135:
                usedCycles += SUBR(6, 5);
                break;
            case 0x0136:
                usedCycles += SUBR(6, 6);
                break;
            case 0x0137:
                usedCycles += SUBR(6, 7);
                break;
            case 0x0138:
                usedCycles += SUBR(7, 0);
                break;
            case 0x0139:
                usedCycles += SUBR(7, 1);
                break;
            case 0x013A:
                usedCycles += SUBR(7, 2);
                break;
            case 0x013B:
                usedCycles += SUBR(7, 3);
                break;
            case 0x013C:
                usedCycles += SUBR(7, 4);
                break;
            case 0x013D:
                usedCycles += SUBR(7, 5);
                break;
            case 0x013E:
                usedCycles += SUBR(7, 6);
                break;
            case 0x013F:
                usedCycles += SUBR(7, 7);
                break;
            case 0x0140:
                usedCycles += CMPR(0, 0);
                break;
            case 0x0141:
                usedCycles += CMPR(0, 1);
                break;
            case 0x0142:
                usedCycles += CMPR(0, 2);
                break;
            case 0x0143:
                usedCycles += CMPR(0, 3);
                break;
            case 0x0144:
                usedCycles += CMPR(0, 4);
                break;
            case 0x0145:
                usedCycles += CMPR(0, 5);
                break;
            case 0x0146:
                usedCycles += CMPR(0, 6);
                break;
            case 0x0147:
                usedCycles += CMPR(0, 7);
                break;
            case 0x0148:
                usedCycles += CMPR(1, 0);
                break;
            case 0x0149:
                usedCycles += CMPR(1, 1);
                break;
            case 0x014A:
                usedCycles += CMPR(1, 2);
                break;
            case 0x014B:
                usedCycles += CMPR(1, 3);
                break;
            case 0x014C:
                usedCycles += CMPR(1, 4);
                break;
            case 0x014D:
                usedCycles += CMPR(1, 5);
                break;
            case 0x014E:
                usedCycles += CMPR(1, 6);
                break;
            case 0x014F:
                usedCycles += CMPR(1, 7);
                break;
            case 0x0150:
                usedCycles += CMPR(2, 0);
                break;
            case 0x0151:
                usedCycles += CMPR(2, 1);
                break;
            case 0x0152:
                usedCycles += CMPR(2, 2);
                break;
            case 0x0153:
                usedCycles += CMPR(2, 3);
                break;
            case 0x0154:
                usedCycles += CMPR(2, 4);
                break;
            case 0x0155:
                usedCycles += CMPR(2, 5);
                break;
            case 0x0156:
                usedCycles += CMPR(2, 6);
                break;
            case 0x0157:
                usedCycles += CMPR(2, 7);
                break;
            case 0x0158:
                usedCycles += CMPR(3, 0);
                break;
            case 0x0159:
                usedCycles += CMPR(3, 1);
                break;
            case 0x015A:
                usedCycles += CMPR(3, 2);
                break;
            case 0x015B:
                usedCycles += CMPR(3, 3);
                break;
            case 0x015C:
                usedCycles += CMPR(3, 4);
                break;
            case 0x015D:
                usedCycles += CMPR(3, 5);
                break;
            case 0x015E:
                usedCycles += CMPR(3, 6);
                break;
            case 0x015F:
                usedCycles += CMPR(3, 7);
                break;
            case 0x0160:
                usedCycles += CMPR(4, 0);
                break;
            case 0x0161:
                usedCycles += CMPR(4, 1);
                break;
            case 0x0162:
                usedCycles += CMPR(4, 2);
                break;
            case 0x0163:
                usedCycles += CMPR(4, 3);
                break;
            case 0x0164:
                usedCycles += CMPR(4, 4);
                break;
            case 0x0165:
                usedCycles += CMPR(4, 5);
                break;
            case 0x0166:
                usedCycles += CMPR(4, 6);
                break;
            case 0x0167:
                usedCycles += CMPR(4, 7);
                break;
            case 0x0168:
                usedCycles += CMPR(5, 0);
                break;
            case 0x0169:
                usedCycles += CMPR(5, 1);
                break;
            case 0x016A:
                usedCycles += CMPR(5, 2);
                break;
            case 0x016B:
                usedCycles += CMPR(5, 3);
                break;
            case 0x016C:
                usedCycles += CMPR(5, 4);
                break;
            case 0x016D:
                usedCycles += CMPR(5, 5);
                break;
            case 0x016E:
                usedCycles += CMPR(5, 6);
                break;
            case 0x016F:
                usedCycles += CMPR(5, 7);
                break;
            case 0x0170:
                usedCycles += CMPR(6, 0);
                break;
            case 0x0171:
                usedCycles += CMPR(6, 1);
                break;
            case 0x0172:
                usedCycles += CMPR(6, 2);
                break;
            case 0x0173:
                usedCycles += CMPR(6, 3);
                break;
            case 0x0174:
                usedCycles += CMPR(6, 4);
                break;
            case 0x0175:
                usedCycles += CMPR(6, 5);
                break;
            case 0x0176:
                usedCycles += CMPR(6, 6);
                break;
            case 0x0177:
                usedCycles += CMPR(6, 7);
                break;
            case 0x0178:
                usedCycles += CMPR(7, 0);
                break;
            case 0x0179:
                usedCycles += CMPR(7, 1);
                break;
            case 0x017A:
                usedCycles += CMPR(7, 2);
                break;
            case 0x017B:
                usedCycles += CMPR(7, 3);
                break;
            case 0x017C:
                usedCycles += CMPR(7, 4);
                break;
            case 0x017D:
                usedCycles += CMPR(7, 5);
                break;
            case 0x017E:
                usedCycles += CMPR(7, 6);
                break;
            case 0x017F:
                usedCycles += CMPR(7, 7);
                break;
            case 0x0180:
                usedCycles += ANDR(0, 0);
                break;
            case 0x0181:
                usedCycles += ANDR(0, 1);
                break;
            case 0x0182:
                usedCycles += ANDR(0, 2);
                break;
            case 0x0183:
                usedCycles += ANDR(0, 3);
                break;
            case 0x0184:
                usedCycles += ANDR(0, 4);
                break;
            case 0x0185:
                usedCycles += ANDR(0, 5);
                break;
            case 0x0186:
                usedCycles += ANDR(0, 6);
                break;
            case 0x0187:
                usedCycles += ANDR(0, 7);
                break;
            case 0x0188:
                usedCycles += ANDR(1, 0);
                break;
            case 0x0189:
                usedCycles += ANDR(1, 1);
                break;
            case 0x018A:
                usedCycles += ANDR(1, 2);
                break;
            case 0x018B:
                usedCycles += ANDR(1, 3);
                break;
            case 0x018C:
                usedCycles += ANDR(1, 4);
                break;
            case 0x018D:
                usedCycles += ANDR(1, 5);
                break;
            case 0x018E:
                usedCycles += ANDR(1, 6);
                break;
            case 0x018F:
                usedCycles += ANDR(1, 7);
                break;
            case 0x0190:
                usedCycles += ANDR(2, 0);
                break;
            case 0x0191:
                usedCycles += ANDR(2, 1);
                break;
            case 0x0192:
                usedCycles += ANDR(2, 2);
                break;
            case 0x0193:
                usedCycles += ANDR(2, 3);
                break;
            case 0x0194:
                usedCycles += ANDR(2, 4);
                break;
            case 0x0195:
                usedCycles += ANDR(2, 5);
                break;
            case 0x0196:
                usedCycles += ANDR(2, 6);
                break;
            case 0x0197:
                usedCycles += ANDR(2, 7);
                break;
            case 0x0198:
                usedCycles += ANDR(3, 0);
                break;
            case 0x0199:
                usedCycles += ANDR(3, 1);
                break;
            case 0x019A:
                usedCycles += ANDR(3, 2);
                break;
            case 0x019B:
                usedCycles += ANDR(3, 3);
                break;
            case 0x019C:
                usedCycles += ANDR(3, 4);
                break;
            case 0x019D:
                usedCycles += ANDR(3, 5);
                break;
            case 0x019E:
                usedCycles += ANDR(3, 6);
                break;
            case 0x019F:
                usedCycles += ANDR(3, 7);
                break;
            case 0x01A0:
                usedCycles += ANDR(4, 0);
                break;
            case 0x01A1:
                usedCycles += ANDR(4, 1);
                break;
            case 0x01A2:
                usedCycles += ANDR(4, 2);
                break;
            case 0x01A3:
                usedCycles += ANDR(4, 3);
                break;
            case 0x01A4:
                usedCycles += ANDR(4, 4);
                break;
            case 0x01A5:
                usedCycles += ANDR(4, 5);
                break;
            case 0x01A6:
                usedCycles += ANDR(4, 6);
                break;
            case 0x01A7:
                usedCycles += ANDR(4, 7);
                break;
            case 0x01A8:
                usedCycles += ANDR(5, 0);
                break;
            case 0x01A9:
                usedCycles += ANDR(5, 1);
                break;
            case 0x01AA:
                usedCycles += ANDR(5, 2);
                break;
            case 0x01AB:
                usedCycles += ANDR(5, 3);
                break;
            case 0x01AC:
                usedCycles += ANDR(5, 4);
                break;
            case 0x01AD:
                usedCycles += ANDR(5, 5);
                break;
            case 0x01AE:
                usedCycles += ANDR(5, 6);
                break;
            case 0x01AF:
                usedCycles += ANDR(5, 7);
                break;
            case 0x01B0:
                usedCycles += ANDR(6, 0);
                break;
            case 0x01B1:
                usedCycles += ANDR(6, 1);
                break;
            case 0x01B2:
                usedCycles += ANDR(6, 2);
                break;
            case 0x01B3:
                usedCycles += ANDR(6, 3);
                break;
            case 0x01B4:
                usedCycles += ANDR(6, 4);
                break;
            case 0x01B5:
                usedCycles += ANDR(6, 5);
                break;
            case 0x01B6:
                usedCycles += ANDR(6, 6);
                break;
            case 0x01B7:
                usedCycles += ANDR(6, 7);
                break;
            case 0x01B8:
                usedCycles += ANDR(7, 0);
                break;
            case 0x01B9:
                usedCycles += ANDR(7, 1);
                break;
            case 0x01BA:
                usedCycles += ANDR(7, 2);
                break;
            case 0x01BB:
                usedCycles += ANDR(7, 3);
                break;
            case 0x01BC:
                usedCycles += ANDR(7, 4);
                break;
            case 0x01BD:
                usedCycles += ANDR(7, 5);
                break;
            case 0x01BE:
                usedCycles += ANDR(7, 6);
                break;
            case 0x01BF:
                usedCycles += ANDR(7, 7);
                break;
            case 0x01C0:
                usedCycles += XORR(0, 0);
                break;
            case 0x01C1:
                usedCycles += XORR(0, 1);
                break;
            case 0x01C2:
                usedCycles += XORR(0, 2);
                break;
            case 0x01C3:
                usedCycles += XORR(0, 3);
                break;
            case 0x01C4:
                usedCycles += XORR(0, 4);
                break;
            case 0x01C5:
                usedCycles += XORR(0, 5);
                break;
            case 0x01C6:
                usedCycles += XORR(0, 6);
                break;
            case 0x01C7:
                usedCycles += XORR(0, 7);
                break;
            case 0x01C8:
                usedCycles += XORR(1, 0);
                break;
            case 0x01C9:
                usedCycles += XORR(1, 1);
                break;
            case 0x01CA:
                usedCycles += XORR(1, 2);
                break;
            case 0x01CB:
                usedCycles += XORR(1, 3);
                break;
            case 0x01CC:
                usedCycles += XORR(1, 4);
                break;
            case 0x01CD:
                usedCycles += XORR(1, 5);
                break;
            case 0x01CE:
                usedCycles += XORR(1, 6);
                break;
            case 0x01CF:
                usedCycles += XORR(1, 7);
                break;
            case 0x01D0:
                usedCycles += XORR(2, 0);
                break;
            case 0x01D1:
                usedCycles += XORR(2, 1);
                break;
            case 0x01D2:
                usedCycles += XORR(2, 2);
                break;
            case 0x01D3:
                usedCycles += XORR(2, 3);
                break;
            case 0x01D4:
                usedCycles += XORR(2, 4);
                break;
            case 0x01D5:
                usedCycles += XORR(2, 5);
                break;
            case 0x01D6:
                usedCycles += XORR(2, 6);
                break;
            case 0x01D7:
                usedCycles += XORR(2, 7);
                break;
            case 0x01D8:
                usedCycles += XORR(3, 0);
                break;
            case 0x01D9:
                usedCycles += XORR(3, 1);
                break;
            case 0x01DA:
                usedCycles += XORR(3, 2);
                break;
            case 0x01DB:
                usedCycles += XORR(3, 3);
                break;
            case 0x01DC:
                usedCycles += XORR(3, 4);
                break;
            case 0x01DD:
                usedCycles += XORR(3, 5);
                break;
            case 0x01DE:
                usedCycles += XORR(3, 6);
                break;
            case 0x01DF:
                usedCycles += XORR(3, 7);
                break;
            case 0x01E0:
                usedCycles += XORR(4, 0);
                break;
            case 0x01E1:
                usedCycles += XORR(4, 1);
                break;
            case 0x01E2:
                usedCycles += XORR(4, 2);
                break;
            case 0x01E3:
                usedCycles += XORR(4, 3);
                break;
            case 0x01E4:
                usedCycles += XORR(4, 4);
                break;
            case 0x01E5:
                usedCycles += XORR(4, 5);
                break;
            case 0x01E6:
                usedCycles += XORR(4, 6);
                break;
            case 0x01E7:
                usedCycles += XORR(4, 7);
                break;
            case 0x01E8:
                usedCycles += XORR(5, 0);
                break;
            case 0x01E9:
                usedCycles += XORR(5, 1);
                break;
            case 0x01EA:
                usedCycles += XORR(5, 2);
                break;
            case 0x01EB:
                usedCycles += XORR(5, 3);
                break;
            case 0x01EC:
                usedCycles += XORR(5, 4);
                break;
            case 0x01ED:
                usedCycles += XORR(5, 5);
                break;
            case 0x01EE:
                usedCycles += XORR(5, 6);
                break;
            case 0x01EF:
                usedCycles += XORR(5, 7);
                break;
            case 0x01F0:
                usedCycles += XORR(6, 0);
                break;
            case 0x01F1:
                usedCycles += XORR(6, 1);
                break;
            case 0x01F2:
                usedCycles += XORR(6, 2);
                break;
            case 0x01F3:
                usedCycles += XORR(6, 3);
                break;
            case 0x01F4:
                usedCycles += XORR(6, 4);
                break;
            case 0x01F5:
                usedCycles += XORR(6, 5);
                break;
            case 0x01F6:
                usedCycles += XORR(6, 6);
                break;
            case 0x01F7:
                usedCycles += XORR(6, 7);
                break;
            case 0x01F8:
                usedCycles += XORR(7, 0);
                break;
            case 0x01F9:
                usedCycles += XORR(7, 1);
                break;
            case 0x01FA:
                usedCycles += XORR(7, 2);
                break;
            case 0x01FB:
                usedCycles += XORR(7, 3);
                break;
            case 0x01FC:
                usedCycles += XORR(7, 4);
                break;
            case 0x01FD:
                usedCycles += XORR(7, 5);
                break;
            case 0x01FE:
                usedCycles += XORR(7, 6);
                break;
            case 0x01FF:
                usedCycles += XORR(7, 7);
                break;
            case 0x0200:
                usedCycles += B(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0201:
                usedCycles += BC(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0202:
                usedCycles += BOV(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0203:
                usedCycles += BPL(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0204:
                usedCycles += BEQ(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0205:
                usedCycles += BLT(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0206:
                usedCycles += BLE(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0207:
                usedCycles += BUSC(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0208:
                usedCycles += NOPP(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0209:
                usedCycles += BNC(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x020A:
                usedCycles += BNOV(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x020B:
                usedCycles += BMI(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x020C:
                usedCycles += BNEQ(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x020D:
                usedCycles += BGE(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x020E:
                usedCycles += BGT(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x020F:
                usedCycles += BESC(PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0210:
                usedCycles += BEXT(0, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0211:
                usedCycles += BEXT(1, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0212:
                usedCycles += BEXT(2, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0213:
                usedCycles += BEXT(3, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0214:
                usedCycles += BEXT(4, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0215:
                usedCycles += BEXT(5, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0216:
                usedCycles += BEXT(6, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0217:
                usedCycles += BEXT(7, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0218:
                usedCycles += BEXT(8, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0219:
                usedCycles += BEXT(9, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x021A:
                usedCycles += BEXT(10,
                    PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x021B:
                usedCycles += BEXT(11,
                    PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x021C:
                usedCycles += BEXT(12,
                    PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x021D:
                usedCycles += BEXT(13,
                    PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x021E:
                usedCycles += BEXT(14,
                    PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x021F:
                usedCycles += BEXT(15,
                    PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0220:
                usedCycles += B(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0221:
                usedCycles += BC(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0222:
                usedCycles += BOV(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0223:
                usedCycles += BPL(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0224:
                usedCycles += BEQ(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0225:
                usedCycles += BLT(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0226:
                usedCycles += BLE(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0227:
                usedCycles += BUSC(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0228:
                usedCycles += NOPP(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0229:
                usedCycles += BNC(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x022A:
                usedCycles += BNOV(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x022B:
                usedCycles += BMI(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x022C:
                usedCycles += BNEQ(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x022D:
                usedCycles += BGE(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x022E:
                usedCycles += BGT(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x022F:
                usedCycles += BESC(-PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0230:
                usedCycles += BEXT(0,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0231:
                usedCycles += BEXT(1,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0232:
                usedCycles += BEXT(2,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0233:
                usedCycles += BEXT(3,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0234:
                usedCycles += BEXT(4,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0235:
                usedCycles += BEXT(5,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0236:
                usedCycles += BEXT(6,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0237:
                usedCycles += BEXT(7,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0238:
                usedCycles += BEXT(8,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0239:
                usedCycles += BEXT(9,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x023A:
                usedCycles += BEXT(10,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x023B:
                usedCycles += BEXT(11,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x023C:
                usedCycles += BEXT(12,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x023D:
                usedCycles += BEXT(13,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x023E:
                usedCycles += BEXT(14,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x023F:
                usedCycles += BEXT(15,
                    -PEEK_FAST((UINT16)(r[7] + 1)) - 1);
                break;
            case 0x0240:
                usedCycles += MVO(0, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0241:
                usedCycles += MVO(1, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0242:
                usedCycles += MVO(2, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0243:
                usedCycles += MVO(3, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0244:
                usedCycles += MVO(4, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0245:
                usedCycles += MVO(5, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0246:
                usedCycles += MVO(6, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0247:
                usedCycles += MVO(7, PEEK_FAST((UINT16)(r[7] + 1)));
                break;
            case 0x0248:
                usedCycles += MVO_ind(1, 0);
                break;
            case 0x0249:
                usedCycles += MVO_ind(1, 1);
                break;
            case 0x024A:
                usedCycles += MVO_ind(1, 2);
                break;
            case 0x024B:
                usedCycles += MVO_ind(1, 3);
                break;
            case 0x024C:
                usedCycles += MVO_ind(1, 4);
                break;
            case 0x024D:
                usedCycles += MVO_ind(1, 5);
                break;
            case 0x024E:
                usedCycles += MVO_ind(1, 6);
                break;
            case 0x024F:
                usedCycles += MVO_ind(1, 7);
                break;
            case 0x0250:
                usedCycles += MVO_ind(2, 0);
                break;
            case 0x0251:
                usedCycles += MVO_ind(2, 1);
                break;
            case 0x0252:
                usedCycles += MVO_ind(2, 2);
                break;
            case 0x0253:
                usedCycles += MVO_ind(2, 3);
                break;
            case 0x0254:
                usedCycles += MVO_ind(2, 4);
                break;
            case 0x0255:
                usedCycles += MVO_ind(2, 5);
                break;
            case 0x0256:
                usedCycles += MVO_ind(2, 6);
                break;
            case 0x0257:
                usedCycles += MVO_ind(2, 7);
                break;
            case 0x0258:
                usedCycles += MVO_ind(3, 0);
                break;
            case 0x0259:
                usedCycles += MVO_ind(3, 1);
                break;
            case 0x025A:
                usedCycles += MVO_ind(3, 2);
                break;
            case 0x025B:
                usedCycles += MVO_ind(3, 3);
                break;
            case 0x025C:
                usedCycles += MVO_ind(3, 4);
                break;
            case 0x025D:
                usedCycles += MVO_ind(3, 5);
                break;
            case 0x025E:
                usedCycles += MVO_ind(3, 6);
                break;
            case 0x025F:
                usedCycles += MVO_ind(3, 7);
                break;
            case 0x0260:
                usedCycles += MVO_ind(4, 0);
                break;
            case 0x0261:
                usedCycles += MVO_ind(4, 1);
                break;
            case 0x0262:
                usedCycles += MVO_ind(4, 2);
                break;
            case 0x0263:
                usedCycles += MVO_ind(4, 3);
                break;
            case 0x0264:
                usedCycles += MVO_ind(4, 4);
                break;
            case 0x0265:
                usedCycles += MVO_ind(4, 5);
                break;
            case 0x0266:
                usedCycles += MVO_ind(4, 6);
                break;
            case 0x0267:
                usedCycles += MVO_ind(4, 7);
                break;
            case 0x0268:
                usedCycles += MVO_ind(5, 0);
                break;
            case 0x0269:
                usedCycles += MVO_ind(5, 1);
                break;
            case 0x026A:
                usedCycles += MVO_ind(5, 2);
                break;
            case 0x026B:
                usedCycles += MVO_ind(5, 3);
                break;
            case 0x026C:
                usedCycles += MVO_ind(5, 4);
                break;
            case 0x026D:
                usedCycles += MVO_ind(5, 5);
                break;
            case 0x026E:
                usedCycles += MVO_ind(5, 6);
                break;
            case 0x026F:
                usedCycles += MVO_ind(5, 7);
                break;
            case 0x0270:
                usedCycles += MVO_ind(6, 0);
                break;
            case 0x0271:
                usedCycles += MVO_ind(6, 1);
                break;
            case 0x0272:
                usedCycles += MVO_ind(6, 2);
                break;
            case 0x0273:
                usedCycles += MVO_ind(6, 3);
                break;
            case 0x0274:
                usedCycles += MVO_ind(6, 4);
                break;
            case 0x0275:
                usedCycles += MVO_ind(6, 5);
                break;
            case 0x0276:
                usedCycles += MVO_ind(6, 6);
                break;
            case 0x0277:
                usedCycles += MVO_ind(6, 7);
                break;
            case 0x0278:
                usedCycles += MVO_ind(7, 0);
                break;
            case 0x0279:
                usedCycles += MVO_ind(7, 1);
                break;
            case 0x027A:
                usedCycles += MVO_ind(7, 2);
                break;
            case 0x027B:
                usedCycles += MVO_ind(7, 3);
                break;
            case 0x027C:
                usedCycles += MVO_ind(7, 4);
                break;
            case 0x027D:
                usedCycles += MVO_ind(7, 5);
                break;
            case 0x027E:
                usedCycles += MVO_ind(7, 6);
                break;
            case 0x027F:
                usedCycles += MVO_ind(7, 7);
                break;
            case 0x0280:
                usedCycles += MVI(PEEK_FAST((UINT16)(r[7] + 1)), 0);
                break;
            case 0x0281:
                usedCycles += MVI(PEEK_FAST((UINT16)(r[7] + 1)), 1);
                break;
            case 0x0282:
                usedCycles += MVI(PEEK_FAST((UINT16)(r[7] + 1)), 2);
                break;
            case 0x0283:
                usedCycles += MVI(PEEK_FAST((UINT16)(r[7] + 1)), 3);
                break;
            case 0x0284:
                usedCycles += MVI(PEEK_FAST((UINT16)(r[7] + 1)), 4);
                break;
            case 0x0285:
                usedCycles += MVI(PEEK_FAST((UINT16)(r[7] + 1)), 5);
                break;
            case 0x0286:
                usedCycles += MVI(PEEK_FAST((UINT16)(r[7] + 1)), 6);
                break;
            case 0x0287:
                usedCycles += MVI(PEEK_FAST((UINT16)(r[7] + 1)), 7);
                break;
            case 0x0288:
                usedCycles += MVI_ind(1, 0);
                break;
            case 0x0289:
                usedCycles += MVI_ind(1, 1);
                break;
            case 0x028A:
                usedCycles += MVI_ind(1, 2);
                break;
            case 0x028B:
                usedCycles += MVI_ind(1, 3);
                break;
            case 0x028C:
                usedCycles += MVI_ind(1, 4);
                break;
            case 0x028D:
                usedCycles += MVI_ind(1, 5);
                break;
            case 0x028E:
                usedCycles += MVI_ind(1, 6);
                break;
            case 0x028F:
                usedCycles += MVI_ind(1, 7);
                break;
            case 0x0290:
                usedCycles += MVI_ind(2, 0);
                break;
            case 0x0291:
                usedCycles += MVI_ind(2, 1);
                break;
            case 0x0292:
                usedCycles += MVI_ind(2, 2);
                break;
            case 0x0293:
                usedCycles += MVI_ind(2, 3);
                break;
            case 0x0294:
                usedCycles += MVI_ind(2, 4);
                break;
            case 0x0295:
                usedCycles += MVI_ind(2, 5);
                break;
            case 0x0296:
                usedCycles += MVI_ind(2, 6);
                break;
            case 0x0297:
                usedCycles += MVI_ind(2, 7);
                break;
            case 0x0298:
                usedCycles += MVI_ind(3, 0);
                break;
            case 0x0299:
                usedCycles += MVI_ind(3, 1);
                break;
            case 0x029A:
                usedCycles += MVI_ind(3, 2);
                break;
            case 0x029B:
                usedCycles += MVI_ind(3, 3);
                break;
            case 0x029C:
                usedCycles += MVI_ind(3, 4);
                break;
            case 0x029D:
                usedCycles += MVI_ind(3, 5);
                break;
            case 0x029E:
                usedCycles += MVI_ind(3, 6);
                break;
            case 0x029F:
                usedCycles += MVI_ind(3, 7);
                break;
            case 0x02A0:
                usedCycles += MVI_ind(4, 0);
                break;
            case 0x02A1:
                usedCycles += MVI_ind(4, 1);
                break;
            case 0x02A2:
                usedCycles += MVI_ind(4, 2);
                break;
            case 0x02A3:
                usedCycles += MVI_ind(4, 3);
                break;
            case 0x02A4:
                usedCycles += MVI_ind(4, 4);
                break;
            case 0x02A5:
                usedCycles += MVI_ind(4, 5);
                break;
            case 0x02A6:
                usedCycles += MVI_ind(4, 6);
                break;
            case 0x02A7:
                usedCycles += MVI_ind(4, 7);
                break;
            case 0x02A8:
                usedCycles += MVI_ind(5, 0);
                break;
            case 0x02A9:
                usedCycles += MVI_ind(5, 1);
                break;
            case 0x02AA:
                usedCycles += MVI_ind(5, 2);
                break;
            case 0x02AB:
                usedCycles += MVI_ind(5, 3);
                break;
            case 0x02AC:
                usedCycles += MVI_ind(5, 4);
                break;
            case 0x02AD:
                usedCycles += MVI_ind(5, 5);
                break;
            case 0x02AE:
                usedCycles += MVI_ind(5, 6);
                break;
            case 0x02AF:
                usedCycles += MVI_ind(5, 7);
                break;
            case 0x02B0:
                usedCycles += MVI_ind(6, 0);
                break;
            case 0x02B1:
                usedCycles += MVI_ind(6, 1);
                break;
            case 0x02B2:
                usedCycles += MVI_ind(6, 2);
                break;
            case 0x02B3:
                usedCycles += MVI_ind(6, 3);
                break;
            case 0x02B4:
                usedCycles += MVI_ind(6, 4);
                break;
            case 0x02B5:
                usedCycles += MVI_ind(6, 5);
                break;
            case 0x02B6:
                usedCycles += MVI_ind(6, 6);
                break;
            case 0x02B7:
                usedCycles += MVI_ind(6, 7);
                break;
            case 0x02B8:
                usedCycles += MVI_ind(7, 0);
                break;
            case 0x02B9:
                usedCycles += MVI_ind(7, 1);
                break;
            case 0x02BA:
                usedCycles += MVI_ind(7, 2);
                break;
            case 0x02BB:
                usedCycles += MVI_ind(7, 3);
                break;
            case 0x02BC:
                usedCycles += MVI_ind(7, 4);
                break;
            case 0x02BD:
                usedCycles += MVI_ind(7, 5);
                break;
            case 0x02BE:
                usedCycles += MVI_ind(7, 6);
                break;
            case 0x02BF:
                usedCycles += MVI_ind(7, 7);
                break;
            case 0x02C0:
                usedCycles += ADD(PEEK_FAST((UINT16)(r[7] + 1)), 0);
                break;
            case 0x02C1:
                usedCycles += ADD(PEEK_FAST((UINT16)(r[7] + 1)), 1);
                break;
            case 0x02C2:
                usedCycles += ADD(PEEK_FAST((UINT16)(r[7] + 1)), 2);
                break;
            case 0x02C3:
                usedCycles += ADD(PEEK_FAST((UINT16)(r[7] + 1)), 3);
                break;
            case 0x02C4:
                usedCycles += ADD(PEEK_FAST((UINT16)(r[7] + 1)), 4);
                break;
            case 0x02C5:
                usedCycles += ADD(PEEK_FAST((UINT16)(r[7] + 1)), 5);
                break;
            case 0x02C6:
                usedCycles += ADD(PEEK_FAST((UINT16)(r[7] + 1)), 6);
                break;
            case 0x02C7:
                usedCycles += ADD(PEEK_FAST((UINT16)(r[7] + 1)), 7);
                break;
            case 0x02C8:
                usedCycles += ADD_ind(1, 0);
                break;
            case 0x02C9:
                usedCycles += ADD_ind(1, 1);
                break;
            case 0x02CA:
                usedCycles += ADD_ind(1, 2);
                break;
            case 0x02CB:
                usedCycles += ADD_ind(1, 3);
                break;
            case 0x02CC:
                usedCycles += ADD_ind(1, 4);
                break;
            case 0x02CD:
                usedCycles += ADD_ind(1, 5);
                break;
            case 0x02CE:
                usedCycles += ADD_ind(1, 6);
                break;
            case 0x02CF:
                usedCycles += ADD_ind(1, 7);
                break;
            case 0x02D0:
                usedCycles += ADD_ind(2, 0);
                break;
            case 0x02D1:
                usedCycles += ADD_ind(2, 1);
                break;
            case 0x02D2:
                usedCycles += ADD_ind(2, 2);
                break;
            case 0x02D3:
                usedCycles += ADD_ind(2, 3);
                break;
            case 0x02D4:
                usedCycles += ADD_ind(2, 4);
                break;
            case 0x02D5:
                usedCycles += ADD_ind(2, 5);
                break;
            case 0x02D6:
                usedCycles += ADD_ind(2, 6);
                break;
            case 0x02D7:
                usedCycles += ADD_ind(2, 7);
                break;
            case 0x02D8:
                usedCycles += ADD_ind(3, 0);
                break;
            case 0x02D9:
                usedCycles += ADD_ind(3, 1);
                break;
            case 0x02DA:
                usedCycles += ADD_ind(3, 2);
                break;
            case 0x02DB:
                usedCycles += ADD_ind(3, 3);
                break;
            case 0x02DC:
                usedCycles += ADD_ind(3, 4);
                break;
            case 0x02DD:
                usedCycles += ADD_ind(3, 5);
                break;
            case 0x02DE:
                usedCycles += ADD_ind(3, 6);
                break;
            case 0x02DF:
                usedCycles += ADD_ind(3, 7);
                break;
            case 0x02E0:
                usedCycles += ADD_ind(4, 0);
                break;
            case 0x02E1:
                usedCycles += ADD_ind(4, 1);
                break;
            case 0x02E2:
                usedCycles += ADD_ind(4, 2);
                break;
            case 0x02E3:
                usedCycles += ADD_ind(4, 3);
                break;
            case 0x02E4:
                usedCycles += ADD_ind(4, 4);
                break;
            case 0x02E5:
                usedCycles += ADD_ind(4, 5);
                break;
            case 0x02E6:
                usedCycles += ADD_ind(4, 6);
                break;
            case 0x02E7:
                usedCycles += ADD_ind(4, 7);
                break;
            case 0x02E8:
                usedCycles += ADD_ind(5, 0);
                break;
            case 0x02E9:
                usedCycles += ADD_ind(5, 1);
                break;
            case 0x02EA:
                usedCycles += ADD_ind(5, 2);
                break;
            case 0x02EB:
                usedCycles += ADD_ind(5, 3);
                break;
            case 0x02EC:
                usedCycles += ADD_ind(5, 4);
                break;
            case 0x02ED:
                usedCycles += ADD_ind(5, 5);
                break;
            case 0x02EE:
                usedCycles += ADD_ind(5, 6);
                break;
            case 0x02EF:
                usedCycles += ADD_ind(5, 7);
                break;
            case 0x02F0:
                usedCycles += ADD_ind(6, 0);
                break;
            case 0x02F1:
                usedCycles += ADD_ind(6, 1);
                break;
            case 0x02F2:
                usedCycles += ADD_ind(6, 2);
                break;
            case 0x02F3:
                usedCycles += ADD_ind(6, 3);
                break;
            case 0x02F4:
                usedCycles += ADD_ind(6, 4);
                break;
            case 0x02F5:
                usedCycles += ADD_ind(6, 5);
                break;
            case 0x02F6:
                usedCycles += ADD_ind(6, 6);
                break;
            case 0x02F7:
                usedCycles += ADD_ind(6, 7);
                break;
            case 0x02F8:
                usedCycles += ADD_ind(7, 0);
                break;
            case 0x02F9:
                usedCycles += ADD_ind(7, 1);
                break;
            case 0x02FA:
                usedCycles += ADD_ind(7, 2);
                break;
            case 0x02FB:
                usedCycles += ADD_ind(7, 3);
                break;
            case 0x02FC:
                usedCycles += ADD_ind(7, 4);
                break;
            case 0x02FD:
                usedCycles += ADD_ind(7, 5);
                break;
            case 0x02FE:
                usedCycles += ADD_ind(7, 6);
                break;
            case 0x02FF:
                usedCycles += ADD_ind(7, 7);
                break;
            case 0x0300:
                usedCycles += SUB(PEEK_FAST((UINT16)(r[7] + 1)), 0);
                break;
            case 0x0301:
                usedCycles += SUB(PEEK_FAST((UINT16)(r[7] + 1)), 1);
                break;
            case 0x0302:
                usedCycles += SUB(PEEK_FAST((UINT16)(r[7] + 1)), 2);
                break;
            case 0x0303:
                usedCycles += SUB(PEEK_FAST((UINT16)(r[7] + 1)), 3);
                break;
            case 0x0304:
                usedCycles += SUB(PEEK_FAST((UINT16)(r[7] + 1)), 4);
                break;
            case 0x0305:
                usedCycles += SUB(PEEK_FAST((UINT16)(r[7] + 1)), 5);
                break;
            case 0x0306:
                usedCycles += SUB(PEEK_FAST((UINT16)(r[7] + 1)), 6);
                break;
            case 0x0307:
                usedCycles += SUB(PEEK_FAST((UINT16)(r[7] + 1)), 7);
                break;
            case 0x0308:
                usedCycles += SUB_ind(1, 0);
                break;
            case 0x0309:
                usedCycles += SUB_ind(1, 1);
                break;
            case 0x030A:
                usedCycles += SUB_ind(1, 2);
                break;
            case 0x030B:
                usedCycles += SUB_ind(1, 3);
                break;
            case 0x030C:
                usedCycles += SUB_ind(1, 4);
                break;
            case 0x030D:
                usedCycles += SUB_ind(1, 5);
                break;
            case 0x030E:
                usedCycles += SUB_ind(1, 6);
                break;
            case 0x030F:
                usedCycles += SUB_ind(1, 7);
                break;
            case 0x0310:
                usedCycles += SUB_ind(2, 0);
                break;
            case 0x0311:
                usedCycles += SUB_ind(2, 1);
                break;
            case 0x0312:
                usedCycles += SUB_ind(2, 2);
                break;
            case 0x0313:
                usedCycles += SUB_ind(2, 3);
                break;
            case 0x0314:
                usedCycles += SUB_ind(2, 4);
                break;
            case 0x0315:
                usedCycles += SUB_ind(2, 5);
                break;
            case 0x0316:
                usedCycles += SUB_ind(2, 6);
                break;
            case 0x0317:
                usedCycles += SUB_ind(2, 7);
                break;
            case 0x0318:
                usedCycles += SUB_ind(3, 0);
                break;
            case 0x0319:
                usedCycles += SUB_ind(3, 1);
                break;
            case 0x031A:
                usedCycles += SUB_ind(3, 2);
                break;
            case 0x031B:
                usedCycles += SUB_ind(3, 3);
                break;
            case 0x031C:
                usedCycles += SUB_ind(3, 4);
                break;
            case 0x031D:
                usedCycles += SUB_ind(3, 5);
                break;
            case 0x031E:
                usedCycles += SUB_ind(3, 6);
                break;
            case 0x031F:
                usedCycles += SUB_ind(3, 7);
                break;
            case 0x0320:
                usedCycles += SUB_ind(4, 0);
                break;
            case 0x0321:
                usedCycles += SUB_ind(4, 1);
                break;
            case 0x0322:
                usedCycles += SUB_ind(4, 2);
                break;
            case 0x0323:
                usedCycles += SUB_ind(4, 3);
                break;
            case 0x0324:
                usedCycles += SUB_ind(4, 4);
                break;
            case 0x0325:
                usedCycles += SUB_ind(4, 5);
                break;
            case 0x0326:
                usedCycles += SUB_ind(4, 6);
                break;
            case 0x0327:
                usedCycles += SUB_ind(4, 7);
                break;
            case 0x0328:
                usedCycles += SUB_ind(5, 0);
                break;
            case 0x0329:
                usedCycles += SUB_ind(5, 1);
                break;
            case 0x032A:
                usedCycles += SUB_ind(5, 2);
                break;
            case 0x032B:
                usedCycles += SUB_ind(5, 3);
                break;
            case 0x032C:
                usedCycles += SUB_ind(5, 4);
                break;
            case 0x032D:
                usedCycles += SUB_ind(5, 5);
                break;
            case 0x032E:
                usedCycles += SUB_ind(5, 6);
                break;
            case 0x032F:
                usedCycles += SUB_ind(5, 7);
                break;
            case 0x0330:
                usedCycles += SUB_ind(6, 0);
                break;
            case 0x0331:
                usedCycles += SUB_ind(6, 1);
                break;
            case 0x0332:
                usedCycles += SUB_ind(6, 2);
                break;
            case 0x0333:
                usedCycles += SUB_ind(6, 3);
                break;
            case 0x0334:
                usedCycles += SUB_ind(6, 4);
                break;
            case 0x0335:
                usedCycles += SUB_ind(6, 5);
                break;
            case 0x0336:
                usedCycles += SUB_ind(6, 6);
                break;
            case 0x0337:
                usedCycles += SUB_ind(6, 7);
                break;
            case 0x0338:
                usedCycles += SUB_ind(7, 0);
                break;
            case 0x0339:
                usedCycles += SUB_ind(7, 1);
                break;
            case 0x033A:
                usedCycles += SUB_ind(7, 2);
                break;
            case 0x033B:
                usedCycles += SUB_ind(7, 3);
                break;
            case 0x033C:
                usedCycles += SUB_ind(7, 4);
                break;
            case 0x033D:
                usedCycles += SUB_ind(7, 5);
                break;
            case 0x033E:
                usedCycles += SUB_ind(7, 6);
                break;
            case 0x033F:
                usedCycles += SUB_ind(7, 7);
                break;
            case 0x0340:
                usedCycles += CMP(PEEK_FAST((UINT16)(r[7] + 1)), 0);
                break;
            case 0x0341:
                usedCycles += CMP(PEEK_FAST((UINT16)(r[7] + 1)), 1);
                break;
            case 0x0342:
                usedCycles += CMP(PEEK_FAST((UINT16)(r[7] + 1)), 2);
                break;
            case 0x0343:
                usedCycles += CMP(PEEK_FAST((UINT16)(r[7] + 1)), 3);
                break;
            case 0x0344:
                usedCycles += CMP(PEEK_FAST((UINT16)(r[7] + 1)), 4);
                break;
            case 0x0345:
                usedCycles += CMP(PEEK_FAST((UINT16)(r[7] + 1)), 5);
                break;
            case 0x0346:
                usedCycles += CMP(PEEK_FAST((UINT16)(r[7] + 1)), 6);
                break;
            case 0x0347:
                usedCycles += CMP(PEEK_FAST((UINT16)(r[7] + 1)), 7);
                break;
            case 0x0348:
                usedCycles += CMP_ind(1, 0);
                break;
            case 0x0349:
                usedCycles += CMP_ind(1, 1);
                break;
            case 0x034A:
                usedCycles += CMP_ind(1, 2);
                break;
            case 0x034B:
                usedCycles += CMP_ind(1, 3);
                break;
            case 0x034C:
                usedCycles += CMP_ind(1, 4);
                break;
            case 0x034D:
                usedCycles += CMP_ind(1, 5);
                break;
            case 0x034E:
                usedCycles += CMP_ind(1, 6);
                break;
            case 0x034F:
                usedCycles += CMP_ind(1, 7);
                break;
            case 0x0350:
                usedCycles += CMP_ind(2, 0);
                break;
            case 0x0351:
                usedCycles += CMP_ind(2, 1);
                break;
            case 0x0352:
                usedCycles += CMP_ind(2, 2);
                break;
            case 0x0353:
                usedCycles += CMP_ind(2, 3);
                break;
            case 0x0354:
                usedCycles += CMP_ind(2, 4);
                break;
            case 0x0355:
                usedCycles += CMP_ind(2, 5);
                break;
            case 0x0356:
                usedCycles += CMP_ind(2, 6);
                break;
            case 0x0357:
                usedCycles += CMP_ind(2, 7);
                break;
            case 0x0358:
                usedCycles += CMP_ind(3, 0);
                break;
            case 0x0359:
                usedCycles += CMP_ind(3, 1);
                break;
            case 0x035A:
                usedCycles += CMP_ind(3, 2);
                break;
            case 0x035B:
                usedCycles += CMP_ind(3, 3);
                break;
            case 0x035C:
                usedCycles += CMP_ind(3, 4);
                break;
            case 0x035D:
                usedCycles += CMP_ind(3, 5);
                break;
            case 0x035E:
                usedCycles += CMP_ind(3, 6);
                break;
            case 0x035F:
                usedCycles += CMP_ind(3, 7);
                break;
            case 0x0360:
                usedCycles += CMP_ind(4, 0);
                break;
            case 0x0361:
                usedCycles += CMP_ind(4, 1);
                break;
            case 0x0362:
                usedCycles += CMP_ind(4, 2);
                break;
            case 0x0363:
                usedCycles += CMP_ind(4, 3);
                break;
            case 0x0364:
                usedCycles += CMP_ind(4, 4);
                break;
            case 0x0365:
                usedCycles += CMP_ind(4, 5);
                break;
            case 0x0366:
                usedCycles += CMP_ind(4, 6);
                break;
            case 0x0367:
                usedCycles += CMP_ind(4, 7);
                break;
            case 0x0368:
                usedCycles += CMP_ind(5, 0);
                break;
            case 0x0369:
                usedCycles += CMP_ind(5, 1);
                break;
            case 0x036A:
                usedCycles += CMP_ind(5, 2);
                break;
            case 0x036B:
                usedCycles += CMP_ind(5, 3);
                break;
            case 0x036C:
                usedCycles += CMP_ind(5, 4);
                break;
            case 0x036D:
                usedCycles += CMP_ind(5, 5);
                break;
            case 0x036E:
                usedCycles += CMP_ind(5, 6);
                break;
            case 0x036F:
                usedCycles += CMP_ind(5, 7);
                break;
            case 0x0370:
                usedCycles += CMP_ind(6, 0);
                break;
            case 0x0371:
                usedCycles += CMP_ind(6, 1);
                break;
            case 0x0372:
                usedCycles += CMP_ind(6, 2);
                break;
            case 0x0373:
                usedCycles += CMP_ind(6, 3);
                break;
            case 0x0374:
                usedCycles += CMP_ind(6, 4);
                break;
            case 0x0375:
                usedCycles += CMP_ind(6, 5);
                break;
            case 0x0376:
                usedCycles += CMP_ind(6, 6);
                break;
            case 0x0377:
                usedCycles += CMP_ind(6, 7);
                break;
            case 0x0378:
                usedCycles += CMP_ind(7, 0);
                break;
            case 0x0379:
                usedCycles += CMP_ind(7, 1);
                break;
            case 0x037A:
                usedCycles += CMP_ind(7, 2);
                break;
            case 0x037B:
                usedCycles += CMP_ind(7, 3);
                break;
            case 0x037C:
                usedCycles += CMP_ind(7, 4);
                break;
            case 0x037D:
                usedCycles += CMP_ind(7, 5);
                break;
            case 0x037E:
                usedCycles += CMP_ind(7, 6);
                break;
            case 0x037F:
                usedCycles += CMP_ind(7, 7);
                break;
            case 0x0380:
                usedCycles += AND(PEEK_FAST((UINT16)(r[7] + 1)), 0);
                break;
            case 0x0381:
                usedCycles += AND(PEEK_FAST((UINT16)(r[7] + 1)), 1);
                break;
            case 0x0382:
                usedCycles += AND(PEEK_FAST((UINT16)(r[7] + 1)), 2);
                break;
            case 0x0383:
                usedCycles += AND(PEEK_FAST((UINT16)(r[7] + 1)), 3);
                break;
            case 0x0384:
                usedCycles += AND(PEEK_FAST((UINT16)(r[7] + 1)), 4);
                break;
            case 0x0385:
                usedCycles += AND(PEEK_FAST((UINT16)(r[7] + 1)), 5);
                break;
            case 0x0386:
                usedCycles += AND(PEEK_FAST((UINT16)(r[7] + 1)), 6);
                break;
            case 0x0387:
                usedCycles += AND(PEEK_FAST((UINT16)(r[7] + 1)), 7);
                break;
            case 0x0388:
                usedCycles += AND_ind(1, 0);
                break;
            case 0x0389:
                usedCycles += AND_ind(1, 1);
                break;
            case 0x038A:
                usedCycles += AND_ind(1, 2);
                break;
            case 0x038B:
                usedCycles += AND_ind(1, 3);
                break;
            case 0x038C:
                usedCycles += AND_ind(1, 4);
                break;
            case 0x038D:
                usedCycles += AND_ind(1, 5);
                break;
            case 0x038E:
                usedCycles += AND_ind(1, 6);
                break;
            case 0x038F:
                usedCycles += AND_ind(1, 7);
                break;
            case 0x0390:
                usedCycles += AND_ind(2, 0);
                break;
            case 0x0391:
                usedCycles += AND_ind(2, 1);
                break;
            case 0x0392:
                usedCycles += AND_ind(2, 2);
                break;
            case 0x0393:
                usedCycles += AND_ind(2, 3);
                break;
            case 0x0394:
                usedCycles += AND_ind(2, 4);
                break;
            case 0x0395:
                usedCycles += AND_ind(2, 5);
                break;
            case 0x0396:
                usedCycles += AND_ind(2, 6);
                break;
            case 0x0397:
                usedCycles += AND_ind(2, 7);
                break;
            case 0x0398:
                usedCycles += AND_ind(3, 0);
                break;
            case 0x0399:
                usedCycles += AND_ind(3, 1);
                break;
            case 0x039A:
                usedCycles += AND_ind(3, 2);
                break;
            case 0x039B:
                usedCycles += AND_ind(3, 3);
                break;
            case 0x039C:
                usedCycles += AND_ind(3, 4);
                break;
            case 0x039D:
                usedCycles += AND_ind(3, 5);
                break;
            case 0x039E:
                usedCycles += AND_ind(3, 6);
                break;
            case 0x039F:
                usedCycles += AND_ind(3, 7);
                break;
            case 0x03A0:
                usedCycles += AND_ind(4, 0);
                break;
            case 0x03A1:
                usedCycles += AND_ind(4, 1);
                break;
            case 0x03A2:
                usedCycles += AND_ind(4, 2);
                break;
            case 0x03A3:
                usedCycles += AND_ind(4, 3);
                break;
            case 0x03A4:
                usedCycles += AND_ind(4, 4);
                break;
            case 0x03A5:
                usedCycles += AND_ind(4, 5);
                break;
            case 0x03A6:
                usedCycles += AND_ind(4, 6);
                break;
            case 0x03A7:
                usedCycles += AND_ind(4, 7);
                break;
            case 0x03A8:
                usedCycles += AND_ind(5, 0);
                break;
            case 0x03A9:
                usedCycles += AND_ind(5, 1);
                break;
            case 0x03AA:
                usedCycles += AND_ind(5, 2);
                break;
            case 0x03AB:
                usedCycles += AND_ind(5, 3);
                break;
            case 0x03AC:
                usedCycles += AND_ind(5, 4);
                break;
            case 0x03AD:
                usedCycles += AND_ind(5, 5);
                break;
            case 0x03AE:
                usedCycles += AND_ind(5, 6);
                break;
            case 0x03AF:
                usedCycles += AND_ind(5, 7);
                break;
            case 0x03B0:
                usedCycles += AND_ind(6, 0);
                break;
            case 0x03B1:
                usedCycles += AND_ind(6, 1);
                break;
            case 0x03B2:
                usedCycles += AND_ind(6, 2);
                break;
            case 0x03B3:
                usedCycles += AND_ind(6, 3);
                break;
            case 0x03B4:
                usedCycles += AND_ind(6, 4);
                break;
            case 0x03B5:
                usedCycles += AND_ind(6, 5);
                break;
            case 0x03B6:
                usedCycles += AND_ind(6, 6);
                break;
            case 0x03B7:
                usedCycles += AND_ind(6, 7);
                break;
            case 0x03B8:
                usedCycles += AND_ind(7, 0);
                break;
            case 0x03B9:
                usedCycles += AND_ind(7, 1);
                break;
            case 0x03BA:
                usedCycles += AND_ind(7, 2);
                break;
            case 0x03BB:
                usedCycles += AND_ind(7, 3);
                break;
            case 0x03BC:
                usedCycles += AND_ind(7, 4);
                break;
            case 0x03BD:
                usedCycles += AND_ind(7, 5);
                break;
            case 0x03BE:
                usedCycles += AND_ind(7, 6);
                break;
            case 0x03BF:
                usedCycles += AND_ind(7, 7);
                break;
            case 0x03C0:
                usedCycles += XOR(PEEK_FAST((UINT16)(r[7] + 1)), 0);
                break;
            case 0x03C1:
                usedCycles += XOR(PEEK_FAST((UINT16)(r[7] + 1)), 1);
                break;
            case 0x03C2:
                usedCycles += XOR(PEEK_FAST((UINT16)(r[7] + 1)), 2);
                break;
            case 0x03C3:
                usedCycles += XOR(PEEK_FAST((UINT16)(r[7] + 1)), 3);
                break;
            case 0x03C4:
                usedCycles += XOR(PEEK_FAST((UINT16)(r[7] + 1)), 4);
                break;
            case 0x03C5:
                usedCycles += XOR(PEEK_FAST((UINT16)(r[7] + 1)), 5);
                break;
            case 0x03C6:
                usedCycles += XOR(PEEK_FAST((UINT16)(r[7] + 1)), 6);
                break;
            case 0x03C7:
                usedCycles += XOR(PEEK_FAST((UINT16)(r[7] + 1)), 7);
                break;
            case 0x03C8:
                usedCycles += XOR_ind(1, 0);
                break;
            case 0x03C9:
                usedCycles += XOR_ind(1, 1);
                break;
            case 0x03CA:
                usedCycles += XOR_ind(1, 2);
                break;
            case 0x03CB:
                usedCycles += XOR_ind(1, 3);
                break;
            case 0x03CC:
                usedCycles += XOR_ind(1, 4);
                break;
            case 0x03CD:
                usedCycles += XOR_ind(1, 5);
                break;
            case 0x03CE:
                usedCycles += XOR_ind(1, 6);
                break;
            case 0x03CF:
                usedCycles += XOR_ind(1, 7);
                break;
            case 0x03D0:
                usedCycles += XOR_ind(2, 0);
                break;
            case 0x03D1:
                usedCycles += XOR_ind(2, 1);
                break;
            case 0x03D2:
                usedCycles += XOR_ind(2, 2);
                break;
            case 0x03D3:
                usedCycles += XOR_ind(2, 3);
                break;
            case 0x03D4:
                usedCycles += XOR_ind(2, 4);
                break;
            case 0x03D5:
                usedCycles += XOR_ind(2, 5);
                break;
            case 0x03D6:
                usedCycles += XOR_ind(2, 6);
                break;
            case 0x03D7:
                usedCycles += XOR_ind(2, 7);
                break;
            case 0x03D8:
                usedCycles += XOR_ind(3, 0);
                break;
            case 0x03D9:
                usedCycles += XOR_ind(3, 1);
                break;
            case 0x03DA:
                usedCycles += XOR_ind(3, 2);
                break;
            case 0x03DB:
                usedCycles += XOR_ind(3, 3);
                break;
            case 0x03DC:
                usedCycles += XOR_ind(3, 4);
                break;
            case 0x03DD:
                usedCycles += XOR_ind(3, 5);
                break;
            case 0x03DE:
                usedCycles += XOR_ind(3, 6);
                break;
            case 0x03DF:
                usedCycles += XOR_ind(3, 7);
                break;
            case 0x03E0:
                usedCycles += XOR_ind(4, 0);
                break;
            case 0x03E1:
                usedCycles += XOR_ind(4, 1);
                break;
            case 0x03E2:
                usedCycles += XOR_ind(4, 2);
                break;
            case 0x03E3:
                usedCycles += XOR_ind(4, 3);
                break;
            case 0x03E4:
                usedCycles += XOR_ind(4, 4);
                break;
            case 0x03E5:
                usedCycles += XOR_ind(4, 5);
                break;
            case 0x03E6:
                usedCycles += XOR_ind(4, 6);
                break;
            case 0x03E7:
                usedCycles += XOR_ind(4, 7);
                break;
            case 0x03E8:
                usedCycles += XOR_ind(5, 0);
                break;
            case 0x03E9:
                usedCycles += XOR_ind(5, 1);
                break;
            case 0x03EA:
                usedCycles += XOR_ind(5, 2);
                break;
            case 0x03EB:
                usedCycles += XOR_ind(5, 3);
                break;
            case 0x03EC:
                usedCycles += XOR_ind(5, 4);
                break;
            case 0x03ED:
                usedCycles += XOR_ind(5, 5);
                break;
            case 0x03EE:
                usedCycles += XOR_ind(5, 6);
                break;
            case 0x03EF:
                usedCycles += XOR_ind(5, 7);
                break;
            case 0x03F0:
                usedCycles += XOR_ind(6, 0);
                break;
            case 0x03F1:
                usedCycles += XOR_ind(6, 1);
                break;
            case 0x03F2:
                usedCycles += XOR_ind(6, 2);
                break;
            case 0x03F3:
                usedCycles += XOR_ind(6, 3);
                break;
            case 0x03F4:
                usedCycles += XOR_ind(6, 4);
                break;
            case 0x03F5:
                usedCycles += XOR_ind(6, 5);
                break;
            case 0x03F6:
                usedCycles += XOR_ind(6, 6);
                break;
            case 0x03F7:
                usedCycles += XOR_ind(6, 7);
                break;
            case 0x03F8:
                usedCycles += XOR_ind(7, 0);
                break;
            case 0x03F9:
                usedCycles += XOR_ind(7, 1);
                break;
            case 0x03FA:
                usedCycles += XOR_ind(7, 2);
                break;
            case 0x03FB:
                usedCycles += XOR_ind(7, 3);
                break;
            case 0x03FC:
                usedCycles += XOR_ind(7, 4);
                break;
            case 0x03FD:
                usedCycles += XOR_ind(7, 5);
                break;
            case 0x03FE:
                usedCycles += XOR_ind(7, 6);
                break;
            case 0x03FF:
            default :
                usedCycles += XOR_ind(7, 7);
        }
    } while ((usedCycles) < min_shifted);

    return (usedCycles<<2);
}


ITCM_CODE UINT16 CP1610::getIndirect(UINT16 registerNum)
{
    UINT16 value;
    
    if (registerNum & 0x04) // Register R4, R5, R6 or R7
    {
        if (registerNum == 6) 
        {
            r[6]--;
            value = memoryBus->peek(r[6]);
            // Undocumented: indirect through R6: Results in two stack reads from top of stack, and ultimately decrements stack pointer once (above)
            if (D)
            {
                value = (value & 0xFF) | ((memoryBus->peek(r[6]) & 0xFF) << 8);
            }
        }
        else 
        {
            value = memoryBus->peek(r[registerNum]++);
            if (D) 
            {
                value = (value & 0xFF) | (memoryBus->peek(r[registerNum]++) & 0xFF) << 8;
            }
        }
        
        return value;        
    }
    else // Register R0, R1, R2 or R3
    {
        value = memoryBus->peek(r[registerNum]);
        if (D) 
        {
            value = (value & 0xFF) | (memoryBus->peek(r[registerNum]) & 0xFF) << 8;
        }
        return value;
    }
}

UINT16 CP1610::HLT() {
    return 4;
}

UINT16 CP1610::SDBD() {
    r[7]++;
    interruptible = FALSE;

    D = TRUE;

    return 4;
}

UINT16 CP1610::EIS() {
    r[7]++;
    interruptible = FALSE;

    I = TRUE;
    bHandleInterrupts = (!bCP1610_PIN_IN_BUSRQ || (I && !bCP1610_PIN_IN_INTRM));

    D = FALSE;
    return 4;
}

UINT16 CP1610::DIS() {
    r[7]++;
    interruptible = FALSE;

    I = FALSE;
    bHandleInterrupts = (!bCP1610_PIN_IN_BUSRQ || (I && !bCP1610_PIN_IN_INTRM));

    D = FALSE;
    return 4;
}

UINT16 CP1610::TCI() {
    r[7]++;
    interruptible = FALSE;

    //What should this really do?

    D = FALSE;
    return 4;
}

UINT16 CP1610::CLRC() {
    r[7]++;
    interruptible = FALSE;

    C = FALSE;

    D = FALSE;
    return 4;
}

UINT16 CP1610::SETC() {
    r[7]++;
    interruptible = FALSE;

    C = TRUE;

    D = FALSE;
    return 4;
}

ITCM_CODE UINT16 CP1610::J(UINT16 target) {
    r[7] = target;
    interruptible = TRUE;

    D = FALSE;
    return 12;
}

ITCM_CODE UINT16 CP1610::JSR(UINT16 registerNum, UINT16 target) {
    r[registerNum] = r[7]+3;
    r[7] = target;
    interruptible = TRUE;

    D = FALSE;
    return 12;
}

ITCM_CODE UINT16 CP1610::JE(UINT16 target) {
    I = TRUE;
    bHandleInterrupts = (!bCP1610_PIN_IN_BUSRQ || (I && !bCP1610_PIN_IN_INTRM));
    r[7] = target;
    interruptible = TRUE;
    D = FALSE;
    return 12;
}

ITCM_CODE UINT16 CP1610::JSRE(UINT16 registerNum, UINT16 target) {
    I = TRUE;
    bHandleInterrupts = (!bCP1610_PIN_IN_BUSRQ || (I && !bCP1610_PIN_IN_INTRM));
    r[registerNum] = r[7]+3;
    r[7] = target;
    interruptible = TRUE;

    D = FALSE;
    return 12;
}

UINT16 CP1610::JD(UINT16 target) {
    I = FALSE;
    bHandleInterrupts = (!bCP1610_PIN_IN_BUSRQ || (I && !bCP1610_PIN_IN_INTRM));
    r[7] = target;
    interruptible = TRUE;

    D = FALSE;
    return 12;
}

UINT16 CP1610::JSRD(UINT16 registerNum, UINT16 target) {
    I = FALSE;
    bHandleInterrupts = (!bCP1610_PIN_IN_BUSRQ || (I && !bCP1610_PIN_IN_INTRM));
    r[registerNum] = r[7]+3;
    r[7] = target;
    interruptible = TRUE;

    D = FALSE;
    return 12;
}

ITCM_CODE UINT16 CP1610::INCR(UINT16 registerNum) {
    r[7]++;
    interruptible = TRUE;

    r[registerNum]++;
    S = !!(r[registerNum] & 0x8000);
    Z = !r[registerNum];

    D = FALSE;
    return 6;
}

ITCM_CODE UINT16 CP1610::DECR(UINT16 registerNum) {
    r[7]++;
    interruptible = TRUE;

    r[registerNum]--;
    S = !!(r[registerNum] & 0x8000);
    Z = !r[registerNum];

    D = FALSE;
    return 6;
}

ITCM_CODE UINT16 CP1610::NEGR(UINT16 registerNum) {
    r[7]++;
    interruptible = TRUE;

    UINT16 op1 = r[registerNum];
    UINT32 newValue = (op1 ^ 0xFFFF) + 1;
    C = !!(newValue & 0x10000);
    O = !!(newValue & op1 & 0x8000);
    S = !!(newValue & 0x8000);
    Z = !(newValue & 0xFFFF);
    r[registerNum] = (UINT16)newValue;

    D = FALSE;
    return 6;
}

ITCM_CODE UINT16 CP1610::ADCR(UINT16 registerNum) {
    r[7]++;
    interruptible = TRUE;

    UINT16 op1 = r[registerNum];
    UINT16 op2 = (C ? 1 : 0);
    UINT32 newValue = op1 + op2;
    C = !!(newValue & 0x10000);
    O = !!((op2 ^ newValue) & ~(op1 ^ op2) & 0x8000);
    S = !!(newValue & 0x8000);
    Z = !(newValue & 0xFFFF);
    r[registerNum] = (UINT16)newValue;

    D = FALSE;
    return 6;
}

UINT16 CP1610::RSWD(UINT16 registerNum) {
    r[7]++;
    interruptible = TRUE;

    UINT16 value = r[registerNum];
    S = !!(value & 0x0080);
    Z = !!(value & 0x0040);
    O = !!(value & 0x0020);
    C = !!(value & 0x0010);

    D = FALSE;
    return 6;
}

UINT16 CP1610::GSWD(UINT16 registerNum) {
    r[7]++;
    interruptible = TRUE;

    UINT16 value = ((S ? 1 : 0) << 7) | ((Z ? 1 : 0) << 6) | 
            ((O ? 1 : 0) << 5) | ((C ? 1 : 0) << 4);
    value |= (value << 8);
    r[registerNum] = value;

    D = FALSE;
    return 6;
}

ITCM_CODE UINT16 CP1610::NOP(UINT16) {
    r[7]++;
    interruptible = TRUE;

    D = FALSE;
    return 6;
}

UINT16 CP1610::SIN(UINT16) {
    r[7]++;
    interruptible = TRUE;

    //TODO: does SIN need to do anything?!

    D = FALSE;
    return 6;
}

UINT16 CP1610::SWAP_1(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum];
    value = ((value & 0xFF00) >> 8) | ((value & 0xFF) << 8);
    S = !!(value & 0x0080);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 6;
}

UINT16 CP1610::SWAP_2(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum] & 0xFF;
    value |= (value << 8);
    S = !!(value & 0x8000);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 8;
}

UINT16 CP1610::COMR(UINT16 registerNum) {
    r[7]++;
    interruptible = TRUE;

    UINT16 value = r[registerNum] ^ 0xFFFF;
    S = !!(value & 0x8000);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 6;
}

UINT16 CP1610::SLL_1(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum] << 1;
    S = !!(value & 0x8000);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 6;
}

UINT16 CP1610::SLL_2(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum] << 2;
    S = !!(value & 0x8000);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 8;
}

UINT16 CP1610::RLC_1(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum];
    UINT16 carry = (C ? 1 : 0);
    C = !!(value & 0x8000);
    value = (value << 1) | carry;
    S = !!(value & 0x8000);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 6;
}

UINT16 CP1610::RLC_2(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum];
    UINT16 carry = (C ? 1 : 0);
    UINT16 overflow = (O ? 1 : 0);
    C = !!(value & 0x8000);
    O = !!(value & 0x4000);
    value = (value << 2) | (carry << 1) | overflow;
    S = !!(value & 0x8000);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 8;
}

UINT16 CP1610::SLLC_1(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum];
    C = !!(value & 0x8000);
    value <<= 1;
    S = !!(value & 0x8000);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 6;
}

UINT16 CP1610::SLLC_2(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum];
    C = !!(value & 0x8000);
    O = !!(value & 0x4000);
    value <<= 2;
    S = !!(value & 0x8000);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 8;
}

UINT16 CP1610::SLR_1(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum] >> 1;
    S = !!(value & 0x0080);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 6;
}

UINT16 CP1610::SLR_2(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum] >> 2;
    S = !!(value & 0x0080);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 8;
}

UINT16 CP1610::SAR_1(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum];
    value = (value >> 1) | (value & 0x8000);
    S = !!(value & 0x0080);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 6;
}

UINT16 CP1610::SAR_2(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum];
    UINT16 s  = value & 0x8000;
    value = (value >> 2) | s | (s >> 1);
    S = !!(value & 0x0080);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 8;
}

UINT16 CP1610::RRC_1(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum];
    UINT16 carry = (C ? 1 : 0);
    C = !!(value & 0x0001);
    value = (value >> 1) | (carry << 15);
    S = !!(value & 0x0080);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 6;
}

UINT16 CP1610::RRC_2(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum];
    UINT16 carry = (C ? 1 : 0);
    UINT16 overflow = (O ? 1 : 0);
    C = !!(value & 0x0001);
    O = !!(value & 0x0002);
    value = (value >> 2) | (carry << 14) | (overflow << 15);
    S = !!(value & 0x0080);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 8;
}

UINT16 CP1610::SARC_1(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum];
    C = !!(value & 0x0001);
    value = (value >> 1) | (value & 0x8000);
    S = !!(value & 0x0080);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 6;
}

UINT16 CP1610::SARC_2(UINT16 registerNum) {
    r[7]++;
    interruptible = FALSE;

    UINT16 value = r[registerNum];
    C = !!(value & 0x0001);
    O = !!(value & 0x0002);
    UINT16 s = value & 0x8000;
    value = (value >> 2) | s | (s >> 1);
    S = !!(value & 0x0080);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 8;
}

ITCM_CODE UINT16 CP1610::MOVR(UINT16 sourceReg, UINT16 destReg) {
    r[7]++;
    interruptible = TRUE;

    UINT16 value = r[sourceReg];
    S = !!(value & 0x8000);
    Z = !value;
    r[destReg] = value;

    D = FALSE;
    return (destReg >= 6 ? 7 : 6);
}

ITCM_CODE UINT16 CP1610::ADDR(UINT16 sourceReg, UINT16 destReg) {
    r[7]++;
    interruptible = TRUE;

    UINT16 op1 = r[sourceReg];
    UINT16 op2 = r[destReg];
    UINT32 newValue = op1 + op2;
    C = !!(newValue & 0x10000);
    O = !!((op2 ^ newValue) & ~(op1 ^ op2) & 0x8000);
    S = !!(newValue & 0x8000);
    Z = !(newValue & 0xFFFF);
    r[destReg] = (UINT16)newValue;

    D = FALSE;
    return 6;
}

ITCM_CODE UINT16 CP1610::SUBR(UINT16 sourceReg, UINT16 destReg) {
    r[7]++;
    interruptible = TRUE;

    UINT16 op1 = r[sourceReg];
    UINT16 op2 = r[destReg];
    UINT32 newValue = op2 + (0xFFFF ^ op1) + 1;
    C = !!(newValue & 0x10000);
    O = !!((op2 ^ newValue) & (op1 ^ op2) & 0x8000);
    S = !!(newValue & 0x8000);
    Z = !(newValue & 0xFFFF);
    r[destReg] = (UINT16)newValue;

    D = FALSE;
    return 6;
}

ITCM_CODE UINT16 CP1610::CMPR(UINT16 sourceReg, UINT16 destReg) {
    r[7]++;
    interruptible = TRUE;

    UINT16 op1 = r[sourceReg];
    UINT16 op2 = r[destReg];
    UINT32 newValue = op2 + (0xFFFF ^ op1) + 1;
    C = !!(newValue & 0x10000);
    O = !!((op2 ^ newValue) & (op1 ^ op2) & 0x8000);
    S = !!(newValue & 0x8000);
    Z = !(newValue & 0xFFFF);

    D = FALSE;
    return 6;
}

ITCM_CODE UINT16 CP1610::ANDR(UINT16 sourceReg, UINT16 destReg) {
    r[7]++;
    interruptible = TRUE;

    UINT16 newValue = r[destReg] & r[sourceReg];
    S = !!(newValue & 0x8000);
    Z = !newValue;
    r[destReg] = newValue;

    D = FALSE;
    return 6;
}

UINT16 CP1610::XORR(UINT16 sourceReg, UINT16 destReg) {
    r[7]++;
    interruptible = TRUE;

    UINT16 newValue = r[destReg] ^ r[sourceReg];
    S = !!(newValue & 0x8000);
    Z = !newValue;
    r[destReg] = newValue;

    D = FALSE;
    return 6;
}

UINT16 CP1610::BEXT(UINT16 condition, INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (ext == condition) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

ITCM_CODE UINT16 CP1610::B(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;
    r[7] = (UINT16)(r[7] + displacement);

    D = FALSE;
    return 9;
}

UINT16 CP1610::NOPP(INT16) {
    r[7] += 2;
    interruptible = TRUE;

    D = FALSE;
    return 7;
}

UINT16 CP1610::BC(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (C) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

UINT16 CP1610::BNC(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (!C) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

UINT16 CP1610::BOV(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (O) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

UINT16 CP1610::BNOV(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (!O) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

UINT16 CP1610::BPL(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (!S) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

UINT16 CP1610::BMI(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (S) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

ITCM_CODE UINT16 CP1610::BEQ(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (Z) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

ITCM_CODE UINT16 CP1610::BNEQ(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (!Z) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

ITCM_CODE UINT16 CP1610::BLT(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (S != O) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

ITCM_CODE UINT16 CP1610::BGE(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (S == O) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

ITCM_CODE UINT16 CP1610::BLE(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (Z || (S != O)) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

ITCM_CODE UINT16 CP1610::BGT(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (!(Z || (S != O))) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

UINT16 CP1610::BUSC(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (C != S) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

UINT16 CP1610::BESC(INT16 displacement) {
    r[7] += 2;
    interruptible = TRUE;

    if (C == S) {
        r[7] = (UINT16)(r[7] + displacement);
        D = FALSE;
        return 9;
    }

    D = FALSE;
    return 7;
}

ITCM_CODE UINT16 CP1610::MVO(UINT16 registerNum, UINT16 address) {
    r[7] += 2;
    interruptible = FALSE;

    memoryBus->poke(address, r[registerNum]);

    D = FALSE;
    return 11;
}

ITCM_CODE UINT16 CP1610::MVO_ind(UINT16 registerWithAddress, UINT16 registerToMove) {
    r[7]++;
    interruptible = FALSE;

    memoryBus->poke(r[registerWithAddress], r[registerToMove]);

    //if the register number is 4-7, increment it
    if (registerWithAddress & 0x04)
        r[registerWithAddress]++;

    D = FALSE;
    return 9;
}

ITCM_CODE UINT16 CP1610::MVI(UINT16 address, UINT16 registerNum) {
    r[7] += 2;
    interruptible = TRUE;

    r[registerNum] = memoryBus->peek(address);

    D = FALSE;
    return 10;
}

ITCM_CODE UINT16 CP1610::MVI_ind(UINT16 registerWithAddress, UINT16 registerToReceive) {
    r[7]++;
    interruptible = TRUE;

    r[registerToReceive] = getIndirect(registerWithAddress);

    UINT8 cycles = (D ? 10 : (registerWithAddress == 6 ? 11 : 8));
    D = FALSE;
    return cycles;
}

ITCM_CODE UINT16 CP1610::ADD(UINT16 address, UINT16 registerNum) {
    r[7] += 2;
    interruptible = TRUE;

    UINT16 op1 = memoryBus->peek(address);
    UINT16 op2 = r[registerNum];
    UINT32 newValue = op1 + op2;
    C = !!(newValue & 0x10000);
    O = !!((op2 ^ newValue) & ~(op1 ^ op2) & 0x8000);
    S = !!(newValue & 0x8000);
    Z = !(newValue & 0xFFFF);
    r[registerNum] = (UINT16)newValue;

    D = FALSE;
    return 10;
}

ITCM_CODE UINT16 CP1610::ADD_ind(UINT16 registerWithAddress, UINT16 registerToReceive) {
    r[7]++;
    interruptible = TRUE;

    UINT16 op1 = getIndirect(registerWithAddress);
    UINT16 op2 = r[registerToReceive];
    UINT32 newValue = op1 + op2;
    C = !!(newValue & 0x10000);
    O = !!((op2 ^ newValue) & ~(op1 ^ op2) & 0x8000);
    S = !!(newValue & 0x8000);
    Z = !(newValue & 0xFFFF);
    r[registerToReceive] = (UINT16)newValue;
    
    UINT8 cycles = (D ? 10 : (registerWithAddress == 6 ? 11 : 8));
    D = FALSE;
    return cycles;
}

ITCM_CODE UINT16 CP1610::SUB(UINT16 address, UINT16 registerNum) {
    r[7] += 2;
    interruptible = TRUE;

    UINT16 op1 = memoryBus->peek(address);
    UINT16 op2 = r[registerNum];
    UINT32 newValue = op2 + (0xFFFF ^ op1) + 1;
    C = !!(newValue & 0x10000);
    O = !!((op2 ^ newValue) & (op1 ^ op2) & 0x8000);
    S = !!(newValue & 0x8000);
    Z = !(newValue & 0xFFFF);
    r[registerNum] = (UINT16)newValue;

    D = FALSE;
    return 10;
}

ITCM_CODE UINT16 CP1610::SUB_ind(UINT16 registerWithAddress, UINT16 registerToReceive) {
    r[7]++;
    interruptible = TRUE;

    UINT16 op1 = getIndirect(registerWithAddress);
    UINT16 op2 = r[registerToReceive];
    UINT32 newValue = op2 + (0xFFFF ^ op1) + 1;
    C = !!(newValue & 0x10000);
    O = !!((op2 ^ newValue) & (op1 ^ op2) & 0x8000);
    S = !!(newValue & 0x8000);
    Z = !(newValue & 0xFFFF);
    r[registerToReceive] = (UINT16)newValue;

    UINT8 cycles = (D ? 10 : (registerWithAddress == 6 ? 11 : 8));
    D = FALSE;
    return cycles;
}

ITCM_CODE UINT16 CP1610::CMP(UINT16 address, UINT16 registerNum) {
    r[7] += 2;
    interruptible = TRUE;

    UINT16 op1 = memoryBus->peek(address);
    UINT16 op2 = r[registerNum];
    UINT32 newValue = op2 + (0xFFFF ^ op1) + 1;
    C = !!(newValue & 0x10000);
    O = !!((op2 ^ newValue) & (op1 ^ op2) & 0x8000);
    S = !!(newValue & 0x8000);
    Z = !(newValue & 0xFFFF);

    D = FALSE;
    return 10;
}

ITCM_CODE UINT16 CP1610::CMP_ind(UINT16 registerWithAddress, UINT16 registerToReceive) {
    r[7]++;
    interruptible = TRUE;

    UINT16 op1 = getIndirect(registerWithAddress);
    UINT16 op2 = r[registerToReceive];
    UINT32 newValue = op2 + (0xFFFF ^ op1) + 1;
    C = !!(newValue & 0x10000);
    O = !!((op2 ^ newValue) & (op1 ^ op2) & 0x8000);
    S = !!(newValue & 0x8000);
    Z = !(newValue & 0xFFFF);

    UINT8 cycles = (D ? 10 : (registerWithAddress == 6 ? 11 : 8));
    D = FALSE;
    return cycles;
}

ITCM_CODE UINT16 CP1610::AND(UINT16 address, UINT16 registerNum) {
    r[7] += 2;
    interruptible = TRUE;

    UINT16 value = memoryBus->peek(address) & r[registerNum];
    S = !!(value & 0x8000);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 10;
}

ITCM_CODE UINT16 CP1610::AND_ind(UINT16 registerWithAddress, UINT16 registerToReceive) {
    r[7]++;
    interruptible = TRUE;

    UINT16 value = getIndirect(registerWithAddress) & r[registerToReceive];
    S = !!(value & 0x8000);
    Z = !value;
    r[registerToReceive] = value;
    
    UINT8 cycles = (D ? 10 : (registerWithAddress == 6 ? 11 : 8));
    D = FALSE;
    return cycles;
}

ITCM_CODE UINT16 CP1610::XOR(UINT16 address, UINT16 registerNum) {
    r[7] += 2;
    interruptible = TRUE;

    UINT16 value = memoryBus->peek(address) ^ r[registerNum];
    S = !!(value & 0x8000);
    Z = !value;
    r[registerNum] = value;

    D = FALSE;
    return 10;
}

UINT16 CP1610::XOR_ind(UINT16 registerWithAddress, UINT16 registerToReceive) {
    r[7]++;
    interruptible = TRUE;

    UINT16 value = getIndirect(registerWithAddress) ^ r[registerToReceive];
    S = !!(value & 0x8000);
    Z = !value;
    r[registerToReceive] = value;
    
    UINT8 cycles = (D ? 10 : (registerWithAddress == 6 ? 11 : 8));
    D = FALSE;
    return cycles;
}

void CP1610::getState(CP1610State *state)
{
    state->S  = S;
    state->Z  = Z;
    state->O  = O;
    state->C  = C;
    state->I  = I;
    state->D  = D;
    state->interruptible = interruptible;
    state->ext = ext;
    state->interruptAddress = interruptAddress;
    state->resetAddress = resetAddress;
    
    state->bCP1610_PIN_IN_BUSRQ = bCP1610_PIN_IN_BUSRQ;
    state->bCP1610_PIN_IN_INTRM = bCP1610_PIN_IN_INTRM; 
    state->bCP1610_PIN_OUT_BUSAK = bCP1610_PIN_OUT_BUSAK;
   
    for (int i=0; i<8; i++)  state->r[i] = r[i];
}
 
void CP1610::setState(CP1610State *state)
{
    S = state->S;
    Z = state->Z;
    O = state->O;
    C = state->C;
    I = state->I;
    D = state->D;
    interruptible = state->interruptible;
    ext = state->ext;
    interruptAddress = state->interruptAddress;
    resetAddress = state->resetAddress;
    
    bCP1610_PIN_IN_BUSRQ = state->bCP1610_PIN_IN_BUSRQ;
    bCP1610_PIN_IN_INTRM = state->bCP1610_PIN_IN_INTRM; 
    bCP1610_PIN_OUT_BUSAK = state->bCP1610_PIN_OUT_BUSAK;
    
    for (int i=0; i<8; i++)  r[i] = state->r[i];
    
    bHandleInterrupts = (!bCP1610_PIN_IN_BUSRQ || (I && !bCP1610_PIN_IN_INTRM));
}
