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
#include "bgBottom-treasure.h"
#include "bgBottom-cloudy.h"
#include "bgBottom-astro.h"
#include "bgBottom-spartans.h"
#include "bgBottom-b17.h"
#include "bgBottom-atlantis.h"
#include "bgBottom-bombsquad.h"
#include "bgBottom-utopia.h"
#include "bgBottom-swords.h"
#include "bgTop.h"
#include "bgFileSel.h"
#include "bgHighScore.h"
#include "bgOptions.h"

#include "Emulator.h"
#include "Rip.h"
#include "highscore.h"

#define DEBUG_ENABLE 0

BOOL InitializeEmulator(void);

typedef enum _RunState 
{
    Stopped,
    Paused,
    Running,
    Quit
} RunState;

bool bStartSoundFifo = false;
bool bUseJLP = false;
bool bForceIvoice=false;
bool bInitEmulator=false;

RunState             runState = Stopped;
Emulator             *currentEmu = NULL;
Rip                  *currentRip = NULL;
VideoBus             *videoBus   = NULL;
AudioMixer           *audioMixer = NULL;

int debug1, debug2;
UINT16 emu_frames=0;
UINT16 frames=0;


struct Overlay_t defaultOverlay[OVL_MAX] =
{
    {120,   155,    30,     60},    // KEY_1
    {158,   192,    30,     60},    // KEY_2
    {195,   230,    30,     60},    // KEY_3
    
    {120,   155,    65,     95},    // KEY_4
    {158,   192,    65,     95},    // KEY_5
    {195,   230,    65,     95},    // KEY_6

    {120,   155,    101,   135},    // KEY_7
    {158,   192,    101,   135},    // KEY_8
    {195,   230,    101,   135},    // KEY_9

    {120,   155,    140,   175},    // KEY_CLEAR
    {158,   192,    140,   175},    // KEY_0
    {195,   230,    140,   175},    // KEY_ENTER

    {255,   255,    255,   255},    // KEY_FIRE
    {255,   255,    255,   255},    // KEY_L_ACT
    {255,   255,    255,   255},    // KEY_R_ACT
    
    { 23,    82,     16,    36},    // META_RESET
    { 23,    82,     45,    65},    // META_LOAD
    { 23,    82,     74,    94},    // META_CONFIG
    { 23,    82,    103,   123},    // META_SCORE
    {255,   255,    255,   255},    // META_QUIT
    { 23,    82,    132,   152},    // META_STATE
    { 23,    82,    161,   181},    // META_MENU
    {255,   255,    255,   255},    // META_SWAP
    {255,   255,    255,   255},    // META_MANUAL
};

struct Overlay_t myOverlay[OVL_MAX];

void dsInitPalette(void);

int bg0, bg0b, bg1b;
bool bFirstGameLoaded = false;

ITCM_CODE void dsPrintValue(int x, int y, unsigned int isSelect, char *pchStr)
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
    bStartSoundFifo=true;
    
	// clears the emulator side of the audio mixer
	AudioMixer::resetProcessor();
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

ITCM_CODE void VideoBusDS::render()
{
    frames++;
	VideoBus::render();

    // Any level of frame skip will skip the render()
    if (myConfig.frame_skip_opt)
    {
        if ((frames & 1) == (myConfig.frame_skip_opt==1 ? 1:0)) return;        // Skip ODD or EVEN Frames as configured
    }
    UINT32 *ds_video=(UINT32*)0x06000000;
    UINT32 *source_video = (UINT32*)pixelBuffer;
    
    for (int j=0; j<192; j++)
    {
        memcpy(ds_video, source_video, 160);
        ds_video += 64;
        source_video += 40;
    }    
}


BOOL LoadCart(const CHAR* filename)
{
    if (strlen(filename) < 5)
        return FALSE;

    //convert .bin and .rom to .rip, since our emulation only knows how to run .rip
    const CHAR* extStart = filename + strlen(filename) - 4;
    if (strcmpi(extStart, ".int") == 0 || strcmpi(extStart, ".bin") == 0)
    {
        //load the bin file as a Rip - use internal database or maybe <filename>.cfg exists... LoadBin() handles all that.
        currentRip = Rip::LoadBin(filename);
        if (currentRip == NULL)
        {
            return FALSE;
        }
    }
    else if (strcmpi(extStart, ".rom") == 0)    // .rom files contain the loading info...
    {
        //load the bin file as a Rip
        currentRip = Rip::LoadRom(filename);
        if (currentRip == NULL)
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    // ---------------------------------------------------------------------
    // New game is loaded... (would have returned FALSE above otherwise)
    // ---------------------------------------------------------------------
    extern UINT32 fudge_timing;
    extern UINT8 bLatched;
    fudge_timing = 0;
    bLatched = false;
    if (currentRip->GetCRC() == 0x5F6E1AF6) fudge_timing = 1000;    // Motocross needs some fudge timing to run... known race condition...
    if (currentRip->GetCRC() == 0x2DEACD15) bLatched = true;        // Stampede must have latched backtab access
    if (currentRip->GetCRC() == 0x573B9B6D) bLatched = true;        // Masters of the Universe must have latched backtab access
    
    FindAndLoadConfig();
    dsShowScreenEmu();
    dsShowScreenMain(false);
    
    bFirstGameLoaded = TRUE;    
    bInitEmulator = true;    
    
    dsInstallSoundEmuFIFO();
    WAITVBL;WAITVBL;
    
    return TRUE;
}

BOOL LoadPeripheralRoms(Peripheral* peripheral)
{
	UINT16 count = peripheral->GetROMCount();
	for (UINT16 i = 0; i < count; i++) {
		ROM* r = peripheral->GetROM(i);
		if (r->isLoaded())
			continue;

        CHAR nextFile[MAX_PATH];
        
        if (myGlobalConfig.bios_dir == 1)        // In: /ROMS/BIOS
        {
            strcpy(nextFile, "/roms/bios/");
        }
        else if (myGlobalConfig.bios_dir == 2)   // In: /ROMS/INTV/BIOS
        {
            strcpy(nextFile, "/roms/intv/bios/");
        }
        else if (myGlobalConfig.bios_dir == 3)   // In: /DATA/BIOS/
        {
            strcpy(nextFile, "/data/bios/");
        }
        else
        {
            strcpy(nextFile, "./");              // In: Same DIR as ROM files
        }

        strcat(nextFile, r->getDefaultFileName());
        if (!r->load(nextFile, r->getDefaultFileOffset())) 
        {
            return FALSE;
        }
	}

    return TRUE;
}

void reset_emu_frames(void)
{
    TIMER0_CR=0;
    TIMER0_DATA=0;
    TIMER0_CR=TIMER_ENABLE | TIMER_DIV_1024;
    emu_frames=0;
}

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
    currentEmu->InitAudio(audioMixer, SOUND_FREQ);
    
    // Clear the audio mixer...
    audioMixer->resetProcessor();

    //put the RIP in the currentEmulator
	currentEmu->SetRip(currentRip);
    
    // Load up the fast ROM memory for quick fetches
    currentEmu->LoadFastMemory();
    
    //Reset everything
    currentEmu->Reset();
    
    // And put the Sound Fifo back at the start...
    bStartSoundFifo = true;
    
    // Make sure we're starting fresh...
    reset_emu_frames();

    return TRUE;
}


#define MAIN_MENU_ITEMS 8
const char *main_menu[MAIN_MENU_ITEMS] = 
{
    "RESET EMULATOR",  
    "LOAD NEW GAME",  
    "GAME CONFIG",  
    "GAME SCORES",  
    "SAVE/RESTORE STATE",  
    "GAME MANUAL",  
    "QUIT EMULATOR",  
    "EXIT THIS MENU",  
};


int menu_entry(void)
{
    UINT8 current_entry = 0;
    extern int bg0, bg0b, bg1b;
    char bDone = 0;

    decompress(bgHighScoreTiles, bgGetGfxPtr(bg0b), LZ77Vram);
    decompress(bgHighScoreMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
    dmaCopy((void *) bgHighScorePal,(u16*) BG_PALETTE_SUB,256*2);
    unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
    dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
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
                        return OVL_META_QUIT;
                        break;
                    case 7:
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
            if (bFirstGameLoaded)
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
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
            break;
  
        case OVL_META_CONFIG:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            dsChooseOptions();
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
            reset_emu_frames();
            dsInitPalette();
            WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            break;

        case OVL_META_SCORES:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (currentRip != NULL) 
            {
                highscore_display(currentRip->GetCRC());
                dsShowScreenMain(false);
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
            break;

        case OVL_META_STATE:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (currentRip != NULL) 
            {
                savestate_entry();      
                dsShowScreenMain(false);
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
            break;

        case OVL_META_MENU:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (currentRip != NULL) 
            {
                ds_handle_meta(menu_entry());
                dsShowScreenMain(false);
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
            break;

        case OVL_META_SWITCH:
            myConfig.controller_type = 1-myConfig.controller_type;
            break;
            
        case OVL_META_MANUAL:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (currentRip != NULL) 
            {
                dsShowManual();
                dsShowScreenMain(false);
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
            break;
            
        case OVL_META_QUIT:
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (dsWaitOnQuit()){ runState = Quit; }
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
            break;
    }
}

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

int target_frames[]         = {60,  66,   72,  78,  84,  90,  999};
int target_frame_timing[]   = {546, 496, 454, 420, 390, 364,    0};

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
            frames=0;
            bInitEmulator = false;
            continue;
        }        

        if (bFirstGameLoaded)
        {
            //flush the audio  -  normaly this would be done AFTER run/render but this gives us maximum accuracy on audio timing
            if (!bStartSoundFifo)
            {
                currentEmu->FlushAudio();
            }
            
            //run the emulation
            currentEmu->Run();

            // render the output
            currentEmu->Render();
            
            // Trick to start the fifo after the first round of sound is in the buffer...
            if (bStartSoundFifo)
            {
                bStartSoundFifo = false;
                dsInstallSoundEmuFIFO();
            }
        }
        
        if (TIMER1_DATA >= 32728)   // 1000MS (1 sec)
        {
            char tmp[15];
            TIMER1_CR = 0;
            TIMER1_DATA = 0;
            TIMER1_CR=TIMER_ENABLE | TIMER_DIV_1024;
            if ((frames > 0) && (myGlobalConfig.show_fps > 0))
            {
                if (frames==(target_frames[myConfig.target_fps]+1)) frames--;
                sprintf(tmp, "%03d", frames);
                dsPrintValue(0,0,0,tmp);
            }
            frames=0;
            debug1++;
            sprintf(tmp, "%4d %4d", debug1, debug2);
            if (DEBUG_ENABLE==1) dsPrintValue(0,1,0,tmp);

            // ------------------------------------------------
            // Hack for Q*Bert...Keep life counter set to 3
            // to avoid original Bliss core bug for this game.
            // ------------------------------------------------
            if (currentRip)
            {
                if (currentRip->GetCRC() == 0xD8C9856A)
                {
                    currentEmu->memoryBus.poke(0x173, 3);
                }
            }
            
        }
    }
}


unsigned int customTiles[24*1024];
unsigned short *customMap = (unsigned short *)0x068A0000; //16K of video memory
unsigned short customPal[512];
char line[256];

void load_custom_overlay(void)
{
    char filename[128];
    FILE *fp = NULL;
    // Read the associated .ovl file and parse it...
    if (currentRip != NULL)
    {
        if (myGlobalConfig.ovl_dir == 1)        // In: /ROMS/OVL
        {
            strcpy(filename, "/roms/ovl/");
        }
        else if (myGlobalConfig.ovl_dir == 2)   // In: /ROMS/INTY/OVL
        {
            strcpy(filename, "/roms/intv/ovl/");
        }
        else if (myGlobalConfig.ovl_dir == 3)   // In: /DATA/OVL/
        {
            strcpy(filename, "/data/ovl/");
        }
        else
        {
            strcpy(filename, "./");              // In: Same DIR as ROM files
        }
        strcat(filename, currentRip->GetFileName());
        filename[strlen(filename)-4] = 0;
        strcat(filename, ".ovl");
        fp = fopen(filename, "rb");
    }
    if (fp != NULL)
    {
      int ov_idx = 0;
      int tiles_idx=0;
      int map_idx=0;
      int pal_idx=0;
      char *token;

      memset(customTiles, 0x00, 24*1024*sizeof(UINT32));
      memset(customMap, 0x00, 16*1024*sizeof(UINT16));
      memset(customPal, 0x00, 512*sizeof(UINT16));

      do
      {
        fgets(line, 255, fp);
        // Handle Overlay Line
        if (strstr(line, ".ovl") != NULL)
        {
            if (ov_idx < OVL_MAX)
            {
                char *ptr = strstr(line, ".ovl");
                ptr += 5;
                myOverlay[ov_idx].x1 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myOverlay[ov_idx].x2 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myOverlay[ov_idx].y1 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myOverlay[ov_idx].y2 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                ov_idx++;                
            }
        }

        // Handle Tile Line
        if (strstr(line, ".tile") != NULL)
        {
            char *ptr = strstr(line, ".tile");
            ptr += 6;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
        }

        // Handle Map Line
        if (strstr(line, ".map") != NULL)
        {
            char *ptr = strstr(line, ".map");
            ptr += 4;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
        }              

        // Handle Palette Line
        if (strstr(line, ".pal") != NULL)
        {
            char *ptr = strstr(line, ".pal");
            ptr += 4;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
        }              
      } while (!feof(fp));
      fclose(fp);

      decompress(customTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(customMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) customPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else
    {
      decompress(bgBottomTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottomMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottomPal,(u16*) BG_PALETTE_SUB,256*2);
    }    
}

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

    // Assume default overlay... custom can change it below...
    memcpy(&myOverlay, &defaultOverlay, sizeof(myOverlay));
    
    swiWaitForVBlank();
    
    if (myConfig.overlay_selected == 1) // Custom Overlay!
    {
        load_custom_overlay();
    }
    else if (myConfig.overlay_selected == 2) // Treasure of Tarmin
    {
      decompress(bgBottom_treasureTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_treasureMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_treasurePal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 3) // Cloudy Mountain
    {
      decompress(bgBottom_cloudyTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_cloudyMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_cloudyPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 4) // Astrosmash
    {
      decompress(bgBottom_astroTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_astroMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_astroPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 5) // Space Spartans
    {
      decompress(bgBottom_spartansTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_spartansMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_spartansPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 6) // B17 Bomber
    {
      decompress(bgBottom_b17Tiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_b17Map, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_b17Pal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 7) // Atlantis
    {
      decompress(bgBottom_atlantisTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_atlantisMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_atlantisPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 8) // Bomb Squad
    {
      decompress(bgBottom_bombsquadTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_bombsquadMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_bombsquadPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 9) // Utopia
    {
      decompress(bgBottom_utopiaTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_utopiaMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_utopiaPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 10) // Swords & Serpents
    {
      decompress(bgBottom_swordsTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_swordsMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_swordsPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else    // Default Overlay...
    {
      decompress(bgBottomTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottomMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottomPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    
    unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
    dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

    REG_BLDCNT=0; REG_BLDCNT_SUB=0; REG_BLDY=0; REG_BLDY_SUB=0;
    
    swiWaitForVBlank();
}

void dsMainLoop(char *initial_file)
{
    // -----------------------------------------------------------------------------------------
    // Eventually these will be used to write to the DS screen and produce DS audio output...
    // -----------------------------------------------------------------------------------------
    videoBus = new VideoBusDS();
    audioMixer = new AudioMixerDS();
    
    FindAndLoadConfig();
    dsShowScreenMain(true);
    
    Run(initial_file);
}

//---------------------------------------------------------------------------------
UINT16 audio_mixer_buffer2[2048];
extern UINT16 audio_mixer_buffer[];
void dsInstallSoundEmuFIFO(void)
{
    memset(audio_mixer_buffer2, 0x00, 2048*sizeof(UINT16));
    TIMER2_DATA = TIMER_FREQ(SOUND_FREQ);
    TIMER2_CR = TIMER_DIV_1 | TIMER_IRQ_REQ | TIMER_ENABLE;
    irqSet(IRQ_TIMER2, (isDSiMode() ? VsoundHandlerDSi:VsoundHandlerDS));
    
    if (myConfig.sound_clock_div != SOUND_DIV_DISABLE)
        irqEnable(IRQ_TIMER2);
    else
        irqDisable(IRQ_TIMER2);
    
    FifoMessage msg;
    msg.SoundPlay.data = audio_mixer_buffer2;
    msg.SoundPlay.freq = SOUND_FREQ;
    msg.SoundPlay.volume = 127;
    msg.SoundPlay.pan = 64;
    msg.SoundPlay.loop = 1;
    msg.SoundPlay.format = ((1)<<4) | SoundFormat_16Bit;
    msg.SoundPlay.loopPoint = 0;
    msg.SoundPlay.dataSize = (2048*sizeof(UINT16)) >> 2;
    msg.type = EMUARM7_PLAY_SND;
    fifoSendDatamsg(FIFO_USER_01, sizeof(msg), (u8*)&msg);
}

// ---------------------------------------------------------------------------
// This is called very frequently (15,300 times per second) to fill the
// pipeline of sound values from the pokey buffer into the Nintendo DS sound
// buffer which will be processed in the background by the ARM 7 processor.
// ---------------------------------------------------------------------------
UINT16 sound_idx            __attribute__((section(".dtcm"))) = 0;
UINT16 lastSample           __attribute__((section(".dtcm"))) = 0;
UINT8 myCurrentSampleIdx    __attribute__((section(".dtcm"))) = 0;

ITCM_CODE void VsoundHandlerDSi(void)
{
  extern UINT8 currentSampleIdx;

  // If there is a fresh sample...
  if (myCurrentSampleIdx != currentSampleIdx)
  {
      lastSample = audio_mixer_buffer[myCurrentSampleIdx++];
  }
  audio_mixer_buffer2[sound_idx++] = lastSample;
  sound_idx &= (2048-1);
}

ITCM_CODE void VsoundHandlerDS(void)
{
  extern UINT8 currentSampleIdx;

  // If there is a fresh sample...
  if (myCurrentSampleIdx != currentSampleIdx)
  {
      lastSample = audio_mixer_buffer[myCurrentSampleIdx++];
      if (myCurrentSampleIdx == SOUND_SIZE) myCurrentSampleIdx=0;
  }
  audio_mixer_buffer2[sound_idx++] = lastSample;
  sound_idx &= (2048-1);
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


const INT8 brightness[] = {0, -3, -6, -9};
void dsInitPalette(void) 
{
    if (!bFirstGameLoaded) return;
    
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
            default:
                r = (unsigned char) ((default_Palette[i%32] & 0x00ff0000) >> 19);
                g = (unsigned char) ((default_Palette[i%32] & 0x0000ff00) >> 11);
                b = (unsigned char) ((default_Palette[i%32] & 0x000000ff) >> 3);
                break;
        }

        BG_PALETTE[i]=RGB15(r,g,b);
    }
    
    setBrightness(2, brightness[myConfig.brightness]);      // Subscreen Brightness
}

void dsInitScreenMain(void)
{
    // Init vbl and hbl func
    vramSetBankD(VRAM_D_LCD );                // Not using this for video but 128K of faster RAM always useful! Mapped at 0x06860000 - 
    vramSetBankE(VRAM_E_LCD );                // Not using this for video but 64K of faster RAM always useful!  Mapped at 0x06880000 - 
    vramSetBankF(VRAM_F_LCD );                // Not using this for video but 16K of faster RAM always useful!  Mapped at 0x06890000 - 
    vramSetBankG(VRAM_G_LCD );                // Not using this for video but 16K of faster RAM always useful!  Mapped at 0x06894000 -
    vramSetBankH(VRAM_H_LCD );                // Not using this for video but 32K of faster RAM always useful!  Mapped at 0x06898000 -
    vramSetBankI(VRAM_I_LCD );                // Not using this for video but 16K of faster RAM always useful!  Mapped at 0x068A0000 - 
    
    WAITVBL;
}

void dsShowScreenEmu(void)
{
  videoSetMode(MODE_5_2D);
  vramSetBankA(VRAM_A_MAIN_BG_0x06000000);  // The main emulated (top screen) display.
  vramSetBankB(VRAM_B_MAIN_BG_0x06060000);  // This is where we will put our frame buffers to aid DMA Copy routines...
    
  bg0 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0,0);
  memset((void*)0x06000000, 0x00, 128*1024);

  int stretch_x = ((160 / 256) << 8) | (160 % 256);
  REG_BG3PA = stretch_x;
  REG_BG3PB = 0; REG_BG3PC = 0;
  REG_BG3PD = ((100 / 100)  << 8) | (100 % 100) ;
  REG_BG3X = 0;
  REG_BG3Y = 0;

  dsInitPalette();
  swiWaitForVBlank();
}

ITCM_CODE void dsInitTimer(void)
{
    TIMER0_DATA=0;
    TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
}

void dsFreeEmu(void) 
{
  // Stop timer of sound
  TIMER2_CR=0; irqDisable(IRQ_TIMER2);
}

//----------------------------------------------------------------------------------
// Load Rom 
//----------------------------------------------------------------------------------
FICA_INTV intvromlist[512];
unsigned short int countintv=0, ucFicAct=0;
char szName[256];
char szName2[256];


// Find files (.int) available
int intvFilescmp (const void *c1, const void *c2) 
{
  FICA_INTV *p1 = (FICA_INTV *) c1;
  FICA_INTV *p2 = (FICA_INTV *) c2;

  if (p1->filename[0] == '.' && p2->filename[0] != '.')
      return -1;
  if (p2->filename[0] == '.' && p1->filename[0] != '.')
      return 1;
  if (p1->directory && !(p2->directory))
      return -1;
  if (p2->directory && !(p1->directory))
      return 1;
  return strcasecmp (p1->filename, p2->filename);    
}

static char filenametmp[255];
void intvFindFiles(void) 
{
  static bool bFirstTime = true;
  DIR *pdir;
  struct dirent *pent;

  countintv = 0;

  // First time in we use the config setting to determine where we open files...
  if (bFirstTime)
  {
      bFirstTime = false;
      if (myGlobalConfig.rom_dir == 1)
      {
         chdir("/ROMS");
      }
      else if (myGlobalConfig.rom_dir == 2)
      {
         chdir("/ROMS/INTV");
      }
  }
    
  pdir = opendir(".");

  if (pdir) {

    while (((pent=readdir(pdir))!=NULL)) 
    {
      strcpy(filenametmp,pent->d_name);
      if (pent->d_type == DT_DIR)
      {
        if (!( (filenametmp[0] == '.') && (strlen(filenametmp) == 1))) 
        {
          intvromlist[countintv].directory = true;
          strcpy(intvromlist[countintv].filename,filenametmp);
          countintv++;
        }
      }
      else 
      {
        // Filter out the BIOS files from the list...
        if (strcasecmp(filenametmp, "grom.bin") == 0) continue;
        if (strcasecmp(filenametmp, "exec.bin") == 0) continue;
        if (strcasecmp(filenametmp, "ivoice.bin") == 0) continue;
          if (strcasecmp(filenametmp, "ecs.bin") == 0) continue;
        if (strstr(filenametmp, "[BIOS]") != NULL) continue;
        if (strstr(filenametmp, "[bios]") != NULL) continue;
          
        if (strlen(filenametmp)>4) {
          if ( (strcasecmp(strrchr(filenametmp, '.'), ".int") == 0) )  {
            intvromlist[countintv].directory = false;
            strcpy(intvromlist[countintv].filename,filenametmp);
            countintv++;
          }
          if ( (strcasecmp(strrchr(filenametmp, '.'), ".bin") == 0) )  {
            intvromlist[countintv].directory = false;
            strcpy(intvromlist[countintv].filename,filenametmp);
            countintv++;
          }
          if ( (strcasecmp(strrchr(filenametmp, '.'), ".rom") == 0) )  {
            intvromlist[countintv].directory = false;
            strcpy(intvromlist[countintv].filename,filenametmp);
            countintv++;
          }
        }
      }
    }
    closedir(pdir);
  }
  if (countintv)
    qsort (intvromlist, countintv, sizeof (FICA_INTV), intvFilescmp);
}

void dsDisplayFiles(unsigned int NoDebGame,u32 ucSel)
{
  unsigned int ucBcl,ucGame;

  // Display all games if possible
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) (bgGetMapPtr(bg1b)),32*24*2);
  sprintf(szName,"%04d/%04d GAMES",(int)(1+ucSel+NoDebGame),countintv);
  dsPrintValue(16-strlen(szName)/2,2,0,szName);
  dsPrintValue(31,4,0,(char *) (NoDebGame>0 ? "<" : " "));
  dsPrintValue(31,21,0,(char *) (NoDebGame+14<countintv ? ">" : " "));
  sprintf(szName, "A=LOAD, X=JLP, Y=IVOICE, B=BACK");
  dsPrintValue(16-strlen(szName)/2,23,0,szName);
  for (ucBcl=0;ucBcl<17; ucBcl++) {
    ucGame= ucBcl+NoDebGame;
    if (ucGame < countintv)
    {
      strcpy(szName,intvromlist[ucGame].filename);
      szName[29]='\0';
      if (intvromlist[ucGame].directory)
      {
        szName[27]='\0';
        sprintf(szName2,"[%s]",szName);
        dsPrintValue(0,4+ucBcl,(ucSel == ucBcl ? 1 :  0),szName2);
      }
      else
      {
        dsPrintValue(1,4+ucBcl,(ucSel == ucBcl ? 1 : 0),szName);
      }
    }
  }
}



unsigned int dsWaitForRom(char *chosen_filename)
{
  bool bDone=false, bRet=false;
  u32 ucHaut=0x00, ucBas=0x00,ucSHaut=0x00, ucSBas=0x00,romSelected= 0, firstRomDisplay=0,nbRomPerPage, uNbRSPage;
  u32 uLenFic=0, ucFlip=0, ucFlop=0;

  strcpy(chosen_filename, "tmpz");
  intvFindFiles();   // Initial get of files...
    
  decompress(bgFileSelTiles, bgGetGfxPtr(bg0b), LZ77Vram);
  decompress(bgFileSelMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
  dmaCopy((void *) bgFileSelPal,(u16*) BG_PALETTE_SUB,256*2);
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

  nbRomPerPage = (countintv>=17 ? 17 : countintv);
  uNbRSPage = (countintv>=5 ? 5 : countintv);
  
  if (ucFicAct>countintv-nbRomPerPage)
  {
    firstRomDisplay=countintv-nbRomPerPage;
    romSelected=ucFicAct-countintv+nbRomPerPage;
  }
  else
  {
    firstRomDisplay=ucFicAct;
    romSelected=0;
  }
  dsDisplayFiles(firstRomDisplay,romSelected);
  while (!bDone)
  {
    if (keysCurrent() & KEY_UP)
    {
      if (!ucHaut)
      {
        ucFicAct = (ucFicAct>0 ? ucFicAct-1 : countintv-1);
        if (romSelected>uNbRSPage) { romSelected -= 1; }
        else {
          if (firstRomDisplay>0) { firstRomDisplay -= 1; }
          else {
            if (romSelected>0) { romSelected -= 1; }
            else {
              firstRomDisplay=countintv-nbRomPerPage;
              romSelected=nbRomPerPage-1;
            }
          }
        }
        ucHaut=0x01;
        dsDisplayFiles(firstRomDisplay,romSelected);
      }
      else {

        ucHaut++;
        if (ucHaut>10) ucHaut=0;
      }
      uLenFic=0; ucFlip=0; ucFlop=0;     
    }
    else
    {
      ucHaut = 0;
    }
    if (keysCurrent() & KEY_DOWN)
    {
      if (!ucBas) {
        ucFicAct = (ucFicAct< countintv-1 ? ucFicAct+1 : 0);
        if (romSelected<uNbRSPage-1) { romSelected += 1; }
        else {
          if (firstRomDisplay<countintv-nbRomPerPage) { firstRomDisplay += 1; }
          else {
            if (romSelected<nbRomPerPage-1) { romSelected += 1; }
            else {
              firstRomDisplay=0;
              romSelected=0;
            }
          }
        }
        ucBas=0x01;
        dsDisplayFiles(firstRomDisplay,romSelected);
      }
      else
      {
        ucBas++;
        if (ucBas>10) ucBas=0;
      }
      uLenFic=0; ucFlip=0; ucFlop=0;     
    }
    else {
      ucBas = 0;
    }
    if ((keysCurrent() & KEY_R) || (keysCurrent() & KEY_RIGHT))
    {
      if (!ucSBas)
      {
        ucFicAct = (ucFicAct< countintv-nbRomPerPage ? ucFicAct+nbRomPerPage : countintv-nbRomPerPage);
        if (firstRomDisplay<countintv-nbRomPerPage) { firstRomDisplay += nbRomPerPage; }
        else { firstRomDisplay = countintv-nbRomPerPage; }
        if (ucFicAct == countintv-nbRomPerPage) romSelected = 0;
        ucSBas=0x01;
        dsDisplayFiles(firstRomDisplay,romSelected);
      }
      else
      {
        ucSBas++;
        if (ucSBas>10) ucSBas=0;
      }
      uLenFic=0; ucFlip=0; ucFlop=0;     
    }
    else {
      ucSBas = 0;
    }
    if ((keysCurrent() & KEY_L) || (keysCurrent() & KEY_LEFT))
    {
      if (!ucSHaut)
      {
        ucFicAct = (ucFicAct> nbRomPerPage ? ucFicAct-nbRomPerPage : 0);
        if (firstRomDisplay>nbRomPerPage) { firstRomDisplay -= nbRomPerPage; }
        else { firstRomDisplay = 0; }
        if (ucFicAct == 0) romSelected = 0;
        if (romSelected > ucFicAct) romSelected = ucFicAct;          
        ucSHaut=0x01;
        dsDisplayFiles(firstRomDisplay,romSelected);
      }
      else
      {
        ucSHaut++;
        if (ucSHaut>10) ucSHaut=0;
      }
      uLenFic=0; ucFlip=0; ucFlop=0;     
    }
    else {
      ucSHaut = 0;
    }

    if ( keysCurrent() & KEY_B )
    {
      bDone=true;
      while (keysCurrent() & KEY_B);
    }

    if (keysCurrent() & KEY_A || keysCurrent() & KEY_Y || keysCurrent() & KEY_X)
    {
      if (!intvromlist[ucFicAct].directory)
      {
        bRet=true;
        bDone=true;
        WAITVBL;
        if (keysCurrent() & KEY_X) bUseJLP = true; else bUseJLP=false;
        if (keysCurrent() & KEY_Y) bForceIvoice = true; else bForceIvoice=false;          
        strcpy(chosen_filename,  intvromlist[ucFicAct].filename);
      }
      else
      {
        chdir(intvromlist[ucFicAct].filename);
        intvFindFiles();
        ucFicAct = 0;
        nbRomPerPage = (countintv>=16 ? 16 : countintv);
        uNbRSPage = (countintv>=5 ? 5 : countintv);
        if (ucFicAct>countintv-nbRomPerPage) {
          firstRomDisplay=countintv-nbRomPerPage;
          romSelected=ucFicAct-countintv+nbRomPerPage;
        }
        else {
          firstRomDisplay=ucFicAct;
          romSelected=0;
        }
        dsDisplayFiles(firstRomDisplay,romSelected);
        while (keysCurrent() & KEY_A);
      }
    }
      
    // If the filename is too long... scroll it.
    if (strlen(intvromlist[ucFicAct].filename) > 29) 
    {
      ucFlip++;
      if (ucFlip >= 15) 
      {
        ucFlip = 0;
        uLenFic++;
        if ((uLenFic+29)>strlen(intvromlist[ucFicAct].filename)) 
        {
          ucFlop++;
          if (ucFlop >= 15) 
          {
            uLenFic=0;
            ucFlop = 0;
          }
          else
            uLenFic--;
        }
        strncpy(szName,intvromlist[ucFicAct].filename+uLenFic,29);
        szName[29] = '\0';
        dsPrintValue(1,4+romSelected,1,szName);
      }
    }
      
    swiWaitForVBlank();
  }

  dsShowScreenMain(false);

  return bRet;
}


bool dsWaitOnQuit(void)
{
  bool bRet=false, bDone=false;
  unsigned int keys_pressed;
  unsigned int posdeb=0;
  char szName[32];

  decompress(bgHighScoreTiles, bgGetGfxPtr(bg0b), LZ77Vram);
  decompress(bgHighScoreMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
  dmaCopy((void *) bgHighScorePal,(u16*) BG_PALETTE_SUB,256*2);
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

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

// End of Line

