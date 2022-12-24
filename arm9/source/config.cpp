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
#include <stdio.h>
#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "ds_tools.h"
#include "savestate.h"
#include "CRC32.h"
#include "bgBottom.h"
#include "config.h"
#include "bgMenu-Green.h"
#include "bgMenu-White.h"
#include "Emulator.h"
#include "Rip.h"
#include "printf.h"

// -------------------------------------------------------------------------
// Configuration structures - we keep configuration at two levels:
//   Global - things that apply to all games (or defaults for new games)
//   Game   - things that apply only to one game - indexed by game CRC32
// We use a master config struct to hold one global config and up to 300
// individual game configs... and we save them out with version number
// and CRC to the SD card. The version number is critical because we can
// change that to force a reset to defaults if we change our master
// configration defaults in code...
// -------------------------------------------------------------------------
struct Config_t         myConfig;
struct GlobalConfig_t   myGlobalConfig;
struct AllConfig_t      allConfigs;

extern AudioMixer *audioMixer;
extern Rip *currentRip;

// 0 = Game Options
// 1 = Global Options
UINT8 options_shown = 0;        // Start with GAME config... can toggle to '1' for Global Options...

int display_options_list(bool);

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
    
    for (int i=0; i<10; i++)
    {
        myGlobalConfig.reserved32[i]        = 0;
    }
    
    myGlobalConfig.bios_dir                 = 0;
    myGlobalConfig.save_dir                 = 0;
    myGlobalConfig.show_fps                 = 0;
    myGlobalConfig.erase_saves              = 0;
    myGlobalConfig.key_START_map_default    = 11;
    myGlobalConfig.key_SELECT_map_default   = 21;
    myGlobalConfig.rom_dir                  = 0;
    myGlobalConfig.def_sound_quality        = (isDSiMode() ? 1:3);
    myGlobalConfig.man_dir                  = 0;
    myGlobalConfig.def_palette              = 0;
    myGlobalConfig.brightness               = 0;
    myGlobalConfig.spare3                   = 0;
    myGlobalConfig.spare4                   = 0;
    myGlobalConfig.spare5                   = 1;
    myGlobalConfig.menu_color               = 1;
    memset(myGlobalConfig.reserved, 0x00, 256);
}

// --------------------------------------------
// Set one game-specific default. We track 
// this in the myConfig global data struct.
// --------------------------------------------
static void SetDefaultGameConfig(void)
{
    myConfig.game_crc                       = 0x00000000;
    myConfig.frame_skip_opt                 = 1;
    myConfig.overlay_selected               = 0;
    myConfig.key_A_map                      = 12;
    myConfig.key_B_map                      = 12;
    myConfig.key_X_map                      = 13;
    myConfig.key_Y_map                      = 14;
    myConfig.key_L_map                      = 0;
    myConfig.key_R_map                      = 1;
    myConfig.key_START_map                  = myGlobalConfig.key_START_map_default;
    myConfig.key_SELECT_map                 = myGlobalConfig.key_SELECT_map_default;
    myConfig.controller_type                = 0;
    myConfig.sound_clock_div                = myGlobalConfig.def_sound_quality;
    myConfig.dpad_config                    = 0;
    myConfig.target_fps                     = 0;
    myConfig.spare0                         = 0;
    myConfig.palette                        = myGlobalConfig.def_palette;
    myConfig.stretch_x                      = ((160 / 256) << 8) | (160 % 256);
    myConfig.offset_x                       = 0;
    myConfig.spare3                         = 0;
    myConfig.spare4                         = 0;
    myConfig.spare5                         = 0;
    myConfig.key_AX_map                     = 0;
    myConfig.key_XY_map                     = 0;
    myConfig.key_YB_map                     = 0;
    myConfig.key_BA_map                     = 0;
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
void FindAndLoadConfig(void)
{
    FILE *fp;

    SetDefaultGameConfig();
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
            SetDefaultGameConfig();
            SaveConfig(FALSE);
            dsPrintValue(0,1,0, (char*)"              ");
        }
        else
        {
            // Now copy out the global config 
            memcpy(&myGlobalConfig, &allConfigs.global_config, sizeof(struct GlobalConfig_t));        
        }
        
        if (currentRip != NULL)
        {
            for (int slot=0; slot<MAX_CONFIGS; slot++)
            {
                if (allConfigs.game_config[slot].game_crc == currentRip->GetCRC())  // Got a match?!
                {
                    memcpy(&myConfig, &allConfigs.game_config[slot], sizeof(struct Config_t));
                    if (myConfig.stretch_x < 0x50) myConfig.stretch_x = ((160 / 256) << 8) | (160 % 256);   // If we haven't saved a stretch... set it to normal
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
        SetDefaultGameConfig();
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
    const char  *option[25];
    UINT8 *option_val;
    UINT8 option_max;
};

const struct options_t Game_Option_Table[] =
{
    {"OVERLAY",     {"GENERIC", "CUSTOM", "MINOTAUR", "ADVENTURE", "ASTROSMASH", "SPACE SPARTANS", "B-17 BOMBER", "ATLANTIS", "BOMB SQUAD", "UTOPIA", "ECS"}, &myConfig.overlay_selected, 11},
    {"A BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT", 
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU", "SWITCH", "MANUAL"},                                                                 &myConfig.key_A_map,        24},
    {"B BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT", 
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU", "SWITCH", "MANUAL"},                                                                 &myConfig.key_B_map,        24},
    {"X BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT", 
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU", "SWITCH", "MANUAL"},                                                                 &myConfig.key_X_map,        24},
    {"Y BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT",       
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU", "SWITCH", "MANUAL"},                                                                 &myConfig.key_Y_map,        24},
    {"L BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT", 
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU", "SWITCH", "MANUAL"},                                                                 &myConfig.key_L_map,        24},
    {"R BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT", 
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU", "SWITCH", "MANUAL"},                                                                 &myConfig.key_R_map,        24},
    {"START BTN",   {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT", 
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU", "SWITCH", "MANUAL"},                                                                 &myConfig.key_START_map,    24},
    {"SELECT BTN",  {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT", 
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU", "SWITCH", "MANUAL"},                                                                 &myConfig.key_SELECT_map,   24},
    
    {"A+X BUTTON",  {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT", 
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU", "SWITCH", "MANUAL"},                                                                 &myConfig.key_AX_map,       24},

    {"X+Y BUTTON",  {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT", 
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU", "SWITCH", "MANUAL"},                                                                 &myConfig.key_XY_map,       24},

    {"Y+B BUTTON",  {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT", 
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU", "SWITCH", "MANUAL"},                                                                 &myConfig.key_YB_map,       24},

    {"B+A BUTTON",  {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT", 
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU", "SWITCH", "MANUAL"},                                                                 &myConfig.key_BA_map,       24},
    
    {"CONTROLLER",  {"LEFT/PLAYER1", "RIGHT/PLAYER2", "DUAL-ACTION A", "DUAL-ACTION B"},                                                                                &myConfig.controller_type,  4},
    {"D-PAD",       {"NORMAL", "SWAP LEFT/RGT", "SWAP UP/DOWN", "DIAGONALS", "STRICT 4-WAY"},                                                                           &myConfig.dpad_config,      5},
    {"FRAMESKIP",   {"OFF", "ON (ODD)", "ON (EVEN)", "AGRESSIVE"},                                                                                                      &myConfig.frame_skip_opt,   4},
    {"SOUND DIV",   {"12 (BEST)", "14 (EXELLENT)", "16 (GOOD)", "20 (AVERAGE)", "24 (FAIR)", "DISABLED"},                                                               &myConfig.sound_clock_div,  6},
    {"TGT SPEED",   {"60 FPS (100%)", "66 FPS (110%)", "72 FPS (120%)", "78 FPS (130%)", "84 FPS (140%)", "90 FPS (150%)", "MAX SPEED"},                                &myConfig.target_fps,       7},
    {"PALETTE",     {"ORIGINAL", "MUTED", "BRIGHT", "PAL", "CUSTOM"},                                                                                                   &myConfig.palette,          5},
    {NULL,          {"",            ""},                                NULL,                   1},
};

const struct options_t Global_Option_Table[] =
{
    {"FPS",         {"OFF", "ON", "ON WITH DEBUG"},                                                                                                                     &myGlobalConfig.show_fps,    3},
    {"SAVE STATE",  {"KEEP ON LOAD", "ERASE ON LOAD"},                                                                                                                  &myGlobalConfig.erase_saves, 2},
    {"BIOS DIR",    {"SAME AS ROMS", "/ROMS/BIOS", "/ROMS/INTV/BIOS", "/DATA/BIOS"},                                                                                    &myGlobalConfig.bios_dir,    4},
    {"SAVE DIR",    {"SAME AS ROMS", "/ROMS/SAV",  "/ROMS/INTV/SAV",  "/DATA/SAV"},                                                                                     &myGlobalConfig.save_dir,    4},
    {"OVL DIR",     {"SAME AS ROMS", "/ROMS/OVL",  "/ROMS/INTV/OVL",  "/DATA/OVL"},                                                                                     &myGlobalConfig.ovl_dir,     4},
    {"ROM DIR",     {"SAME AS EMU",  "/ROMS",      "/ROMS/INTV"},                                                                                                       &myGlobalConfig.rom_dir,     3},
    {"MAN DIR",     {"SAME AS ROMS", "/ROMS/MAN",  "/ROMS/INTV/MAN",  "/DATA/MAN"},                                                                                     &myGlobalConfig.man_dir,     4},    
    {"START DEF",   {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT", 
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU"},                                                                                     &myGlobalConfig.key_START_map_default, 22},
    {"SELECT DEF",  {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT", 
                     "RESET", "LOAD", "CONFIG", "SCORES", "QUIT", "STATE", "MENU"},                                                                                     &myGlobalConfig.key_SELECT_map_default, 22},
    {"SOUND DEF",   {"12 (BEST)", "14 (EXELLENT)", "16 (GOOD)", "20 (FAIR)", "24 (POOR)", "DISABLED"},                                                                  &myGlobalConfig.def_sound_quality,  6},
    {"PALETTE DEF", {"ORIGINAL", "MUTED", "BRIGHT", "PAL", "CUSTOM"},                                                                                                   &myGlobalConfig.def_palette,        5},
    {"BRIGTNESS",   {"MAX", "DIM", "DIMMER", "DIMEST"},                                                                                                                 &myGlobalConfig.brightness,         4},
    {"MENU COLOR",  {"WHITE", "GREEN"},                                                                                                                                 &myGlobalConfig.menu_color,         2},
    
    {NULL,          {"",            ""},                                NULL,                   1},
};

struct options_t *Current_Option_Table;

// -------------------------------------------------------------------------------------------------
// After settings hae changed, we call this to apply the new options to the game being played.
// This is also called when loading a game and after the configuration if read from NINTV-DS.DAT
// -------------------------------------------------------------------------------------------------
void ApplyOptions(void)
{
    // Change the sound div if needed... affects sound quality and speed 
    extern  INT32 clockDivisor, clocksPerSample;
    static UINT32 sound_divs[] = {12,14,16,20,24,SOUND_DIV_DISABLE};
    clockDivisor = sound_divs[myConfig.sound_clock_div];
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
int display_options_list(bool bFullDisplay)
{
    char strBuf[35];
    int len=0;
    
    if (bFullDisplay)
    {
        Current_Option_Table = (options_shown==0 ? (options_t *)Game_Option_Table : (options_t *)Global_Option_Table);
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
    if (options_shown == 0)
    {
        dsPrintValue(0,23, 0, (char *)"PRESS SELECT FOR GLOBAL OPTIONS ");
    }
    else
    {
        dsPrintValue(0,23, 0, (char *)" PRESS SELECT FOR GAME OPTIONS  ");
    }
    return len;    
}


// -----------------------------------------------------------------------------
// Allows the user to move the cursor up and down through the various table 
// enties  above to select options for the game they wish to play. 
// -----------------------------------------------------------------------------
void dsChooseOptions(int global)
{
    int optionHighlighted;
    int idx;
    bool bDone=false;
    int keys_pressed;
    int last_keys_pressed = 999;
    char strBuf[35];

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
            if (keysCurrent() & KEY_SELECT)  // Toggle Between Game and Global Options
            {
                options_shown = (1-options_shown);
                idx=display_options_list(true);
                optionHighlighted = 0;                
            }
            if ((keysCurrent() & KEY_B) || (keysCurrent() & KEY_A))  // Exit options
            {
                break;
            }
        }
        swiWaitForVBlank();
    }

    ApplyOptions();

    // Restore original bottom graphic
    dsShowScreenMain(false);
    
    // Give a third of a second time delay...
    for (int i=0; i<20; i++)
    {
        swiWaitForVBlank();
    }

    return;
}

// End of Line

