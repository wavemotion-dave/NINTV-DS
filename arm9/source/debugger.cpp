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
#include <nds/fifomessages.h>

#include<stdio.h>

#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "ds_tools.h"
#include "bgBottom.h"
#include "bgTop.h"
#include "bgDebug.h"
#include "Emulator.h"
#include "overlay.h"
#include "debugger.h"
#include "AY38914_Channel.h"
#include "AY38914.h"
#include "AudioOutputLine.h"
#include "AudioMixer.h"

#ifdef DEBUG_ENABLE

INT32 debug[6] = {0};

UINT32 debug_frames=0;
UINT32 debug_opcodes=0;

extern int bg0, bg0b, bg1b;
extern struct Overlay_t defaultOverlay[OVL_MAX];
extern Emulator             *currentEmu;

AY38900 *debug_stic = NULL;

void display_debug(void);

#define PEEK_FAST(x) *((UINT16 *)0x06880000 + (x))

#define PEEK_SLOW(x) currentEmu->memoryBus.peek(x)

struct Overlay_t debuggerOverlay[OVL_MAX] =
{
    {255,   255,    255,   255},    // KEY_1
    {255,   255,    255,   255},    // KEY_2
    {255,   255,    255,   255},    // KEY_3
    {255,   255,    255,   255},    // KEY_4
    {255,   255,    255,   255},    // KEY_5
    {255,   255,    255,   255},    // KEY_6
    {255,   255,    255,   255},    // KEY_7
    {255,   255,    255,   255},    // KEY_8
    {255,   255,    255,   255},    // KEY_9
    {255,   255,    255,   255},    // KEY_CLEAR
    {255,   255,    255,   255},    // KEY_0
    {255,   255,    255,   255},    // KEY_ENTER
    {255,   255,    255,   255},    // KEY_FIRE
    {255,   255,    255,   255},    // KEY_L_ACT
    {255,   255,    255,   255},    // KEY_R_ACT
    
    {255,   255,    255,   255},    // META_RESET
    {128,   254,      1,   150},    // META_LOAD
    {255,   255,    255,   255},    // META_CONFIG
    {255,   255,    255,   255},    // META_SCORE
    {255,   255,    255,   255},    // META_QUIT
    {255,   255,    255,   255},    // META_STATE
    {1,     128,      1,   150},    // META_MENU
    {255,   255,    255,   255},    // META_SWAP
    {255,   255,    255,   255},    // META_MANUAL
};

//the six flags available in the CP1610
extern UINT8 S;
extern UINT8 Z;
extern UINT8 O;
extern UINT8 C;
extern UINT8 I;
extern UINT8 D;

extern UINT16 op;
extern UINT8 oneSecTick;

#define DBG_PRESS_PLAY   0
#define DBG_PRESS_STOP   1
#define DBG_PRESS_STEP   2
#define DBG_PRESS_FRAME  3
#define DBG_PRESS_NONE   255

#define DBG_MODE_PLAY    0
#define DBG_MODE_STOP    1
#define DBG_MODE_STEP    2
#define DBG_MODE_FRAME   3

UINT8 debug_mode = DBG_MODE_PLAY;

#define DBG_SHOW_CPU    0
#define DBG_SHOW_RAM    1
#define DBG_SHOW_PSG    2
#define DBG_SHOW_STIC   3

UINT8 debug_show = DBG_SHOW_CPU;

UINT8 debug_show_ram = 0;
UINT8 debug_show_stic = 0;
UINT8 debug_show_psg = 0;


const char *dbg_opcode(UINT16 op)
{
    switch (op & 0x3FF) {
        case 0x0000:
            return "HLT()";
        case 0x0001:
            return "SDBD()";
        case 0x0002:
            return "EIS()";
        case 0x0003:
            return "DIS()";
        case 0x0004:
		    return "JJEJD()";
        case 0x0005:
            return "TCI()";
        case 0x0006:
            return "CLRC()";
        case 0x0007:
            return "SETC()";
        case 0x0008:
            return "INCR(0)";
        case 0x0009:
            return "INCR(1)";
        case 0x000A:
            return "INCR(2)";
        case 0x000B:
            return "INCR(3)";
        case 0x000C:
            return "INCR(4)";
        case 0x000D:
            return "INCR(5)";
        case 0x000E:
            return "INCR(6)";
        case 0x000F:
            return "INCR(7)";
        case 0x0010:
            return "DECR(0)";

        case 0x0011:
            return "DECR(1)";

        case 0x0012:
            return "DECR(2)";

        case 0x0013:
            return "DECR(3)";

        case 0x0014:
            return "DECR(4)";

        case 0x0015:
            return "DECR(5)";

        case 0x0016:
            return "DECR(6)";

        case 0x0017:
            return "DECR(7)";

        case 0x0018:
            return "COMR(0)";

        case 0x0019:
            return "COMR(1)";

        case 0x001A:
            return "COMR(2)";

        case 0x001B:
            return "COMR(3)";

        case 0x001C:
            return "COMR(4)";

        case 0x001D:
            return "COMR(5)";

        case 0x001E:
            return "COMR(6)";

        case 0x001F:
            return "COMR(7)";

        case 0x0020:
            return "NEGR(0)";

        case 0x0021:
            return "NEGR(1)";

        case 0x0022:
            return "NEGR(2)";

        case 0x0023:
            return "NEGR(3)";

        case 0x0024:
            return "NEGR(4)";

        case 0x0025:
            return "NEGR(5)";

        case 0x0026:
            return "NEGR(6)";

        case 0x0027:
            return "NEGR(7)";

        case 0x0028:
            return "ADCR(0)";

        case 0x0029:
            return "ADCR(1)";

        case 0x002A:
            return "ADCR(2)";

        case 0x002B:
            return "ADCR(3)";

        case 0x002C:
            return "ADCR(4)";

        case 0x002D:
            return "ADCR(5)";

        case 0x002E:
            return "ADCR(6)";

        case 0x002F:
            return "ADCR(7)";

        case 0x0030:
            return "GSWD(0)";

        case 0x0031:
            return "GSWD(1)";

        case 0x0032:
            return "GSWD(2)";

        case 0x0033:
            return "GSWD(3)";

        case 0x0034:
            return "NOP(0)";

        case 0x0035:
            return "NOP(1)";

        case 0x0036:
            return "SIN(0)";

        case 0x0037:
            return "SIN(1)";

        case 0x0038:
            return "RSWD(0)";

        case 0x0039:
            return "RSWD(1)";

        case 0x003A:
            return "RSWD(2)";

        case 0x003B:
            return "RSWD(3)";

        case 0x003C:
            return "RSWD(4)";

        case 0x003D:
            return "RSWD(5)";

        case 0x003E:
            return "RSWD(6)";

        case 0x003F:
            return "RSWD(7)";

        case 0x0040:
            return "SWAP_1(0)";

        case 0x0041:
            return "SWAP_1(1)";

        case 0x0042:
            return "SWAP_1(2)";

        case 0x0043:
            return "SWAP_1(3)";

        case 0x0044:
            return "SWAP_2(0)";

        case 0x0045:
            return "SWAP_2(1)";

        case 0x0046:
            return "SWAP_2(2)";

        case 0x0047:
            return "SWAP_2(3)";

        case 0x0048:
            return "SLL_1(0)";

        case 0x0049:
            return "SLL_1(1)";

        case 0x004A:
            return "SLL_1(2)";

        case 0x004B:
            return "SLL_1(3)";

        case 0x004C:
            return "SLL_2(0)";

        case 0x004D:
            return "SLL_2(1)";

        case 0x004E:
            return "SLL_2(2)";

        case 0x004F:
            return "SLL_2(3)";

        case 0x0050:
            return "RLC_1(0)";

        case 0x0051:
            return "RLC_1(1)";

        case 0x0052:
            return "RLC_1(2)";

        case 0x0053:
            return "RLC_1(3)";

        case 0x0054:
            return "RLC_2(0)";

        case 0x0055:
            return "RLC_2(1)";

        case 0x0056:
            return "RLC_2(2)";

        case 0x0057:
            return "RLC_2(3)";

        case 0x0058:
            return "SLLC_1(0)";

        case 0x0059:
            return "SLLC_1(1)";

        case 0x005A:
            return "SLLC_1(2)";

        case 0x005B:
            return "SLLC_1(3)";

        case 0x005C:
            return "SLLC_2(0)";

        case 0x005D:
            return "SLLC_2(1)";

        case 0x005E:
            return "SLLC_2(2)";

        case 0x005F:
            return "SLLC_2(3)";

        case 0x0060:
            return "SLR_1(0)";

        case 0x0061:
            return "SLR_1(1)";

        case 0x0062:
            return "SLR_1(2)";

        case 0x0063:
            return "SLR_1(3)";

        case 0x0064:
            return "SLR_2(0)";

        case 0x0065:
            return "SLR_2(1)";

        case 0x0066:
            return "SLR_2(2)";

        case 0x0067:
            return "SLR_2(3)";

        case 0x0068:
            return "SAR_1(0)";

        case 0x0069:
            return "SAR_1(1)";

        case 0x006A:
            return "SAR_1(2)";

        case 0x006B:
            return "SAR_1(3)";

        case 0x006C:
            return "SAR_2(0)";

        case 0x006D:
            return "SAR_2(1)";

        case 0x006E:
            return "SAR_2(2)";

        case 0x006F:
            return "SAR_2(3)";

        case 0x0070:
            return "RRC_1(0)";

        case 0x0071:
            return "RRC_1(1)";

        case 0x0072:
            return "RRC_1(2)";

        case 0x0073:
            return "RRC_1(3)";

        case 0x0074:
            return "RRC_2(0)";

        case 0x0075:
            return "RRC_2(1)";

        case 0x0076:
            return "RRC_2(2)";

        case 0x0077:
            return "RRC_2(3)";

        case 0x0078:
            return "SARC_1(0)";

        case 0x0079:
            return "SARC_1(1)";

        case 0x007A:
            return "SARC_1(2)";

        case 0x007B:
            return "SARC_1(3)";

        case 0x007C:
            return "SARC_2(0)";

        case 0x007D:
            return "SARC_2(1)";

        case 0x007E:
            return "SARC_2(2)";

        case 0x007F:
            return "SARC_2(3)";

        case 0x0080:
            return "MOVR(0, 0)";

        case 0x0081:
            return "MOVR(0, 1)";

        case 0x0082:
            return "MOVR(0, 2)";

        case 0x0083:
            return "MOVR(0, 3)";

        case 0x0084:
            return "MOVR(0, 4)";

        case 0x0085:
            return "MOVR(0, 5)";

        case 0x0086:
            return "MOVR(0, 6)";

        case 0x0087:
            return "MOVR(0, 7)";

        case 0x0088:
            return "MOVR(1, 0)";

        case 0x0089:
            return "MOVR(1, 1)";

        case 0x008A:
            return "MOVR(1, 2)";

        case 0x008B:
            return "MOVR(1, 3)";

        case 0x008C:
            return "MOVR(1, 4)";

        case 0x008D:
            return "MOVR(1, 5)";

        case 0x008E:
            return "MOVR(1, 6)";

        case 0x008F:
            return "MOVR(1, 7)";

        case 0x0090:
            return "MOVR(2, 0)";

        case 0x0091:
            return "MOVR(2, 1)";

        case 0x0092:
            return "MOVR(2, 2)";

        case 0x0093:
            return "MOVR(2, 3)";

        case 0x0094:
            return "MOVR(2, 4)";

        case 0x0095:
            return "MOVR(2, 5)";

        case 0x0096:
            return "MOVR(2, 6)";

        case 0x0097:
            return "MOVR(2, 7)";

        case 0x0098:
            return "MOVR(3, 0)";

        case 0x0099:
            return "MOVR(3, 1)";

        case 0x009A:
            return "MOVR(3, 2)";

        case 0x009B:
            return "MOVR(3, 3)";

        case 0x009C:
            return "MOVR(3, 4)";

        case 0x009D:
            return "MOVR(3, 5)";

        case 0x009E:
            return "MOVR(3, 6)";

        case 0x009F:
            return "MOVR(3, 7)";

        case 0x00A0:
            return "MOVR(4, 0)";

        case 0x00A1:
            return "MOVR(4, 1)";

        case 0x00A2:
            return "MOVR(4, 2)";

        case 0x00A3:
            return "MOVR(4, 3)";

        case 0x00A4:
            return "MOVR(4, 4)";

        case 0x00A5:
            return "MOVR(4, 5)";

        case 0x00A6:
            return "MOVR(4, 6)";

        case 0x00A7:
            return "MOVR(4, 7)";

        case 0x00A8:
            return "MOVR(5, 0)";

        case 0x00A9:
            return "MOVR(5, 1)";

        case 0x00AA:
            return "MOVR(5, 2)";

        case 0x00AB:
            return "MOVR(5, 3)";

        case 0x00AC:
            return "MOVR(5, 4)";

        case 0x00AD:
            return "MOVR(5, 5)";

        case 0x00AE:
            return "MOVR(5, 6)";

        case 0x00AF:
            return "MOVR(5, 7)";

        case 0x00B0:
            return "MOVR(6, 0)";

        case 0x00B1:
            return "MOVR(6, 1)";

        case 0x00B2:
            return "MOVR(6, 2)";

        case 0x00B3:
            return "MOVR(6, 3)";

        case 0x00B4:
            return "MOVR(6, 4)";

        case 0x00B5:
            return "MOVR(6, 5)";

        case 0x00B6:
            return "MOVR(6, 6)";

        case 0x00B7:
            return "MOVR(6, 7)";

        case 0x00B8:
            return "MOVR(7, 0)";

        case 0x00B9:
            return "MOVR(7, 1)";

        case 0x00BA:
            return "MOVR(7, 2)";

        case 0x00BB:
            return "MOVR(7, 3)";

        case 0x00BC:
            return "MOVR(7, 4)";

        case 0x00BD:
            return "MOVR(7, 5)";

        case 0x00BE:
            return "MOVR(7, 6)";

        case 0x00BF:
            return "MOVR(7, 7)";

        case 0x00C0:
            return "ADDR(0, 0)";

        case 0x00C1:
            return "ADDR(0, 1)";

        case 0x00C2:
            return "ADDR(0, 2)";

        case 0x00C3:
            return "ADDR(0, 3)";

        case 0x00C4:
            return "ADDR(0, 4)";

        case 0x00C5:
            return "ADDR(0, 5)";

        case 0x00C6:
            return "ADDR(0, 6)";

        case 0x00C7:
            return "ADDR(0, 7)";

        case 0x00C8:
            return "ADDR(1, 0)";

        case 0x00C9:
            return "ADDR(1, 1)";

        case 0x00CA:
            return "ADDR(1, 2)";

        case 0x00CB:
            return "ADDR(1, 3)";

        case 0x00CC:
            return "ADDR(1, 4)";

        case 0x00CD:
            return "ADDR(1, 5)";

        case 0x00CE:
            return "ADDR(1, 6)";

        case 0x00CF:
            return "ADDR(1, 7)";

        case 0x00D0:
            return "ADDR(2, 0)";

        case 0x00D1:
            return "ADDR(2, 1)";

        case 0x00D2:
            return "ADDR(2, 2)";

        case 0x00D3:
            return "ADDR(2, 3)";

        case 0x00D4:
            return "ADDR(2, 4)";

        case 0x00D5:
            return "ADDR(2, 5)";

        case 0x00D6:
            return "ADDR(2, 6)";

        case 0x00D7:
            return "ADDR(2, 7)";

        case 0x00D8:
            return "ADDR(3, 0)";

        case 0x00D9:
            return "ADDR(3, 1)";

        case 0x00DA:
            return "ADDR(3, 2)";

        case 0x00DB:
            return "ADDR(3, 3)";

        case 0x00DC:
            return "ADDR(3, 4)";

        case 0x00DD:
            return "ADDR(3, 5)";

        case 0x00DE:
            return "ADDR(3, 6)";

        case 0x00DF:
            return "ADDR(3, 7)";

        case 0x00E0:
            return "ADDR(4, 0)";

        case 0x00E1:
            return "ADDR(4, 1)";

        case 0x00E2:
            return "ADDR(4, 2)";

        case 0x00E3:
            return "ADDR(4, 3)";

        case 0x00E4:
            return "ADDR(4, 4)";

        case 0x00E5:
            return "ADDR(4, 5)";

        case 0x00E6:
            return "ADDR(4, 6)";

        case 0x00E7:
            return "ADDR(4, 7)";

        case 0x00E8:
            return "ADDR(5, 0)";

        case 0x00E9:
            return "ADDR(5, 1)";

        case 0x00EA:
            return "ADDR(5, 2)";

        case 0x00EB:
            return "ADDR(5, 3)";

        case 0x00EC:
            return "ADDR(5, 4)";

        case 0x00ED:
            return "ADDR(5, 5)";

        case 0x00EE:
            return "ADDR(5, 6)";

        case 0x00EF:
            return "ADDR(5, 7)";

        case 0x00F0:
            return "ADDR(6, 0)";

        case 0x00F1:
            return "ADDR(6, 1)";

        case 0x00F2:
            return "ADDR(6, 2)";

        case 0x00F3:
            return "ADDR(6, 3)";

        case 0x00F4:
            return "ADDR(6, 4)";

        case 0x00F5:
            return "ADDR(6, 5)";

        case 0x00F6:
            return "ADDR(6, 6)";

        case 0x00F7:
            return "ADDR(6, 7)";

        case 0x00F8:
            return "ADDR(7, 0)";

        case 0x00F9:
            return "ADDR(7, 1)";

        case 0x00FA:
            return "ADDR(7, 2)";

        case 0x00FB:
            return "ADDR(7, 3)";

        case 0x00FC:
            return "ADDR(7, 4)";

        case 0x00FD:
            return "ADDR(7, 5)";

        case 0x00FE:
            return "ADDR(7, 6)";

        case 0x00FF:
            return "ADDR(7, 7)";

        case 0x0100:
            return "SUBR(0, 0)";

        case 0x0101:
            return "SUBR(0, 1)";

        case 0x0102:
            return "SUBR(0, 2)";

        case 0x0103:
            return "SUBR(0, 3)";

        case 0x0104:
            return "SUBR(0, 4)";

        case 0x0105:
            return "SUBR(0, 5)";

        case 0x0106:
            return "SUBR(0, 6)";

        case 0x0107:
            return "SUBR(0, 7)";

        case 0x0108:
            return "SUBR(1, 0)";

        case 0x0109:
            return "SUBR(1, 1)";

        case 0x010A:
            return "SUBR(1, 2)";

        case 0x010B:
            return "SUBR(1, 3)";

        case 0x010C:
            return "SUBR(1, 4)";

        case 0x010D:
            return "SUBR(1, 5)";

        case 0x010E:
            return "SUBR(1, 6)";

        case 0x010F:
            return "SUBR(1, 7)";

        case 0x0110:
            return "SUBR(2, 0)";

        case 0x0111:
            return "SUBR(2, 1)";

        case 0x0112:
            return "SUBR(2, 2)";

        case 0x0113:
            return "SUBR(2, 3)";

        case 0x0114:
            return "SUBR(2, 4)";

        case 0x0115:
            return "SUBR(2, 5)";

        case 0x0116:
            return "SUBR(2, 6)";

        case 0x0117:
            return "SUBR(2, 7)";

        case 0x0118:
            return "SUBR(3, 0)";

        case 0x0119:
            return "SUBR(3, 1)";

        case 0x011A:
            return "SUBR(3, 2)";

        case 0x011B:
            return "SUBR(3, 3)";

        case 0x011C:
            return "SUBR(3, 4)";

        case 0x011D:
            return "SUBR(3, 5)";

        case 0x011E:
            return "SUBR(3, 6)";

        case 0x011F:
            return "SUBR(3, 7)";

        case 0x0120:
            return "SUBR(4, 0)";

        case 0x0121:
            return "SUBR(4, 1)";

        case 0x0122:
            return "SUBR(4, 2)";

        case 0x0123:
            return "SUBR(4, 3)";

        case 0x0124:
            return "SUBR(4, 4)";

        case 0x0125:
            return "SUBR(4, 5)";

        case 0x0126:
            return "SUBR(4, 6)";

        case 0x0127:
            return "SUBR(4, 7)";

        case 0x0128:
            return "SUBR(5, 0)";

        case 0x0129:
            return "SUBR(5, 1)";

        case 0x012A:
            return "SUBR(5, 2)";

        case 0x012B:
            return "SUBR(5, 3)";

        case 0x012C:
            return "SUBR(5, 4)";

        case 0x012D:
            return "SUBR(5, 5)";

        case 0x012E:
            return "SUBR(5, 6)";

        case 0x012F:
            return "SUBR(5, 7)";

        case 0x0130:
            return "SUBR(6, 0)";

        case 0x0131:
            return "SUBR(6, 1)";

        case 0x0132:
            return "SUBR(6, 2)";

        case 0x0133:
            return "SUBR(6, 3)";

        case 0x0134:
            return "SUBR(6, 4)";

        case 0x0135:
            return "SUBR(6, 5)";

        case 0x0136:
            return "SUBR(6, 6)";

        case 0x0137:
            return "SUBR(6, 7)";

        case 0x0138:
            return "SUBR(7, 0)";

        case 0x0139:
            return "SUBR(7, 1)";

        case 0x013A:
            return "SUBR(7, 2)";

        case 0x013B:
            return "SUBR(7, 3)";

        case 0x013C:
            return "SUBR(7, 4)";

        case 0x013D:
            return "SUBR(7, 5)";

        case 0x013E:
            return "SUBR(7, 6)";

        case 0x013F:
            return "SUBR(7, 7)";

        case 0x0140:
            return "CMPR(0, 0)";

        case 0x0141:
            return "CMPR(0, 1)";

        case 0x0142:
            return "CMPR(0, 2)";

        case 0x0143:
            return "CMPR(0, 3)";

        case 0x0144:
            return "CMPR(0, 4)";

        case 0x0145:
            return "CMPR(0, 5)";

        case 0x0146:
            return "CMPR(0, 6)";

        case 0x0147:
            return "CMPR(0, 7)";

        case 0x0148:
            return "CMPR(1, 0)";

        case 0x0149:
            return "CMPR(1, 1)";

        case 0x014A:
            return "CMPR(1, 2)";

        case 0x014B:
            return "CMPR(1, 3)";

        case 0x014C:
            return "CMPR(1, 4)";

        case 0x014D:
            return "CMPR(1, 5)";

        case 0x014E:
            return "CMPR(1, 6)";

        case 0x014F:
            return "CMPR(1, 7)";

        case 0x0150:
            return "CMPR(2, 0)";

        case 0x0151:
            return "CMPR(2, 1)";

        case 0x0152:
            return "CMPR(2, 2)";

        case 0x0153:
            return "CMPR(2, 3)";

        case 0x0154:
            return "CMPR(2, 4)";

        case 0x0155:
            return "CMPR(2, 5)";

        case 0x0156:
            return "CMPR(2, 6)";

        case 0x0157:
            return "CMPR(2, 7)";

        case 0x0158:
            return "CMPR(3, 0)";

        case 0x0159:
            return "CMPR(3, 1)";

        case 0x015A:
            return "CMPR(3, 2)";

        case 0x015B:
            return "CMPR(3, 3)";

        case 0x015C:
            return "CMPR(3, 4)";

        case 0x015D:
            return "CMPR(3, 5)";

        case 0x015E:
            return "CMPR(3, 6)";

        case 0x015F:
            return "CMPR(3, 7)";

        case 0x0160:
            return "CMPR(4, 0)";

        case 0x0161:
            return "CMPR(4, 1)";

        case 0x0162:
            return "CMPR(4, 2)";

        case 0x0163:
            return "CMPR(4, 3)";

        case 0x0164:
            return "CMPR(4, 4)";

        case 0x0165:
            return "CMPR(4, 5)";

        case 0x0166:
            return "CMPR(4, 6)";

        case 0x0167:
            return "CMPR(4, 7)";

        case 0x0168:
            return "CMPR(5, 0)";

        case 0x0169:
            return "CMPR(5, 1)";

        case 0x016A:
            return "CMPR(5, 2)";

        case 0x016B:
            return "CMPR(5, 3)";

        case 0x016C:
            return "CMPR(5, 4)";

        case 0x016D:
            return "CMPR(5, 5)";

        case 0x016E:
            return "CMPR(5, 6)";

        case 0x016F:
            return "CMPR(5, 7)";

        case 0x0170:
            return "CMPR(6, 0)";

        case 0x0171:
            return "CMPR(6, 1)";

        case 0x0172:
            return "CMPR(6, 2)";

        case 0x0173:
            return "CMPR(6, 3)";

        case 0x0174:
            return "CMPR(6, 4)";

        case 0x0175:
            return "CMPR(6, 5)";

        case 0x0176:
            return "CMPR(6, 6)";

        case 0x0177:
            return "CMPR(6, 7)";

        case 0x0178:
            return "CMPR(7, 0)";

        case 0x0179:
            return "CMPR(7, 1)";

        case 0x017A:
            return "CMPR(7, 2)";

        case 0x017B:
            return "CMPR(7, 3)";

        case 0x017C:
            return "CMPR(7, 4)";

        case 0x017D:
            return "CMPR(7, 5)";

        case 0x017E:
            return "CMPR(7, 6)";

        case 0x017F:
            return "CMPR(7, 7)";

        case 0x0180:
            return "ANDR(0, 0)";

        case 0x0181:
            return "ANDR(0, 1)";

        case 0x0182:
            return "ANDR(0, 2)";

        case 0x0183:
            return "ANDR(0, 3)";

        case 0x0184:
            return "ANDR(0, 4)";

        case 0x0185:
            return "ANDR(0, 5)";

        case 0x0186:
            return "ANDR(0, 6)";

        case 0x0187:
            return "ANDR(0, 7)";

        case 0x0188:
            return "ANDR(1, 0)";

        case 0x0189:
            return "ANDR(1, 1)";

        case 0x018A:
            return "ANDR(1, 2)";

        case 0x018B:
            return "ANDR(1, 3)";

        case 0x018C:
            return "ANDR(1, 4)";

        case 0x018D:
            return "ANDR(1, 5)";

        case 0x018E:
            return "ANDR(1, 6)";

        case 0x018F:
            return "ANDR(1, 7)";

        case 0x0190:
            return "ANDR(2, 0)";

        case 0x0191:
            return "ANDR(2, 1)";

        case 0x0192:
            return "ANDR(2, 2)";

        case 0x0193:
            return "ANDR(2, 3)";

        case 0x0194:
            return "ANDR(2, 4)";

        case 0x0195:
            return "ANDR(2, 5)";

        case 0x0196:
            return "ANDR(2, 6)";

        case 0x0197:
            return "ANDR(2, 7)";

        case 0x0198:
            return "ANDR(3, 0)";

        case 0x0199:
            return "ANDR(3, 1)";

        case 0x019A:
            return "ANDR(3, 2)";

        case 0x019B:
            return "ANDR(3, 3)";

        case 0x019C:
            return "ANDR(3, 4)";

        case 0x019D:
            return "ANDR(3, 5)";

        case 0x019E:
            return "ANDR(3, 6)";

        case 0x019F:
            return "ANDR(3, 7)";

        case 0x01A0:
            return "ANDR(4, 0)";

        case 0x01A1:
            return "ANDR(4, 1)";

        case 0x01A2:
            return "ANDR(4, 2)";

        case 0x01A3:
            return "ANDR(4, 3)";

        case 0x01A4:
            return "ANDR(4, 4)";

        case 0x01A5:
            return "ANDR(4, 5)";

        case 0x01A6:
            return "ANDR(4, 6)";

        case 0x01A7:
            return "ANDR(4, 7)";

        case 0x01A8:
            return "ANDR(5, 0)";

        case 0x01A9:
            return "ANDR(5, 1)";

        case 0x01AA:
            return "ANDR(5, 2)";

        case 0x01AB:
            return "ANDR(5, 3)";

        case 0x01AC:
            return "ANDR(5, 4)";

        case 0x01AD:
            return "ANDR(5, 5)";

        case 0x01AE:
            return "ANDR(5, 6)";

        case 0x01AF:
            return "ANDR(5, 7)";

        case 0x01B0:
            return "ANDR(6, 0)";

        case 0x01B1:
            return "ANDR(6, 1)";

        case 0x01B2:
            return "ANDR(6, 2)";

        case 0x01B3:
            return "ANDR(6, 3)";

        case 0x01B4:
            return "ANDR(6, 4)";

        case 0x01B5:
            return "ANDR(6, 5)";

        case 0x01B6:
            return "ANDR(6, 6)";

        case 0x01B7:
            return "ANDR(6, 7)";

        case 0x01B8:
            return "ANDR(7, 0)";

        case 0x01B9:
            return "ANDR(7, 1)";

        case 0x01BA:
            return "ANDR(7, 2)";

        case 0x01BB:
            return "ANDR(7, 3)";

        case 0x01BC:
            return "ANDR(7, 4)";

        case 0x01BD:
            return "ANDR(7, 5)";

        case 0x01BE:
            return "ANDR(7, 6)";

        case 0x01BF:
            return "ANDR(7, 7)";

        case 0x01C0:
            return "XORR(0, 0)";

        case 0x01C1:
            return "XORR(0, 1)";

        case 0x01C2:
            return "XORR(0, 2)";

        case 0x01C3:
            return "XORR(0, 3)";

        case 0x01C4:
            return "XORR(0, 4)";

        case 0x01C5:
            return "XORR(0, 5)";

        case 0x01C6:
            return "XORR(0, 6)";

        case 0x01C7:
            return "XORR(0, 7)";

        case 0x01C8:
            return "XORR(1, 0)";

        case 0x01C9:
            return "XORR(1, 1)";

        case 0x01CA:
            return "XORR(1, 2)";

        case 0x01CB:
            return "XORR(1, 3)";

        case 0x01CC:
            return "XORR(1, 4)";

        case 0x01CD:
            return "XORR(1, 5)";

        case 0x01CE:
            return "XORR(1, 6)";

        case 0x01CF:
            return "XORR(1, 7)";

        case 0x01D0:
            return "XORR(2, 0)";

        case 0x01D1:
            return "XORR(2, 1)";

        case 0x01D2:
            return "XORR(2, 2)";

        case 0x01D3:
            return "XORR(2, 3)";

        case 0x01D4:
            return "XORR(2, 4)";

        case 0x01D5:
            return "XORR(2, 5)";

        case 0x01D6:
            return "XORR(2, 6)";

        case 0x01D7:
            return "XORR(2, 7)";

        case 0x01D8:
            return "XORR(3, 0)";

        case 0x01D9:
            return "XORR(3, 1)";

        case 0x01DA:
            return "XORR(3, 2)";

        case 0x01DB:
            return "XORR(3, 3)";

        case 0x01DC:
            return "XORR(3, 4)";

        case 0x01DD:
            return "XORR(3, 5)";

        case 0x01DE:
            return "XORR(3, 6)";

        case 0x01DF:
            return "XORR(3, 7)";

        case 0x01E0:
            return "XORR(4, 0)";

        case 0x01E1:
            return "XORR(4, 1)";

        case 0x01E2:
            return "XORR(4, 2)";

        case 0x01E3:
            return "XORR(4, 3)";

        case 0x01E4:
            return "XORR(4, 4)";

        case 0x01E5:
            return "XORR(4, 5)";

        case 0x01E6:
            return "XORR(4, 6)";

        case 0x01E7:
            return "XORR(4, 7)";

        case 0x01E8:
            return "XORR(5, 0)";

        case 0x01E9:
            return "XORR(5, 1)";

        case 0x01EA:
            return "XORR(5, 2)";

        case 0x01EB:
            return "XORR(5, 3)";

        case 0x01EC:
            return "XORR(5, 4)";

        case 0x01ED:
            return "XORR(5, 5)";

        case 0x01EE:
            return "XORR(5, 6)";

        case 0x01EF:
            return "XORR(5, 7)";

        case 0x01F0:
            return "XORR(6, 0)";

        case 0x01F1:
            return "XORR(6, 1)";

        case 0x01F2:
            return "XORR(6, 2)";

        case 0x01F3:
            return "XORR(6, 3)";

        case 0x01F4:
            return "XORR(6, 4)";

        case 0x01F5:
            return "XORR(6, 5)";

        case 0x01F6:
            return "XORR(6, 6)";

        case 0x01F7:
            return "XORR(6, 7)";

        case 0x01F8:
            return "XORR(7, 0)";

        case 0x01F9:
            return "XORR(7, 1)";

        case 0x01FA:
            return "XORR(7, 2)";

        case 0x01FB:
            return "XORR(7, 3)";

        case 0x01FC:
            return "XORR(7, 4)";

        case 0x01FD:
            return "XORR(7, 5)";

        case 0x01FE:
            return "XORR(7, 6)";

        case 0x01FF:
            return "XORR(7, 7)";

        case 0x0200:
            return "B(7)";

        case 0x0201:
            return "BC(7)";

        case 0x0202:
            return "BOV(7)";

        case 0x0203:
            return "BPL(7)";

        case 0x0204:
            return "BEQ(7)";

        case 0x0205:
            return "BLT(7)";

        case 0x0206:
            return "BLE(7)";

        case 0x0207:
            return "BUSC(7)";

        case 0x0208:
            return "NOPP(7)";

        case 0x0209:
            return "BNC(7)";

        case 0x020A:
            return "BNOV(7)";

        case 0x020B:
            return "BMI(7)";

        case 0x020C:
            return "BNEQ(7)";

        case 0x020D:
            return "BGE(7)";

        case 0x020E:
            return "BGT(7)";

        case 0x020F:
            return "BESC(7)";

        case 0x0210:
            return "BEXT(0, 7)";

        case 0x0211:
            return "BEXT(1, 7)";

        case 0x0212:
            return "BEXT(2, 7)";

        case 0x0213:
            return "BEXT(3, 7)";

        case 0x0214:
            return "BEXT(4, 7)";

        case 0x0215:
            return "BEXT(5, 7)";

        case 0x0216:
            return "BEXT(6, 7)";

        case 0x0217:
            return "BEXT(7, 7)";

        case 0x0218:
            return "BEXT(8, 7)";

        case 0x0219:
            return "BEXT(9, 7)";

        case 0x021A:
            return "BEXT(10,7)";

        case 0x021B:
            return "BEXT(11,7)";

        case 0x021C:
            return "BEXT(12,7)";

        case 0x021D:
            return "BEXT(13,7)";

        case 0x021E:
            return "BEXT(14,7)";

        case 0x021F:
            return "BEXT(15,7)";

        case 0x0220:
            return "B(-R7)";

        case 0x0221:
            return "BC(-R7)";

        case 0x0222:
            return "BOV(-R7)";

        case 0x0223:
            return "BPL(-R7)";

        case 0x0224:
            return "BEQ(-R7)";

        case 0x0225:
            return "BLT(-R7)";

        case 0x0226:
            return "BLE(-R7)";

        case 0x0227:
            return "BUSC(-R7)";

        case 0x0228:
            return "NOPP(-R7)";

        case 0x0229:
            return "BNC(-R7)";

        case 0x022A:
            return "BNOV(-R7)";

        case 0x022B:
            return "BMI(-R7)";

        case 0x022C:
            return "BNEQ(-R7)";

        case 0x022D:
            return "BGE(-R7)";

        case 0x022E:
            return "BGT(-R7)";

        case 0x022F:
            return "BESC(-R7)";

        case 0x0230:
            return "BEXT(0,-R7)";

        case 0x0231:
            return "BEXT(1,-R7)";

        case 0x0232:
            return "BEXT(2,-R7)";

        case 0x0233:
            return "BEXT(3,-R7)";

        case 0x0234:
            return "BEXT(4,-R7)";

        case 0x0235:
            return "BEXT(5,-R7)";

        case 0x0236:
            return "BEXT(6,-R7)";

        case 0x0237:
            return "BEXT(7,-R7)";

        case 0x0238:
            return "BEXT(8,-R7)";

        case 0x0239:
            return "BEXT(9,-R7)";

        case 0x023A:
            return "BEXT(10,-R7)";

        case 0x023B:
            return "BEXT(11,-R7)";

        case 0x023C:
            return "BEXT(12,-R7)";

        case 0x023D:
            return "BEXT(13,-R7)";

        case 0x023E:
            return "BEXT(14,-R7)";

        case 0x023F:
            return "BEXT(15,-R7)";

        case 0x0240:
            return "MVO(0, 7)";

        case 0x0241:
            return "MVO(1, 7)";

        case 0x0242:
            return "MVO(2, 7)";

        case 0x0243:
            return "MVO(3, 7)";

        case 0x0244:
            return "MVO(4, 7)";

        case 0x0245:
            return "MVO(5, 7)";

        case 0x0246:
            return "MVO(6, 7)";

        case 0x0247:
            return "MVO(7, 7)";

        case 0x0248:
            return "MVO_ind(1, 0)";

        case 0x0249:
            return "MVO_ind(1, 1)";

        case 0x024A:
            return "MVO_ind(1, 2)";

        case 0x024B:
            return "MVO_ind(1, 3)";

        case 0x024C:
            return "MVO_ind(1, 4)";

        case 0x024D:
            return "MVO_ind(1, 5)";

        case 0x024E:
            return "MVO_ind(1, 6)";

        case 0x024F:
            return "MVO_ind(1, 7)";

        case 0x0250:
            return "MVO_ind(2, 0)";

        case 0x0251:
            return "MVO_ind(2, 1)";

        case 0x0252:
            return "MVO_ind(2, 2)";

        case 0x0253:
            return "MVO_ind(2, 3)";

        case 0x0254:
            return "MVO_ind(2, 4)";

        case 0x0255:
            return "MVO_ind(2, 5)";

        case 0x0256:
            return "MVO_ind(2, 6)";

        case 0x0257:
            return "MVO_ind(2, 7)";

        case 0x0258:
            return "MVO_ind(3, 0)";

        case 0x0259:
            return "MVO_ind(3, 1)";

        case 0x025A:
            return "MVO_ind(3, 2)";

        case 0x025B:
            return "MVO_ind(3, 3)";

        case 0x025C:
            return "MVO_ind(3, 4)";

        case 0x025D:
            return "MVO_ind(3, 5)";

        case 0x025E:
            return "MVO_ind(3, 6)";

        case 0x025F:
            return "MVO_ind(3, 7)";

        case 0x0260:
            return "MVO_ind(4, 0)";

        case 0x0261:
            return "MVO_ind(4, 1)";

        case 0x0262:
            return "MVO_ind(4, 2)";

        case 0x0263:
            return "MVO_ind(4, 3)";

        case 0x0264:
            return "MVO_ind(4, 4)";

        case 0x0265:
            return "MVO_ind(4, 5)";

        case 0x0266:
            return "MVO_ind(4, 6)";

        case 0x0267:
            return "MVO_ind(4, 7)";

        case 0x0268:
            return "MVO_ind(5, 0)";

        case 0x0269:
            return "MVO_ind(5, 1)";

        case 0x026A:
            return "MVO_ind(5, 2)";

        case 0x026B:
            return "MVO_ind(5, 3)";

        case 0x026C:
            return "MVO_ind(5, 4)";

        case 0x026D:
            return "MVO_ind(5, 5)";

        case 0x026E:
            return "MVO_ind(5, 6)";

        case 0x026F:
            return "MVO_ind(5, 7)";

        case 0x0270:
            return "MVO_ind(6, 0)";

        case 0x0271:
            return "MVO_ind(6, 1)";

        case 0x0272:
            return "MVO_ind(6, 2)";

        case 0x0273:
            return "MVO_ind(6, 3)";

        case 0x0274:
            return "MVO_ind(6, 4)";

        case 0x0275:
            return "MVO_ind(6, 5)";

        case 0x0276:
            return "MVO_ind(6, 6)";

        case 0x0277:
            return "MVO_ind(6, 7)";

        case 0x0278:
            return "MVO_ind(7, 0)";

        case 0x0279:
            return "MVO_ind(7, 1)";

        case 0x027A:
            return "MVO_ind(7, 2)";

        case 0x027B:
            return "MVO_ind(7, 3)";

        case 0x027C:
            return "MVO_ind(7, 4)";

        case 0x027D:
            return "MVO_ind(7, 5)";

        case 0x027E:
            return "MVO_ind(7, 6)";

        case 0x027F:
            return "MVO_ind(7, 7)";

        case 0x0280:
            return "MVI(7, 0)";

        case 0x0281:
            return "MVI(7, 1)";

        case 0x0282:
            return "MVI(7, 2)";

        case 0x0283:
            return "MVI(7, 3)";

        case 0x0284:
            return "MVI(7, 4)";

        case 0x0285:
            return "MVI(7, 5)";

        case 0x0286:
            return "MVI(7, 6)";

        case 0x0287:
            return "MVI(7, 7)";

        case 0x0288:
            return "MVI_ind(1, 0)";

        case 0x0289:
            return "MVI_ind(1, 1)";

        case 0x028A:
            return "MVI_ind(1, 2)";

        case 0x028B:
            return "MVI_ind(1, 3)";

        case 0x028C:
            return "MVI_ind(1, 4)";

        case 0x028D:
            return "MVI_ind(1, 5)";

        case 0x028E:
            return "MVI_ind(1, 6)";

        case 0x028F:
            return "MVI_ind(1, 7)";

        case 0x0290:
            return "MVI_ind(2, 0)";

        case 0x0291:
            return "MVI_ind(2, 1)";

        case 0x0292:
            return "MVI_ind(2, 2)";

        case 0x0293:
            return "MVI_ind(2, 3)";

        case 0x0294:
            return "MVI_ind(2, 4)";

        case 0x0295:
            return "MVI_ind(2, 5)";

        case 0x0296:
            return "MVI_ind(2, 6)";

        case 0x0297:
            return "MVI_ind(2, 7)";

        case 0x0298:
            return "MVI_ind(3, 0)";

        case 0x0299:
            return "MVI_ind(3, 1)";

        case 0x029A:
            return "MVI_ind(3, 2)";

        case 0x029B:
            return "MVI_ind(3, 3)";

        case 0x029C:
            return "MVI_ind(3, 4)";

        case 0x029D:
            return "MVI_ind(3, 5)";

        case 0x029E:
            return "MVI_ind(3, 6)";

        case 0x029F:
            return "MVI_ind(3, 7)";

        case 0x02A0:
            return "MVI_ind(4, 0)";

        case 0x02A1:
            return "MVI_ind(4, 1)";

        case 0x02A2:
            return "MVI_ind(4, 2)";

        case 0x02A3:
            return "MVI_ind(4, 3)";

        case 0x02A4:
            return "MVI_ind(4, 4)";

        case 0x02A5:
            return "MVI_ind(4, 5)";

        case 0x02A6:
            return "MVI_ind(4, 6)";

        case 0x02A7:
            return "MVI_ind(4, 7)";

        case 0x02A8:
            return "MVI_ind(5, 0)";

        case 0x02A9:
            return "MVI_ind(5, 1)";

        case 0x02AA:
            return "MVI_ind(5, 2)";

        case 0x02AB:
            return "MVI_ind(5, 3)";

        case 0x02AC:
            return "MVI_ind(5, 4)";

        case 0x02AD:
            return "MVI_ind(5, 5)";

        case 0x02AE:
            return "MVI_ind(5, 6)";

        case 0x02AF:
            return "MVI_ind(5, 7)";

        case 0x02B0:
            return "MVI_ind(6, 0)";

        case 0x02B1:
            return "MVI_ind(6, 1)";

        case 0x02B2:
            return "MVI_ind(6, 2)";

        case 0x02B3:
            return "MVI_ind(6, 3)";

        case 0x02B4:
            return "MVI_ind(6, 4)";

        case 0x02B5:
            return "MVI_ind(6, 5)";

        case 0x02B6:
            return "MVI_ind(6, 6)";

        case 0x02B7:
            return "MVI_ind(6, 7)";

        case 0x02B8:
            return "MVI_ind(7, 0)";

        case 0x02B9:
            return "MVI_ind(7, 1)";

        case 0x02BA:
            return "MVI_ind(7, 2)";

        case 0x02BB:
            return "MVI_ind(7, 3)";

        case 0x02BC:
            return "MVI_ind(7, 4)";

        case 0x02BD:
            return "MVI_ind(7, 5)";

        case 0x02BE:
            return "MVI_ind(7, 6)";

        case 0x02BF:
            return "MVI_ind(7, 7)";

        case 0x02C0:
            return "ADD(7, 0)";

        case 0x02C1:
            return "ADD(7, 1)";

        case 0x02C2:
            return "ADD(7, 2)";

        case 0x02C3:
            return "ADD(7, 3)";

        case 0x02C4:
            return "ADD(7, 4)";

        case 0x02C5:
            return "ADD(7, 5)";

        case 0x02C6:
            return "ADD(7, 6)";

        case 0x02C7:
            return "ADD(7, 7)";

        case 0x02C8:
            return "ADD_ind(1, 0)";

        case 0x02C9:
            return "ADD_ind(1, 1)";

        case 0x02CA:
            return "ADD_ind(1, 2)";

        case 0x02CB:
            return "ADD_ind(1, 3)";

        case 0x02CC:
            return "ADD_ind(1, 4)";

        case 0x02CD:
            return "ADD_ind(1, 5)";

        case 0x02CE:
            return "ADD_ind(1, 6)";

        case 0x02CF:
            return "ADD_ind(1, 7)";

        case 0x02D0:
            return "ADD_ind(2, 0)";

        case 0x02D1:
            return "ADD_ind(2, 1)";

        case 0x02D2:
            return "ADD_ind(2, 2)";

        case 0x02D3:
            return "ADD_ind(2, 3)";

        case 0x02D4:
            return "ADD_ind(2, 4)";

        case 0x02D5:
            return "ADD_ind(2, 5)";

        case 0x02D6:
            return "ADD_ind(2, 6)";

        case 0x02D7:
            return "ADD_ind(2, 7)";

        case 0x02D8:
            return "ADD_ind(3, 0)";

        case 0x02D9:
            return "ADD_ind(3, 1)";

        case 0x02DA:
            return "ADD_ind(3, 2)";

        case 0x02DB:
            return "ADD_ind(3, 3)";

        case 0x02DC:
            return "ADD_ind(3, 4)";

        case 0x02DD:
            return "ADD_ind(3, 5)";

        case 0x02DE:
            return "ADD_ind(3, 6)";

        case 0x02DF:
            return "ADD_ind(3, 7)";

        case 0x02E0:
            return "ADD_ind(4, 0)";

        case 0x02E1:
            return "ADD_ind(4, 1)";

        case 0x02E2:
            return "ADD_ind(4, 2)";

        case 0x02E3:
            return "ADD_ind(4, 3)";

        case 0x02E4:
            return "ADD_ind(4, 4)";

        case 0x02E5:
            return "ADD_ind(4, 5)";

        case 0x02E6:
            return "ADD_ind(4, 6)";

        case 0x02E7:
            return "ADD_ind(4, 7)";

        case 0x02E8:
            return "ADD_ind(5, 0)";

        case 0x02E9:
            return "ADD_ind(5, 1)";

        case 0x02EA:
            return "ADD_ind(5, 2)";

        case 0x02EB:
            return "ADD_ind(5, 3)";

        case 0x02EC:
            return "ADD_ind(5, 4)";

        case 0x02ED:
            return "ADD_ind(5, 5)";

        case 0x02EE:
            return "ADD_ind(5, 6)";

        case 0x02EF:
            return "ADD_ind(5, 7)";

        case 0x02F0:
            return "ADD_ind(6, 0)";

        case 0x02F1:
            return "ADD_ind(6, 1)";

        case 0x02F2:
            return "ADD_ind(6, 2)";

        case 0x02F3:
            return "ADD_ind(6, 3)";

        case 0x02F4:
            return "ADD_ind(6, 4)";

        case 0x02F5:
            return "ADD_ind(6, 5)";

        case 0x02F6:
            return "ADD_ind(6, 6)";

        case 0x02F7:
            return "ADD_ind(6, 7)";

        case 0x02F8:
            return "ADD_ind(7, 0)";

        case 0x02F9:
            return "ADD_ind(7, 1)";

        case 0x02FA:
            return "ADD_ind(7, 2)";

        case 0x02FB:
            return "ADD_ind(7, 3)";

        case 0x02FC:
            return "ADD_ind(7, 4)";

        case 0x02FD:
            return "ADD_ind(7, 5)";

        case 0x02FE:
            return "ADD_ind(7, 6)";

        case 0x02FF:
            return "ADD_ind(7, 7)";

        case 0x0300:
            return "SUB(7, 0)";

        case 0x0301:
            return "SUB(7, 1)";

        case 0x0302:
            return "SUB(7, 2)";

        case 0x0303:
            return "SUB(7, 3)";

        case 0x0304:
            return "SUB(7, 4)";

        case 0x0305:
            return "SUB(7, 5)";

        case 0x0306:
            return "SUB(7, 6)";

        case 0x0307:
            return "SUB(7, 7)";

        case 0x0308:
            return "SUB_ind(1, 0)";

        case 0x0309:
            return "SUB_ind(1, 1)";

        case 0x030A:
            return "SUB_ind(1, 2)";

        case 0x030B:
            return "SUB_ind(1, 3)";

        case 0x030C:
            return "SUB_ind(1, 4)";

        case 0x030D:
            return "SUB_ind(1, 5)";

        case 0x030E:
            return "SUB_ind(1, 6)";

        case 0x030F:
            return "SUB_ind(1, 7)";

        case 0x0310:
            return "SUB_ind(2, 0)";

        case 0x0311:
            return "SUB_ind(2, 1)";

        case 0x0312:
            return "SUB_ind(2, 2)";

        case 0x0313:
            return "SUB_ind(2, 3)";

        case 0x0314:
            return "SUB_ind(2, 4)";

        case 0x0315:
            return "SUB_ind(2, 5)";

        case 0x0316:
            return "SUB_ind(2, 6)";

        case 0x0317:
            return "SUB_ind(2, 7)";

        case 0x0318:
            return "SUB_ind(3, 0)";

        case 0x0319:
            return "SUB_ind(3, 1)";

        case 0x031A:
            return "SUB_ind(3, 2)";

        case 0x031B:
            return "SUB_ind(3, 3)";

        case 0x031C:
            return "SUB_ind(3, 4)";

        case 0x031D:
            return "SUB_ind(3, 5)";

        case 0x031E:
            return "SUB_ind(3, 6)";

        case 0x031F:
            return "SUB_ind(3, 7)";

        case 0x0320:
            return "SUB_ind(4, 0)";

        case 0x0321:
            return "SUB_ind(4, 1)";

        case 0x0322:
            return "SUB_ind(4, 2)";

        case 0x0323:
            return "SUB_ind(4, 3)";

        case 0x0324:
            return "SUB_ind(4, 4)";

        case 0x0325:
            return "SUB_ind(4, 5)";

        case 0x0326:
            return "SUB_ind(4, 6)";

        case 0x0327:
            return "SUB_ind(4, 7)";

        case 0x0328:
            return "SUB_ind(5, 0)";

        case 0x0329:
            return "SUB_ind(5, 1)";

        case 0x032A:
            return "SUB_ind(5, 2)";

        case 0x032B:
            return "SUB_ind(5, 3)";

        case 0x032C:
            return "SUB_ind(5, 4)";

        case 0x032D:
            return "SUB_ind(5, 5)";

        case 0x032E:
            return "SUB_ind(5, 6)";

        case 0x032F:
            return "SUB_ind(5, 7)";

        case 0x0330:
            return "SUB_ind(6, 0)";

        case 0x0331:
            return "SUB_ind(6, 1)";

        case 0x0332:
            return "SUB_ind(6, 2)";

        case 0x0333:
            return "SUB_ind(6, 3)";

        case 0x0334:
            return "SUB_ind(6, 4)";

        case 0x0335:
            return "SUB_ind(6, 5)";

        case 0x0336:
            return "SUB_ind(6, 6)";

        case 0x0337:
            return "SUB_ind(6, 7)";

        case 0x0338:
            return "SUB_ind(7, 0)";

        case 0x0339:
            return "SUB_ind(7, 1)";

        case 0x033A:
            return "SUB_ind(7, 2)";

        case 0x033B:
            return "SUB_ind(7, 3)";

        case 0x033C:
            return "SUB_ind(7, 4)";

        case 0x033D:
            return "SUB_ind(7, 5)";

        case 0x033E:
            return "SUB_ind(7, 6)";

        case 0x033F:
            return "SUB_ind(7, 7)";

        case 0x0340:
            return "CMP(7, 0)";

        case 0x0341:
            return "CMP(7, 1)";

        case 0x0342:
            return "CMP(7, 2)";

        case 0x0343:
            return "CMP(7, 3)";

        case 0x0344:
            return "CMP(7, 4)";

        case 0x0345:
            return "CMP(7, 5)";

        case 0x0346:
            return "CMP(7, 6)";

        case 0x0347:
            return "CMP(7, 7)";

        case 0x0348:
            return "CMP_ind(1, 0)";

        case 0x0349:
            return "CMP_ind(1, 1)";

        case 0x034A:
            return "CMP_ind(1, 2)";

        case 0x034B:
            return "CMP_ind(1, 3)";

        case 0x034C:
            return "CMP_ind(1, 4)";

        case 0x034D:
            return "CMP_ind(1, 5)";

        case 0x034E:
            return "CMP_ind(1, 6)";

        case 0x034F:
            return "CMP_ind(1, 7)";

        case 0x0350:
            return "CMP_ind(2, 0)";

        case 0x0351:
            return "CMP_ind(2, 1)";

        case 0x0352:
            return "CMP_ind(2, 2)";

        case 0x0353:
            return "CMP_ind(2, 3)";

        case 0x0354:
            return "CMP_ind(2, 4)";

        case 0x0355:
            return "CMP_ind(2, 5)";

        case 0x0356:
            return "CMP_ind(2, 6)";

        case 0x0357:
            return "CMP_ind(2, 7)";

        case 0x0358:
            return "CMP_ind(3, 0)";

        case 0x0359:
            return "CMP_ind(3, 1)";

        case 0x035A:
            return "CMP_ind(3, 2)";

        case 0x035B:
            return "CMP_ind(3, 3)";

        case 0x035C:
            return "CMP_ind(3, 4)";

        case 0x035D:
            return "CMP_ind(3, 5)";

        case 0x035E:
            return "CMP_ind(3, 6)";

        case 0x035F:
            return "CMP_ind(3, 7)";

        case 0x0360:
            return "CMP_ind(4, 0)";

        case 0x0361:
            return "CMP_ind(4, 1)";

        case 0x0362:
            return "CMP_ind(4, 2)";

        case 0x0363:
            return "CMP_ind(4, 3)";

        case 0x0364:
            return "CMP_ind(4, 4)";

        case 0x0365:
            return "CMP_ind(4, 5)";

        case 0x0366:
            return "CMP_ind(4, 6)";

        case 0x0367:
            return "CMP_ind(4, 7)";

        case 0x0368:
            return "CMP_ind(5, 0)";

        case 0x0369:
            return "CMP_ind(5, 1)";

        case 0x036A:
            return "CMP_ind(5, 2)";

        case 0x036B:
            return "CMP_ind(5, 3)";

        case 0x036C:
            return "CMP_ind(5, 4)";

        case 0x036D:
            return "CMP_ind(5, 5)";

        case 0x036E:
            return "CMP_ind(5, 6)";

        case 0x036F:
            return "CMP_ind(5, 7)";

        case 0x0370:
            return "CMP_ind(6, 0)";

        case 0x0371:
            return "CMP_ind(6, 1)";

        case 0x0372:
            return "CMP_ind(6, 2)";

        case 0x0373:
            return "CMP_ind(6, 3)";

        case 0x0374:
            return "CMP_ind(6, 4)";

        case 0x0375:
            return "CMP_ind(6, 5)";

        case 0x0376:
            return "CMP_ind(6, 6)";

        case 0x0377:
            return "CMP_ind(6, 7)";

        case 0x0378:
            return "CMP_ind(7, 0)";

        case 0x0379:
            return "CMP_ind(7, 1)";

        case 0x037A:
            return "CMP_ind(7, 2)";

        case 0x037B:
            return "CMP_ind(7, 3)";

        case 0x037C:
            return "CMP_ind(7, 4)";

        case 0x037D:
            return "CMP_ind(7, 5)";

        case 0x037E:
            return "CMP_ind(7, 6)";

        case 0x037F:
            return "CMP_ind(7, 7)";

        case 0x0380:
            return "AND(7, 0)";

        case 0x0381:
            return "AND(7, 1)";

        case 0x0382:
            return "AND(7, 2)";

        case 0x0383:
            return "AND(7, 3)";

        case 0x0384:
            return "AND(7, 4)";

        case 0x0385:
            return "AND(7, 5)";

        case 0x0386:
            return "AND(7, 6)";

        case 0x0387:
            return "AND(7, 7)";

        case 0x0388:
            return "AND_ind(1, 0)";

        case 0x0389:
            return "AND_ind(1, 1)";

        case 0x038A:
            return "AND_ind(1, 2)";

        case 0x038B:
            return "AND_ind(1, 3)";

        case 0x038C:
            return "AND_ind(1, 4)";

        case 0x038D:
            return "AND_ind(1, 5)";

        case 0x038E:
            return "AND_ind(1, 6)";

        case 0x038F:
            return "AND_ind(1, 7)";

        case 0x0390:
            return "AND_ind(2, 0)";

        case 0x0391:
            return "AND_ind(2, 1)";

        case 0x0392:
            return "AND_ind(2, 2)";

        case 0x0393:
            return "AND_ind(2, 3)";

        case 0x0394:
            return "AND_ind(2, 4)";

        case 0x0395:
            return "AND_ind(2, 5)";

        case 0x0396:
            return "AND_ind(2, 6)";

        case 0x0397:
            return "AND_ind(2, 7)";

        case 0x0398:
            return "AND_ind(3, 0)";

        case 0x0399:
            return "AND_ind(3, 1)";

        case 0x039A:
            return "AND_ind(3, 2)";

        case 0x039B:
            return "AND_ind(3, 3)";

        case 0x039C:
            return "AND_ind(3, 4)";

        case 0x039D:
            return "AND_ind(3, 5)";

        case 0x039E:
            return "AND_ind(3, 6)";

        case 0x039F:
            return "AND_ind(3, 7)";

        case 0x03A0:
            return "AND_ind(4, 0)";

        case 0x03A1:
            return "AND_ind(4, 1)";

        case 0x03A2:
            return "AND_ind(4, 2)";

        case 0x03A3:
            return "AND_ind(4, 3)";

        case 0x03A4:
            return "AND_ind(4, 4)";

        case 0x03A5:
            return "AND_ind(4, 5)";

        case 0x03A6:
            return "AND_ind(4, 6)";

        case 0x03A7:
            return "AND_ind(4, 7)";

        case 0x03A8:
            return "AND_ind(5, 0)";

        case 0x03A9:
            return "AND_ind(5, 1)";

        case 0x03AA:
            return "AND_ind(5, 2)";

        case 0x03AB:
            return "AND_ind(5, 3)";

        case 0x03AC:
            return "AND_ind(5, 4)";

        case 0x03AD:
            return "AND_ind(5, 5)";

        case 0x03AE:
            return "AND_ind(5, 6)";

        case 0x03AF:
            return "AND_ind(5, 7)";

        case 0x03B0:
            return "AND_ind(6, 0)";

        case 0x03B1:
            return "AND_ind(6, 1)";

        case 0x03B2:
            return "AND_ind(6, 2)";

        case 0x03B3:
            return "AND_ind(6, 3)";

        case 0x03B4:
            return "AND_ind(6, 4)";

        case 0x03B5:
            return "AND_ind(6, 5)";

        case 0x03B6:
            return "AND_ind(6, 6)";

        case 0x03B7:
            return "AND_ind(6, 7)";

        case 0x03B8:
            return "AND_ind(7, 0)";

        case 0x03B9:
            return "AND_ind(7, 1)";

        case 0x03BA:
            return "AND_ind(7, 2)";

        case 0x03BB:
            return "AND_ind(7, 3)";

        case 0x03BC:
            return "AND_ind(7, 4)";

        case 0x03BD:
            return "AND_ind(7, 5)";

        case 0x03BE:
            return "AND_ind(7, 6)";

        case 0x03BF:
            return "AND_ind(7, 7)";

        case 0x03C0:
            return "XOR(7, 0)";

        case 0x03C1:
            return "XOR(7, 1)";

        case 0x03C2:
            return "XOR(7, 2)";

        case 0x03C3:
            return "XOR(7, 3)";

        case 0x03C4:
            return "XOR(7, 4)";

        case 0x03C5:
            return "XOR(7, 5)";

        case 0x03C6:
            return "XOR(7, 6)";

        case 0x03C7:
            return "XOR(7, 7)";

        case 0x03C8:
            return "XOR_ind(1, 0)";

        case 0x03C9:
            return "XOR_ind(1, 1)";

        case 0x03CA:
            return "XOR_ind(1, 2)";

        case 0x03CB:
            return "XOR_ind(1, 3)";

        case 0x03CC:
            return "XOR_ind(1, 4)";

        case 0x03CD:
            return "XOR_ind(1, 5)";

        case 0x03CE:
            return "XOR_ind(1, 6)";

        case 0x03CF:
            return "XOR_ind(1, 7)";

        case 0x03D0:
            return "XOR_ind(2, 0)";

        case 0x03D1:
            return "XOR_ind(2, 1)";

        case 0x03D2:
            return "XOR_ind(2, 2)";

        case 0x03D3:
            return "XOR_ind(2, 3)";

        case 0x03D4:
            return "XOR_ind(2, 4)";

        case 0x03D5:
            return "XOR_ind(2, 5)";

        case 0x03D6:
            return "XOR_ind(2, 6)";

        case 0x03D7:
            return "XOR_ind(2, 7)";

        case 0x03D8:
            return "XOR_ind(3, 0)";

        case 0x03D9:
            return "XOR_ind(3, 1)";

        case 0x03DA:
            return "XOR_ind(3, 2)";

        case 0x03DB:
            return "XOR_ind(3, 3)";

        case 0x03DC:
            return "XOR_ind(3, 4)";

        case 0x03DD:
            return "XOR_ind(3, 5)";

        case 0x03DE:
            return "XOR_ind(3, 6)";

        case 0x03DF:
            return "XOR_ind(3, 7)";

        case 0x03E0:
            return "XOR_ind(4, 0)";

        case 0x03E1:
            return "XOR_ind(4, 1)";

        case 0x03E2:
            return "XOR_ind(4, 2)";

        case 0x03E3:
            return "XOR_ind(4, 3)";

        case 0x03E4:
            return "XOR_ind(4, 4)";

        case 0x03E5:
            return "XOR_ind(4, 5)";

        case 0x03E6:
            return "XOR_ind(4, 6)";

        case 0x03E7:
            return "XOR_ind(4, 7)";

        case 0x03E8:
            return "XOR_ind(5, 0)";

        case 0x03E9:
            return "XOR_ind(5, 1)";

        case 0x03EA:
            return "XOR_ind(5, 2)";

        case 0x03EB:
            return "XOR_ind(5, 3)";

        case 0x03EC:
            return "XOR_ind(5, 4)";

        case 0x03ED:
            return "XOR_ind(5, 5)";

        case 0x03EE:
            return "XOR_ind(5, 6)";

        case 0x03EF:
            return "XOR_ind(5, 7)";

        case 0x03F0:
            return "XOR_ind(6, 0)";

        case 0x03F1:
            return "XOR_ind(6, 1)";

        case 0x03F2:
            return "XOR_ind(6, 2)";

        case 0x03F3:
            return "XOR_ind(6, 3)";

        case 0x03F4:
            return "XOR_ind(6, 4)";

        case 0x03F5:
            return "XOR_ind(6, 5)";

        case 0x03F6:
            return "XOR_ind(6, 6)";

        case 0x03F7:
            return "XOR_ind(6, 7)";

        case 0x03F8:
            return "XOR_ind(7, 0)";

        case 0x03F9:
            return "XOR_ind(7, 1)";

        case 0x03FA:
            return "XOR_ind(7, 2)";

        case 0x03FB:
            return "XOR_ind(7, 3)";

        case 0x03FC:
            return "XOR_ind(7, 4)";

        case 0x03FD:
            return "XOR_ind(7, 5)";

        case 0x03FE:
            return "XOR_ind(7, 6)";

        case 0x03FF:
            return "XOR_ind(7, 7)";
    }
    
    return "UNKNOWN";
}

void show_debug_overlay(void)
{
    // Debugger always uses a special overlay...
    memcpy(&myOverlay, &debuggerOverlay, sizeof(myOverlay));
    
    decompress(bgDebugTiles, bgGetGfxPtr(bg0b), LZ77Vram);
    decompress(bgDebugMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
    dmaCopy((void *) bgDebugPal,(u16*) BG_PALETTE_SUB,256*2);
    unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
    dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

    REG_BLDCNT=0; REG_BLDCNT_SUB=0; REG_BLDY=0; REG_BLDY_SUB=0;
    
    swiWaitForVBlank();    
}


void debug_clear_scr(void)
{
    for (int i=0; i<20; i++)
    {
        dsPrintValue(0, i, 0, (char*)"                                ");
    }
}

void debugger_wait_for_input(void)
{
    bool bDone = false;
    touchPosition touch;
    
    while (!bDone)
    {
        while ((keysCurrent() & KEY_TOUCH) == 0)
        {
            asm("nop");
        }
        touchRead(&touch);
        UINT8 pressed = debugger_input(touch.px, touch.py);
        
        if ((pressed != DBG_PRESS_NONE) && (pressed != DBG_PRESS_STOP))
        {
            bDone=true;
        }
        else
        {
            display_debug();
        }
        WAITVBL;WAITVBL;WAITVBL;        
    }
}

UINT8 debugger_input(int tx, int ty)
{
    UINT8 pressed = DBG_PRESS_NONE;
    
    if ((tx > 5)   && (tx < 40)   && (ty > 160)) {pressed = DBG_PRESS_STOP;  debug_mode = DBG_MODE_STOP;}
    if ((tx > 40)  && (tx < 75)   && (ty > 160)) {pressed = DBG_PRESS_PLAY;  debug_mode = DBG_MODE_PLAY;}
    if ((tx > 75)  && (tx < 110)  && (ty > 160)) {pressed = DBG_PRESS_FRAME; debug_mode = DBG_MODE_FRAME;}
    if ((tx > 110) && (tx < 145)  && (ty > 160)) {pressed = DBG_PRESS_STEP;  debug_mode = DBG_MODE_STEP;}

    if ((tx > 163) && (tx < 198)  && (ty > 160) && (ty < 180)) 
    {
        debug_clear_scr(); 
        debug_show = DBG_SHOW_CPU;
    }
    if ((tx > 198) && (tx < 236)  && (ty > 160) && (ty < 180)) 
    {
        debug_clear_scr(); 
        if (debug_show == DBG_SHOW_RAM) debug_show_ram = (debug_show_ram+1) % 3;
        else debug_show_ram = 0;
        debug_show = DBG_SHOW_RAM;
    }
    if ((tx > 163) && (tx < 198)  && (ty > 180)) 
    {
        debug_clear_scr(); 
        if (debug_show == DBG_SHOW_PSG) debug_show_psg = (debug_show_psg+1) % 2;
        else debug_show_psg = 0;
        debug_show = DBG_SHOW_PSG;
    }
    if ((tx > 198) && (tx < 236)  && (ty > 180)) 
    {
        debug_clear_scr(); 
        if (debug_show == DBG_SHOW_STIC) debug_show_stic = (debug_show_stic+1) % 8;
        else debug_show_stic = 0;
        debug_show = DBG_SHOW_STIC;
    }
    return pressed;
}

void display_debug(void)
{
    char dbg[40];
    UINT8 idx=0;
    
    if (debug_show == DBG_SHOW_CPU)
    {
        for (int i=0; i<8; i++)
        {
            sprintf(dbg, "R%d: %-5d %04X", i, r[i], r[i]);
            dsPrintValue(0, idx++, 0, dbg);
        }
        idx++;
        sprintf(dbg, " S: %02X", S);    dsPrintValue(0, idx++, 0, dbg);
        sprintf(dbg, " Z: %02X", Z);    dsPrintValue(0, idx++, 0, dbg);
        sprintf(dbg, " O: %02X", O);    dsPrintValue(0, idx++, 0, dbg);
        sprintf(dbg, " C: %02X", C);    dsPrintValue(0, idx++, 0, dbg);
        sprintf(dbg, " I: %02X", I);    dsPrintValue(0, idx++, 0, dbg);
        sprintf(dbg, " D: %02X", D);    dsPrintValue(0, idx++, 0, dbg);
        idx++;
        sprintf(dbg, "OP: %03X [%-15s]", op, dbg_opcode(op));   dsPrintValue(0, idx++, 0, dbg);
        idx++;
        sprintf(dbg, "Total Frames: %u", debug_frames);   dsPrintValue(0, idx++, 0, dbg);
        sprintf(dbg, "Total OpCode: %u", debug_opcodes);   dsPrintValue(0, idx++, 0, dbg);

        idx=0;
        for (UINT16 addr = r[7]-3; addr <= r[7]+4; addr++)
        {
            sprintf(dbg, "A=%04X : D=%04X", addr, PEEK_FAST(addr));
            dsPrintValue(16, idx++, (addr == r[7] ? 1:0), dbg);
        }
        idx++;
        for (int i=0; i<6; i++)
        {
            sprintf(dbg, "D%d=%-09d %08X", i, debug[i], debug[i]);
            dsPrintValue(10, idx++, 0, dbg);
        }
        
    }
    if (debug_show == DBG_SHOW_RAM)
    {
        idx=0;
        switch (debug_show_ram)
        {
            case 0:
                for (UINT16 addr = 0x100; addr <= 0x190; addr += 8)
                {
                    sprintf(dbg, "%03X : %02X %02X %02X %02X %02X %02X %02X %02X", addr, PEEK_FAST(addr)&0xFF, PEEK_FAST(addr+1)&0xFF, PEEK_FAST(addr+2)&0xFF, PEEK_FAST(addr+3)&0xFF, PEEK_FAST(addr+4)&0xFF, PEEK_FAST(addr+5)&0xFF, PEEK_FAST(addr+6)&0xFF, PEEK_FAST(addr+7)&0xFF);
                    dsPrintValue(0, idx++, 0, dbg);
                }
                break;
            case 1:
                for (UINT16 addr = 0x190; addr <= 0x1EF; addr += 8)
                {
                    sprintf(dbg, "%03X : %02X %02X %02X %02X %02X %02X %02X %02X", addr, PEEK_FAST(addr)&0xFF, PEEK_FAST(addr+1)&0xFF, PEEK_FAST(addr+2)&0xFF, PEEK_FAST(addr+3)&0xFF, PEEK_FAST(addr+4)&0xFF, PEEK_FAST(addr+5)&0xFF, PEEK_FAST(addr+6)&0xFF, PEEK_FAST(addr+7)&0xFF);
                    dsPrintValue(0, idx++, 0, dbg);
                }
                for (UINT16 addr = 0x2F0; addr <= 0x30F; addr += 4)
                {
                    sprintf(dbg, "%03X : %04X %04X %04X %04X", addr, PEEK_FAST(addr), PEEK_FAST(addr+1), PEEK_FAST(addr+2), PEEK_FAST(addr+3));
                    dsPrintValue(0, idx++, 0, dbg);
                }
                break;
            case 2:
                for (UINT16 addr = 0x310; addr <= 0x35F; addr += 4)
                {
                    sprintf(dbg, "%03X : %04X %04X %04X %04X", addr, PEEK_FAST(addr), PEEK_FAST(addr+1), PEEK_FAST(addr+2), PEEK_FAST(addr+3));
                    dsPrintValue(0, idx++, 0, dbg);
                }
                break;
        }
    }
    
    if (debug_show == DBG_SHOW_PSG)
    {
        idx=0;
        extern struct Channel_t channel0;
        extern struct Channel_t channel1;
        extern struct Channel_t channel2;
        
        switch (debug_show_psg)
        {
            case 0:
                sprintf(dbg, "          CHAN1  CHAN2  CHAN3");
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "Period:   %-5d   %-5d   %-5d", channel0.period, channel1.period, channel2.period);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "PerVal:   %-5d   %-5d   %-5d", channel0.periodValue, channel1.periodValue, channel2.periodValue);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "Volume:   %-5d   %-5d   %-5d", channel0.volume, channel1.volume, channel2.volume);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "ToneCt:   %-5d   %-5d   %-5d", channel0.toneCounter, channel1.toneCounter, channel2.toneCounter);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "Tone:     %-5d   %-5d   %-5d", channel0.tone, channel1.tone, channel2.tone);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "Envlop:   %-5d   %-5d   %-5d", channel0.envelope, channel1.envelope, channel2.envelope);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "TonDis:   %-5d   %-5d   %-5d", channel0.toneDisabled, channel1.toneDisabled, channel2.toneDisabled);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "NoiDis:   %-5d   %-5d   %-5d", channel0.noiseDisabled, channel1.noiseDisabled, channel2.noiseDisabled);
                dsPrintValue(0, idx++, 0, dbg);
                idx++;
                sprintf(dbg, "clockDivisor:     %-9d", clockDivisor);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "clocksPerSample:  %-9d", clocksPerSample);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "envelopeIdle:     %-9d", envelopeIdle);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "envelopePeriod:   %-9d", envelopePeriod);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "envelopePValue:   %-9d", envelopePeriodValue);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "noiseIdle:        %-9d", noiseIdle);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "noisePeriod:      %-9d", noisePeriod);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "noisePeriodValue: %-9d", noisePeriodValue);
                dsPrintValue(0, idx++, 0, dbg);
                break;
                
            case 1:
                sprintf(dbg, "AUDIO MIXER BUFFER");
                dsPrintValue(0, idx++, 0, dbg); idx++;
                sprintf(dbg, "curSampIdx8   %-4d", currentSampleIdx8);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "curSampIdx16  %-4d", currentSampleIdx16);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "comClksPerTik %-9d", commonClocksPerTick);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "sampleBuf[0]  %-17lld", (long long)sampleBuffer[0]);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "sampleBuf[1]  %-17lld", (long long)sampleBuffer[1]);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "comClkCntr[0] %-17lld", (long long)commonClockCounter[0]);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "comClkCntr[1] %-17lld", (long long)commonClockCounter[1]);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "comClkPerS[0] %-17lld", (long long)commonClocksPerSample[0]);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "comClkPerS[1] %-17lld", (long long)commonClocksPerSample[1]);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "prevSample[]  %04X %04X", previousSample[0], previousSample[1]);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "curSample[]   %04X %04X", currentSample[0], currentSample[1]);
                dsPrintValue(0, idx++, 0, dbg);
                idx++;
                for (UINT16 i=0; i<30; i+=6)
                {
                    sprintf(dbg, "%04X %04X %04X %04X %04X %04X", audio_mixer_buffer[i], audio_mixer_buffer[i+1], audio_mixer_buffer[i+2], audio_mixer_buffer[i+3], audio_mixer_buffer[i+4], audio_mixer_buffer[i+5]);
                    dsPrintValue(0, idx++, 0, dbg);
                }
                break;
        }
    }    

    if (debug_show == DBG_SHOW_STIC)
    {
        idx=0;
        switch (debug_show_stic)
        {
            case 0:
                sprintf(dbg, "MOBS:");
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "XLOC:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].xLocation, debug_stic->mobs[1].xLocation, debug_stic->mobs[2].xLocation, debug_stic->mobs[3].xLocation, debug_stic->mobs[4].xLocation, debug_stic->mobs[5].xLocation, debug_stic->mobs[6].xLocation, debug_stic->mobs[7].xLocation);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "YLOC:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].yLocation, debug_stic->mobs[1].yLocation, debug_stic->mobs[2].yLocation, debug_stic->mobs[3].yLocation, debug_stic->mobs[4].yLocation, debug_stic->mobs[5].yLocation, debug_stic->mobs[6].yLocation, debug_stic->mobs[7].yLocation);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "CARD:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].cardNumber, debug_stic->mobs[1].cardNumber, debug_stic->mobs[2].cardNumber, debug_stic->mobs[3].cardNumber, debug_stic->mobs[4].cardNumber, debug_stic->mobs[5].cardNumber, debug_stic->mobs[6].cardNumber, debug_stic->mobs[7].cardNumber);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "FORE:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].foregroundColor, debug_stic->mobs[1].foregroundColor, debug_stic->mobs[2].foregroundColor, debug_stic->mobs[3].foregroundColor, debug_stic->mobs[4].foregroundColor, debug_stic->mobs[5].foregroundColor, debug_stic->mobs[6].foregroundColor, debug_stic->mobs[7].foregroundColor);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "GROM:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].isGrom, debug_stic->mobs[1].isGrom, debug_stic->mobs[2].isGrom, debug_stic->mobs[3].isGrom, debug_stic->mobs[4].isGrom, debug_stic->mobs[5].isGrom, debug_stic->mobs[6].isGrom, debug_stic->mobs[7].isGrom);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "VISB:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].isVisible, debug_stic->mobs[1].isVisible, debug_stic->mobs[2].isVisible, debug_stic->mobs[3].isVisible, debug_stic->mobs[4].isVisible, debug_stic->mobs[5].isVisible, debug_stic->mobs[6].isVisible, debug_stic->mobs[7].isVisible);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "DBLW:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].doubleWidth, debug_stic->mobs[1].doubleWidth, debug_stic->mobs[2].doubleWidth, debug_stic->mobs[3].doubleWidth, debug_stic->mobs[4].doubleWidth, debug_stic->mobs[5].doubleWidth, debug_stic->mobs[6].doubleWidth, debug_stic->mobs[7].doubleWidth);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "DBLY:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].doubleYResolution, debug_stic->mobs[1].doubleYResolution, debug_stic->mobs[2].doubleYResolution, debug_stic->mobs[3].doubleYResolution, debug_stic->mobs[4].doubleYResolution, debug_stic->mobs[5].doubleYResolution, debug_stic->mobs[6].doubleYResolution, debug_stic->mobs[7].doubleYResolution);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "DBLH:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].doubleHeight, debug_stic->mobs[1].doubleHeight, debug_stic->mobs[2].doubleHeight, debug_stic->mobs[3].doubleHeight, debug_stic->mobs[4].doubleHeight, debug_stic->mobs[5].doubleHeight, debug_stic->mobs[6].doubleHeight, debug_stic->mobs[7].doubleHeight);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "QUAH:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].quadHeight, debug_stic->mobs[1].quadHeight, debug_stic->mobs[2].quadHeight, debug_stic->mobs[3].quadHeight, debug_stic->mobs[4].quadHeight, debug_stic->mobs[5].quadHeight, debug_stic->mobs[6].quadHeight, debug_stic->mobs[7].quadHeight);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "FLAG:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].flagCollisions, debug_stic->mobs[1].flagCollisions, debug_stic->mobs[2].flagCollisions, debug_stic->mobs[3].flagCollisions, debug_stic->mobs[4].flagCollisions, debug_stic->mobs[5].flagCollisions, debug_stic->mobs[6].flagCollisions, debug_stic->mobs[7].flagCollisions);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "HMIR:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].horizontalMirror, debug_stic->mobs[1].horizontalMirror, debug_stic->mobs[2].horizontalMirror, debug_stic->mobs[3].horizontalMirror, debug_stic->mobs[4].horizontalMirror, debug_stic->mobs[5].horizontalMirror, debug_stic->mobs[6].horizontalMirror, debug_stic->mobs[7].horizontalMirror);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "VMIR:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].verticalMirror, debug_stic->mobs[1].verticalMirror, debug_stic->mobs[2].verticalMirror, debug_stic->mobs[3].verticalMirror, debug_stic->mobs[4].verticalMirror, debug_stic->mobs[5].verticalMirror, debug_stic->mobs[6].verticalMirror, debug_stic->mobs[7].verticalMirror);
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "BEHI:  %02X %02X %02X %02X %02X %02X %02X %02X", debug_stic->mobs[0].behindForeground, debug_stic->mobs[1].behindForeground, debug_stic->mobs[2].behindForeground, debug_stic->mobs[3].behindForeground, debug_stic->mobs[4].behindForeground, debug_stic->mobs[5].behindForeground, debug_stic->mobs[6].behindForeground, debug_stic->mobs[7].behindForeground);
                dsPrintValue(0, idx++, 0, dbg);
                idx++;
                sprintf(dbg, "COLLISIONS:");
                dsPrintValue(0, idx++, 0, dbg);
                sprintf(dbg, "%03X %03X %03X %03X %03X %03X %03X %03X", debug_stic->mobs[0].collisionRegister, debug_stic->mobs[1].collisionRegister, debug_stic->mobs[2].collisionRegister, debug_stic->mobs[3].collisionRegister, debug_stic->mobs[4].collisionRegister, debug_stic->mobs[5].collisionRegister, debug_stic->mobs[6].collisionRegister, debug_stic->mobs[7].collisionRegister);
                dsPrintValue(0, idx++, 0, dbg);
                break;
            case 1:
                for (UINT16 addr = 0x200; addr <= 0x25F; addr += 5)
                {
                    sprintf(dbg, "%03X : %04X %04X %04X %04X %04X", addr, PEEK_FAST(addr), PEEK_FAST(addr+1), PEEK_FAST(addr+2), PEEK_FAST(addr+3), PEEK_FAST(addr+4));
                    dsPrintValue(0, idx++, 0, dbg);
                }
                break;
            case 2:
                for (UINT16 addr = 0x260; addr <= 0x2BF; addr += 5)
                {
                    sprintf(dbg, "%03X : %04X %04X %04X %04X %04X", addr, PEEK_FAST(addr), PEEK_FAST(addr+1), PEEK_FAST(addr+2), PEEK_FAST(addr+3), PEEK_FAST(addr+4));
                    dsPrintValue(0, idx++, 0, dbg);
                }
                break;
            case 3:
                for (UINT16 addr = 0x2C0; addr <= 0x2EF; addr += 5)
                {
                    sprintf(dbg, "%03X : %04X %04X %04X %04X %04X", addr, PEEK_FAST(addr), PEEK_FAST(addr+1), PEEK_FAST(addr+2), PEEK_FAST(addr+3), PEEK_FAST(addr+4));
                    dsPrintValue(0, idx++, 0, dbg);
                }
                break;
            case 4:
                for (UINT16 addr = 0x3800; addr <= 0x387F; addr += 8)
                {
                    sprintf(dbg, "%04X: %02X %02X %02X %02X %02X %02X %02X %02X", addr, PEEK_SLOW(addr)&0xFF, PEEK_SLOW(addr+1)&0xFF, PEEK_SLOW(addr+2)&0xFF, PEEK_SLOW(addr+3)&0xFF, PEEK_SLOW(addr+4)&0xFF, PEEK_SLOW(addr+5)&0xFF, PEEK_SLOW(addr+6)&0xFF, PEEK_SLOW(addr+7)&0xFF);
                    dsPrintValue(0, idx++, 0, dbg);
                }
                break;
            case 5:
                for (UINT16 addr = 0x3880; addr <= 0x38FF; addr += 8)
                {
                    sprintf(dbg, "%04X: %02X %02X %02X %02X %02X %02X %02X %02X", addr, PEEK_SLOW(addr)&0xFF, PEEK_SLOW(addr+1)&0xFF, PEEK_SLOW(addr+2)&0xFF, PEEK_SLOW(addr+3)&0xFF, PEEK_SLOW(addr+4)&0xFF, PEEK_SLOW(addr+5)&0xFF, PEEK_SLOW(addr+6)&0xFF, PEEK_SLOW(addr+7)&0xFF);
                    dsPrintValue(0, idx++, 0, dbg);
                }
                break;
            case 6:
                for (UINT16 addr = 0x3900; addr <= 0x397F; addr += 8)
                {
                    sprintf(dbg, "%04X: %02X %02X %02X %02X %02X %02X %02X %02X", addr, PEEK_SLOW(addr)&0xFF, PEEK_SLOW(addr+1)&0xFF, PEEK_SLOW(addr+2)&0xFF, PEEK_SLOW(addr+3)&0xFF, PEEK_SLOW(addr+4)&0xFF, PEEK_SLOW(addr+5)&0xFF, PEEK_SLOW(addr+6)&0xFF, PEEK_SLOW(addr+7)&0xFF);
                    dsPrintValue(0, idx++, 0, dbg);
                }
                break;
            case 7:
                for (UINT16 addr = 0x3980; addr <= 0x39FF; addr += 8)
                {
                    sprintf(dbg, "%04X: %02X %02X %02X %02X %02X %02X %02X %02X", addr, PEEK_SLOW(addr)&0xFF, PEEK_SLOW(addr+1)&0xFF, PEEK_SLOW(addr+2)&0xFF, PEEK_SLOW(addr+3)&0xFF, PEEK_SLOW(addr+4)&0xFF, PEEK_SLOW(addr+5)&0xFF, PEEK_SLOW(addr+6)&0xFF, PEEK_SLOW(addr+7)&0xFF);
                    dsPrintValue(0, idx++, 0, dbg);
                }
                break;
        }
    }    
}

void debugger(void)
{
    static UINT32 myLastFrame = 0;
    
    if (debug_mode == DBG_MODE_PLAY) 
    {
        if (oneSecTick)
        {
            oneSecTick = FALSE;
            display_debug();
        }
        return;
    }
    
    if (((debug_mode == DBG_MODE_FRAME) && (myLastFrame != debug_frames)) || (debug_mode == DBG_MODE_STEP) || (debug_mode == DBG_MODE_STOP))
    {
        myLastFrame = debug_frames;
        display_debug();
        debugger_wait_for_input();
    }
}


#endif // DEBUG_ENABLE

// End of Line

