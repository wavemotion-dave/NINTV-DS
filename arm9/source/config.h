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
#ifndef __CONFIG_H
#define __CONFIG_H

#include <nds.h>
#include "types.h"

// ---------------------------
// Config handling...
// ---------------------------
#define CONFIG_VER  0x0004

#define MAX_CONFIGS 300

#define SOUND_DIV_DISABLE  4

struct Config_t 
{
    UINT32 game_crc;                 // CRC32 of the game itself
    UINT8  frame_skip_opt;
    UINT8  overlay_selected;
    UINT8  key_A_map;
    UINT8  key_B_map;
    UINT8  key_X_map;
    UINT8  key_Y_map;
    UINT8  key_L_map;
    UINT8  key_R_map;
    UINT8  key_START_map;
    UINT8  key_SELECT_map;
    UINT8  controller_type;
    UINT8  sound_clock_div;
    UINT8  dpad_config;
    UINT8  target_fps;
    UINT8  spare0;
    UINT8  palette;
    UINT16 stretch_x;
    INT8   offset_x;
    UINT8  spare3;
    UINT8  spare4;
    UINT8  spare5;
    UINT8  key_AX_map;
    UINT8  key_XY_map;
    UINT8  key_YB_map;
    UINT8  key_BA_map;    
};

struct GlobalConfig_t
{
    char   last_path[256];              // Not used yet... maybe for the future
    char   last_game[256];              // Not used yet... maybe for the future
    char   exec_bios_filename[256];     // Not used yet... maybe for the future
    char   grom_bios_filename[256];     // Not used yet... maybe for the future
    char   ecs_bios_filename[256];      // Not used yet... maybe for the future
    char   ivoice_bios_filename[256];   // Not used yet... maybe for the future
    UINT32 favorites[10];               // Not used yet... maybe for the future
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
    UINT8  spare3;
    UINT8  spare4;
    UINT8  spare5;
    UINT8  spare6;
    UINT8  reserved[512];               // Not used yet... future use...
};

struct AllConfig_t
{
    UINT16                  config_ver;
    struct GlobalConfig_t   global_config;
    struct Config_t         game_config[MAX_CONFIGS];
    UINT32                  crc32;
};

extern struct Config_t       myConfig;
extern struct GlobalConfig_t myGlobalConfig;
extern struct AllConfig_t    allConfigs;

extern void FindAndLoadConfig(void);
extern void dsChooseOptions(void);

#endif
