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

#ifndef __VIDEOAUD_H
#define __VIDEOAUD_H

#include <nds.h>
#include "types.h"
#include "AudioMixer.h"
#include "VideoBus.h"

typedef enum {
  EMUARM7_INIT_SND = 0x123C,
  EMUARM7_STOP_SND = 0x123D,
  EMUARM7_PLAY_SND = 0x123E,
} FifoMesType;

// ---------------------------------------------------------------------
// The Audio Mixer receives audio data from the AudioMixer.cpp buffers 
// and this is what translates that into NDS sounds. 
// ---------------------------------------------------------------------
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


// ---------------------------------------------------------------------
// The Video Bus allows the NDS to copy out the internal Intellivision
// 'pixelBuffer' and render it onto the actual top screen display.
// We use DMA copy to do this because it's slightly more efficient...
// ---------------------------------------------------------------------
class VideoBusDS : public VideoBus
{
public:
    VideoBusDS();
    void init(UINT32 width, UINT32 height);
    void release();
    void render();
};


extern void dsInstallSoundEmuFIFO(void);
extern void dsInitPalette(void);

#endif
