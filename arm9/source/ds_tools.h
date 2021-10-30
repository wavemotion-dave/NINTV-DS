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

typedef enum _RunState 
{
    Stopped,
    Paused,
    Running,
    Quit
} RunState;


extern UINT16 emu_frames;
extern UINT16 frames_per_sec_calc;
extern UINT8  oneSecTick;

extern INT32 debug[];

extern UINT8 b_dsi_mode;

extern int bg0, bg0b, bg1b;

extern UINT8 bStartSoundFifo;
extern UINT8 bUseJLP;
extern UINT8 bForceIvoice;
extern UINT8 bInitEmulator;
extern UINT8 bUseDiscOverlay;
extern UINT8 bGameLoaded;

extern UINT16 global_frames;

extern UINT16 target_frames[];

#define WAITVBL swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank();

extern void dsPrintValue(int x, int y, unsigned int isSelect, char *pchStr);
extern void dsMainLoop(char *initial_file);
extern void dsInitScreenMain(void);
extern void dsInitTimer(void);
extern void dsFreeEmu(void);
extern void dsShowScreenEmu(void);
extern bool dsWaitOnQuit(void);
extern void dsChooseOptions(int global);
extern void dsShowScreenMain(bool bFull);
extern void ApplyOptions(void);
extern void dsShowMenu(void);
extern void reset_emu_frames(void);

#endif
