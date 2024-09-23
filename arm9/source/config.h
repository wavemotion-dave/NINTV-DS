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

#ifndef __CONFIG_H
#define __CONFIG_H

#include <nds.h>
#include "types.h"

// ---------------------------
// Config handling...
// ---------------------------
#define CONFIG_VER  0x000B

#define MAX_CONFIGS 625

#define SOUND_DIV_DISABLE  64

struct Config_t 
{
    UINT32 game_crc;                 // CRC32 of the game itself
    UINT8  frame_skip;
    UINT8  spare1;
    UINT8  key_A_map;
    UINT8  key_B_map;
    UINT8  key_X_map;
    UINT8  key_Y_map;
    UINT8  key_L_map;
    UINT8  key_R_map;
    UINT8  key_START_map;
    UINT8  key_SELECT_map;
    UINT8  key_AX_map;
    UINT8  key_XY_map;
    UINT8  key_YB_map;
    UINT8  key_BA_map;    
    UINT8  controller_type;
    UINT8  sound_quality;
    UINT8  dpad_config;
    UINT8  target_fps;
    UINT8  load_options;
    UINT8  palette;
    UINT16 stretch_x;
    INT8   offset_x;
    UINT8  bLatched;
    UINT8  fudgeTiming;
    UINT8  key_click;
    UINT8  bSkipBlanks;
    UINT8  gramSize;
    UINT8  spare6;
    UINT8  spare7;
    UINT8  spare8;
    UINT8  spare9;
    UINT8  spare10;
    UINT8  spare11;
    UINT8  spare12;
    UINT8  spare13;
    UINT8  spare14;
    UINT8  spare15;
    UINT8  spare16;
    UINT8  spare17;
    UINT8  spare18;
    UINT8  spare19;
    UINT16 spare20;
};

struct GlobalConfig_t
{
    char   last_path[256];              // Not used yet... maybe for the future
    char   last_game[256];              // Not used yet... maybe for the future
    char   exec_bios_filename[256];     // Not used yet... maybe for the future
    char   grom_bios_filename[256];     // Not used yet... maybe for the future
    char   ivoice_bios_filename[256];   // Not used yet... maybe for the future
    UINT32 favorites[64];
    UINT32 reserved32[64];              // Not used yet... maybe for the future
    UINT8  bios_dir;
    UINT8  save_dir;
    UINT8  ovl_dir;
    UINT8  rom_dir;
    UINT8  show_fps;
    UINT8  erase_saves;
    UINT8  def_sound_quality;
    UINT8  key_START_map_default;
    UINT8  key_SELECT_map_default;
    UINT8  man_dir;
    UINT8  def_palette;
    UINT8  brightness;
    UINT8  menu_color;
    UINT8  spare1;
    UINT8  spare2;
    UINT8  spare3;
    UINT8  spare4;
    UINT8  spare5;
    UINT8  frame_skip;
    UINT8  reserved[512];               // Not used yet... future use...
};

struct AllConfig_t
{
    UINT16                  config_ver;
    struct GlobalConfig_t   global_config;
    struct Config_t         game_config[MAX_CONFIGS];
    UINT32                  crc32;
};

#define DPAD_NORMAL             0
#define DPAD_REV_LEFT_RIGHT     1
#define DPAD_REV_UP_DOWN        2
#define DPAD_DIAGONALS          3
#define DPAD_STRICT_4WAY        4

#define GRAM_512B               0
#define GRAM_2K                 1

extern struct Config_t       myConfig;
extern struct GlobalConfig_t myGlobalConfig;
extern struct AllConfig_t    allConfigs;

extern void FindAndLoadConfig(UINT32 crc);
extern void dsChooseOptions(void);
extern void SaveConfig(UINT32 crc, bool bShow);

extern UINT8 bConfigWasFound;

#endif
