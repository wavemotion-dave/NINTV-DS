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
#include <nds/fifomessages.h>

#include<stdio.h>

#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "nintv-ds.h"
#include "videoaud.h"
#include "savestate.h"
#include "config.h"
#include "cheat.h"
#include "manual.h"
#include "bgBottom.h"
#include "bgTop.h"
#include "bgMenu-Green.h"
#include "bgMenu-White.h"
#include "Emulator.h"
#include "Rip.h"
#include "highscore.h"
#include "overlay.h"
#include "loadgame.h"
#include "debugger.h"
#include "AudioMixer.h"
#include "HandController.h"
#include "printf.h"
#include "CRC32.h"
#include "mus_intro_wav.h"
#include "keyclick_wav.h"
#include "screenshot.h"

// --------------------------------------------------------
// A set of boolean values so we know what to load and 
// how to load it. Put them in fast memory for tiny boost.
// --------------------------------------------------------
UINT8 bStartSoundFifo   __attribute__((section(".dtcm"))) = false;
UINT8 bUseJLP           __attribute__((section(".dtcm"))) = false;
UINT8 bUseECS           __attribute__((section(".dtcm"))) = false;
UINT8 bUseIVoice        __attribute__((section(".dtcm"))) = false;
UINT8 bInitEmulator     __attribute__((section(".dtcm"))) = false;
UINT8 bUseDiscOverlay   __attribute__((section(".dtcm"))) = false;
UINT8 bGameLoaded       __attribute__((section(".dtcm"))) = false;
UINT8 bMetaSpeedup      __attribute__((section(".dtcm"))) = false;
UINT8 bShowDisc         __attribute__((section(".dtcm"))) = false;

UINT8 hud_x = 3;
UINT8 hud_y = 0;

UINT16 keypad_pressed = 0;

// -------------------------------------------------------------
// This one is accessed rather often so we'll put it in .dtcm
// -------------------------------------------------------------
UINT8 b_dsi_mode __attribute__((section(".dtcm"))) = true;

// ------------------------------------------------------------------------------
// The user can configure how many frames should be run per second (FPS). 
// This is useful for games like Treasure of Tarmin where the user might
// want to run the game at 120% speed to get faster gameplay in the same time.
// ------------------------------------------------------------------------------
UINT16 target_frames[]         __attribute__((section(".dtcm"))) = {60,  66,   72,  78,  84,  90,  54, 999};
UINT32 target_frame_timing[]   __attribute__((section(".dtcm"))) = {546, 496, 454, 420, 390, 364,  600,  0};

// ---------------------------------------------------------------------------------
// Here are the main classes for the emulator, the RIP (game rom), video bus, etc.
// ---------------------------------------------------------------------------------
RunState             runState    __attribute__((section(".dtcm"))) = Stopped;
Emulator             *currentEmu __attribute__((section(".dtcm"))) = NULL;
Rip                  *currentRip __attribute__((section(".dtcm"))) = NULL;
VideoBus             *videoBus   __attribute__((section(".dtcm"))) = NULL;
AudioMixer           *audioMixer __attribute__((section(".dtcm"))) = NULL;

// ---------------------------------------------------------------------------------
// Some emulator frame calcualtions and once/second computations for frame rate...
// ---------------------------------------------------------------------------------
UINT16 emu_frames                __attribute__((section(".dtcm"))) = 0;
UINT16 frames_per_sec_calc       __attribute__((section(".dtcm"))) = 0;
UINT8  oneSecTick                __attribute__((section(".dtcm"))) = FALSE;
bool bIsFatalError               __attribute__((section(".dtcm"))) = false;

// -------------------------------------------------------------
// Background screen buffer indexes for the DS video engine...
// -------------------------------------------------------------
int bg0, bg0b, bg1b;

// ---------------------------------------------------------------
// When we hit our target frames per second (default NTSC 60 FPS)
// we start again - clear the timer and clear the counter...
// ---------------------------------------------------------------
void reset_emu_frames(void)
{
    TIMER0_CR=0;
    TIMER0_DATA=0;
    TIMER0_CR=TIMER_ENABLE | TIMER_DIV_1024;
    emu_frames=0;
}


void FatalError(const char *msg)
{
    dsPrintValue(0,1,0,(char*)msg);
    bIsFatalError = true;
    WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
}

// ---------------------------------------------------------------------------------
// A handy function to output a simple pre-built font that is stored just below
// the 192 pixel row marker on the lower screen background image... this is
// our "printf()" for the DS lower screen and is used to give simple error 
// messages, produce the menu/file loading text and output the FPS counter.
// ---------------------------------------------------------------------------------
void dsPrintValue(int x, int y, unsigned int isSelect, char *pchStr)
{
  u16 *pusEcran,*pusMap;
  u16 usCharac;
  char *pTrTxt=pchStr;
  char ch;

  pusEcran=(u16*) (bgGetMapPtr(bg1b))+x+(y<<5);
  pusMap=(u16*) (bgGetMapPtr(bg0b)+(2*isSelect+24)*32);

  while((*pTrTxt)!='\0' )
  {
    ch = *pTrTxt;
    if (ch >= 'a' && ch <= 'z') ch -= 32; // Faster than strcpy/strtoupper
    usCharac=0x0000;
    if ((ch) == '|')
      usCharac=*(pusMap);
    else if (((ch)<' ') || ((ch)>'_'))
      usCharac=*(pusMap);
    else if((ch)<'@')
      usCharac=*(pusMap+(ch)-' ');
    else
      usCharac=*(pusMap+32+(ch)-'@');
    *pusEcran++=usCharac;
    pTrTxt++;
  }
}

void dsPrintFPS(char *pchStr)
{
  u16 *pusEcran,*pusMap;
  char *pTrTxt=pchStr;

  pusEcran=(u16*) (bgGetMapPtr(bg1b))+0+(0<<5);
  pusMap=(u16*) (bgGetMapPtr(bg0b)+(2*0+24)*32);

  while((*pTrTxt)!='\0')
  {
    *pusEcran++ = *(pusMap+(*pTrTxt++)-' ');
  }
}


// ------------------------------------------------------------------
// Setup the emulator and basic perhipheral chips (BIOS, etc). 
// ------------------------------------------------------------------
BOOL InitializeEmulator(void)
{
    //find the currentEmulator required to run this RIP
    if (currentRip == NULL) return FALSE;
    
    currentEmu = Emulator::GetEmulator();
    if (currentEmu == NULL) 
    {
        return FALSE;
    }
    
    //load the BIOS files required for this currentEmulator
    if (!LoadPeripheralRoms(currentEmu))
    {
        FatalError("BIOS (EXEC, GROM) MISSING");
        return FALSE;
    }

    //load peripheral roms
    INT32 count = currentEmu->GetPeripheralCount();
    for (INT32 i = 0; i < count; i++) 
    {
        Peripheral* p = currentEmu->GetPeripheral(i);
        PeripheralCompatibility usage = currentRip->GetPeripheralUsage(p->GetShortName());
        if (usage == PERIPH_INCOMPATIBLE || usage == PERIPH_COMPATIBLE) {
            currentEmu->UsePeripheral(i, FALSE);
            continue;
        }

        BOOL loaded = LoadPeripheralRoms(p);
        if (loaded) 
        {
            //peripheral loaded, might as well use it.
            currentEmu->UsePeripheral(i, TRUE);
        }
        else if (usage == PERIPH_OPTIONAL) 
        {
            //didn't load, but the peripheral is optional, so just skip it
            currentEmu->UsePeripheral(i, FALSE);
        }
        else //usage == PERIPH_REQUIRED, but it didn't load
        {
            if (bUseECS)
                FatalError("NO ECS.BIN   ");
            else
                FatalError("NO IVOICE.BIN");
            return FALSE;
        }
    }
    
    // No audio to start... it will turn on 1 frame in...
    TIMER2_CR=0; irqDisable(IRQ_TIMER2);
    
    //hook the audio and video up to the currentEmulator
    currentEmu->InitVideo(videoBus,currentEmu->GetVideoWidth(),currentEmu->GetVideoHeight());
    currentEmu->InitAudio(audioMixer, mySoundFrequency);
    
    // Clear the audio mixer...
    audioMixer->resetProcessor();

    //put the RIP in the currentEmulator
    currentEmu->SetRip(currentRip, TRUE);
    
    // Read out any Cheats from disk...
    LoadCheats();
    
    // Apply any cheats/hacks to the current game (do this before loading Fast Memory)
    currentEmu->ApplyCheats();

    //Reset everything
    currentEmu->Reset();

    // Load up the fast ROM memory for quick fetches
    currentEmu->LoadFastMemory();
    
    // Make sure we're starting fresh...
    reset_emu_frames();
    
    // And put the sound engine back to the start...
    bStartSoundFifo = true;

    return TRUE;
}

// ----------------------------------------------------------------------------------
// The Intellivision renders output at 160x192 but the DS is 256x192... so there
// is some stretching involved but it's not CPU intensive since the DS hardware
// has an aline stretch built in... but we allow the user to tweak these stretch
// and offset settings to try and produce the best video output possible...
// ----------------------------------------------------------------------------------
char tmpStr[33];
void HandleScreenStretch(void)
{
    dsShowBannerScreen();
    swiWaitForVBlank();
    
    dsPrintValue(2, 5, 0,  (char*)"PRESS UP/DN TO STRETCH SCREEN");
    dsPrintValue(2, 7, 0,  (char*)"LEFT/RIGHT TO SHIFT SCREEN");
    dsPrintValue(2, 15, 0, (char*)"PRESS X TO RESET TO DEFAULTS");
    dsPrintValue(1, 16,0,  (char*)"START to SAVE, PRESS B TO EXIT");
    
    bool bDone = false;
    short int last_stretch_x = -99;
    short int last_offset_x = -99;
    while (!bDone)
    {
        int keys_pressed = keysCurrent();
        if (keys_pressed & KEY_UP)    
        {
            if (myConfig.stretch_x > 0x0080) REG_BG3PA = --myConfig.stretch_x;
        }
        else if (keys_pressed & KEY_DOWN)
        {
            if (myConfig.stretch_x < 0x0100) REG_BG3PA = ++myConfig.stretch_x;
        }
        else if (keys_pressed & KEY_RIGHT)
        {
            REG_BG3X = (++myConfig.offset_x) << 8;
        }
        else if (keys_pressed & KEY_LEFT)
        {
            REG_BG3X = (--myConfig.offset_x) << 8;
        }
        else if (keys_pressed & KEY_X)
        {
            myConfig.stretch_x = ((160 / 256) << 8) | (160 % 256);
            myConfig.offset_x = 0;
            REG_BG3PA = myConfig.stretch_x;
            REG_BG3X = (myConfig.offset_x) << 8;
        }
        else if ((keys_pressed & KEY_B) || (keys_pressed & KEY_A))
        {
            bDone = true;
        }
        else if (keys_pressed & KEY_START)
        {
            extern void SaveConfig(bool bShow);
            dsPrintValue(2,20,0, (char*)"    SAVING CONFIGURATION     ");
            SaveConfig(FALSE);
            WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            dsPrintValue(2,20,0, (char*)"                             ");
        }
        
        // If any values changed... output them.
        if ((myConfig.stretch_x != last_stretch_x) || (myConfig.offset_x != last_offset_x))
        {
            last_stretch_x = myConfig.stretch_x;
            last_offset_x = myConfig.offset_x;
            sprintf(tmpStr, "STRETCH_X = 0x%04X", myConfig.stretch_x);
            dsPrintValue(6, 11, 0, (char*)tmpStr);
            sprintf(tmpStr, " OFFSET_X = %-5d", myConfig.offset_x);
            dsPrintValue(6, 12, 0, (char*)tmpStr);
        }

        WAITVBL;WAITVBL;
    }
}

// -------------------------------------------------------------------------
// Show some information about the game/emulator on the bottom LCD screen.
// -------------------------------------------------------------------------
void dsShowEmuInfo(void)
{
    UINT8 bDone = false;
    UINT8 idx = 3;
    
    dsShowBannerScreen();
    swiWaitForVBlank();
    
    if (currentRip != NULL)
    {
        sprintf(tmpStr, "Build Date:    %s",        __DATE__);                           dsPrintValue(0, idx++, 0, tmpStr);
        sprintf(tmpStr, "CPU Mode:      %s",        isDSiMode() ? "DSI 134MHz 16MB":"DS 67MHz 4 MB");  dsPrintValue(0, idx++, 0, tmpStr);
        sprintf(tmpStr, "Frame Skip:    %s",        myConfig.frame_skip ? "YES":"NO ");  dsPrintValue(0, idx++, 0, tmpStr);
        sprintf(tmpStr, "Binary Size:   %-9u ",     currentRip->GetSize());              dsPrintValue(0, idx++, 0, tmpStr);
        sprintf(tmpStr, "Binary CRC:    %08X ",     currentRip->GetCRC());               dsPrintValue(0, idx++, 0, tmpStr);
        sprintf(tmpStr, "Intellivoice:  %s   ",     (bUseIVoice ? "YES":"NO"));          dsPrintValue(0, idx++, 0, tmpStr);
        sprintf(tmpStr, "JLP Enabled:   %s   ",     (bUseJLP    ? "YES":"NO"));          dsPrintValue(0, idx++, 0, tmpStr);
        sprintf(tmpStr, "ECS Enabled:   %s   ",     (bUseECS    ? "YES":"NO"));          dsPrintValue(0, idx++, 0, tmpStr);
        sprintf(tmpStr, "Total Frames:  %-9u ",     global_frames);                      dsPrintValue(0, idx++, 0, tmpStr);
        sprintf(tmpStr, "Memory Used:   %-9d ",     getMemUsed());                       dsPrintValue(0, idx++, 0, tmpStr);        
        sprintf(tmpStr, "RAM Indexes:   %d / %d / %d", fast_ram16_idx, slow_ram16_idx, slow_ram8_idx);  dsPrintValue(0, idx++, 0, tmpStr);        
        sprintf(tmpStr, "MEMS MAPPED:   %-9d ",     currentEmu->memoryBus.getMemCount());dsPrintValue(0, idx++, 0, tmpStr);    
        sprintf(tmpStr, "RIP ROM Count: %-9d ",     currentRip->GetROMCount());          dsPrintValue(0, idx++, 0, tmpStr);        
        sprintf(tmpStr, "RIP RAM Count: %-9d ",     currentRip->GetRAMCount());          dsPrintValue(0, idx++, 0, tmpStr);
        sprintf(tmpStr, "Compat Tags:   %02X %02X %02X %02X %02X", tag_compatibility[0], 
                tag_compatibility[1], tag_compatibility[2], tag_compatibility[3], 
                tag_compatibility[4]);                                                dsPrintValue(0, idx++, 0, tmpStr);
    }
    else
    {
        sprintf(tmpStr, "Build Date:    %s",        __DATE__);                           dsPrintValue(0, idx++, 0, tmpStr);
        sprintf(tmpStr, "CPU Mode:      %s",        isDSiMode() ? "DSI 134MHz 16MB":"DS 67MHz 4 MB");  dsPrintValue(0, idx++, 0, tmpStr);
        sprintf(tmpStr, "Memory Used:   %-9d ",     getMemUsed());                       dsPrintValue(0, idx++, 0, tmpStr);        
        sprintf(tmpStr, "NO GAME IS LOADED!");                                           dsPrintValue(0, idx++, 0, tmpStr);        
    }

    while (!bDone)
    {
        if (keysCurrent() & (KEY_A | KEY_B | KEY_Y | KEY_X))
        {
            bDone = true;
        }
        WAITVBL;
    }
}



// -------------------------------------------------------------------------------------
// The main menu provides a full set of options the user can pick. We started to run
// out of room on the main screen to show all the possible things the user can pick
// so we now just store all the extra goodies in this menu... By default the SELECT
// button will bring this up.
// -------------------------------------------------------------------------------------
#define MAIN_MENU_ITEMS 12
const char *main_menu[MAIN_MENU_ITEMS] = 
{
    "RESET EMULATOR",  
    "LOAD NEW GAME",  
    "GAME CONFIG",  
    "GAME SCORES",  
    "SAVE/RESTORE STATE",  
    "GAME MANUAL",  
    "SCREEN STRETCH",
    "GLOBAL CONFIG",  
    "SELECT CHEATS",  
    "GAME/EMULATOR INFO",
    "QUIT EMULATOR",  
    "EXIT THIS MENU",  
};


int menu_entry(void)
{
    UINT8 current_entry = 0;
    char bDone = 0;

    dsShowBannerScreen();
    swiWaitForVBlank();
    dsPrintValue(8,3,0, (char*)"MAIN MENU");
    dsPrintValue(4,20,0, (char*)"PRESS UP/DOWN AND A=SELECT");

    for (int i=0; i<MAIN_MENU_ITEMS; i++)
    {
        dsPrintValue(8,5+i, (i==0 ? 1:0), (char*)main_menu[i]);
    }
    
    int last_keys_pressed = -1;
    while (!bDone)
    {
        int keys_pressed = keysCurrent();
        
        if (keys_pressed != last_keys_pressed)
        {
            last_keys_pressed = keys_pressed;
            if (keys_pressed & KEY_DOWN)
            {
                dsPrintValue(8,5+current_entry, 0, (char*)main_menu[current_entry]);
                if (current_entry < (MAIN_MENU_ITEMS-1)) current_entry++; else current_entry=0;
                dsPrintValue(8,5+current_entry, 1, (char*)main_menu[current_entry]);
            }
            if (keys_pressed & KEY_UP)
            {
                dsPrintValue(8,5+current_entry, 0, (char*)main_menu[current_entry]);
                if (current_entry > 0) current_entry--; else current_entry=(MAIN_MENU_ITEMS-1);
                dsPrintValue(8,5+current_entry, 1, (char*)main_menu[current_entry]);
            }
            if (keys_pressed & KEY_SELECT)
            {
                return OVL_META_CONFIG;
            }
            if (keys_pressed & KEY_A)
            {
                switch (current_entry)
                {
                    case 0:
                        return OVL_META_RESET;
                        break;
                    case 1:
                        return OVL_META_LOAD;
                        break;
                    case 2:
                        return OVL_META_CONFIG;
                        break;
                    case 3:
                        return OVL_META_SCORES;
                        break;
                    case 4:
                        return OVL_META_STATE;
                        break;
                    case 5:
                        return OVL_META_MANUAL;
                        break;
                    case 6:
                        return OVL_META_STRETCH;
                        break;
                    case 7:
                        return OVL_META_GCONFIG;
                        break;
                    case 8:
                        return OVL_META_CHEATS;
                        break;
                    case 9:
                        return OVL_META_EMUINFO;
                        break;
                    case 10:
                        return OVL_META_QUIT;
                        break;
                    case 11:
                        bDone=1;
                        break;
                }
            }
            
            if (keys_pressed & KEY_B)
            {
                bDone=1;
            }
            swiWaitForVBlank();swiWaitForVBlank();swiWaitForVBlank();
        }
    }    
    return OVL_MAX;
}

// -------------------------------------------------------------------------------------
// Meta keys are things that aren't specifically emulating the Intellivision controller.
// For example: Game Config, Quit or Load Game are all meta-commands.
// -------------------------------------------------------------------------------------
char newFile[256];
void ds_handle_meta(int meta_key)
{
    if (meta_key == OVL_MAX) return;
    
    // -------------------------------------------------------------------
    // On the way in, make sure no keys are pressed before continuing...
    // -------------------------------------------------------------------
    while (keysCurrent())
    {
        WAITVBL;
    }
    switch (meta_key)
    {
        case OVL_META_LOAD:
            audioRampDown();
            if (dsWaitForRom(newFile))
            {
                if (LoadCart(newFile)) 
                {
                    dsInitPalette();
                }
                else return; // We've already set FatalError() from LoadCart()
            }
            bStartSoundFifo = true;
            break;
  
        case OVL_META_CONFIG:
            audioRampDown();
            dsChooseOptions((currentRip == NULL ? 2:0)); // If no game selected, show global options
            reset_emu_frames();
            dsInitPalette();
            WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            bStartSoundFifo = true;
            break;

        case OVL_META_GCONFIG:
            audioRampDown();
            dsChooseOptions(2);
            reset_emu_frames();
            dsInitPalette();
            WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            bStartSoundFifo = true;
            break;

        case OVL_META_CHEATS:
            audioRampDown();
            CheatMenu();
            reset_emu_frames();
            dsInitPalette();
            WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            bStartSoundFifo = true;
            extern u8 bCheatEngineReset;
            if (bCheatEngineReset == false)
            {
                break;
            } // Else fall through to reset!
            
        case OVL_META_RESET:
            if (bGameLoaded)
            {
                extern u8 bCheatChanged;
                // -------------------------------------------------------------------------------------------
                // If any CHEAT has been changed, we must load back the original RIP and re-apply any cheats.
                // -------------------------------------------------------------------------------------------
                if (bCheatChanged)   
                {
                    bCheatChanged = false;
                    
                    //put the RIP in the currentEmulator
                    currentEmu->SetRip(currentRip, FALSE);
                    
                    // Apply any cheats/hacks to the current game (do this before loading Fast Memory)
                    currentEmu->ApplyCheats();
                }

                // Perform the actual reset on everything
                currentEmu->Reset();
                
                // Load up the fast ROM memory for quick fetches
                currentEmu->LoadFastMemory();
                
                // And put the Sound Fifo back at the start...
                bStartSoundFifo = true;

                // Make sure we're starting fresh...
                reset_emu_frames();
            }                
            break;            
        case OVL_META_SCORES:
            audioRampDown();
            if (currentRip != NULL) 
            {
                highscore_display(currentRip->GetCRC());
                dsShowScreenMain(false, false);
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            bStartSoundFifo = true;
            break;

        case OVL_META_STATE:
            audioRampDown();
            if (currentRip != NULL) 
            {
                savestate_entry();      
                dsShowScreenMain(false, false);
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            bStartSoundFifo = true;
            break;

        case OVL_META_MENU:
            audioRampDown();
            if (currentRip != NULL) 
            {
                ds_handle_meta(menu_entry());
                if (currentRip != NULL)
                {
                    dsShowScreenMain(false, false);
                }
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            bStartSoundFifo = true;
            break;

        case OVL_META_SWITCH:
            if (currentRip != NULL) 
            {
                myConfig.controller_type = 1-myConfig.controller_type;
                dsPrintValue(0,0,0,(char*) (myConfig.controller_type ? "P2 " : "P1 "));
                WAITVBL;WAITVBL;WAITVBL;
                dsPrintValue(0,0,0, (char*)"   ");
            }
            break;
            
        case OVL_META_MANUAL:
            audioRampDown();
            if (currentRip != NULL) 
            {
                dsShowManual();
                dsShowScreenMain(false, false);
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            bStartSoundFifo = true;
            break;

        case OVL_META_EMUINFO:
            audioRampDown();
            dsShowEmuInfo();
            dsShowScreenMain(false, false);
            WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            bStartSoundFifo = true;
            break;
            
        case OVL_META_STRETCH:
            audioRampDown();
            if (currentRip != NULL) 
            {
                HandleScreenStretch();
                dsShowScreenMain(false, false);
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            bStartSoundFifo = true;
            break;
            
        case OVL_META_QUIT:
            audioRampDown();
            if (dsWaitOnQuit()){ runState = Quit; }
            bStartSoundFifo = true;
            break;
            
        case OVL_META_DISC:
            bShowDisc ^= 1;
            show_overlay(bShowDisc);
            break;
    }
}

UINT8 poll_touch_screen(UINT16 ctrl_disc, UINT16 ctrl_keys, UINT16 ctrl_side)
{
    UINT8 pad_pressed = 0;
    touchPosition touch;
    touchRead(&touch);
#ifdef DEBUG_ENABLE
    if (debugger_input(touch.px, touch.py) == DBG_PRESS_META)
    {
        while (keysCurrent() & KEY_TOUCH)   // Wait for release
        {           
            WAITVBL;
        }       
        WAITVBL;
    }
#endif
    // -----------------------------------------------------------
    // Did we map any hotspots on the overlay to disc directions?
    // -----------------------------------------------------------
    if (bUseDiscOverlay) 
    {
        for (int i=0; i < DISC_MAX; i++)
        {
            if (touch.px > myDisc[i].x1  && touch.px < myDisc[i].x2 && touch.py > myDisc[i].y1 && touch.py < myDisc[i].y2) ds_disc_input[ctrl_disc][i] = 1;
        }
    }

    // ----------------------------------------------------------------------
    // Handle the 12 keypad keys on the intellivision controller overlay...
    // ----------------------------------------------------------------------
    for (int i=0; i <= OVL_KEY_ENTER; i++)
    {
        if (touch.px > myOverlay[i].x1  && touch.px < myOverlay[i].x2 && touch.py > myOverlay[i].y1 && touch.py < myOverlay[i].y2)   {ds_key_input[ctrl_keys][i] = 1; pad_pressed=1;}
    }

    // ----------------------------------------------------------------------
    // Handle the 3 side buttons (top=Fire... Left-Action and Right-Action)
    // ----------------------------------------------------------------------
    for (int i=OVL_BTN_FIRE; i<=OVL_BTN_R_ACT; i++)
    {
        if (touch.px > myOverlay[i].x1  && touch.px < myOverlay[i].x2 && touch.py > myOverlay[i].y1 && touch.py < myOverlay[i].y2)   ds_key_input[ctrl_side][i] = 1;
    }

    // ----------------------------------------------------------------------------------
    // Handled "META" keys here... this includes things like RESET, LOAD, CONFIG, etc
    // ----------------------------------------------------------------------------------

    // RESET
    if (touch.px > myOverlay[OVL_META_RESET].x1  && touch.px < myOverlay[OVL_META_RESET].x2 && touch.py > myOverlay[OVL_META_RESET].y1 && touch.py < myOverlay[OVL_META_RESET].y2) 
    {
        ds_handle_meta(OVL_META_RESET);
    }
    // LOAD
    else if (touch.px > myOverlay[OVL_META_LOAD].x1  && touch.px < myOverlay[OVL_META_LOAD].x2 && touch.py > myOverlay[OVL_META_LOAD].y1 && touch.py < myOverlay[OVL_META_LOAD].y2) 
    {
        ds_handle_meta(OVL_META_LOAD);
    }
    // CONFIG
    else if (touch.px > myOverlay[OVL_META_CONFIG].x1  && touch.px < myOverlay[OVL_META_CONFIG].x2 && touch.py > myOverlay[OVL_META_CONFIG].y1 && touch.py < myOverlay[OVL_META_CONFIG].y2) 
    {
        ds_handle_meta(OVL_META_CONFIG);
    }
    // HIGHSCORES
    else if (touch.px > myOverlay[OVL_META_SCORES].x1  && touch.px < myOverlay[OVL_META_SCORES].x2 && touch.py > myOverlay[OVL_META_SCORES].y1 && touch.py < myOverlay[OVL_META_SCORES].y2) 
    {
        ds_handle_meta(OVL_META_SCORES);
    }
    // STATE
    else if (touch.px > myOverlay[OVL_META_STATE].x1  && touch.px < myOverlay[OVL_META_STATE].x2 && touch.py > myOverlay[OVL_META_STATE].y1 && touch.py < myOverlay[OVL_META_STATE].y2) 
    {
        ds_handle_meta(OVL_META_STATE);
    }
    // QUIT
    else if (touch.px > myOverlay[OVL_META_QUIT].x1  && touch.px < myOverlay[OVL_META_QUIT].x2 && touch.py > myOverlay[OVL_META_QUIT].y1 && touch.py < myOverlay[OVL_META_QUIT].y2) 
    {
        ds_handle_meta(OVL_META_QUIT);
    }
    // MENU
    else if (touch.px > myOverlay[OVL_META_MENU].x1  && touch.px < myOverlay[OVL_META_MENU].x2 && touch.py > myOverlay[OVL_META_MENU].y1 && touch.py < myOverlay[OVL_META_MENU].y2) 
    {
        ds_handle_meta(OVL_META_MENU);
    }
    // SWITCH
    else if (touch.px > myOverlay[OVL_META_SWITCH].x1  && touch.px < myOverlay[OVL_META_SWITCH].x2 && touch.py > myOverlay[OVL_META_SWITCH].y1 && touch.py < myOverlay[OVL_META_SWITCH].y2) 
    {
        ds_handle_meta(OVL_META_SWITCH);
    }
    // MANUAL
    else if (touch.px > myOverlay[OVL_META_MANUAL].x1  && touch.px < myOverlay[OVL_META_MANUAL].x2 && touch.py > myOverlay[OVL_META_MANUAL].y1 && touch.py < myOverlay[OVL_META_MANUAL].y2) 
    {
        ds_handle_meta(OVL_META_MANUAL);
    }

    // ---------------------------------------------------------------------------------------------------------
    // And, finally, if the ECS mini-keypad is being shown, we can directly check for any ECS keyboard keys...
    // ---------------------------------------------------------------------------------------------------------
    if (myConfig.overlay == 1)
    {
        if ((touch.px > 5) && (touch.px < 98))
        {
            if      (touch.py >= 25 && touch.py < 43)   // Row:  1 2 3 4 5
            {
                if      (touch.px <= 23) ecs_key_pressed = 1;
                else if (touch.px <= 41) ecs_key_pressed = 2;
                else if (touch.px <= 60) ecs_key_pressed = 3;
                else if (touch.px <= 78) ecs_key_pressed = 4;
                else if (touch.px <= 97) ecs_key_pressed = 5;

            }
            else if (touch.py >= 43 && touch.py < 60)   // Row:  6 7 8 9 0
            {
                if      (touch.px <= 23) ecs_key_pressed = 6;
                else if (touch.px <= 41) ecs_key_pressed = 7;
                else if (touch.px <= 60) ecs_key_pressed = 8;
                else if (touch.px <= 78) ecs_key_pressed = 9;
                else if (touch.px <= 97) ecs_key_pressed = 10;
            }
            else if (touch.py >= 60 && touch.py < 78)   // Row:  A B C D E
            {
                if      (touch.px <= 23) ecs_key_pressed = 11;
                else if (touch.px <= 41) ecs_key_pressed = 12;
                else if (touch.px <= 60) ecs_key_pressed = 13;
                else if (touch.px <= 78) ecs_key_pressed = 14;
                else if (touch.px <= 97) ecs_key_pressed = 15;
            }
            else if (touch.py >= 78 && touch.py < 95)   // Row:  F G H I J
            {
                if      (touch.px <= 23) ecs_key_pressed = 16;
                else if (touch.px <= 41) ecs_key_pressed = 17;
                else if (touch.px <= 60) ecs_key_pressed = 18;
                else if (touch.px <= 78) ecs_key_pressed = 19;
                else if (touch.px <= 97) ecs_key_pressed = 20;
            }
            else if (touch.py >= 95 && touch.py < 112)  // Row:  K L M N O
            {
                if      (touch.px <= 23) ecs_key_pressed = 21;
                else if (touch.px <= 41) ecs_key_pressed = 22;
                else if (touch.px <= 60) ecs_key_pressed = 23;
                else if (touch.px <= 78) ecs_key_pressed = 24;
                else if (touch.px <= 97) ecs_key_pressed = 25;
            }
            else if (touch.py >= 112 && touch.py < 130) // Row:  P Q R S T
            {
                if      (touch.px <= 23) ecs_key_pressed = 26;
                else if (touch.px <= 41) ecs_key_pressed = 27;
                else if (touch.px <= 60) ecs_key_pressed = 28;
                else if (touch.px <= 78) ecs_key_pressed = 29;
                else if (touch.px <= 97) ecs_key_pressed = 30;
            }
            else if (touch.py >= 130 && touch.py < 148) // Row:  U V W X Y
            {
                if      (touch.px <= 23) ecs_key_pressed = 31;
                else if (touch.px <= 41) ecs_key_pressed = 32;
                else if (touch.px <= 60) ecs_key_pressed = 33;
                else if (touch.px <= 78) ecs_key_pressed = 34;
                else if (touch.px <= 97) ecs_key_pressed = 35;
            }
            else if (touch.py >= 148 && touch.py < 166) // Row:  Z [arrows]
            {
                if      (touch.px <= 23) ecs_key_pressed = 36;
                else if (touch.px <= 41) ecs_key_pressed = 37;
                else if (touch.px <= 60) ecs_key_pressed = 38;
                else if (touch.px <= 78) ecs_key_pressed = 39;
                else if (touch.px <= 97) ecs_key_pressed = 40;
            }
            else if (touch.py >= 166 && touch.py < 190) // Row:  SPC  RET
            {
                if      (touch.px <= 50) ecs_key_pressed = 41;
                else if (touch.px <= 97) ecs_key_pressed = 42;
            }
        }
    }
    else if (bShowDisc) // Full touch-screen disc overlay is shown... do our best to map to the 16-position disc using approximating rectangular areas...
    {
             if ((touch.px >= 117) && (touch.px < 144) && (touch.py >= 16)  && (touch.py < 61))  ds_disc_input[ctrl_disc][0] = 1;   // North
        else if ((touch.px >= 144) && (touch.px < 162) && (touch.py >= 18)  && (touch.py < 51))  ds_disc_input[ctrl_disc][1] = 1;
        else if ((touch.px >= 163) && (touch.px < 196) && (touch.py >= 26)  && (touch.py < 60))  ds_disc_input[ctrl_disc][2] = 1;
        else if ((touch.px >= 154) && (touch.px < 170) && (touch.py >= 51)  && (touch.py < 70))  ds_disc_input[ctrl_disc][2] = 1;
        else if ((touch.px >= 171) && (touch.px < 205) && (touch.py >= 61)  && (touch.py < 80))  ds_disc_input[ctrl_disc][3] = 1;
        else if ((touch.px >= 162) && (touch.px < 211) && (touch.py >= 80)  && (touch.py < 107)) ds_disc_input[ctrl_disc][4] = 1;   // East
        else if ((touch.px >= 173) && (touch.px < 202) && (touch.py >= 107) && (touch.py < 127)) ds_disc_input[ctrl_disc][5] = 1;
        else if ((touch.px >= 163) && (touch.px < 196) && (touch.py >= 127) && (touch.py < 163)) ds_disc_input[ctrl_disc][6] = 1;
        else if ((touch.px >= 155) && (touch.px < 173) && (touch.py >= 115) && (touch.py < 136)) ds_disc_input[ctrl_disc][6] = 1;
        else if ((touch.px >= 144) && (touch.px < 164) && (touch.py >= 136) && (touch.py < 170)) ds_disc_input[ctrl_disc][7] = 1;
        else if ((touch.px >= 117) && (touch.px < 144) && (touch.py >= 126) && (touch.py < 173)) ds_disc_input[ctrl_disc][8] = 1;   // South
        else if ((touch.px >=  95) && (touch.px < 117) && (touch.py >= 136) && (touch.py < 171)) ds_disc_input[ctrl_disc][9] = 1;
        else if ((touch.px >=  86) && (touch.px < 103) && (touch.py >= 116) && (touch.py < 136)) ds_disc_input[ctrl_disc][10]= 1;
        else if ((touch.px >=  62) && (touch.px < 95)  && (touch.py >= 127) && (touch.py < 163)) ds_disc_input[ctrl_disc][10]= 1;
        else if ((touch.px >=  51) && (touch.px < 86)  && (touch.py >= 106) && (touch.py < 127)) ds_disc_input[ctrl_disc][11]= 1;
        else if ((touch.px >=  49) && (touch.px < 96)  && (touch.py >= 83)  && (touch.py < 106)) ds_disc_input[ctrl_disc][12]= 1;   // West
        else if ((touch.px >=  50) && (touch.px < 87)  && (touch.py >= 64)  && (touch.py < 83))  ds_disc_input[ctrl_disc][13]= 1;
        else if ((touch.px >=  62) && (touch.px < 96)  && (touch.py >= 28)  && (touch.py < 64))  ds_disc_input[ctrl_disc][14]= 1;
        else if ((touch.px >=  87) && (touch.px < 104) && (touch.py >= 53)  && (touch.py < 73))  ds_disc_input[ctrl_disc][14]= 1;
        else if ((touch.px >=  96) && (touch.px < 117) && (touch.py >= 20)  && (touch.py < 52))  ds_disc_input[ctrl_disc][15]= 1;
    }

    return pad_pressed;
}

// -------------------------------------------------------------------------------------------------
// Read the DS keys for input and handle it... this could be meta commands (LOAD, QUIT, CONFIG) or
// it could be actual intellivision keypad emulation (KEY_1, KEY_2, DISC movement, etc).
// This is called every frame - so 60 times per second which is fairly responsive...
// -------------------------------------------------------------------------------------------------
UINT8 breather = 0;

ITCM_CODE void pollInputs(void)
{
    UINT16 ctrl_disc, ctrl_keys, ctrl_side;
    unsigned short keys_pressed = keysCurrent();
    static short last_pressed = -1;

    for (int i=0; i<15; i++) {ds_key_input[0][i] = 0; ds_key_input[1][i] = 0;}
    for (int i=0; i<16; i++) {ds_disc_input[0][i] = 0; ds_disc_input[1][i] = 0;}
    
    // Assume NO speedup unless that specific META key is held down...
    bMetaSpeedup = false;
    
    // Check for Dual Action
    if (myConfig.controller_type == CONTROLLER_DUAL_ACTION_A)  // Standard Dual-Action (disc and side buttons on controller #1... keypad from controller #2)
    {
        ctrl_disc = 0;
        ctrl_side = 0;
        ctrl_keys = 1;
    }
    else if (myConfig.controller_type == CONTROLLER_DUAL_ACTION_B) // Same as #2 above except side-buttons use controller #2
    {
        ctrl_disc = 0;
        ctrl_side = 1;
        ctrl_keys = 1;
    }
    else
    {
        ctrl_disc = myConfig.controller_type;
        ctrl_side = myConfig.controller_type;
        ctrl_keys = myConfig.controller_type;
    }
    
    // ---------------------------------------------------------------
    // Handle 8 directions on keypad... best we can do with the d-pad
    // unless there is a custom .ovl overlay file mapping the disc.
    // ---------------------------------------------------------------
    
    // Unless told otherwise, we are NOT in un-throttle mode...
    bMetaSpeedup = false;

    // If one of the NDS keys is pressed...
    if (keys_pressed & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_L | KEY_R | KEY_A | KEY_B | KEY_X | KEY_Y | KEY_SELECT | KEY_START))
    {
        if (myConfig.dpad_config == DPAD_NORMAL)  // Normal handling
        {
            if (keys_pressed & KEY_UP)    
            {
                if (keys_pressed & KEY_RIGHT)     ds_disc_input[ctrl_disc][2]  = 1;
                else if (keys_pressed & KEY_LEFT) ds_disc_input[ctrl_disc][14] = 1;
                else ds_disc_input[ctrl_disc][0]  = 1;
            }
            else if (keys_pressed & KEY_DOWN)
            {
                if (keys_pressed & KEY_RIGHT)     ds_disc_input[ctrl_disc][6]  = 1;
                else if (keys_pressed & KEY_LEFT) ds_disc_input[ctrl_disc][10] = 1;
                else ds_disc_input[ctrl_disc][8]  = 1;
            }
            else if (keys_pressed & KEY_RIGHT)
            {
                ds_disc_input[ctrl_disc][4]  = 1;
            }
            else if (keys_pressed & KEY_LEFT)
            {
                ds_disc_input[ctrl_disc][12] = 1;
            }
        }
        else if (myConfig.dpad_config == DPAD_REV_LEFT_RIGHT) // Reverse Left/Right
        {
            if (keys_pressed & KEY_UP)    
            {
                if (keys_pressed & KEY_RIGHT)     ds_disc_input[ctrl_disc][14]  = 1;
                else if (keys_pressed & KEY_LEFT) ds_disc_input[ctrl_disc][2] = 1;
                else ds_disc_input[ctrl_disc][0]  = 1;
            }
            else if (keys_pressed & KEY_DOWN)
            {
                if (keys_pressed & KEY_RIGHT)     ds_disc_input[ctrl_disc][10]  = 1;
                else if (keys_pressed & KEY_LEFT) ds_disc_input[ctrl_disc][6] = 1;
                else ds_disc_input[ctrl_disc][8]  = 1;
            }
            else if (keys_pressed & KEY_RIGHT)
            {
                ds_disc_input[ctrl_disc][12]  = 1;
            }
            else if (keys_pressed & KEY_LEFT)
            {
                ds_disc_input[ctrl_disc][4] = 1;
            }
        }
        else if (myConfig.dpad_config == DPAD_REV_UP_DOWN)  // Reverse Up/Down
        {
            if (keys_pressed & KEY_UP)    
            {
                if (keys_pressed & KEY_RIGHT)     ds_disc_input[ctrl_disc][6]  = 1;
                else if (keys_pressed & KEY_LEFT) ds_disc_input[ctrl_disc][10] = 1;
                else ds_disc_input[ctrl_disc][8]  = 1;
            }
            else if (keys_pressed & KEY_DOWN)
            {
                if (keys_pressed & KEY_RIGHT)     ds_disc_input[ctrl_disc][2]  = 1;
                else if (keys_pressed & KEY_LEFT) ds_disc_input[ctrl_disc][14] = 1;
                else ds_disc_input[ctrl_disc][0]  = 1;
            }
            else if (keys_pressed & KEY_RIGHT)
            {
                ds_disc_input[ctrl_disc][4]  = 1;
            }
            else if (keys_pressed & KEY_LEFT)
            {
                ds_disc_input[ctrl_disc][12] = 1;
            }
        }    
        else if (myConfig.dpad_config == DPAD_DIAGONALS)  // Diagnoals
        {
            if (keys_pressed & KEY_UP)    
            {
                ds_disc_input[ctrl_disc][2]  = 1;
            }
            else if (keys_pressed & KEY_DOWN)
            {
                ds_disc_input[ctrl_disc][10] = 1;
            }
            else if (keys_pressed & KEY_RIGHT)
            {
                ds_disc_input[ctrl_disc][6]  = 1;
            }
            else if (keys_pressed & KEY_LEFT)
            {
                ds_disc_input[ctrl_disc][14] = 1;
            }
        }
        else if (myConfig.dpad_config == DPAD_STRICT_4WAY)  // Strict 4-way
        {
            // -----------------------------------------------------------------
            // To help with qames like Beauty and the Beast, we provide a
            // small 4 frame debounce between left/right switching to up/down.
            // -----------------------------------------------------------------
            if (keys_pressed & KEY_UP)    
            {
                if (breather) breather--;
                else ds_disc_input[ctrl_disc][0]  = 1;
            }
            else if (keys_pressed & KEY_DOWN)
            {
                if (breather) breather--;
                else ds_disc_input[ctrl_disc][8]  = 1;
            }
            else if (keys_pressed & KEY_RIGHT)
            {
                ds_disc_input[ctrl_disc][4]  = 1;
                breather = 4;
            }
            else if (keys_pressed & KEY_LEFT)
            {
                ds_disc_input[ctrl_disc][12] = 1;
                breather = 4;
            }
        }
    
        // -------------------------------------------------------------------------------------
        // Now handle the main DS keys... these can be re-mapped to any Intellivision function
        // -------------------------------------------------------------------------------------
        if ((keys_pressed & KEY_L) && (keys_pressed & KEY_R))
        {
            dsPrintValue(hud_x+2,hud_y,0,(char*)"SNAP");
            screenshot();
            WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            dsPrintValue(hud_x+2,hud_y,0,(char*)"    ");
        }
        else
        if ((keys_pressed & KEY_A) && (keys_pressed & KEY_X))
        {
            if (myConfig.key_AX_map >= OVL_META_RESET)
            {
                if (myConfig.key_AX_map == OVL_META_DISC_UP)        ds_disc_input[ctrl_disc][0]  = 1;
                else if (myConfig.key_AX_map == OVL_META_DISC_DN)   ds_disc_input[ctrl_disc][8]  = 1;
                else if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_AX_map);
            }
            else if (myConfig.key_AX_map >= OVL_BTN_FIRE) 
                ds_key_input[ctrl_side][myConfig.key_AX_map]  = 1;
            else
                ds_key_input[ctrl_keys][myConfig.key_AX_map]  = 1;
        }
        else
        if ((keys_pressed & KEY_X) && (keys_pressed & KEY_Y))
        {
            if (myConfig.key_XY_map >= OVL_META_RESET)
            {
                if (myConfig.key_XY_map == OVL_META_DISC_UP)        ds_disc_input[ctrl_disc][0]  = 1;
                else if (myConfig.key_XY_map == OVL_META_DISC_DN)   ds_disc_input[ctrl_disc][8]  = 1;
                else if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_XY_map);
            }
            else if (myConfig.key_XY_map >= OVL_BTN_FIRE) 
                ds_key_input[ctrl_side][myConfig.key_XY_map]  = 1;
            else
                ds_key_input[ctrl_keys][myConfig.key_XY_map]  = 1;
        }
        else
        if ((keys_pressed & KEY_Y) && (keys_pressed & KEY_B))
        {
            if (myConfig.key_YB_map >= OVL_META_RESET)
            {
                if (myConfig.key_YB_map == OVL_META_DISC_UP)        ds_disc_input[ctrl_disc][0]  = 1;
                else if (myConfig.key_YB_map == OVL_META_DISC_DN)   ds_disc_input[ctrl_disc][8]  = 1;
                else if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_YB_map);
            }
            else if (myConfig.key_YB_map >= OVL_BTN_FIRE) 
                ds_key_input[ctrl_side][myConfig.key_YB_map]  = 1;
            else
                ds_key_input[ctrl_keys][myConfig.key_YB_map]  = 1;
        }
        else
        if ((keys_pressed & KEY_B) && (keys_pressed & KEY_A))
        {
            if (myConfig.key_BA_map >= OVL_META_RESET)
            {
                if (myConfig.key_BA_map == OVL_META_DISC_UP)        ds_disc_input[ctrl_disc][0]  = 1;
                else if (myConfig.key_BA_map == OVL_META_DISC_DN)   ds_disc_input[ctrl_disc][8]  = 1;
                else if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_BA_map);
            }
            else if (myConfig.key_BA_map >= OVL_BTN_FIRE) 
                ds_key_input[ctrl_side][myConfig.key_BA_map]  = 1;
            else
                ds_key_input[ctrl_keys][myConfig.key_BA_map]  = 1;
        }
        else
        {
            if (keys_pressed & KEY_A)       
            {
                if (myConfig.key_A_map >= OVL_META_RESET)
                {
                    if (myConfig.key_A_map == OVL_META_DISC_UP)        ds_disc_input[ctrl_disc][0]  = 1;
                    else if (myConfig.key_A_map == OVL_META_DISC_DN)   ds_disc_input[ctrl_disc][8]  = 1;
                    else if (myConfig.key_A_map == OVL_META_SPEEDUP)   bMetaSpeedup = true;
                    else if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_A_map);
                }
                else if (myConfig.key_A_map >= OVL_BTN_FIRE) 
                    ds_key_input[ctrl_side][myConfig.key_A_map]  = 1;
                else
                    ds_key_input[ctrl_keys][myConfig.key_A_map]  = 1;
            }

            if (keys_pressed & KEY_B)
            {
                if (myConfig.key_B_map >= OVL_META_RESET)
                {
                    if (myConfig.key_B_map == OVL_META_DISC_UP)        ds_disc_input[ctrl_disc][0]  = 1;
                    else if (myConfig.key_B_map == OVL_META_DISC_DN)   ds_disc_input[ctrl_disc][8]  = 1;
                    else if (myConfig.key_B_map == OVL_META_SPEEDUP)   bMetaSpeedup = true;
                    else if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_B_map);
                }
                else if (myConfig.key_B_map >= OVL_BTN_FIRE) 
                    ds_key_input[ctrl_side][myConfig.key_B_map]  = 1;
                else
                    ds_key_input[ctrl_keys][myConfig.key_B_map]  = 1;
            }

            if (keys_pressed & KEY_X)
            {
                if (myConfig.key_X_map >= OVL_META_RESET)
                {
                    if (myConfig.key_X_map == OVL_META_DISC_UP)        ds_disc_input[ctrl_disc][0]  = 1;
                    else if (myConfig.key_X_map == OVL_META_DISC_DN)   ds_disc_input[ctrl_disc][8]  = 1;
                    else if (myConfig.key_X_map == OVL_META_SPEEDUP)   bMetaSpeedup = true;
                    else if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_X_map);
                }
                else if (myConfig.key_X_map >= OVL_BTN_FIRE) 
                    ds_key_input[ctrl_side][myConfig.key_X_map]  = 1;
                else
                    ds_key_input[ctrl_keys][myConfig.key_X_map]  = 1;
            }

            if (keys_pressed & KEY_Y)
            {
                if (myConfig.key_Y_map >= OVL_META_RESET)
                {
                    if (myConfig.key_Y_map == OVL_META_DISC_UP)        ds_disc_input[ctrl_disc][0]  = 1;
                    else if (myConfig.key_Y_map == OVL_META_DISC_DN)   ds_disc_input[ctrl_disc][8]  = 1;
                    else if (myConfig.key_Y_map == OVL_META_SPEEDUP)   bMetaSpeedup = true;
                    else if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_Y_map);
                }
                else if (myConfig.key_Y_map >= OVL_BTN_FIRE) 
                    ds_key_input[ctrl_side][myConfig.key_Y_map]  = 1;
                else
                    ds_key_input[ctrl_keys][myConfig.key_Y_map]  = 1;
            }
        }

        if (keys_pressed & KEY_L)
        {
            if (myConfig.key_L_map >= OVL_META_RESET)
            {
                if (myConfig.key_L_map == OVL_META_DISC_UP)        ds_disc_input[ctrl_disc][0]  = 1;
                else if (myConfig.key_L_map == OVL_META_DISC_DN)   ds_disc_input[ctrl_disc][8]  = 1;
                else if (myConfig.key_L_map == OVL_META_SPEEDUP)   bMetaSpeedup = true;
                else if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_L_map);
            }
            else if (myConfig.key_L_map >= OVL_BTN_FIRE) 
                ds_key_input[ctrl_side][myConfig.key_L_map]  = 1;
            else
                ds_key_input[ctrl_keys][myConfig.key_L_map]  = 1;
        }

        if (keys_pressed & KEY_R)
        {
            if (myConfig.key_R_map >= OVL_META_RESET)
            {
                if (myConfig.key_R_map == OVL_META_DISC_UP)        ds_disc_input[ctrl_disc][0]  = 1;
                else if (myConfig.key_R_map == OVL_META_DISC_DN)   ds_disc_input[ctrl_disc][8]  = 1;
                else if (myConfig.key_R_map == OVL_META_SPEEDUP)   bMetaSpeedup = true;
                else if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_R_map);
            }
            else if (myConfig.key_R_map >= OVL_BTN_FIRE) 
                ds_key_input[ctrl_side][myConfig.key_R_map]  = 1;
            else
                ds_key_input[ctrl_keys][myConfig.key_R_map]  = 1;
        }

        if (keys_pressed & KEY_START)   
        {
            if (myConfig.key_START_map >= OVL_META_RESET)
            {
                if (myConfig.key_START_map == OVL_META_DISC_UP)        ds_disc_input[ctrl_disc][0]  = 1;
                else if (myConfig.key_START_map == OVL_META_DISC_DN)   ds_disc_input[ctrl_disc][8]  = 1;
                else if (myConfig.key_START_map == OVL_META_SPEEDUP)   bMetaSpeedup = true;
                else if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_START_map);
            }
            else if (myConfig.key_START_map >= OVL_BTN_FIRE) 
                ds_key_input[ctrl_side][myConfig.key_START_map]  = 1;
            else
                ds_key_input[ctrl_keys][myConfig.key_START_map]  = 1;
        }

        if (keys_pressed & KEY_SELECT)  
        {
            if (myConfig.key_SELECT_map >= OVL_META_RESET)
            {
                if (myConfig.key_SELECT_map == OVL_META_DISC_UP)        ds_disc_input[ctrl_disc][0]  = 1;
                else if (myConfig.key_SELECT_map == OVL_META_DISC_DN)   ds_disc_input[ctrl_disc][8]  = 1;
                else if (myConfig.key_SELECT_map == OVL_META_SPEEDUP)   bMetaSpeedup = true;
                else if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_SELECT_map);
            }
            else if (myConfig.key_SELECT_map >= OVL_BTN_FIRE) 
                ds_key_input[ctrl_side][myConfig.key_SELECT_map]  = 1;
            else
                ds_key_input[ctrl_keys][myConfig.key_SELECT_map]  = 1;
        }
    }
    
    last_pressed = keys_pressed;
    
    ecs_key_pressed = (myConfig.overlay == 1) ? 0:255;
    
    // -----------------------------------------------------------------
    // Now handle the on-screen Intellivision overlay and meta keys...
    // -----------------------------------------------------------------
    if (keys_pressed & KEY_TOUCH)
    {
        if (poll_touch_screen(ctrl_disc, ctrl_keys, ctrl_side)) // Returns non-zero if we pressed one of the 12 keypad keys
        {
            if (++keypad_pressed == 3) // Need to see it pressed for several frames
            {
                if (myConfig.key_click)
                {
                    soundPlaySample(keyclick_wav, SoundFormat_16Bit, keyclick_wav_size, 44100, 127, 64, false, 0);
                }
            }
        } else keypad_pressed = 0;            
    } else keypad_pressed = 0;
}


// ------------------------------------------------------------------------------
// This shows the main upper screen which is where the emulation takes place... 
// the lower screen is used for overlay and debugger information.
// ------------------------------------------------------------------------------
void dsShowScreenMain(bool bFull, bool bPlayJingle) 
{
    // Init BG mode for 16 bits colors
    if (bFull)
    {
      videoSetMode(MODE_0_2D | DISPLAY_BG0_ACTIVE );
      videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE);
      vramSetBankA(VRAM_A_MAIN_BG); vramSetBankC(VRAM_C_SUB_BG);
      bg0 = bgInit(0, BgType_Text8bpp, BgSize_T_256x256, 31,0);
      bg0b = bgInitSub(0, BgType_Text8bpp, BgSize_T_256x256, 31,0);
      bg1b = bgInitSub(1, BgType_Text8bpp, BgSize_T_256x256, 30,0);
      bgSetPriority(bg0b,1);bgSetPriority(bg1b,0);

      decompress(bgTopTiles, bgGetGfxPtr(bg0), LZ77Vram);
      decompress(bgTopMap, (void*) bgGetMapPtr(bg0), LZ77Vram);
      dmaCopy((void *) bgTopPal,(u16*) BG_PALETTE,256*2);
      
      if (bPlayJingle)
      {
        soundPlaySample((const void *) mus_intro_wav, SoundFormat_16Bit, mus_intro_wav_size, 22050, 127, 64, false, 0);
      }
    }
    
     hud_x = 3;
     hud_y = 0;

    // Now show the bottom screen - usualy some form of overlay...
#ifdef DEBUG_ENABLE
    show_debug_overlay();
#else
    show_overlay(bShowDisc);
#endif
}

// --------------------------------------------------------------------------------------------------------
// Most of the main screen video memory is not used so we we will re-purpose it for emulation memory. 
// The DS VRAM is 16-bit access and you can only write 16-bits at a time... but this is fine for the
// Intellivision 16-bit CPU. The memory is not as fast as a main memory cache-hit but is much faster 
// than a main memory cache miss... so on average this memory tends to be quite useful! Also, since
// this emulator is using a large chunk of the 4MB available in the older DS-LITE/PHAT hardware, using
// almost 400k of video memory reduces the burden of memory usage on the older hardware.
// --------------------------------------------------------------------------------------------------------
void dsInitScreenMain(void)
{
    vramSetBankB(VRAM_B_LCD);           // Not using this for video but 128K of faster RAM always useful! Mapped at 0x06820000 - Used for Memory Bus Read Counter
    vramSetBankD(VRAM_D_LCD);           // Not using this for video but 128K of faster RAM always useful! Mapped at 0x06860000 - Used for Cart "Fast Buffer" 64k x 16 = 128k
    vramSetBankE(VRAM_E_LCD);           // Not using this for video but 64K of faster RAM always useful!  Mapped at 0x06880000 - Used for Custom Tile Overlay Buffer (60K for tile[] buffer, 4K for map[] buffer)
    vramSetBankF(VRAM_F_LCD);           // Not using this for video but 16K of faster RAM always useful!  Mapped at 0x06890000 - Slow 8-bit RAM buffer for ECS and games like USFC Chess and Land Battle (8K)
    vramSetBankG(VRAM_G_LCD);           // Not using this for video but 16K of faster RAM always useful!  Mapped at 0x06894000 - JLP RAM Buffer (8K Words or 16K Bytes)
    vramSetBankH(VRAM_H_LCD);           // Not using this for video but 32K of faster RAM always useful!  Mapped at 0x06898000 - Slow 16-bit RAM buffer (16K Words or 32K Bytes)
    vramSetBankI(VRAM_I_LCD);           // Not using this for video but 16K of faster RAM always useful!  Mapped at 0x068A0000 - Unused

    WAITVBL;
}


// ----------------------------------------------------------------------------------------------
// Generic show-menu which is just the lower screen that is mostly blank with the NINTELLIVISION 
// banner at the top. The menu has 2 font choices... a high-contrast white on black and a more 
// retro feel green on black (default). The user can change this option in global configuration.
// ----------------------------------------------------------------------------------------------
void dsShowBannerScreen(void)
{
    if (myGlobalConfig.menu_color == 0)
    {
        decompress(bgMenu_WhiteTiles, bgGetGfxPtr(bg0b), LZ77Vram);
        decompress(bgMenu_WhiteMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
        dmaCopy((void *) bgMenu_WhitePal,(u16*) BG_PALETTE_SUB,256*2);
        unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
        dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    }
    else
    {
        decompress(bgMenu_GreenTiles, bgGetGfxPtr(bg0b), LZ77Vram);
        decompress(bgMenu_GreenMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
        dmaCopy((void *) bgMenu_GreenPal,(u16*) BG_PALETTE_SUB,256*2);
        unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
        dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    }        
}


// ------------------------------------------------------------------------------------------
// Generic show the top screen... which is rendered as an 8-bit bitmap handling. The video 
// render code will basically dump the internal Intellivision "pixelBuffer" into the video
// memory via a DMA copy (slightly faster than memcpy or for-loops).
// ------------------------------------------------------------------------------------------
void dsShowScreenEmu(void)
{
  videoSetMode(MODE_5_2D);
  vramSetBankA(VRAM_A_MAIN_BG_0x06000000);  // The main emulated (top screen) display.
    
  bg0 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0,0);
  memset((void*)0x06000000, 0x00, 128*1024);

  REG_BG3PA = myConfig.stretch_x;
  REG_BG3PB = 0; REG_BG3PC = 0;
  REG_BG3PD = ((100 / 100)  << 8) | (100 % 100) ;
  REG_BG3X = (myConfig.offset_x) << 8;
  REG_BG3Y = 0;

  dsInitPalette();
  swiWaitForVBlank();
}

// ------------------------------------------------------------------------------------------
// Start Timer 0 - this is our frame timer counter...
// ------------------------------------------------------------------------------------------
void dsInitTimer(void)
{
    TIMER0_DATA=0;
    TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
}

// ------------------------------------------------------------------------------------------
// When we're done with the emulator, we stop the audio... 
// ------------------------------------------------------------------------------------------
void dsFreeEmu(void) 
{
  // Stop timer of sound
  TIMER2_CR=0; irqDisable(IRQ_TIMER2);
}

// ------------------------------------------------------------------------------------------
// When the user asks to QUIT we want to confirm and make sure before we shut down...
// ------------------------------------------------------------------------------------------
bool dsWaitOnQuit(void)
{
  bool bRet=false, bDone=false;
  unsigned int keys_pressed;
  unsigned int posdeb=0;
  static char szName[32];

  dsShowBannerScreen();
    
  strcpy(szName,"Quit NINTV-DS?");
  dsPrintValue(16-strlen(szName)/2,2,0,szName);
  sprintf(szName,"%s","A TO CONFIRM, B TO GO BACK");
  dsPrintValue(16-strlen(szName)/2,23,0,szName);

  while(!bDone)
  {
    strcpy(szName,"          YES          ");
    dsPrintValue(5,10+0,(posdeb == 0 ? 1 :  0),szName);
    strcpy(szName,"          NO           ");
    dsPrintValue(5,14+1,(posdeb == 2 ? 1 :  0),szName);
    swiWaitForVBlank();

    // Check pad
    keys_pressed=keysCurrent();
    if (keys_pressed & KEY_UP) {
      if (posdeb) posdeb-=2;
    }
    if (keys_pressed & KEY_DOWN) {
      if (posdeb<1) posdeb+=2;
    }
    if (keys_pressed & KEY_A) {
      bRet = (posdeb ? false : true);
      bDone = true;
    }
    if (keys_pressed & KEY_B) {
      bDone = true;
    }
  }
    
  dsShowScreenMain(false, false);

  return bRet;
}


// ------------------------------------------------------------------------------------------
// Our main loop! This is the top leel master loop which runs the emulation... we sit in 
// this loop looking for input and calling into the emulation engine to handle 1 frame at
// a time and then render that frame to the DS video screen... it all starts here folks!
// ------------------------------------------------------------------------------------------
ITCM_CODE void Run(char *initial_file)
{
    // Setup 1 second timer for things like FPS
    TIMER1_CR = 0;
    TIMER1_DATA = 0;
    TIMER1_CR=TIMER_ENABLE | TIMER_DIV_1024;
    
    reset_emu_frames();
    
    runState = Running;
    
    // ------------------------------------------------------
    // If we were passed in a file on the command line...
    // ------------------------------------------------------
    if (initial_file)
    {
        if (LoadCart(initial_file)) 
        {
            dsInitPalette();
        }
    }
    
    while(runState == Running) 
    {
        // If we have been told to start up the sound FIFO... do so here...
        if (bStartSoundFifo)
        {
            if (currentRip != NULL)
            {
                dsInstallSoundEmuFIFO();
                bStartSoundFifo = false;
                reset_emu_frames();
            }
        }
    
        // Time 1 frame...
        if (!bMetaSpeedup)
        {
            while(TIMER0_DATA < (target_frame_timing[myConfig.target_fps]*(emu_frames+1)))
                ;
        }

        // Have we processed target (default 60) frames... start over...
        if (++emu_frames >= target_frames[myConfig.target_fps])
        {
            reset_emu_frames();
        }       
        
        //poll the input
        pollInputs();
        
        if (bInitEmulator)  // If the inputs told us we loaded a new file... cleanup and start over...
        {
            InitializeEmulator();
            bInitEmulator = false;
            continue;
        }        

        if (bGameLoaded && !bIsFatalError)
        {
            //run the emulation
            currentEmu->Run();

            // render the output
            currentEmu->Render();
        }
        
        if (TIMER1_DATA >= 32728)   // 1000MS (1 sec)
        {
            char tmp[33];
            TIMER1_CR = 0;
            TIMER1_DATA = 0;
            TIMER1_CR=TIMER_ENABLE | TIMER_DIV_1024;
            oneSecTick=TRUE;
            if ((frames_per_sec_calc > 0) && (myGlobalConfig.show_fps > 0))
            {
                if (frames_per_sec_calc==(target_frames[myConfig.target_fps]+1)) frames_per_sec_calc--;
                tmp[0] = '0' + (frames_per_sec_calc/100);
                tmp[1] = '0' + ((frames_per_sec_calc % 100) /10);
                tmp[2] = '0' + ((frames_per_sec_calc % 100) %10);
                tmp[3] = 0;
                dsPrintFPS(tmp);
            }
            frames_per_sec_calc=0;
            
            // If we've been asked to put out the debug info at the top of the screen... mostly for developer use
            if (myGlobalConfig.show_fps == 2)
            {
                sprintf(tmp,"%-6d %-6d %-5d %-5d", debug[0], debug[1], debug[2], debug[3]);
                //sprintf(tmp,"%04X %04X %04X %04X %02X %02X", debug[0], debug[1], debug[2], debug[3], debug[4], debug[5]);
                dsPrintValue(5,0,0,tmp);
            }
            
            // If we are using the JLP, we call into the 1 second tick function in case there is a dirty flash that needs writing...
            if (bUseJLP)
            {
                currentRip->JLP16Bit->tick_one_second();
            }
        }

#ifdef DEBUG_ENABLE
        debugger();
#endif        
    }
}


// ----------------------------------------------------------------------------------------------
// Entry point called from main(). Optionally can be passed a file to start running - this 
// is necessary for TWL++ which may pass us a .int file on the command line.
// ----------------------------------------------------------------------------------------------
void dsMainLoop(char *initial_file)
{
    // -------------------------------------------------------------------------
    // These are used to write to the DS screen and produce DS audio output...
    // -------------------------------------------------------------------------
    videoBus = new VideoBusDS();
    audioMixer = new AudioMixerDS();
    
    FindAndLoadConfig(CRC32::getCrc(initial_file));
    
    dsShowScreenMain(true, (initial_file ? false:true));    // Don't play the jingle if we are loading a file directly for play
    
    Run(initial_file);      // This will only return if the user opts to QUIT
}

void _putchar(char character) {};   // Not used but needed to link printf()

// End of Line
