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
#include "bgTop.h"
#include "bgFileSel.h"
#include "bgOptions.h"

#include "clickNoQuit_wav.h"
#include "clickQuit_wav.h"

#include "Emulator.h"
#include "InputProducerManager.h"
#include "Rip.h"

typedef enum _RunState 
{
    Stopped,
    Paused,
    Running,
    Quit
} RunState;

UINT16 frame_skip_opt=1;
static UINT16 overlay_selected = 0;
static UINT16 key_A_map = 12;
static UINT16 key_B_map = 12;
static UINT16 key_X_map = 13;
static UINT16 key_Y_map = 14;
static UINT16 key_L_map = 0;
static UINT16 key_R_map = 1;
static UINT16 key_START_map  = 2;
static UINT16 key_SELECT_map = 3;

#define WAITVBL swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank(); swiWaitForVBlank();

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
	AudioMixerDS(UINT16* buffer);
	void resetProcessor();
	void flushAudio();
	void init(UINT32 sampleRate);
	void release();

	UINT16* outputBuffer;
	UINT32  outputBufferSize;
	UINT32  outputBufferWritePosition;
};

AudioMixerDS::AudioMixerDS(UINT16* buffer)
  : outputBuffer(NULL),
    outputBufferSize(0),
    outputBufferWritePosition(0)
{
	this->outputBuffer = buffer;
	this->outputBufferWritePosition = 0;
	this->outputBufferSize = 2048;
}

void AudioMixerDS::resetProcessor()
{
	if (outputBuffer) 
    {
		outputBufferWritePosition = 0;
	}

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
    if (sampleCount < this->sampleSize) {
		return;
	}

    // Write to DS ARM7 audio buffer?
    memcpy(outputBuffer, this->sampleBuffer, this->sampleBufferSize);
    
	// clear the sample count
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
static int frames=0;
ITCM_CODE void VideoBusDS::render()
{
    frames++;
	VideoBus::render();

    // Any level of frame skip will skip the render()
    if (frame_skip_opt > 0)
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

RunState             runState;
Emulator             *currentEmu;
Rip                  *currentRip;
VideoBus             *videoBus;
AudioMixer           *audioMixer;


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
        //load the designated Rip
        currentRip = Rip::LoadRip(filename);
        if (currentRip == NULL)
        {
            return FALSE;
        }
    }

    dsShowScreenEmu();
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
    //release the current emulator
    //ReleaseEmulator();

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
    currentEmu->InitAudio(audioMixer,22050);

    //put the RIP in the currentEmulator
	currentEmu->SetRip(currentRip);

    //finally, run everything
    currentEmu->Reset();

    return TRUE;
}

UINT16 controller_type = 0;
char newFile[256];
void pollInputs(void)
{
    UINT16 ctrl_disc, ctrl_keys;
    extern int ds_key_input[3][16];   // Set to '1' if pressed... 0 if released
    extern int ds_disc_input[3][16];  // Set to '1' if pressed... 0 if released.
    unsigned short keys_pressed = keysCurrent();

    for (int j=0; j<3; j++)
    {
        for (int i=0; i<15; i++) ds_key_input[j][i] = 0;
        for (int i=0; i<16; i++) ds_disc_input[j][i] = 0;
    }
    
    // Check for Dual Action
    if (controller_type == 2)
    {
        ctrl_disc = 0;
        ctrl_keys = 1;
    }
    else
    {
        ctrl_disc = controller_type;
        ctrl_keys = controller_type;
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
    
    if (keys_pressed & KEY_A)       ds_key_input[ctrl_keys][key_A_map]  = 1;
    if (keys_pressed & KEY_B)       ds_key_input[ctrl_keys][key_B_map]  = 1;
    if (keys_pressed & KEY_X)       ds_key_input[ctrl_keys][key_X_map]  = 1;
    if (keys_pressed & KEY_Y)       ds_key_input[ctrl_keys][key_Y_map]  = 1;
    if (keys_pressed & KEY_L)       ds_key_input[ctrl_keys][key_L_map] = 1;
    if (keys_pressed & KEY_R)       ds_key_input[ctrl_keys][key_R_map] = 1;
    if (keys_pressed & KEY_START)   ds_key_input[ctrl_keys][key_START_map] = 1; 
    if (keys_pressed & KEY_SELECT)  ds_key_input[ctrl_keys][key_SELECT_map] = 1;
    
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
        if (touch.px > 23  && touch.px < 82 && touch.py > 34 && touch.py < 53) 
        {
            if (bFirstGameLoaded) currentEmu->Reset();
        }
        
        // LOAD
        if (touch.px > 23  && touch.px < 82 && touch.py > 72 && touch.py < 91) 
        {
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            if (dsWaitForRom(newFile))
            {
                LoadCart(newFile); 
                InitializeEmulator();
            }
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
        }
        
        // CONFIG
        if (touch.px > 23  && touch.px < 82 && touch.py > 111 && touch.py < 130) 
        {
            fifoSendValue32(FIFO_USER_01,(1<<16) | (0) | SOUND_SET_VOLUME);
            dsChooseOptions();
            fifoSendValue32(FIFO_USER_01,(1<<16) | (127) | SOUND_SET_VOLUME);
        }
        
        // QUIT
        if (touch.px > 23  && touch.px < 82 && touch.py > 150 && touch.py < 170) 
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

int full_speed=0;
int emu_frames=1;
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
        if (!full_speed)
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
            //run the emulation
            currentEmu->Run();

            // render the output
            currentEmu->Render();

            //flush the audio
            currentEmu->FlushAudio();
        }
        
        if (TIMER1_DATA >= 32728)   // 1000MS (1 sec)
        {
            char tmp[15];
            TIMER1_CR = 0;
            TIMER1_DATA = 0;
            TIMER1_CR=TIMER_ENABLE | TIMER_DIV_1024;
            if (frames > 0)
            {
                sprintf(tmp, "%03d", frames);
                dsPrintValue(0,0,0,tmp);
            }
            sprintf(tmp, "%4d %4d", debug1, debug2);
            dsPrintValue(0,1,0,tmp);
            frames=0;
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

    if (overlay_selected == 1) // Treasure of Tarmin
    {
      decompress(bgBottom_treasureTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_treasureMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_treasurePal,(u16*) BG_PALETTE_SUB,256*2);
      unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
      dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
    }
    else if (overlay_selected == 2) // Cloudy Mountain
    {
      decompress(bgBottom_cloudyTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_cloudyMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_cloudyPal,(u16*) BG_PALETTE_SUB,256*2);
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
    
  // ShowStatusLine();    

  swiWaitForVBlank();
}

#define SOUND_SIZE  734
UINT16 soundBuf[SOUND_SIZE+8] = {0};
void dsMainLoop(void)
{
    // -----------------------------------------------------------------------------------------
    // Eventually these will be used to write to the DS screen and produce DS audio output...
    // -----------------------------------------------------------------------------------------
    videoBus = new VideoBusDS();
    audioMixer = new AudioMixerDS(soundBuf);
    
    dsShowScreenMain(true);
    InitializeEmulator();

    Run();    
}

//---------------------------------------------------------------------------------
void dsInstallSoundEmuFIFO(void)
{
    FifoMessage msg;
    msg.SoundPlay.data = &soundBuf;
    msg.SoundPlay.freq = 22050;
    msg.SoundPlay.volume = 127;
    msg.SoundPlay.pan = 64;
    msg.SoundPlay.loop = 1;
    msg.SoundPlay.format = ((1)<<4) | SoundFormat_16Bit;
    msg.SoundPlay.loopPoint = 0;
    msg.SoundPlay.dataSize = SOUND_SIZE >> 2;
    msg.type = EMUARM7_PLAY_SND;
    fifoSendDatamsg(FIFO_USER_01, sizeof(msg), (u8*)&msg);
}


void vblankIntr() 
{
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
    SetYtrigger(190); //trigger 2 lines before vsync
    irqSet(IRQ_VBLANK, vblankIntr);
    irqEnable(IRQ_VBLANK);
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
FICA2600 vcsromlist[512];
unsigned short int countvcs=0, ucFicAct=0;
char szName[256];
char szName2[256];


// Find files (.int) available
int a26Filescmp (const void *c1, const void *c2) 
{
  FICA2600 *p1 = (FICA2600 *) c1;
  FICA2600 *p2 = (FICA2600 *) c2;

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
void vcsFindFiles(void) 
{
  DIR *pdir;
  struct dirent *pent;

  countvcs = 0;

  pdir = opendir(".");

  if (pdir) {

    while (((pent=readdir(pdir))!=NULL)) 
    {
      strcpy(filenametmp,pent->d_name);
      if (pent->d_type == DT_DIR)
      {
        if (!( (filenametmp[0] == '.') && (strlen(filenametmp) == 1))) {
          vcsromlist[countvcs].directory = true;
          strcpy(vcsromlist[countvcs].filename,filenametmp);
          countvcs++;
        }
      }
      else {
        if (strlen(filenametmp)>4) {
          if ( (strcasecmp(strrchr(filenametmp, '.'), ".int") == 0) )  {
            vcsromlist[countvcs].directory = false;
            strcpy(vcsromlist[countvcs].filename,filenametmp);
            countvcs++;
          }
          if ( (strcasecmp(strrchr(filenametmp, '.'), ".rom") == 0) )  {
            vcsromlist[countvcs].directory = false;
            strcpy(vcsromlist[countvcs].filename,filenametmp);
            countvcs++;
          }
        }
      }
    }
    closedir(pdir);
  }
  if (countvcs)
    qsort (vcsromlist, countvcs, sizeof (FICA2600), a26Filescmp);
}

void dsDisplayFiles(unsigned int NoDebGame,u32 ucSel)
{
  unsigned int ucBcl,ucGame;

  // Display all games if possible
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) (bgGetMapPtr(bg1b)),32*24*2);
  sprintf(szName,"%04d/%04d GAMES",(int)(1+ucSel+NoDebGame),countvcs);
  dsPrintValue(16-strlen(szName)/2,2,0,szName);
  dsPrintValue(31,5,0,(char *) (NoDebGame>0 ? "<" : " "));
  dsPrintValue(31,22,0,(char *) (NoDebGame+14<countvcs ? ">" : " "));
  sprintf(szName, "A=CHOOSE   B=BACK ");
  dsPrintValue(16-strlen(szName)/2,23,0,szName);
  for (ucBcl=0;ucBcl<17; ucBcl++) {
    ucGame= ucBcl+NoDebGame;
    if (ucGame < countvcs)
    {
      strcpy(szName,vcsromlist[ucGame].filename);
      szName[29]='\0';
      if (vcsromlist[ucGame].directory)
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
  vcsFindFiles();   // Initial get of files...
    
  decompress(bgFileSelTiles, bgGetGfxPtr(bg0b), LZ77Vram);
  decompress(bgFileSelMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
  dmaCopy((void *) bgFileSelPal,(u16*) BG_PALETTE_SUB,256*2);
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

  nbRomPerPage = (countvcs>=17 ? 17 : countvcs);
  uNbRSPage = (countvcs>=5 ? 5 : countvcs);
  
  if (ucFicAct>countvcs-nbRomPerPage)
  {
    firstRomDisplay=countvcs-nbRomPerPage;
    romSelected=ucFicAct-countvcs+nbRomPerPage;
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
        ucFicAct = (ucFicAct>0 ? ucFicAct-1 : countvcs-1);
        if (romSelected>uNbRSPage) { romSelected -= 1; }
        else {
          if (firstRomDisplay>0) { firstRomDisplay -= 1; }
          else {
            if (romSelected>0) { romSelected -= 1; }
            else {
              firstRomDisplay=countvcs-nbRomPerPage;
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
        ucFicAct = (ucFicAct< countvcs-1 ? ucFicAct+1 : 0);
        if (romSelected<uNbRSPage-1) { romSelected += 1; }
        else {
          if (firstRomDisplay<countvcs-nbRomPerPage) { firstRomDisplay += 1; }
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
        ucFicAct = (ucFicAct< countvcs-nbRomPerPage ? ucFicAct+nbRomPerPage : countvcs-nbRomPerPage);
        if (firstRomDisplay<countvcs-nbRomPerPage) { firstRomDisplay += nbRomPerPage; }
        else { firstRomDisplay = countvcs-nbRomPerPage; }
        if (ucFicAct == countvcs-nbRomPerPage) romSelected = 0;
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

    if (keysCurrent() & KEY_A || keysCurrent() & KEY_Y)
    {
      if (!vcsromlist[ucFicAct].directory)
      {
        bRet=true;
        bDone=true;
        strcpy(chosen_filename,  vcsromlist[ucFicAct].filename);
        if (keysCurrent() & KEY_Y) full_speed=1; else full_speed=0;
      }
      else
      {
        chdir(vcsromlist[ucFicAct].filename);
        vcsFindFiles();
        ucFicAct = 0;
        nbRomPerPage = (countvcs>=16 ? 16 : countvcs);
        uNbRSPage = (countvcs>=5 ? 5 : countvcs);
        if (ucFicAct>countvcs-nbRomPerPage) {
          firstRomDisplay=countvcs-nbRomPerPage;
          romSelected=ucFicAct-countvcs+nbRomPerPage;
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
    if (strlen(vcsromlist[ucFicAct].filename) > 29) 
    {
      ucFlip++;
      if (ucFlip >= 15) 
      {
        ucFlip = 0;
        uLenFic++;
        if ((uLenFic+29)>strlen(vcsromlist[ucFicAct].filename)) 
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
        strncpy(szName,vcsromlist[ucFicAct].filename+uLenFic,29);
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

  decompress(bgFileSelTiles, bgGetGfxPtr(bg0b), LZ77Vram);
  decompress(bgFileSelMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
  dmaCopy((void *) bgFileSelPal,(u16*) BG_PALETTE_SUB,256*2);
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

UINT16 sound_clock_div = 3;
// -----------------------------------------------------------------------------
// Options are handled here... we have a number of things the user can tweak
// and these options are applied immediately. The user can also save off 
// their option choices for the currently running game into the XEGS.DAT
// configuration database. When games are loaded back up, XEGS.DAT is read
// to see if we have a match and the user settings can be restored for the 
// game.
// -----------------------------------------------------------------------------
struct options_t
{
    const char  *label;
    const char  *option[16];
    UINT16 *option_val;
    UINT16 option_max;
};

const struct options_t Option_Table[] =
{
 
    {"OVERLAY",     {"GENERIC", "MINOTAUR", "ADVENTURE"},                                                                                                        &overlay_selected,  3},
    {"A BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &key_A_map,        15},
    {"B BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &key_B_map,        15},
    {"X BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &key_X_map,        15},
    {"Y BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &key_Y_map,        15},
    {"L BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &key_L_map,        15},
    {"R BUTTON",    {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &key_R_map,        15},
    {"START BTN",   {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &key_START_map,    15},
    {"SELECT BTN",  {"KEY-1", "KEY-2", "KEY-3", "KEY-4", "KEY-5", "KEY-6", "KEY-7", "KEY-8", "KEY-9", "KEY-CLR", "KEY-0", "KEY-ENT", "FIRE", "R-ACT", "L-ACT"},  &key_SELECT_map,   15},
    {"CONTROLLER",  {"LEFT/PLAYER1", "RIGHT/PLAYER2", "DUAL-ACTION"},                                                                                            &controller_type,  3},
    {"FRAMESKIP",   {"OFF", "ON", "ON-AGGRESSIVE"},                                                                                                              &frame_skip_opt,   3},   
    {"SOUND DIV",   {"1", "2", "4", "8", "16", "32"},                                                                                                            &sound_clock_div,  6},
    {NULL,          {"",            ""},                                NULL,                   2},
};

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
        sprintf(strBuf, " %-12s  : %-12s ", Option_Table[idx].label, Option_Table[idx].option[*(Option_Table[idx].option_val)]);
        dsPrintValue(1,5+idx, (idx==0 ? 1:0), strBuf);
        idx++;
        if (Option_Table[idx].label == NULL) break;
    }

    dsPrintValue(0,23, 0, (char *)"D-PAD LEFT/RIGHT TOGGLE. A=EXIT");
    optionHighlighted = 0;
    while (!bDone)
    {
        keys_pressed = keysCurrent();
        if (keys_pressed != last_keys_pressed)
        {
            last_keys_pressed = keys_pressed;
            if (keysCurrent() & KEY_UP) // Previous option
            {
                sprintf(strBuf, " %-12s  : %-12s ", Option_Table[optionHighlighted].label, Option_Table[optionHighlighted].option[*(Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,5+optionHighlighted,0, strBuf);
                if (optionHighlighted > 0) optionHighlighted--; else optionHighlighted=(idx-1);
                sprintf(strBuf, " %-12s  : %-12s ", Option_Table[optionHighlighted].label, Option_Table[optionHighlighted].option[*(Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,5+optionHighlighted,1, strBuf);
            }
            if (keysCurrent() & KEY_DOWN) // Next option
            {
                sprintf(strBuf, " %-12s  : %-12s ", Option_Table[optionHighlighted].label, Option_Table[optionHighlighted].option[*(Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,5+optionHighlighted,0, strBuf);
                if (optionHighlighted < (idx-1)) optionHighlighted++;  else optionHighlighted=0;
                sprintf(strBuf, " %-12s  : %-12s ", Option_Table[optionHighlighted].label, Option_Table[optionHighlighted].option[*(Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,5+optionHighlighted,1, strBuf);
            }

            if (keysCurrent() & KEY_RIGHT)  // Toggle option clockwise
            {
                *(Option_Table[optionHighlighted].option_val) = (*(Option_Table[optionHighlighted].option_val) + 1) % Option_Table[optionHighlighted].option_max;
                sprintf(strBuf, " %-12s  : %-12s ", Option_Table[optionHighlighted].label, Option_Table[optionHighlighted].option[*(Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,5+optionHighlighted,1, strBuf);
            }
            if (keysCurrent() & KEY_LEFT)  // Toggle option counterclockwise
            {
                if ((*(Option_Table[optionHighlighted].option_val)) == 0)
                    *(Option_Table[optionHighlighted].option_val) = Option_Table[optionHighlighted].option_max -1;
                else
                    *(Option_Table[optionHighlighted].option_val) = (*(Option_Table[optionHighlighted].option_val) - 1) % Option_Table[optionHighlighted].option_max;
                sprintf(strBuf, " %-12s  : %-12s ", Option_Table[optionHighlighted].label, Option_Table[optionHighlighted].option[*(Option_Table[optionHighlighted].option_val)]);
                dsPrintValue(1,5+optionHighlighted,1, strBuf);
            }
            if (keysCurrent() & KEY_START)  // Save Options
            {
                // TODO: write them out... dsWriteConfig();
            }
            if ((keysCurrent() & KEY_B) || (keysCurrent() & KEY_A))  // Exit options
            {
                break;
            }
        }
        swiWaitForVBlank();
    }
    
    // Change the sound div if needed... affects sound quality and speed 
    extern  INT32 clockDivisor;
    static UINT32 sound_divs[] = {1,2,4,8,16,32};
    clockDivisor = sound_divs[sound_clock_div];

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

