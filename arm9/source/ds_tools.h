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
#ifndef __DS_TOOLS_H
#define __DS_TOOLS_H

#include <nds.h>
#include "types.h"

typedef enum {
  EMUARM7_INIT_SND = 0x123C,
  EMUARM7_STOP_SND = 0x123D,
  EMUARM7_PLAY_SND = 0x123E,
} FifoMesType;

typedef struct FICtoLoad {
  char filename[255];
  bool directory;
} FICA_INTV;

// ---------------------------
// Overlay handling...
// ---------------------------
struct Overlay_t
{
    UINT8   x1;
    UINT8   x2;
    UINT8   y1;
    UINT8   y2;
};

#define OVL_KEY_1        0
#define OVL_KEY_2        1
#define OVL_KEY_3        2
#define OVL_KEY_4        3
#define OVL_KEY_5        4
#define OVL_KEY_6        5
#define OVL_KEY_7        6
#define OVL_KEY_8        7
#define OVL_KEY_9        8
#define OVL_KEY_CLEAR    9
#define OVL_KEY_0        10
#define OVL_KEY_ENTER    11
 
#define OVL_BTN_FIRE     12
#define OVL_BTN_L_ACT    13
#define OVL_BTN_R_ACT    14
 
#define OVL_META_RESET   15
#define OVL_META_LOAD    16
#define OVL_META_CONFIG  17
#define OVL_META_SCORES  18
#define OVL_META_QUIT    19
#define OVL_META_STATE   20
#define OVL_META_MENU    21
#define OVL_META_SWITCH  22
#define OVL_META_MANUAL  23
#define OVL_META_STRETCH 24

#define OVL_MAX          25

extern struct Overlay_t myOverlay[OVL_MAX];

extern UINT16 frames;
extern UINT16 emu_frames;
    
#define WAITVBL swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank();

extern void dsPrintValue(int x, int y, unsigned int isSelect, char *pchStr);
extern void dsMainLoop(char *initial_file);
extern void dsInstallSoundEmuFIFO(void);
extern void dsInitScreenMain(void);
extern void dsInitTimer(void);
extern void dsFreeEmu(void);
extern void dsShowScreenEmu(void);
extern bool dsWaitOnQuit(void);
extern void dsChooseOptions(void);
extern void dsShowScreenMain(bool bFull);
extern void ApplyOptions(void);
extern unsigned int dsWaitForRom(char *chosen_filename);
extern void VsoundHandlerDSi(void);
extern void VsoundHandlerDS(void);
extern void dsInitPalette(void);

#endif
