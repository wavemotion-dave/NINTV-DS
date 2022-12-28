// =====================================================================================
// Copyright (c) 2021-2023 Dave Bernazzani (wavemotion-dave)
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

#include "ds_tools.h"
#include "videoaud.h"
#include "config.h"
#include "Emulator.h"
#include "AudioMixer.h"

// ---------------------------------------------------------------------------
// This is called very frequently (15,360 times per second) to fill the
// pipeline of sound values from the Audio Mixer into the Nintendo DS sound
// buffer which will be processed in the background by the ARM 7 processor.
// ---------------------------------------------------------------------------
UINT32 audio_arm7_xfer_buffer __attribute__ ((aligned (4))) = 0;
UINT32* aptr                  __attribute__((section(".dtcm"))) = (UINT32*) (&audio_arm7_xfer_buffer + 0xA000000/4);
UINT16 myCurrentSampleIdx16   __attribute__((section(".dtcm"))) = 0;
UINT8  myCurrentSampleIdx8    __attribute__((section(".dtcm"))) = 0;    

ITCM_CODE void VsoundHandlerDSi(void)
{
  UINT16 sample[2];
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
  UINT16 sample[2];
  // If there is a fresh sample...
  if (myCurrentSampleIdx16 != currentSampleIdx16)
  {
      sample[0] = audio_mixer_buffer[myCurrentSampleIdx16++]; 
      sample[1] = sample[0];
      *aptr = *((UINT32*)&sample);
      if (myCurrentSampleIdx16 == SOUND_SIZE) myCurrentSampleIdx16=0;
  }
}


// -----------------------------------------------------------------------------------------------------------------
// This starts the sound engine... the ARM7 is told about the 1-sample buffer and runs at 2x the normal processing
// frequency so that it samples fast enough that no sounds are dropped (Nyquist would have something to say here).
// The internal VSoundHandler() is setup to run at the frequency at which we want to place samples into the buffer.
// -----------------------------------------------------------------------------------------------------------------
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





// ------------------------------------------------------------------
// Game palette arrays... we support 3 built-in palettes and one
// custom palette that can be read from NINTV-DS.PAL
// ------------------------------------------------------------------
const UINT32 muted_gamePalette[32] = 
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

const UINT32 bright_gamePalette[32] = 
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

const UINT32 pal_gamePalette[32] = 
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


// ---------------------------------------------------------------------------------------
// To save battery and/or if the user simply wants to de-ephasize the bottom "overlay" 
// screen while playing on the top screen, we support dimming that bottom screen. This
// is easy since the NDS has hardware support for this...
// ---------------------------------------------------------------------------------------
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


// ---------------------------------------------------------------
// Our main frameskip boolean table... a 1 in this table means
// that we will render the full display on that frame.
// ---------------------------------------------------------------
UINT8 renderz[4][4] __attribute__((section(".dtcm")))  = 
{
    {1,1,1,1},      // Frameskip Off
    {1,0,1,0},      // Frameskip Odd
    {0,1,0,1},      // Frameskip Even
    {1,0,0,0}       // Frameskip Agressive
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

// -----------------------------------------------------------------------------
// Here we take the fully rendered Intellivision 'pixelBuffer' which is 160x192
// and copy it over to the NDS video memory. We use DMA copy as it's found to 
// be slightly more efficient. We are utilizing two DMA channels because there
// isn't anything else using it and we gain a bit of speed...
// -----------------------------------------------------------------------------
ITCM_CODE void VideoBusDS::render()
{
    frames_per_sec_calc++;
    global_frames++;
    VideoBus::render();

    // ------------------------------------------------------
    // Check if we are skipping rendering this frame...
    // ------------------------------------------------------
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
            chan++; chan = 2+(chan&1);  // Use channels 2 and 3 for the copy...
        }    
    }
}

// End of Line


