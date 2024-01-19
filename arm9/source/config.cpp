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

#include <nds.h>
#include <stdio.h>
#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "nintv-ds.h"
#include "savestate.h"
#include "CRC32.h"
#include "bgBottom.h"
#include "config.h"
#include "bgMenu-Green.h"
#include "bgMenu-White.h"
#include "Emulator.h"
#include "Rip.h"
#include "overlay.h"
#include "printf.h"

// -------------------------------------------------------------------------
// Configuration structures - we keep configuration at two levels:
//   Global - things that apply to all games (or defaults for new games)
//   Game   - things that apply only to one game - indexed by game CRC32
// We use a master config struct to hold one global config and up to 625
// individual game configs... and we save them out with version number
// and CRC to the SD card. The version number is critical because we can
// change that to force a reset to defaults if we change our master
// configration defaults in code...
// -------------------------------------------------------------------------
struct Config_t         myConfig            __attribute__((section(".dtcm")));
struct GlobalConfig_t   myGlobalConfig;
struct AllConfig_t      allConfigs;

extern AudioMixer *audioMixer;
extern Rip *currentRip;

// 0 = Global Options
// 1,2 = Game Options
UINT8 options_shown = 0;        // Start with Global config... can toggle to game options as needed

UINT8 bConfigWasFound = FALSE;

short int display_options_list(bool);

// --------------------------------------------
// Set the global configuration defaults...
// --------------------------------------------
static void SetDefaultGlobalConfig(void)
{
    memset(myGlobalConfig.last_path,            0x00, 256);
    memset(myGlobalConfig.last_game,            0x00, 256);
    memset(myGlobalConfig.exec_bios_filename,   0x00, 256);
    memset(myGlobalConfig.grom_bios_filename,   0x00, 256);
    memset(myGlobalConfig.ivoice_bios_filename, 0x00, 256);
    
    memset(myGlobalConfig.favorites,            0x00, 256);
    
    for (int i=0; i<64; i++)
    {
        myGlobalConfig.reserved32[i]        = 0;
    }
    
    // --------------------------------------------------------------------
    // Look for some common directories and set these as global defaults 
    // --------------------------------------------------------------------
    DIR* bdir = opendir("/roms/bios");       // See if directory exists
    if (bdir) 
    {
        closedir(bdir);
        myGlobalConfig.bios_dir             = 1; // in /roms/bios
    }
    else
    {
        DIR* bdir = opendir("/data/bios");   // See if directory exists
        if (bdir)
        {
            closedir(bdir);
            myGlobalConfig.bios_dir         = 3;    // in /data/bios
        }
        else
        {
            myGlobalConfig.bios_dir         = 0;    // Same as ROMS
        }
    }
    
    DIR* dir = opendir("/roms/intv");       // See if directory exists
    if (dir) 
    {
        closedir(dir);
        myGlobalConfig.save_dir             = 2;
        myGlobalConfig.rom_dir              = 2;
        myGlobalConfig.man_dir              = 2;
        myGlobalConfig.ovl_dir              = 2;
    }
    else //Same as ROMS
    {
        DIR* dir = opendir("/roms");   // See if directory exists
        if (dir) 
        {
            myGlobalConfig.save_dir         = 1;
            myGlobalConfig.rom_dir          = 1;
            myGlobalConfig.man_dir          = 1;
            myGlobalConfig.ovl_dir          = 1;
        }
        else // Set 'Same as ROMs' which is a sensible fall-back default
        {
            myGlobalConfig.save_dir         = 0;
            myGlobalConfig.rom_dir          = 0;
            myGlobalConfig.man_dir          = 0;
            myGlobalConfig.ovl_dir          = 0;
        }
    }
    
    myGlobalConfig.show_fps                 = 0;
    myGlobalConfig.erase_saves              = 0;
    myGlobalConfig.key_START_map_default    = OVL_KEY_ENTER;
    myGlobalConfig.key_SELECT_map_default   = OVL_META_MENU;
    myGlobalConfig.def_sound_quality        = (isDSiMode() ? 2:0);
    myGlobalConfig.def_palette              = 0;
    myGlobalConfig.brightness               = 0;
    myGlobalConfig.menu_color               = 1;
    myGlobalConfig.spare1                   = 0;
    myGlobalConfig.spare2                   = 0;
    myGlobalConfig.spare3                   = 0;
    myGlobalConfig.spare4                   = 0;
    myGlobalConfig.spare5                   = 0;
    myGlobalConfig.frame_skip               = (isDSiMode() ? 0:1);
    memset(myGlobalConfig.reserved, 0x00, 256);
}

// --------------------------------------------
// Set one game-specific default. We track 
// this in the myConfig global data struct.
// --------------------------------------------
static void SetDefaultGameConfig(UINT32 crc)
{
    myConfig.game_crc                       = 0x00000000;
    myConfig.frame_skip                     = myGlobalConfig.frame_skip;
    myConfig.overlay                        = 0;
    myConfig.key_A_map                      = OVL_BTN_FIRE;
    myConfig.key_B_map                      = OVL_BTN_FIRE;
    myConfig.key_X_map                      = OVL_BTN_R_ACT;
    myConfig.key_Y_map                      = OVL_BTN_L_ACT;
    myConfig.key_L_map                      = OVL_KEY_1;
    myConfig.key_R_map                      = OVL_KEY_2;
    myConfig.key_START_map                  = myGlobalConfig.key_START_map_default;
    myConfig.key_SELECT_map                 = myGlobalConfig.key_SELECT_map_default;
    myConfig.key_AX_map                     = OVL_BTN_FIRE;
    myConfig.key_XY_map                     = OVL_BTN_FIRE;
    myConfig.key_YB_map                     = OVL_BTN_FIRE;
    myConfig.key_BA_map                     = OVL_BTN_FIRE;
    myConfig.controller_type                = CONTROLLER_P1;
    myConfig.sound_quality                  = myGlobalConfig.def_sound_quality;
    myConfig.dpad_config                    = DPAD_NORMAL;
    myConfig.target_fps                     = 0;
    myConfig.load_options                   = 0x00;
    myConfig.palette                        = myGlobalConfig.def_palette;
    myConfig.stretch_x                      = ((160 / 256) << 8) | (160 % 256);
    myConfig.offset_x                       = 0;
    myConfig.bLatched                       = 0;
    myConfig.fudgeTiming                    = 0;
    myConfig.spare3                         = 0;
    myConfig.spare4                         = 0;
    myConfig.spare5                         = 0;
    myConfig.spare6                         = 0;
    myConfig.spare7                         = 0;
    myConfig.spare8                         = 0;
    myConfig.spare9                         = 0;
    myConfig.spare10                        = 0;
    myConfig.spare11                        = 0;
    myConfig.spare12                        = 0;
    myConfig.spare13                        = 0;
    myConfig.spare14                        = 0;
    myConfig.spare15                        = 0;
    myConfig.spare16                        = 0;
    myConfig.spare17                        = 1;
    myConfig.spare18                        = 1;
    myConfig.spare19                        = 1;
    myConfig.spare20                        = 0x0000;
    
    // -----------------------------------------------------------------------------------------
    // Now patch up some of the more common games that work best with non-default settings...
    // -----------------------------------------------------------------------------------------
    if (crc == 0x2DEACD15) myConfig.bLatched        = true;                     // Stampede must have latched backtab access
    if (crc == 0x573B9B6D) myConfig.bLatched        = true;                     // Masters of the Universe must have latched backtab access
    if (crc == 0x8AD19AB3) myConfig.frame_skip      = 0;                        // B-17 Bomber no frame skip
    if (crc == 0x5F6E1AF6) myConfig.fudgeTiming     = 2;                        // Motocross needs some fudge timing to run... known race condition...
    if (crc == 0xfab2992c) myConfig.controller_type = CONTROLLER_DUAL_ACTION_B; // Astrosmash is best with Dual Action B
    if (crc == 0xd0f83698) myConfig.controller_type = CONTROLLER_DUAL_ACTION_B; // Astrosmash (competition) is best with Dual Action B
    if (crc == 0x2a1e0c1c) myConfig.controller_type = CONTROLLER_DUAL_ACTION_B; // Buzz Bombers is best with Dual Action B
    if (crc == 0x2a1e0c1c) myConfig.controller_type = CONTROLLER_DUAL_ACTION_B; // Buzz Bombers is best with Dual Action B
    if (crc == 0xc047d487) myConfig.controller_type = CONTROLLER_DUAL_ACTION_B; // Beauty and the Beast is best with Dual Action B
    if (crc == 0xc047d487) myConfig.dpad_config     = DPAD_STRICT_4WAY;         // Beauty and the Beast is best with Strict 4-way
    if (crc == 0xD8C9856A) myConfig.dpad_config     = DPAD_DIAGONALS;           // Q-Bert is best with diagonal
}


static void ToggleXYABasDirections(void)
{
    if (myConfig.key_A_map == OVL_KEY_6) // If we were AD&D map, go to TARMIN map
    {
        myConfig.key_A_map                      = OVL_KEY_7;
        myConfig.key_B_map                      = OVL_KEY_4;
        myConfig.key_X_map                      = OVL_KEY_6;
        myConfig.key_Y_map                      = OVL_KEY_1;
        myConfig.key_AX_map                     = OVL_BTN_FIRE;
        myConfig.key_XY_map                     = OVL_BTN_FIRE;
        myConfig.key_YB_map                     = OVL_BTN_FIRE;
        myConfig.key_BA_map                     = OVL_BTN_FIRE;
        myConfig.controller_type                = CONTROLLER_P1;
        myConfig.key_L_map                      = OVL_KEY_3;
        myConfig.key_R_map                      = OVL_BTN_FIRE;
        myConfig.key_START_map                  = OVL_KEY_5;
    }
    else if (myConfig.key_A_map == OVL_BTN_FIRE) // If we were normal map, go to AD&D map
    {
        myConfig.key_A_map                      = OVL_KEY_6;
        myConfig.key_B_map                      = OVL_KEY_8;
        myConfig.key_X_map                      = OVL_KEY_2;
        myConfig.key_Y_map                      = OVL_KEY_4;
        myConfig.key_AX_map                     = OVL_KEY_3;
        myConfig.key_XY_map                     = OVL_KEY_1;
        myConfig.key_YB_map                     = OVL_KEY_7;
        myConfig.key_BA_map                     = OVL_KEY_9;
        myConfig.controller_type                = CONTROLLER_DUAL_ACTION_A;
        myConfig.key_L_map                      = OVL_BTN_FIRE;
        myConfig.key_R_map                      = OVL_BTN_FIRE;
        myConfig.key_START_map                  = myGlobalConfig.key_START_map_default;
    }
    else // Just cycle back to normal key map
    {
        myConfig.key_A_map                      = OVL_BTN_FIRE;
        myConfig.key_B_map                      = OVL_BTN_FIRE;
        myConfig.key_X_map                      = OVL_BTN_R_ACT;
        myConfig.key_Y_map                      = OVL_BTN_L_ACT;
        myConfig.key_AX_map                     = OVL_BTN_FIRE;
        myConfig.key_XY_map                     = OVL_BTN_FIRE;
        myConfig.key_YB_map                     = OVL_BTN_FIRE;
        myConfig.key_BA_map                     = OVL_BTN_FIRE;
        myConfig.controller_type                = CONTROLLER_P1;
        myConfig.key_L_map                      = OVL_KEY_1;
        myConfig.key_R_map                      = OVL_KEY_2;
        myConfig.key_START_map                  = myGlobalConfig.key_START_map_default;
    }
}

// ---------------------------------------------------------------------------
// Write out the NINTV-DS.DAT configuration file to capture the settings for
// each game.  This one file contains global settings + 300 game settings.
// ---------------------------------------------------------------------------
void SaveConfig(bool bShow)
{
    FILE *fp;
    int slot = 0;
    
    if (bShow) dsPrintValue(0,23,0, (char*)"     SAVING CONFIGURATION       ");

    // Set the global configuration version number...
    allConfigs.config_ver = CONFIG_VER;

    // Copy in the global configuration
    memcpy(&allConfigs.global_config, &myGlobalConfig, sizeof(struct GlobalConfig_t));
    
    // If there is a game loaded, save that into a slot... re-use the same slot if it exists
    if (currentRip != NULL)
    {
        myConfig.game_crc = currentRip->GetCRC();
        // Find the slot we should save into...
        for (slot=0; slot<MAX_CONFIGS; slot++)
        {
            if (allConfigs.game_config[slot].game_crc == myConfig.game_crc)  // Got a match?!
            {
                break;                           
            }
            if (allConfigs.game_config[slot].game_crc == 0x00000000)  // Didn't find it... use a blank slot...
            {
                break;                           
            }
        }

        memcpy(&allConfigs.game_config[slot], &myConfig, sizeof(struct Config_t));
    }
    
    // -------------------------------------------------------------------------------------
    // Compute the CRC32 of everything and we can check this as integrity in the future...
    // -------------------------------------------------------------------------------------
    allConfigs.crc32 = CRC32::getCrc((UINT8*)&allConfigs, sizeof(allConfigs) - sizeof(UINT32));

    DIR* dir = opendir("/data");
    if (dir)
    {
        closedir(dir);  // Directory exists.
    }
    else
    {
        mkdir("/data", 0777);   // Doesn't exist - make it...
    }
    fp = fopen("/data/NINTV-DS.DAT", "wb+");
    if (fp != NULL)
    {
        fwrite(&allConfigs, sizeof(allConfigs), 1, fp);
        fclose(fp);
    } else dsPrintValue(2,20,0, (char*)"  ERROR SAVING CONFIG FILE  ");

    if (bShow) 
    {
        WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
        display_options_list(false);
    }
}


// -------------------------------------------------------------------------
// Find the NINTV-DS.DAT file and load it... if it doesn't exist, then
// default values will be used for the entire configuration database...
// -------------------------------------------------------------------------
void FindAndLoadConfig(UINT32 crc)
{
    FILE *fp;

    bConfigWasFound = FALSE;
    SetDefaultGameConfig(crc);
    fp = fopen("/data/NINTV-DS.DAT", "rb");
    if (fp != NULL)
    {
        fread(&allConfigs, sizeof(allConfigs), 1, fp);
        fclose(fp);
        
        if (allConfigs.config_ver != CONFIG_VER)
        {
            dsPrintValue(0,1,0, (char*)"PLEASE WAIT...");
            memset(&allConfigs, 0x00, sizeof(allConfigs));
            allConfigs.config_ver = CONFIG_VER;
            SetDefaultGlobalConfig();
            SetDefaultGameConfig(crc);
            SaveConfig(FALSE);
            dsPrintValue(0,1,0, (char*)"              ");
        }
        else
        {
            // Now copy out the global config 
            memcpy(&myGlobalConfig, &allConfigs.global_config, sizeof(struct GlobalConfig_t));        
        }
        
        if (crc != 0xFFFFFFFF)
        {
            for (int slot=0; slot<MAX_CONFIGS; slot++)
            {
                if (allConfigs.game_config[slot].game_crc == crc)  // Got a match?!
                {
                    bConfigWasFound = TRUE;
                    memcpy(&myConfig, &allConfigs.game_config[slot], sizeof(struct Config_t));
                    break;                           
                }
            }
        }
    }
    else    // Not found... init the entire database...
    {
        dsPrintValue(0,1,0, (char*)"PLEASE WAIT...");
        memset(&allConfigs, 0x00, sizeof(allConfigs));
        allConfigs.config_ver = CONFIG_VER;
        SetDefaultGlobalConfig();
        SetDefaultGameConfig(crc);
        SaveConfig(FALSE);
        dsPrintValue(0,1,0, (char*)"              ");
    }
    
    ApplyOptions();
}


// ------------------------------------------------------------------------------
// Options are handled here... we have a number of things the user can tweak
// and these options are applied immediately. The user can also save off 
// their option choices for the currently running game into the NINTV-DS.DAT
// configuration database. When games are loaded back up, NINTV-DS.DAT is read
// to see if we have a match and the user settings can be restored for the game.
// ------------------------------------------------------------------------------
struct options_t
{
    const char  *label;
    const char  *option[28];
    UINT8 *option_val;
    UINT8 option_max;
};

#define KEY_MAP_OPTIONS "KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "L-ACT", "R-ACT", "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU", "SWITCH", "MANUAL", "DISC UP", "DISC DOWN", "SPEEDUP"

const struct options_t Option_Table[3][20] =
{
    // Page 1 options
    {
        {"OVERLAY",     {"GENERIC", "ECS"},                                                                                                                         &myConfig.overlay,          2},
        {"A BUTTON",    {KEY_MAP_OPTIONS},                                                                                                                          &myConfig.key_A_map,        27},
        {"B BUTTON",    {KEY_MAP_OPTIONS},                                                                                                                          &myConfig.key_B_map,        27},
        {"X BUTTON",    {KEY_MAP_OPTIONS},                                                                                                                          &myConfig.key_X_map,        27},
        {"Y BUTTON",    {KEY_MAP_OPTIONS},                                                                                                                          &myConfig.key_Y_map,        27},
        {"L BUTTON",    {KEY_MAP_OPTIONS},                                                                                                                          &myConfig.key_L_map,        27},
        {"R BUTTON",    {KEY_MAP_OPTIONS},                                                                                                                          &myConfig.key_R_map,        27},
        {"START BTN",   {KEY_MAP_OPTIONS},                                                                                                                          &myConfig.key_START_map,    27},
        {"SELECT BTN",  {KEY_MAP_OPTIONS},                                                                                                                          &myConfig.key_SELECT_map,   27},
        {"A+X BUTTON",  {KEY_MAP_OPTIONS},                                                                                                                          &myConfig.key_AX_map,       26}, // These can't be mapped to SPEEDUP
        {"X+Y BUTTON",  {KEY_MAP_OPTIONS},                                                                                                                          &myConfig.key_XY_map,       26},
        {"Y+B BUTTON",  {KEY_MAP_OPTIONS},                                                                                                                          &myConfig.key_YB_map,       26},
        {"B+A BUTTON",  {KEY_MAP_OPTIONS},                                                                                                                          &myConfig.key_BA_map,       26},
        {"CONTROLLER",  {"LEFT/PLAYER1", "RIGHT/PLAYER2", "DUAL-ACTION A", "DUAL-ACTION B"},                                                                        &myConfig.controller_type,  4},
        {"D-PAD",       {"NORMAL", "SWAP LEFT/RGT", "SWAP UP/DOWN", "DIAGONALS", "STRICT 4-WAY"},                                                                   &myConfig.dpad_config,      5},
        {"FRAMESKIP",   {"OFF", "ON (ODD)", "ON (EVEN)", "AGRESSIVE"},                                                                                              &myConfig.frame_skip,       4},
        {"SOUND QUAL",  {"GOOD", "GREAT", "BEST"},                                                                                                                  &myConfig.sound_quality,    3},
        {"TGT SPEED",   {"60 FPS (100%)", "66 FPS (110%)", "72 FPS (120%)", "78 FPS (130%)", "84 FPS (140%)", "90 FPS (150%)", "54 FPS (90%)", "MAX SPEED"},        &myConfig.target_fps,       8},
        {"PALETTE",     {"ORIGINAL", "MUTED", "BRIGHT", "PAL", "CUSTOM"},                                                                                           &myConfig.palette,          5},
        {NULL,          {"",            ""},                                                                                                                        NULL,                       1},
    },
    
    // Page 2 options
    {
        {"BACKTAB",     {"NOT LATCHED", "LATCHED"},                                                                                                                 &myConfig.bLatched,         2},
        {"CPU FUDGE",   {"NONE", "LOW", "MEDIUM", "HIGH", "MAX"},                                                                                                   &myConfig.fudgeTiming,      5},
        {NULL,          {"",            ""},                                                                                                                        NULL,                       1},
    },
    
    // Global Options
    {   
        {"FPS",         {"OFF", "ON", "ON WITH DEBUG"},                                                                                                 &myGlobalConfig.show_fps,               3},
        {"SAVE STATE",  {"KEEP ON LOAD", "ERASE ON LOAD"},                                                                                              &myGlobalConfig.erase_saves,            2},
        {"BIOS DIR",    {"SAME AS ROMS", "/ROMS/BIOS", "/ROMS/INTV/BIOS", "/DATA/BIOS"},                                                                &myGlobalConfig.bios_dir,               4},
        {"SAVE DIR",    {"SAME AS ROMS", "/ROMS/SAV",  "/ROMS/INTV/SAV",  "/DATA/SAV"},                                                                 &myGlobalConfig.save_dir,               4},
        {"OVL DIR",     {"SAME AS ROMS", "/ROMS/OVL",  "/ROMS/INTV/OVL",  "/DATA/OVL"},                                                                 &myGlobalConfig.ovl_dir,                4},
        {"ROM DIR",     {"SAME AS EMU",  "/ROMS",      "/ROMS/INTV"},                                                                                   &myGlobalConfig.rom_dir,                3},
        {"MAN DIR",     {"SAME AS ROMS", "/ROMS/MAN",  "/ROMS/INTV/MAN",  "/DATA/MAN"},                                                                 &myGlobalConfig.man_dir,                4},    
        {"START DEF",   {KEY_MAP_OPTIONS},                                                                                                              &myGlobalConfig.key_START_map_default,  27},
        {"SELECT DEF",  {KEY_MAP_OPTIONS},                                                                                                              &myGlobalConfig.key_SELECT_map_default, 27},
        {"DEF SOUND",   {"GOOD", "GREAT", "BEST"},                                                                                                      &myGlobalConfig.def_sound_quality,      3},
        {"DEF PALETTE", {"ORIGINAL", "MUTED", "BRIGHT", "PAL", "CUSTOM"},                                                                               &myGlobalConfig.def_palette,            5},
        {"DEF FRAMSKP", {"OFF", "ON (ODD)", "ON (EVEN)", "AGRESSIVE"},                                                                                  &myGlobalConfig.frame_skip,             4},
        {"BRIGTNESS",   {"MAX", "DIM", "DIMMER", "DIMEST"},                                                                                             &myGlobalConfig.brightness,             4},
        {"MENU COLOR",  {"WHITE", "GREEN"},                                                                                                             &myGlobalConfig.menu_color,             2},
        {NULL,          {"",            ""},                                                                                                            NULL,                                   1},
    }
};

struct options_t *Current_Option_Table;
static char strBuf[35];

// -------------------------------------------------------------------------------------------------
// After settings hae changed, we call this to apply the new options to the game being played.
// This is also called when loading a game and after the configuration if read from NINTV-DS.DAT
// -------------------------------------------------------------------------------------------------
void ApplyOptions(void)
{
    // Change the sound div if needed... affects sound quality and speed 
    extern INT32 clockDivisor, clocksPerSample;
    static UINT32 sound_divs[] = {15, 12, 8};
    clockDivisor = sound_divs[myConfig.sound_quality];
    clocksPerSample = clockDivisor<<4;

    // Check if the sound changed...
    fifoSendValue32(FIFO_USER_01,(1<<16) | SOUND_KILL);
    bStartSoundFifo=true;
    // clears the emulator side of the audio mixer
    audioMixer->resetProcessor();

    // Set the screen scaling options for the game selected
    REG_BG3PA = myConfig.stretch_x;
    REG_BG3X = (myConfig.offset_x) << 8;
}


// ------------------------------------------------------------------
// Display the current list of options: either the global list
// or the individual game list of options...
// ------------------------------------------------------------------
short int display_options_list(bool bFullDisplay)
{
    short int len=0;
    
    if (bFullDisplay)
    {
        Current_Option_Table = (options_t *)Option_Table[options_shown];
        while (true)
        {
            sprintf(strBuf, " %-11s : %-15s", Current_Option_Table[len].label, Current_Option_Table[len].option[*(Current_Option_Table[len].option_val)]);
            dsPrintValue(1,2+len, (len==0 ? 1:0), strBuf); len++;
            if (Current_Option_Table[len].label == NULL) break;
        }

        // Blank out rest of the screen... option menus are of different lengths...
        for (int i=len; i<22; i++) 
        {
            dsPrintValue(1,2+i, 0, (char *)"                               ");
        }
    }

    dsPrintValue(0,22, 0, (char *)"D-PAD TOGGLE. B=EXIT, START=SAVE");
    dsPrintValue(0,23, 0, (char *)"X=SWAP.  SELECT FOR MORE OPTIONS");
    return len;    
}


// -----------------------------------------------------------------------------
// Allows the user to move the cursor up and down through the various table 
// enties  above to select options for the game they wish to play. 
// -----------------------------------------------------------------------------
void dsChooseOptions(int global)
{
    short int optionHighlighted;
    short int idx;
    bool bDone=false;
    int keys_pressed;
    short int last_keys_pressed = 999;

    options_shown = global; // Always start with GAME options unless told otherwise
    
    // Show the Options background...
    dsShowBannerScreen();

    idx=display_options_list(true);
    optionHighlighted = 0;
    while (!bDone)
    {
        keys_pressed = keysCurrent();
        if (keys_pressed != last_keys_pressed)
        {
            last_keys_pressed = keys_pressed;
            if (keysCurrent() & KEY_UP) // Previous option
            {
                sprintf(strBuf, " %-11s : %-15s", Current_Option_Table[optionHighlighted].label, Current_Option_Table[optionHighlighted].option[*(Current_Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,2+optionHighlighted,0, strBuf);
                if (optionHighlighted > 0) optionHighlighted--; else optionHighlighted=(idx-1);
                sprintf(strBuf, " %-11s : %-15s", Current_Option_Table[optionHighlighted].label, Current_Option_Table[optionHighlighted].option[*(Current_Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,2+optionHighlighted,1, strBuf);
            }
            if (keysCurrent() & KEY_DOWN) // Next option
            {
                sprintf(strBuf, " %-11s : %-15s", Current_Option_Table[optionHighlighted].label, Current_Option_Table[optionHighlighted].option[*(Current_Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,2+optionHighlighted,0, strBuf);
                if (optionHighlighted < (idx-1)) optionHighlighted++;  else optionHighlighted=0;
                sprintf(strBuf, " %-11s : %-15s", Current_Option_Table[optionHighlighted].label, Current_Option_Table[optionHighlighted].option[*(Current_Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,2+optionHighlighted,1, strBuf);
            }

            if (keysCurrent() & KEY_RIGHT)  // Toggle option clockwise
            {
                *(Current_Option_Table[optionHighlighted].option_val) = (*(Current_Option_Table[optionHighlighted].option_val) + 1) % Current_Option_Table[optionHighlighted].option_max;
                sprintf(strBuf, " %-11s : %-15s", Current_Option_Table[optionHighlighted].label, Current_Option_Table[optionHighlighted].option[*(Current_Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,2+optionHighlighted,1, strBuf);
            }
            if (keysCurrent() & KEY_LEFT)  // Toggle option counterclockwise
            {
                if ((*(Current_Option_Table[optionHighlighted].option_val)) == 0)
                    *(Current_Option_Table[optionHighlighted].option_val) = Current_Option_Table[optionHighlighted].option_max -1;
                else
                    *(Current_Option_Table[optionHighlighted].option_val) = (*(Current_Option_Table[optionHighlighted].option_val) - 1) % Current_Option_Table[optionHighlighted].option_max;
                sprintf(strBuf, " %-11s : %-15s", Current_Option_Table[optionHighlighted].label, Current_Option_Table[optionHighlighted].option[*(Current_Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,2+optionHighlighted,1, strBuf);
            }
            if (keysCurrent() & KEY_START)  // Save Options
            {
                SaveConfig(TRUE);
            }
            if (keysCurrent() & (KEY_SELECT | KEY_L | KEY_R))  // Toggle Between Option Pages
            {
                options_shown = (options_shown + 1) % 3;
                idx=display_options_list(true);
                optionHighlighted = 0;
            }
            if ((keysCurrent() & KEY_B) || (keysCurrent() & KEY_A))  // Exit options
            {
                break;
            }
            if (keysCurrent() & KEY_X)  // Toggle the various ABXY button options (so we can quickly map into AD&D or Tron Deadly Discs, etc)
            {
                ToggleXYABasDirections();
                idx=display_options_list(true);
                optionHighlighted = 0;
                WAITVBL;WAITVBL;WAITVBL;
            }
        }
        swiWaitForVBlank();
    }

    ApplyOptions();

    // Restore original bottom graphic
    dsShowScreenMain(false, false);
    
    // Give a third of a second time delay...
    for (int i=0; i<20; i++)
    {
        swiWaitForVBlank();
    }

    return;
}

// End of Line

