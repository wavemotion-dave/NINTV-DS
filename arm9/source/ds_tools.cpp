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
#include <nds/fifomessages.h>

#include<stdio.h>

#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "ds_tools.h"
#include "savestate.h"
#include "config.h"
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

// --------------------------------------------------------
// A set of boolean values so we know what to load and 
// how to load it. These are not accessed often enough
// to warrant putting them in .dtcm fast memory.
// --------------------------------------------------------
bool bStartSoundFifo = false;
bool bUseJLP = false;
bool bForceIvoice=false;
bool bInitEmulator=false;
bool bUseDiscOverlay=false;
bool bGameLoaded = false;

// -------------------------------------------------------------
// This one is accessed rather often so we'll put it in .dtcm
// -------------------------------------------------------------
UINT8 b_dsi_mode __attribute__((section(".dtcm"))) = true;

// ------------------------------------------------------------------------------
// The user can configure how many frames should be run per second (FPS). 
// This is useful for games like Treasure of Tarmin where the user might
// want to run the game at 120% speed to get faster gameplay in the same time.
// ------------------------------------------------------------------------------
UINT16 target_frames[]         __attribute__((section(".dtcm"))) = {60,  66,   72,  78,  84,  90,  999};
UINT32 target_frame_timing[]   __attribute__((section(".dtcm"))) = {546, 496, 454, 420, 390, 364,    0};

// ---------------------------------------------------------------------------------
// Here are the main classes for the emulator, the RIP (game rom), video bus, etc.
// ---------------------------------------------------------------------------------
RunState             runState = Stopped;
Emulator             *currentEmu = NULL;
Rip                  *currentRip = NULL;
VideoBus             *videoBus   = NULL;
AudioMixer           *audioMixer = NULL;

// ---------------------------------------------------------------------------------
// Some emulator frame calcualtions and once/second computations for frame rate...
// ---------------------------------------------------------------------------------
UINT16 emu_frames=0;
UINT16 frames_per_sec_calc=0;
UINT8  oneSecTick=FALSE;


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

class AudioMixerDS : public AudioMixer
{
public:
    AudioMixerDS();
    void resetProcessor();
    void flushAudio();
    void init(UINT32 sampleRate);
    void release();

    UINT16* outputBuffer;
    UINT16  outputBufferSize;
    UINT16  outputBufferWritePosition;
};

AudioMixerDS::AudioMixerDS()
  : outputBuffer(NULL),
    outputBufferSize(0),
    outputBufferWritePosition(0)
{
    this->outputBufferWritePosition = 0;
    this->outputBufferSize = SOUND_SIZE;
}

void AudioMixerDS::resetProcessor()
{
    if (outputBuffer) 
    {
        outputBufferWritePosition = 0;
    }

    fifoSendValue32(FIFO_USER_01,(1<<16) | SOUND_KILL);
   
    // clears the emulator side of the audio mixer
    AudioMixer::resetProcessor();
    
    // restarts the ARM7 side of the audio...
    bStartSoundFifo = true;
    
    // Set the emulation back to the start...
    reset_emu_frames();
}


void AudioMixerDS::init(UINT32 sampleRate)
{
    release();

    outputBufferWritePosition = 0;

    AudioMixer::init(sampleRate);
}

void AudioMixerDS::release()
{
    AudioMixer::release();
}


void AudioMixerDS::flushAudio()
{
    AudioMixer::flushAudio();
}

class VideoBusDS : public VideoBus
{
public:
    VideoBusDS();
    void init(UINT32 width, UINT32 height);
    void release();
    void render();
};

VideoBusDS::VideoBusDS()  { }

void VideoBusDS::init(UINT32 width, UINT32 height)
{
    release();

    // initialize the pixelBuffer
    VideoBus::init(width, height);
}

void VideoBusDS::release()
{
    VideoBus::release();
}

UINT8 renderz[4][4] = 
{
    {1,1,1,1},      // Frameskip Off
    {1,0,1,0},      // Frameskip Odd
    {0,1,0,1},      // Frameskip Even
    {1,0,0,0}       // Frameskip Agressive
};
    
ITCM_CODE void VideoBusDS::render()
{
    frames_per_sec_calc++;
    global_frames++;
    VideoBus::render();

    if (renderz[myConfig.frame_skip_opt][global_frames&3])
    {
        UINT8 chan = 0;
        UINT32 *ds_video=(UINT32*)0x06000000;
        UINT32 *source_video = (UINT32*)pixelBuffer;

        for (int j=0; j<192; j++)
        {
            while (dmaBusy(chan)) asm("nop");
            dmaCopyWordsAsynch (chan, source_video, ds_video, 160);
            source_video += 40;
            ds_video += 64;
            chan = (chan + 1) & 3;
        }    
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
        dsPrintValue(0,1,0, (char*) "NO BIOS FILES");
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
        if (loaded) {
            //peripheral loaded, might as well use it.
            currentEmu->UsePeripheral(i, TRUE);
        }
        else if (usage == PERIPH_OPTIONAL) {
            //didn't load, but the peripheral is optional, so just skip it
            currentEmu->UsePeripheral(i, FALSE);
        }
        else //usage == PERIPH_REQUIRED, but it didn't load
        {
            dsPrintValue(0,1,0, (char*) "NO IVOICE.BIN");
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
    currentEmu->SetRip(currentRip);
    
    // Load up the fast ROM memory for quick fetches
    currentEmu->LoadFastMemory();
    
    //Reset everything
    currentEmu->Reset();
    
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
void HandleScreenStretch(void)
{
    char tmpStr[33];
    
    dsShowMenu();
    swiWaitForVBlank();
    
    dsPrintValue(2, 5, 0,  (char*)"PRESS UP/DN TO STRETCH SCREEN");
    dsPrintValue(2, 7, 0,  (char*)"LEFT/RIGHT TO SHIFT SCREEN");
    dsPrintValue(2, 15, 0, (char*)"PRESS X TO RESET TO DEFAULTS");
    dsPrintValue(1, 16,0,  (char*)"START to SAVE, PRESS B TO EXIT");
    
    bool bDone = false;
    int last_stretch_x = -99;
    int last_offset_x = -99;
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

// -------------------------------------------------------------------------------------
// The main menu provides a full set of options the user can pick. We started to run
// out of room on the main screen to show all the possible things the user can pick
// so we now just store all the extra goodies in this menu... By default the SELECT
// button will bring this up.
// -------------------------------------------------------------------------------------
#define MAIN_MENU_ITEMS 10
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
    "QUIT EMULATOR",  
    "EXIT THIS MENU",  
};


int menu_entry(void)
{
    UINT8 current_entry = 0;
    extern int bg0, bg0b, bg1b;
    char bDone = 0;

    dsShowMenu();
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
                        return OVL_META_QUIT;
                        break;
                    case 9:
                        bDone=1;
                        break;
                }
            }
            
            if (keys_pressed & KEY_B)
            {
                bDone=1;
            }
            swiWaitForVBlank();
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
        case OVL_META_RESET:
            if (bGameLoaded)
            {
                currentEmu->Reset();
                // And put the Sound Fifo back at the start...
                bStartSoundFifo = true;

                // Make sure we're starting fresh...
                reset_emu_frames();
            }                
            break;
 
        case OVL_META_LOAD:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (dsWaitForRom(newFile))
            {
                if (LoadCart(newFile)) 
                {
                    dsInitPalette();
                }
                else
                {
                    dsPrintValue(0,1,0, (char*) "UNKNOWN GAME ");
                }
            }
            bStartSoundFifo = true;
            break;
  
        case OVL_META_CONFIG:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (currentRip != NULL) 
            {
                dsChooseOptions(0);
                reset_emu_frames();
                dsInitPalette();
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            bStartSoundFifo = true;
            break;

        case OVL_META_GCONFIG:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            dsChooseOptions(1);
            reset_emu_frames();
            dsInitPalette();
            WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            bStartSoundFifo = true;
            break;
            
        case OVL_META_SCORES:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (currentRip != NULL) 
            {
                highscore_display(currentRip->GetCRC());
                dsShowScreenMain(false);
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            bStartSoundFifo = true;
            break;

        case OVL_META_STATE:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (currentRip != NULL) 
            {
                savestate_entry();      
                dsShowScreenMain(false);
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            bStartSoundFifo = true;
            break;

        case OVL_META_MENU:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            ds_handle_meta(menu_entry());
            dsShowScreenMain(false);
            WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            bStartSoundFifo = true;
            break;

        case OVL_META_SWITCH:
            if (currentRip != NULL) 
            {
                myConfig.controller_type = 1-myConfig.controller_type;
            }
            break;
            
        case OVL_META_MANUAL:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (currentRip != NULL) 
            {
                dsShowManual();
                dsShowScreenMain(false);
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            bStartSoundFifo = true;
            break;
            
        case OVL_META_STRETCH:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (currentRip != NULL) 
            {
                HandleScreenStretch();
                dsShowScreenMain(false);
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            bStartSoundFifo = true;
            break;
            
        case OVL_META_QUIT:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (dsWaitOnQuit()){ runState = Quit; }
            bStartSoundFifo = true;
            break;
    }
}

// -------------------------------------------------------------------------------------------------
// Read the DS keys for input and handle it... this could be meta commands (LOAD, QUIT, CONFIG) or
// it could be actual intellivision keypad emulation (KEY_1, KEY_2, DISC movement, etc).
// This is called every frame - so 60 times per second which is fairly responsive...
// -------------------------------------------------------------------------------------------------
ITCM_CODE void pollInputs(void)
{
    UINT16 ctrl_disc, ctrl_keys, ctrl_side;
    extern int ds_key_input[3][16];   // Set to '1' if pressed... 0 if released
    extern int ds_disc_input[3][16];  // Set to '1' if pressed... 0 if released.
    unsigned short keys_pressed = keysCurrent();
    static short last_pressed = -1;

    for (int j=0; j<3; j++)
    {
        for (int i=0; i<15; i++) ds_key_input[j][i] = 0;
        for (int i=0; i<16; i++) ds_disc_input[j][i] = 0;
    }
    
    // Check for Dual Action
    if (myConfig.controller_type == 2)  // Standard Dual-Action (disc and side buttons on controller #1... keypad from controller #2)
    {
        ctrl_disc = 0;
        ctrl_side = 0;
        ctrl_keys = 1;
    }
    else if (myConfig.controller_type == 3) // Same as #2 above except side-buttons use controller #2
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

    if (myConfig.dpad_config == 0)  // Normal handling
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
    else if (myConfig.dpad_config == 1) // Reverse Left/Right
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
    else if (myConfig.dpad_config == 2)  // Reverse Up/Down
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
    else if (myConfig.dpad_config == 3)  // Diagnoals
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
    else if (myConfig.dpad_config == 4)  // Strict 4-way
    {
        if (keys_pressed & KEY_UP)    
        {
            ds_disc_input[ctrl_disc][0]  = 1;
        }
        else if (keys_pressed & KEY_DOWN)
        {
            ds_disc_input[ctrl_disc][8]  = 1;
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
    
    // -------------------------------------------------------------------------------------
    // Now handle the main DS keys... these can be re-mapped to any Intellivision function
    // -------------------------------------------------------------------------------------
    if ((keys_pressed & KEY_A) && (keys_pressed & KEY_X))
    {
        if (myConfig.key_AX_map >= OVL_META_RESET)
        {
            if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_AX_map);
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
            if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_XY_map);
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
            if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_YB_map);
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
            if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_BA_map);
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
                if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_A_map);
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
                if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_B_map);
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
                if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_X_map);
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
                if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_Y_map);
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
            if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_L_map);
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
            if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_R_map);
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
            if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_START_map);
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
            if (last_pressed != keys_pressed) ds_handle_meta(myConfig.key_SELECT_map);
        }
        else if (myConfig.key_SELECT_map >= OVL_BTN_FIRE) 
            ds_key_input[ctrl_side][myConfig.key_SELECT_map]  = 1;
        else
            ds_key_input[ctrl_keys][myConfig.key_SELECT_map]  = 1;
    }
    
    last_pressed = keys_pressed;
    
    // -----------------------------------------------------------------
    // Now handle the on-screen Intellivision overlay and meta keys...
    // -----------------------------------------------------------------
    if (keys_pressed & KEY_TOUCH)
    {
        touchPosition touch;
        touchRead(&touch);
        
#ifdef DEBUG_ENABLE
        debugger_input(touch.px, touch.py);
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
            if (touch.px > myOverlay[i].x1  && touch.px < myOverlay[i].x2 && touch.py > myOverlay[i].y1 && touch.py < myOverlay[i].y2)   ds_key_input[ctrl_keys][i] = 1;
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
    }
    
}


// ---------------------------------------------------------------------------
// This is called very frequently (15,360 times per second) to fill the
// pipeline of sound values from the Audio Mixer into the Nintendo DS sound
// buffer which will be processed in the background by the ARM 7 processor.
// ---------------------------------------------------------------------------
UINT32 audio_arm7_xfer_buffer __attribute__ ((aligned (4))) = 0;
UINT32* aptr __attribute__((section(".dtcm"))) = (UINT32*) (&audio_arm7_xfer_buffer + 0xA000000/4);
UINT16 myCurrentSampleIdx16  __attribute__((section(".dtcm"))) = 0;
UINT8  myCurrentSampleIdx8   __attribute__((section(".dtcm"))) = 0;    
UINT16 sample[2]    __attribute__((section(".dtcm"))) __attribute__ ((aligned (4)));    

ITCM_CODE void VsoundHandlerDSi(void)
{
  // If there is a fresh sample...
  if (myCurrentSampleIdx8 != currentSampleIdx8)
  {
      sample[0] = audio_mixer_buffer[myCurrentSampleIdx8++]; 
      sample[1] = sample[0];
      *aptr = *((UINT32*)&sample);
  }
}

ITCM_CODE void VsoundHandler(void)
{
  // If there is a fresh sample...
  if (myCurrentSampleIdx16 != currentSampleIdx16)
  {
      sample[0] = audio_mixer_buffer[myCurrentSampleIdx16++]; 
      sample[1] = sample[0];
      *aptr = *((UINT32*)&sample);
      if (myCurrentSampleIdx16 == SOUND_SIZE) myCurrentSampleIdx16=0;
  }
}


void dsInstallSoundEmuFIFO(void)
{
    // Clear out the sound buffers...
    currentSampleIdx8 = 0;
    currentSampleIdx16 = 0;
    
    // -----------------------------------------------------------------------------
    // We are going to move out the memory access to the non-cached mirros since
    // this audio buffer is shared between ARM7 and ARM9 and we want to ensure
    // that the data is never cached as we want the ARM7 to have updated values.
    // -----------------------------------------------------------------------------
    if (isDSiMode())
    {
        b_dsi_mode = true;
        aptr = (UINT32*) (&audio_arm7_xfer_buffer + 0xA000000/4);
    }
    else
    {
        b_dsi_mode = false;
        aptr = (UINT32*) (&audio_arm7_xfer_buffer + 0x00400000/4);
    }
    
    *aptr = 0x00000000;
    memset(audio_mixer_buffer, 0x00, 256 * sizeof(UINT16));
    
    fifoSendValue32(FIFO_USER_01,(1<<16) | SOUND_KILL);
    swiWaitForVBlank();swiWaitForVBlank();    // Wait 2 vertical blanks... that's enough for the ARM7 to stop...
    
    FifoMessage msg;
    msg.SoundPlay.data = &audio_arm7_xfer_buffer;
    msg.SoundPlay.freq = mySoundFrequency*2;
    msg.SoundPlay.volume = 127;
    msg.SoundPlay.pan = 64;
    msg.SoundPlay.loop = 1;
    msg.SoundPlay.format = ((1)<<4) | SoundFormat_16Bit;
    msg.SoundPlay.loopPoint = 0;
    msg.SoundPlay.dataSize = (4 >> 2);  // Just 1 32-bit value
    msg.type = EMUARM7_PLAY_SND;
    fifoSendDatamsg(FIFO_USER_01, sizeof(msg), (u8*)&msg);
    
    swiWaitForVBlank();swiWaitForVBlank();    // Wait 2 vertical blanks... that's enough for the ARM7 to start chugging...

    // Now setup to feed the audio mixer buffer into the ARM7 core via shared memory
    UINT16 tFreq = mySoundFrequency;
    if (target_frames[myConfig.target_fps] != 999)  // If not running MAX, adjust sound to match speed emulator is running at.
    {
        tFreq = (UINT16)(((UINT32)mySoundFrequency * (UINT32)target_frames[myConfig.target_fps]) / (UINT32)60) + 2;
    }
    TIMER2_DATA = TIMER_FREQ(tFreq);
    TIMER2_CR = TIMER_DIV_1 | TIMER_IRQ_REQ | TIMER_ENABLE;
    irqSet(IRQ_TIMER2, b_dsi_mode ? VsoundHandlerDSi:VsoundHandler);
    
    if (myConfig.sound_clock_div != SOUND_DIV_DISABLE)
        irqEnable(IRQ_TIMER2);
    else
        irqDisable(IRQ_TIMER2);
    
}


UINT32 muted_gamePalette[32] = 
{
    0x000000,  0x0021AD,  0xE03904,  0xCECE94,
    0x1E4912,  0x01812E,  0xF7E64A,  0xFFFFFF,
    0xA5ADA5,  0x51B7E5,  0xEF9C00,  0x424A08,
    0xFF3173,  0x9A8AEF,  0x4AA552,  0x950457,
    
    0x000000,  0x0021AD,  0xE03904,  0xCECE94,
    0x1E4912,  0x01812E,  0xF7E64A,  0xFFFFFF,
    0xA5ADA5,  0x51B7E5,  0xEF9C00,  0x424A08,
    0xFF3173,  0x9A8AEF,  0x4AA552,  0x950457,
};

UINT32 bright_gamePalette[32] = 
{
    0x000000,    0x1438F7,    0xE35B0E,    0xCBF168,
    0x009428,    0x07C200,    0xFFFF01,    0xFFFFFF,
    0xC8C8C8,    0x23CEC3,    0xFD9918,    0x3A8A00,
    0xF0463C,    0xD383FF,    0x7CED00,    0xB81178,
    
    0x000000,    0x1438F7,    0xE35B0E,    0xCBF168,
    0x009428,    0x07C200,    0xFFFF01,    0xFFFFFF,
    0xC8C8C8,    0x23CEC3,    0xFD9918,    0x3A8A00,
    0xF0463C,    0xD383FF,    0x7CED00,    0xB81178,
};

UINT32 pal_gamePalette[32] = 
{
    0x000000,  0x0075FF,  0xFF4C39,  0xD1B951,
    0x09B900,  0x30DF10,  0xFFE501,  0xFFFFFF,
    0x8C8C8C,  0x28E5C0,  0xFFA02E,  0x646700,
    0xFF29FF,  0x8C8FFF,  0x7CED00,  0xC42BFC,
    
    0x000000,  0x0075FF,  0xFF4C39,  0xD1B951,
    0x09B900,  0x30DF10,  0xFFE501,  0xFFFFFF,
    0x8C8C8C,  0x28E5C0,  0xFFA02E,  0x646700,
    0xFF29FF,  0x8C8FFF,  0x7CED00,  0xC42BFC,
};


UINT32 default_Palette[32] = 
{
    // Optmized
    0x000000,  0x002DFF,  0xFF3D10,  0xC8D271,
    0x386B3F,  0x00A756,  0xFAEA50,  0xFFFFFF,
    0x9B9DA1,  0x24B8FF,  0xFFB41F,  0x3D5A02,
    0xFF4E57,  0xA496FF,  0x75CC80,  0xB51A58,
     
    0x000000,  0x002DFF,  0xFF3D10,  0xC8D271,
    0x386B3F,  0x00A756,  0xFAEA50,  0xFFFFFF,
    0x9B9DA1,  0x24B8FF,  0xFFB41F,  0x3D5A02,
    0xFF4E57,  0xA496FF,  0x75CC80,  0xB51A58,
};

UINT32 custom_Palette[32] = 
{
    // Optmized
    0x000000,  0x002DFF,  0xFF3D10,  0xC8D271,
    0x386B3F,  0x00A756,  0xFAEA50,  0xFFFFFF,
    0x9B9DA1,  0x24B8FF,  0xFFB41F,  0x3D5A02,
    0xFF4E57,  0xA496FF,  0x75CC80,  0xB51A58,
     
    0x000000,  0x002DFF,  0xFF3D10,  0xC8D271,
    0x386B3F,  0x00A756,  0xFAEA50,  0xFFFFFF,
    0x9B9DA1,  0x24B8FF,  0xFFB41F,  0x3D5A02,
    0xFF4E57,  0xA496FF,  0x75CC80,  0xB51A58,
};


const INT8 brightness[] = {0, -3, -6, -9};
void dsInitPalette(void) 
{
    extern char szName[];
    static UINT8 bFirstTime = TRUE;
    
    if (bFirstTime)
    {
        // Load Custom Palette (if it exists)
        FILE *fp;
        fp = fopen("/data/NINTV-DS.PAL", "r");
        if (fp != NULL)
        {
            UINT8 pal_idx=0;
            while (!feof(fp))
            {
                fgets(szName, 255, fp);
                if (!feof(fp))
                {
                    if (strstr(szName, "#") != NULL)
                    {
                        char *ptr = szName;
                        while (*ptr == ' ' || *ptr == '#') ptr++;
                        custom_Palette[pal_idx] = strtoul(ptr, &ptr, 16);
                        custom_Palette[pal_idx+16] = custom_Palette[pal_idx];
                        if (pal_idx < 16) pal_idx++;
                    }
                }
            }
            fclose(fp);   
        }
        bFirstTime = FALSE;
    }
    if (!bGameLoaded) return;
    
    // Init DS Specific palette for the Intellivision (16 colors...)
    for(int i = 0; i < 256; i++)   
    {
        unsigned char r, g, b;

        switch (myConfig.palette)
        {
            case 1: // MUTED
                r = (unsigned char) ((muted_gamePalette[i%32] & 0x00ff0000) >> 19);
                g = (unsigned char) ((muted_gamePalette[i%32] & 0x0000ff00) >> 11);
                b = (unsigned char) ((muted_gamePalette[i%32] & 0x000000ff) >> 3);
                break;
            case 2: // BRIGHT
                r = (unsigned char) ((bright_gamePalette[i%32] & 0x00ff0000) >> 19);
                g = (unsigned char) ((bright_gamePalette[i%32] & 0x0000ff00) >> 11);
                b = (unsigned char) ((bright_gamePalette[i%32] & 0x000000ff) >> 3);
                break;
            case 3: // PAL
                r = (unsigned char) ((pal_gamePalette[i%32] & 0x00ff0000) >> 19);
                g = (unsigned char) ((pal_gamePalette[i%32] & 0x0000ff00) >> 11);
                b = (unsigned char) ((pal_gamePalette[i%32] & 0x000000ff) >> 3);
                break;
            case 4: // CUSTOM
                r = (unsigned char) ((custom_Palette[i%32] & 0x00ff0000) >> 19);
                g = (unsigned char) ((custom_Palette[i%32] & 0x0000ff00) >> 11);
                b = (unsigned char) ((custom_Palette[i%32] & 0x000000ff) >> 3);
                break;
            default:
                r = (unsigned char) ((default_Palette[i%32] & 0x00ff0000) >> 19);
                g = (unsigned char) ((default_Palette[i%32] & 0x0000ff00) >> 11);
                b = (unsigned char) ((default_Palette[i%32] & 0x000000ff) >> 3);
                break;
        }

        BG_PALETTE[i]=RGB15(r,g,b);
    }
    
    setBrightness(2, brightness[myGlobalConfig.brightness]);      // Subscreen Brightness
}


// ------------------------------------------------------------------------------
// This shows the main upper screen which is where the emulation takes place... 
// the lower screen is used for overlay and debugger information.
// ------------------------------------------------------------------------------
void dsShowScreenMain(bool bFull) 
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
    }

    // Now show the bottom screen - usualy some form of overlay...
#ifdef DEBUG_ENABLE
    show_debug_overlay();
#else
    show_overlay();
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
    vramSetBankB(VRAM_B_LCD );                // Not using this for video but 128K of faster RAM always useful! Mapped at 0x06820000 - Used for Memory Bus Read Counter
    vramSetBankD(VRAM_D_LCD );                // Not using this for video but 128K of faster RAM always useful! Mapped at 0x06860000 - Used for Custom Tile Overlay Buffer
    vramSetBankE(VRAM_E_LCD );                // Not using this for video but 64K of faster RAM always useful!  Mapped at 0x06880000 - Used for Cart "Fast Buffer" 64k x 16 = 128k (so also spills into F,G,H below)
    vramSetBankF(VRAM_F_LCD );                // Not using this for video but 16K of faster RAM always useful!  Mapped at 0x06890000 -   ..
    vramSetBankG(VRAM_G_LCD );                // Not using this for video but 16K of faster RAM always useful!  Mapped at 0x06894000 -   ..
    vramSetBankH(VRAM_H_LCD );                // Not using this for video but 32K of faster RAM always useful!  Mapped at 0x06898000 -   ..
    vramSetBankI(VRAM_I_LCD );                // Not using this for video but 16K of faster RAM always useful!  Mapped at 0x068A0000 - Used for Custom Tile Map Buffer
    
    WAITVBL;
}


// ---------------------------------------------------------------------------------------------------
// Generic show-menu which is just the lower screen that is mostly blank with the NINTELLIVISION 
// banner at the top. The menu has 2 font choices... a high-contrast white on black and a more 
// retro feel green on black (default). The user can change this option in global configuration.
// ---------------------------------------------------------------------------------------------
void dsShowMenu(void)
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
ITCM_CODE void dsInitTimer(void)
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
  char szName[32];

  dsShowMenu();
    
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
    
  dsShowScreenMain(false);

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
            dsInstallSoundEmuFIFO();
            bStartSoundFifo = false;
            reset_emu_frames();
        }
    
        // Time 1 frame...
        while(TIMER0_DATA < (target_frame_timing[myConfig.target_fps]*(emu_frames+1)))
            ;

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

        if (bGameLoaded)
        {
            //run the emulation
            currentEmu->Run();

            // render the output
            currentEmu->Render();
        }
        
        if (TIMER1_DATA >= 32728)   // 1000MS (1 sec)
        {
            TIMER1_CR = 0;
            TIMER1_DATA = 0;
            TIMER1_CR=TIMER_ENABLE | TIMER_DIV_1024;
            oneSecTick=TRUE;
            if ((frames_per_sec_calc > 0) && (myGlobalConfig.show_fps > 0))
            {
                char tmp[15];
                if (frames_per_sec_calc==(target_frames[myConfig.target_fps]+1)) frames_per_sec_calc--;
                tmp[0] = '0' + (frames_per_sec_calc/100);
                tmp[1] = '0' + ((frames_per_sec_calc % 100) /10);
                tmp[2] = '0' + ((frames_per_sec_calc % 100) %10);
                tmp[3] = 0;
                dsPrintValue(0,0,0,tmp);
            }
            frames_per_sec_calc=0;
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
    
    FindAndLoadConfig();
    dsShowScreenMain(true);
    
    Run(initial_file);      // This will only return if the user opts to QUIT
}

// End of Line
