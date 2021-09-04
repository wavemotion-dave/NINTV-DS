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


struct Config_t 
{
    UINT32 crc;
    UINT16 frame_skip_opt;
    UINT16 overlay_selected;
    UINT16 key_A_map;
    UINT16 key_B_map;
    UINT16 key_X_map;
    UINT16 key_Y_map;
    UINT16 key_L_map;
    UINT16 key_R_map;
    UINT16 key_START_map;
    UINT16 key_SELECT_map;
    UINT16 controller_type;
    UINT16 sound_clock_div;
    UINT16 show_fps;
    UINT16 spare0;
    UINT16 spare1;
    UINT16 spare2;
    UINT16 spare3;
    UINT16 spare4;
    UINT16 spare5;
    UINT16 spare6;
    UINT16 spare7;
    UINT16 spare8;
    UINT16 spare9;
};

#define MAX_CONFIGS 600

extern void dsPrintValue(int x, int y, unsigned int isSelect, char *pchStr);
extern void dsMainLoop(void);
extern void dsInstallSoundEmuFIFO(void);
extern void dsInitScreenMain(void);
extern void dsInitTimer(void);
extern void dsFreeEmu(void);
extern void dsShowScreenEmu(void);
extern bool dsWaitOnQuit(void);
extern void dsChooseOptions(void);
extern void dsShowScreenMain(bool bFull);
extern unsigned int dsWaitForRom(char *chosen_filename);

#endif
