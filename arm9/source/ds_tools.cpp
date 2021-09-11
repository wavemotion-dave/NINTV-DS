// =====================================================================================================
// NINTELLIVISION (c) 2021 by wavemotion-dave
// =====================================================================================================
#include <nds.h>
#include <nds/fifomessages.h>

#include<stdio.h>

#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "ds_tools.h"
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

typedef enum _RunState 
{
    Stopped,
    Paused,
    Running,
    Quit
} RunState;

bool bStartSoundFifo = false;
bool bUseJLP = false;

RunState             runState = Stopped;
Emulator             *currentEmu = NULL;
Rip                  *currentRip = NULL;
VideoBus             *videoBus = NULL;
AudioMixer           *audioMixer = NULL;

int emu_frames=1;


struct Config_t  myConfig;
struct Config_t  allConfigs[MAX_CONFIGS];

void SetDefaultConfig(void)
{
    myConfig.crc                = 0x00000000;
    myConfig.frame_skip_opt     = 1;
    myConfig.overlay_selected   = 0;
    myConfig.key_A_map          = 12;
    myConfig.key_B_map          = 12;
    myConfig.key_X_map          = 13;
    myConfig.key_Y_map          = 14;
    myConfig.key_L_map          = 0;
    myConfig.key_R_map          = 1;
    myConfig.key_START_map      = 2;
    myConfig.key_SELECT_map     = 3;
    myConfig.controller_type    = 0;
    myConfig.sound_clock_div    = (isDSiMode() ? 1:2);
    myConfig.show_fps           = 0;
    myConfig.spare0             = 0;
    myConfig.spare1             = 0;
    myConfig.spare2             = 0;
    myConfig.spare3             = 0;
    myConfig.spare4             = 0;
    myConfig.spare5             = 0;
    myConfig.spare6             = 1;
    myConfig.spare7             = 1;
    myConfig.spare8             = 1;
    myConfig.spare9             = 2;
    myConfig.config_ver         = CONFIG_VER;
}

// ---------------------------------------------------------------------------
// Write out the XEGS.DAT configuration file to capture the settings for
// each game.
// ---------------------------------------------------------------------------
void SaveConfig(bool bShow)
{
    FILE *fp;
    int slot = 0;
    
    if (currentRip == NULL) return;

    if (bShow) dsPrintValue(0,2,0, (char*)"SAVE CONFIG");
    
    myConfig.crc = currentRip->GetCRC();
    
    // Find the slot we should save into...
    for (slot=0; slot<MAX_CONFIGS; slot++)
    {
        if (allConfigs[slot].crc == myConfig.crc)  // Got a match?!
        {
            break;                           
        }
        if (allConfigs[slot].crc == 0x00000000)  // End of useful list...
        {
            break;                           
        }
    }
    
    memcpy(&allConfigs[slot], &myConfig, sizeof(struct Config_t));

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
    } else dsPrintValue(0,2,0, (char*)"   ERROR   ");

    if (bShow) 
    {
        WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
        dsPrintValue(0,2,0, (char*)"           ");
    }
}


void FindAndLoadConfig(void)
{
    FILE *fp;

    SetDefaultConfig();
    fp = fopen("/data/NINTV-DS.DAT", "rb");
    if (fp != NULL)
    {
        fread(&allConfigs, sizeof(allConfigs), 1, fp);
        fclose(fp);
        
        if (allConfigs[0].config_ver != CONFIG_VER)
        {
            dsPrintValue(0,1,0, (char*)"PLEASE WAIT...");
            memset(&allConfigs, 0x00, sizeof(allConfigs));
            for (int i=0; i<MAX_CONFIGS; i++) allConfigs[i].config_ver = CONFIG_VER;
            SaveConfig(FALSE);
            dsPrintValue(0,1,0, (char*)"              ");
        }
        
        if (currentRip != NULL)
        {
            for (int slot=0; slot<MAX_CONFIGS; slot++)
            {
                if (allConfigs[slot].crc == currentRip->GetCRC())  // Got a match?!
                {
                    memcpy(&myConfig, &allConfigs[slot], sizeof(struct Config_t));
                    break;                           
                }
            }
        }
        else
        {
            SetDefaultConfig();
        }
    }
    else    // Not found... init the entire database...
    {
        dsPrintValue(0,1,0, (char*)"PLEASE WAIT...");
        memset(&allConfigs, 0x00, sizeof(allConfigs));
        for (int i=0; i<MAX_CONFIGS; i++) allConfigs[i].config_ver = CONFIG_VER;
        SaveConfig(FALSE);
        dsPrintValue(0,1,0, (char*)"              ");
    }
    
    ApplyOptions();
}

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

int debug1, debug2;
UINT16 frames=0;
ITCM_CODE void VideoBusDS::render()
{
    frames++;
	VideoBus::render();

    // Any level of frame skip will skip the render()
    if (myConfig.frame_skip_opt)
    {
        if (frames & 1) return;
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
        //load the bin file as a Rip
        CHAR* cfgFilename = (CHAR*)"knowncarts.cfg";
        currentRip = Rip::LoadBin(filename, cfgFilename);
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
    FindAndLoadConfig();
    dsShowScreenEmu();
    dsShowScreenMain(false);
    bFirstGameLoaded = TRUE;    
    
    return TRUE;
}

BOOL LoadPeripheralRoms(Peripheral* peripheral)
{
	UINT16 count = peripheral->GetROMCount();
	for (UINT16 i = 0; i < count; i++) {
		ROM* r = peripheral->GetROM(i);
		if (r->isLoaded())
			continue;

		//TODO: get filenames from config file
		//TODO: handle file loading errors
        CHAR nextFile[MAX_PATH];
        strcpy(nextFile, ".");
        strcat(nextFile, "/");
        strcat(nextFile, r->getDefaultFileName());
        if (!r->load(nextFile, r->getDefaultFileOffset())) 
        {
            return FALSE;
        }
	}

    return TRUE;
}


BOOL InitializeEmulator(void)
{
    //find the currentEmulator required to run this RIP
	currentEmu = Emulator::GetEmulatorByID(currentRip->GetTargetSystemID());
    if (currentEmu == NULL) 
    {
        return FALSE;
    }
    
    //load the BIOS files required for this currentEmulator
    if (!LoadPeripheralRoms(currentEmu))
        return FALSE;

    //load peripheral roms
    INT32 count = currentEmu->GetPeripheralCount();
    for (INT32 i = 0; i < count; i++) {
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
            return FALSE;
    }
    
	//hook the audio and video up to the currentEmulator
	currentEmu->InitVideo(videoBus,currentEmu->GetVideoWidth(),currentEmu->GetVideoHeight());
    currentEmu->InitAudio(audioMixer, SOUND_FREQ);

    //put the RIP in the currentEmulator
	currentEmu->SetRip(currentRip);
    
    // Load up the fast ROM memory for quick fetches
    currentEmu->LoadFastMemory();
    
    //Reset everything
    currentEmu->Reset();
    
    // And put the Sound Fifo back at the start...
    bStartSoundFifo = true;
    
    TIMER0_CR=0;
    TIMER0_DATA=0;
    TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
    emu_frames=1;

    return TRUE;
}

char newFile[256];
void pollInputs(void)
{
    UINT16 ctrl_disc, ctrl_keys, ctrl_side;
    extern int ds_key_input[3][16];   // Set to '1' if pressed... 0 if released
    extern int ds_disc_input[3][16];  // Set to '1' if pressed... 0 if released.
    unsigned short keys_pressed = keysCurrent();

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
    
    // Handle 8 directions on keypad... best we can do...
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
    
    if (keys_pressed & KEY_A)       
    {
        if (myConfig.key_A_map > 11) 
            ds_key_input[ctrl_side][myConfig.key_A_map]  = 1;
        else
            ds_key_input[ctrl_keys][myConfig.key_A_map]  = 1;
    }
    
    if (keys_pressed & KEY_B)
    {
        if (myConfig.key_B_map > 11) 
            ds_key_input[ctrl_side][myConfig.key_B_map]  = 1;
        else
            ds_key_input[ctrl_keys][myConfig.key_B_map]  = 1;
    }

    if (keys_pressed & KEY_X)
    {
        if (myConfig.key_X_map > 11) 
            ds_key_input[ctrl_side][myConfig.key_X_map]  = 1;
        else
            ds_key_input[ctrl_keys][myConfig.key_X_map]  = 1;
    }

    if (keys_pressed & KEY_Y)
    {
        if (myConfig.key_Y_map > 11) 
            ds_key_input[ctrl_side][myConfig.key_Y_map]  = 1;
        else
            ds_key_input[ctrl_keys][myConfig.key_Y_map]  = 1;
    }

    if (keys_pressed & KEY_L)
    {
        if (myConfig.key_L_map > 11) 
            ds_key_input[ctrl_side][myConfig.key_L_map]  = 1;
        else
            ds_key_input[ctrl_keys][myConfig.key_L_map]  = 1;
    }

    if (keys_pressed & KEY_R)
    {
        if (myConfig.key_R_map > 11) 
            ds_key_input[ctrl_side][myConfig.key_R_map]  = 1;
        else
            ds_key_input[ctrl_keys][myConfig.key_R_map]  = 1;
    }

    if (keys_pressed & KEY_START)   
    {
        if (myConfig.key_START_map > 11) 
            ds_key_input[ctrl_side][myConfig.key_START_map]  = 1;
        else
            ds_key_input[ctrl_keys][myConfig.key_START_map]  = 1;
    }
    
    static int prev_select=0;
    if (keys_pressed & KEY_SELECT)  
    {
#if 1
        if (myConfig.key_SELECT_map > 11) 
            ds_key_input[ctrl_side][myConfig.key_SELECT_map]  = 1;
        else
            ds_key_input[ctrl_keys][myConfig.key_SELECT_map]  = 1;
#else   
        if (prev_select == 0)
        {
            fifoSendValue32(FIFO_USER_01,(1<<16) | SOUND_PAUSE);
            swiWaitForVBlank();
            fifoSendValue32(FIFO_USER_01,(1<<16) | SOUND_RESUME);
            swiWaitForVBlank();
            prev_select=1;
        }

#endif        
    } else prev_select=0;
    
    // Now handle the on-screen Intellivision keypad... 
    if (keys_pressed & KEY_TOUCH)
    {
        //char tmp[12];
        touchPosition touch;
        touchRead(&touch);
        //sprintf(tmp, "%3d %3d", touch.px, touch.py);
        //dsPrintValue(0,1,0,tmp);

        if (touch.px > 120  && touch.px < 155 && touch.py > 30 && touch.py < 60)   ds_key_input[ctrl_keys][0] = 1;
        if (touch.px > 158  && touch.px < 192 && touch.py > 30 && touch.py < 60)   ds_key_input[ctrl_keys][1] = 1;
        if (touch.px > 195  && touch.px < 230 && touch.py > 30 && touch.py < 60)   ds_key_input[ctrl_keys][2] = 1;

        if (touch.px > 120  && touch.px < 155 && touch.py > 65 && touch.py < 95)   ds_key_input[ctrl_keys][3] = 1;
        if (touch.px > 158  && touch.px < 192 && touch.py > 65 && touch.py < 95)   ds_key_input[ctrl_keys][4] = 1;
        if (touch.px > 195  && touch.px < 230 && touch.py > 65 && touch.py < 95)   ds_key_input[ctrl_keys][5] = 1;

        if (touch.px > 120  && touch.px < 155 && touch.py > 101 && touch.py < 135)   ds_key_input[ctrl_keys][6] = 1;
        if (touch.px > 158  && touch.px < 192 && touch.py > 101 && touch.py < 135)   ds_key_input[ctrl_keys][7] = 1;
        if (touch.px > 195  && touch.px < 230 && touch.py > 101 && touch.py < 135)   ds_key_input[ctrl_keys][8] = 1;

        if (touch.px > 120  && touch.px < 155 && touch.py > 140 && touch.py < 175)   ds_key_input[ctrl_keys][9]  = 1;
        if (touch.px > 158  && touch.px < 192 && touch.py > 140 && touch.py < 175)   ds_key_input[ctrl_keys][10] = 1;
        if (touch.px > 195  && touch.px < 230 && touch.py > 140 && touch.py < 175)   ds_key_input[ctrl_keys][11] = 1;

    
        // RESET
        if (touch.px > 23  && touch.px < 82 && touch.py > 25 && touch.py < 46) 
        {
            if (bFirstGameLoaded) currentEmu->Reset();
        }
        
        // LOAD
        if (touch.px > 23  && touch.px < 82 && touch.py > 55 && touch.py < 76) 
        {
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (dsWaitForRom(newFile))
            {
                if (LoadCart(newFile)) 
                {
                    dsPrintValue(0,1,0, (char*) "           ");
                    InitializeEmulator();
                    for (int i=0; i<20; i++) currentEmu->Run();
                    if (LoadCart(newFile)) InitializeEmulator();
                }
                else
                {
                    dsPrintValue(0,1,0, (char*) "LOAD FAILED");
                }
            }
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
        }
        
        // CONFIG
        if (touch.px > 23  && touch.px < 82 && touch.py > 86 && touch.py < 106) 
        {
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            dsChooseOptions();
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
        }

        // HIGHSCORES
        if (touch.px > 23  && touch.px < 82 && touch.py > 116 && touch.py < 136) 
        {
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (currentRip != NULL) 
            {
                highscore_display(currentRip->GetCRC());
                dsShowScreenMain(false);
                WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            }
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
        }
        
        // QUIT
        if (touch.px > 23  && touch.px < 82 && touch.py > 145 && touch.py < 170) 
        {
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (dsWaitOnQuit())
            {
                runState = Quit;
            }
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
        }
    }
    
}

void Run()
{
    TIMER1_CR = 0;
    TIMER1_DATA = 0;
    TIMER1_CR=TIMER_ENABLE | TIMER_DIV_1024;
    
    TIMER0_CR=0;
    TIMER0_DATA=0;
    TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
    
    runState = Running;
	while(runState == Running) 
    {
        // If not full speed, time 1 frame...
        if (myConfig.show_fps != 2)
        {
            while(TIMER0_DATA < (546*emu_frames))
                ;
        }

        // Have we processed 60 frames... start over...
        if (++emu_frames == 60)
        {
            TIMER0_CR=0;
            TIMER0_DATA=0;
            TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
            emu_frames=1;
        }       
        
        //poll the input
		pollInputs();

        if (bFirstGameLoaded)
        {
            //flush the audio  -  normaly this would be done AFTER run/render but this gives us maximum accuracy on audio timing
            currentEmu->FlushAudio();
            
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
            if ((frames > 0) && (myConfig.show_fps > 0))
            {
                sprintf(tmp, "%03d", frames);
                dsPrintValue(0,0,0,tmp);
            }
            frames=0;
            //sprintf(tmp, "%4d %4d", debug1, debug2);
            //dsPrintValue(0,1,0,tmp);
        }
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

    if (myConfig.overlay_selected == 1) // Treasure of Tarmin
    {
      decompress(bgBottom_treasureTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_treasureMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_treasurePal,(u16*) BG_PALETTE_SUB,256*2);
      unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
      dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    }
    else if (myConfig.overlay_selected == 2) // Cloudy Mountain
    {
      decompress(bgBottom_cloudyTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_cloudyMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_cloudyPal,(u16*) BG_PALETTE_SUB,256*2);
      unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
      dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    }
    else if (myConfig.overlay_selected == 3) // Astrosmash
    {
      decompress(bgBottom_astroTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_astroMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_astroPal,(u16*) BG_PALETTE_SUB,256*2);
      unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
      dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    }
    else if (myConfig.overlay_selected == 4) // Space Spartans
    {
      decompress(bgBottom_spartansTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_spartansMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_spartansPal,(u16*) BG_PALETTE_SUB,256*2);
      unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
      dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    }
    else if (myConfig.overlay_selected == 5) // B17 Bomber
    {
      decompress(bgBottom_b17Tiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_b17Map, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_b17Pal,(u16*) BG_PALETTE_SUB,256*2);
      unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
      dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    }
    else if (myConfig.overlay_selected == 6) // Atlantis
    {
      decompress(bgBottom_atlantisTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_atlantisMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_atlantisPal,(u16*) BG_PALETTE_SUB,256*2);
      unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
      dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    }
    else if (myConfig.overlay_selected == 7) // Bomb Squad
    {
      decompress(bgBottom_bombsquadTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_bombsquadMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_bombsquadPal,(u16*) BG_PALETTE_SUB,256*2);
      unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
      dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    }
    else if (myConfig.overlay_selected == 8) // Utopia
    {
      decompress(bgBottom_utopiaTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_utopiaMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_utopiaPal,(u16*) BG_PALETTE_SUB,256*2);
      unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
      dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    }
    else if (myConfig.overlay_selected == 9) // Swords & Serpents
    {
      decompress(bgBottom_swordsTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_swordsMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_swordsPal,(u16*) BG_PALETTE_SUB,256*2);
      unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
      dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    }
    else
    {
      decompress(bgBottomTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottomMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottomPal,(u16*) BG_PALETTE_SUB,256*2);
      unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
      dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    }

    REG_BLDCNT=0; REG_BLDCNT_SUB=0; REG_BLDY=0; REG_BLDY_SUB=0;
    
    swiWaitForVBlank();
}

void dsMainLoop(void)
{
    // -----------------------------------------------------------------------------------------
    // Eventually these will be used to write to the DS screen and produce DS audio output...
    // -----------------------------------------------------------------------------------------
    videoBus = new VideoBusDS();
    audioMixer = new AudioMixerDS();
    
    dsShowScreenMain(true);
    InitializeEmulator();

    Run();    
}

//---------------------------------------------------------------------------------
UINT16 audio_mixer_buffer2[2048];
extern UINT16 audio_mixer_buffer[];
void dsInstallSoundEmuFIFO(void)
{
    memset(audio_mixer_buffer2, 0x00, 2048*sizeof(UINT16));
    TIMER2_DATA = TIMER_FREQ(SOUND_FREQ);
    TIMER2_CR = TIMER_DIV_1 | TIMER_IRQ_REQ | TIMER_ENABLE;
    irqSet(IRQ_TIMER2, VsoundHandler);
    
    if (myConfig.sound_clock_div != 3)
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

ITCM_CODE void VsoundHandler(void)
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


UINT32 gamePalette[32] = 
{
    0x000000, 0x002DFF, 0xFF3D10, 0xC9CFAB,
    0x386B3F, 0x00A756, 0xFAEA50, 0xFFFCFF,
    0xBDACC8, 0x24B8FF, 0xFFB41F, 0x546E00,
    0xFF4E57, 0xA496FF, 0x75CC80, 0xB51A58,
    0x000000, 0x002DFF, 0xFF3D10, 0xC9CFAB,
    0x386B3F, 0x00A756, 0xFAEA50, 0xFFFCFF,
    0xBDACC8, 0x24B8FF, 0xFFB41F, 0x546E00,
    0xFF4E57, 0xA496FF, 0x75CC80, 0xB51A58,
};


void dsInitPalette(void) 
{
    // Init DS Specific palette
    for(int i = 0; i < 256; i++)   
    {
        unsigned char r, g, b;

        r = (unsigned char) ((gamePalette[i%32] & 0x00ff0000) >> 19);
        g = (unsigned char) ((gamePalette[i%32] & 0x0000ff00) >> 11);
        b = (unsigned char) ((gamePalette[i%32] & 0x000000ff) >> 3);

        BG_PALETTE[i]=RGB15(r,g,b);
    }
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
int a26Filescmp (const void *c1, const void *c2) 
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
  DIR *pdir;
  struct dirent *pent;

  countintv = 0;

  pdir = opendir(".");

  if (pdir) {

    while (((pent=readdir(pdir))!=NULL)) 
    {
      strcpy(filenametmp,pent->d_name);
      if (pent->d_type == DT_DIR)
      {
        if (!( (filenametmp[0] == '.') && (strlen(filenametmp) == 1))) {
          intvromlist[countintv].directory = true;
          strcpy(intvromlist[countintv].filename,filenametmp);
          countintv++;
        }
      }
      else {
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
    qsort (intvromlist, countintv, sizeof (FICA_INTV), a26Filescmp);
}

void dsDisplayFiles(unsigned int NoDebGame,u32 ucSel)
{
  unsigned int ucBcl,ucGame;

  // Display all games if possible
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) (bgGetMapPtr(bg1b)),32*24*2);
  sprintf(szName,"%04d/%04d GAMES",(int)(1+ucSel+NoDebGame),countintv);
  dsPrintValue(16-strlen(szName)/2,2,0,szName);
  dsPrintValue(31,5,0,(char *) (NoDebGame>0 ? "<" : " "));
  dsPrintValue(31,22,0,(char *) (NoDebGame+14<countintv ? ">" : " "));
  sprintf(szName, "A=LOAD, X=LOAD JLP, B=BACK");
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
        dsPrintValue(0,5+ucBcl,(ucSel == ucBcl ? 1 :  0),szName2);
      }
      else
      {
        dsPrintValue(1,5+ucBcl,(ucSel == ucBcl ? 1 : 0),szName);
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
        if (keysCurrent() & KEY_X) bUseJLP = true; else bUseJLP=false;
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
        dsPrintValue(1,5+romSelected,1,szName);
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
    const char  *option[16];
    UINT16 *option_val;
    UINT16 option_max;
};

const struct options_t Option_Table[] =
{
    {"OVERLAY",     {"GENERIC", "MINOTAUR", "ADVENTURE", "ASTROSMASH", "SPACE SPARTAN", "B-17 BOMBER", "ATLANTIS", "BOMB SQUAD", "UTOPIA", "SWORD & SERPT"},     &myConfig.overlay_selected, 10},
    {"A BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &myConfig.key_A_map,        15},
    {"B BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &myConfig.key_B_map,        15},
    {"X BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &myConfig.key_X_map,        15},
    {"Y BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &myConfig.key_Y_map,        15},
    {"L BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &myConfig.key_L_map,        15},
    {"R BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &myConfig.key_R_map,        15},
    {"START BTN",   {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &myConfig.key_START_map,    15},
    {"SELECT BTN",  {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &myConfig.key_SELECT_map,   15},
    {"CONTROLLER",  {"LEFT/PLAYER1", "RIGHT/PLAYER2", "DUAL-ACTION A", "DUAL-ACTION B"},                                                                         &myConfig.controller_type,  4},
    {"FRAMESKIP",   {"OFF", "ON"}                                   ,                                                                                            &myConfig.frame_skip_opt,   2},   
    {"SOUND DIV",   {"20 (HIGHQ)", "24 (LOW/FAST)", "28 (LOWEST)", "DISABLED"},                                                                                  &myConfig.sound_clock_div,  4},
    {"FPS",         {"OFF", "ON", "ON-TURBO"},                                                                                                                   &myConfig.show_fps,         3},
    {NULL,          {"",            ""},                                NULL,                   2},
};

void ApplyOptions(void)
{
    // Change the sound div if needed... affects sound quality and speed 
    extern  INT32 clockDivisor;
    static UINT32 sound_divs[] = {20,24,28,64};
    clockDivisor = sound_divs[myConfig.sound_clock_div];

    // Check if the sound changed...
    fifoSendValue32(FIFO_USER_01,(1<<16) | SOUND_KILL);
    bStartSoundFifo=true;
	// clears the emulator side of the audio mixer
	audioMixer->resetProcessor();
}

// -----------------------------------------------------------------------------
// Allows the user to move the cursor up and down through the various table 
// enties  above to select options for the game they wish to play. 
// -----------------------------------------------------------------------------
void dsChooseOptions(void)
{
    int optionHighlighted;
    int idx;
    bool bDone=false;
    int keys_pressed;
    int last_keys_pressed = 999;
    char strBuf[64];

    // Show the Options background...
    decompress(bgOptionsTiles, bgGetGfxPtr(bg0b), LZ77Vram);
    decompress(bgOptionsMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
    dmaCopy((void *) bgOptionsPal,(u16*) BG_PALETTE_SUB,256*2);
    unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
    dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

    idx=0;
    while (true)
    {
        sprintf(strBuf, " %-11s : %-13s ", Option_Table[idx].label, Option_Table[idx].option[*(Option_Table[idx].option_val)]);
        dsPrintValue(1,5+idx, (idx==0 ? 1:0), strBuf);
        idx++;
        if (Option_Table[idx].label == NULL) break;
    }

    dsPrintValue(0,23, 0, (char *)"D-PAD TOGGLE. A=EXIT, START=SAVE");
    optionHighlighted = 0;
    while (!bDone)
    {
        keys_pressed = keysCurrent();
        if (keys_pressed != last_keys_pressed)
        {
            last_keys_pressed = keys_pressed;
            if (keysCurrent() & KEY_UP) // Previous option
            {
                sprintf(strBuf, " %-11s : %-13s ", Option_Table[optionHighlighted].label, Option_Table[optionHighlighted].option[*(Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,5+optionHighlighted,0, strBuf);
                if (optionHighlighted > 0) optionHighlighted--; else optionHighlighted=(idx-1);
                sprintf(strBuf, " %-11s : %-13s ", Option_Table[optionHighlighted].label, Option_Table[optionHighlighted].option[*(Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,5+optionHighlighted,1, strBuf);
            }
            if (keysCurrent() & KEY_DOWN) // Next option
            {
                sprintf(strBuf, " %-11s : %-13s ", Option_Table[optionHighlighted].label, Option_Table[optionHighlighted].option[*(Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,5+optionHighlighted,0, strBuf);
                if (optionHighlighted < (idx-1)) optionHighlighted++;  else optionHighlighted=0;
                sprintf(strBuf, " %-11s : %-13s ", Option_Table[optionHighlighted].label, Option_Table[optionHighlighted].option[*(Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,5+optionHighlighted,1, strBuf);
            }

            if (keysCurrent() & KEY_RIGHT)  // Toggle option clockwise
            {
                *(Option_Table[optionHighlighted].option_val) = (*(Option_Table[optionHighlighted].option_val) + 1) % Option_Table[optionHighlighted].option_max;
                sprintf(strBuf, " %-11s : %-13s ", Option_Table[optionHighlighted].label, Option_Table[optionHighlighted].option[*(Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,5+optionHighlighted,1, strBuf);
            }
            if (keysCurrent() & KEY_LEFT)  // Toggle option counterclockwise
            {
                if ((*(Option_Table[optionHighlighted].option_val)) == 0)
                    *(Option_Table[optionHighlighted].option_val) = Option_Table[optionHighlighted].option_max -1;
                else
                    *(Option_Table[optionHighlighted].option_val) = (*(Option_Table[optionHighlighted].option_val) - 1) % Option_Table[optionHighlighted].option_max;
                sprintf(strBuf, " %-11s : %-13s ", Option_Table[optionHighlighted].label, Option_Table[optionHighlighted].option[*(Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,5+optionHighlighted,1, strBuf);
            }
            if (keysCurrent() & KEY_START)  // Save Options
            {
                SaveConfig(TRUE);
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

