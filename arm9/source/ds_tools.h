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
// Config handling...
// ---------------------------
#define CONFIG_VER  0x0001

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
    UINT16 dpad_config;
    UINT16 target_fps;
    UINT16 spare2;
    UINT16 spare3;
    UINT16 spare4;
    UINT16 spare5;
    UINT16 spare6;
    UINT16 spare7;
    UINT16 spare8;
    UINT16 spare9;
    UINT16 config_ver;
};

extern struct Config_t  myConfig;

#define MAX_CONFIGS 300


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

#define OVL_KEY_1       0
#define OVL_KEY_2       1
#define OVL_KEY_3       2
#define OVL_KEY_4       3
#define OVL_KEY_5       4
#define OVL_KEY_6       5
#define OVL_KEY_7       6
#define OVL_KEY_8       7
#define OVL_KEY_9       8
#define OVL_KEY_CLEAR   9
#define OVL_KEY_0       10
#define OVL_KEY_ENTER   11

#define OVL_BTN_FIRE    12
#define OVL_BTN_L_ACT   13
#define OVL_BTN_R_ACT   14

#define OVL_META_RESET  15
#define OVL_META_LOAD   16
#define OVL_META_CONFIG 17
#define OVL_META_SCORES 18
#define OVL_META_QUIT   19
#define OVL_META_STATE  20

#define OVL_META_RES2   21
#define OVL_META_RES3   22
#define OVL_MAX         23

extern struct Overlay_t myOverlay[OVL_MAX];

extern UINT16 frames;
extern UINT16 emu_frames;
    
#define WAITVBL swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank();

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
extern void ApplyOptions(void);
extern unsigned int dsWaitForRom(char *chosen_filename);
extern void VsoundHandler(void);

#endif
