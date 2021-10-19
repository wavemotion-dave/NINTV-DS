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

extern UINT16 frames;
extern UINT16 emu_frames;
extern INT32 debug[];

extern UINT8 b_dsi_mode;

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
extern void VsoundHandler(void);
extern void VsoundHandlerDouble(void);
extern void dsInitPalette(void);

#endif
