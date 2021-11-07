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
#include "Emulator.h"
#include "Intellivision.h"

Intellivision Emulator::inty;

Emulator* Emulator::GetEmulator(void)
{
    return &inty;
}

Emulator::Emulator(const char* name)
    : Peripheral(name, name),
      currentRip(NULL),
      peripheralCount(0)
{
    memset(peripherals, 0, sizeof(peripherals));
    memset(usePeripheralIndicators, FALSE, sizeof(usePeripheralIndicators));
}

// -------------------------------------------------------------------------------------
// We pre-load the 64k 16-bit memory into a fast buffer in VRAM where we have faster 
// memory access (on average) than main RAM. Main RAM is faster if we have a cache
// hit but a cache miss is very slow compared to VRAM. So we opt for the VRAM which
// is very consistent in terms of reasonably fast access and it's 16-bit wide which
// is perfect for the CP1610 emulation core.
// -------------------------------------------------------------------------------------
void Emulator::LoadFastMemory()
{
    UINT16 *fast_memory;
    fast_memory = (UINT16 *)0x06880000;     // LCD RAM area... possibly faster 16-bit access...
    for (int i=0x0000; i<=0xFFFF; i++)
    {
        fast_memory[i] = memoryBus.peek_slow_and_safe(i);
    }
}

void Emulator::AddPeripheral(Peripheral* p)
{
    peripherals[peripheralCount] = p;
    peripheralCount++;
}

UINT8 Emulator::GetPeripheralCount()
{
    return peripheralCount;
}

Peripheral* Emulator::GetPeripheral(UINT8 i)
{
    return peripherals[i];
}

void Emulator::UsePeripheral(UINT8 i, BOOL b)
{
    usePeripheralIndicators[i] = b;
}

UINT16 Emulator::GetVideoWidth()
{
    return videoWidth;
}

UINT16 Emulator::GetVideoHeight()
{
    return videoHeight;
}

void Emulator::InitVideo(VideoBus* video, UINT32 width, UINT32 height)
{
    if ( video != NULL ) {
        videoBus = video;
    }

    videoBus->init(width, height);
}

void Emulator::ReleaseVideo()
{
    if (videoBus) {
        videoBus->release();
        videoBus = NULL;
    }
}

void Emulator::InitAudio(AudioMixer* audio, UINT32 sampleRate)
{
    if (audio != NULL) {
        audioMixer = audio;
    }

    for (UINT16 i = 0; i < GetProcessorCount(); i++) {
        Processor* p = GetProcessor(i);
        if (p == audio) {
            RemoveProcessor(audio);
        }
    }

    AddProcessor(audioMixer);
    audioMixer->init(sampleRate);
}

void Emulator::ReleaseAudio()
{
    if (audioMixer) {
        audioMixer->release();
        RemoveProcessor(audioMixer);
        audioMixer = NULL;
    }
}

void Emulator::Reset()
{
    processorBus.reset();
    memoryBus.reset();
}

void Emulator::SetRip(Rip* rip)
{
    if (this->currentRip != NULL) {
        processorBus.removeAll();
        memoryBus.removeAll();
        videoBus->removeAll();
        audioMixer->removeAll();
        inputConsumerBus.removeAll();
    }

    currentRip = rip;

    if (this->currentRip != NULL) 
    {
        InsertPeripheral(this);

        //use the desired peripherals
        for (INT32 i = 0; i < peripheralCount; i++) 
        {
            if (usePeripheralIndicators[i])
                InsertPeripheral(peripherals[i]);
        }

        InsertPeripheral(currentRip);
    }
}

void Emulator::InsertPeripheral(Peripheral* p)
{
    UINT16 i;
    
    //processors
    UINT16 count = p->GetProcessorCount();
    for (i = 0; i < count; i++)
    {    
        processorBus.addProcessor(p->GetProcessor(i));
    }

    //RAM
    count = p->GetRAMCount();
    for (i = 0; i < count; i++)
    {
        memoryBus.addMemory(p->GetRAM(i));
    }

    //ROM
    count = p->GetROMCount();
    for (i = 0; i < count; i++) 
    {
        ROM* nextRom = p->GetROM(i);
        if (!nextRom->isInternal())
            memoryBus.addMemory(nextRom);
    }

    //video producers
    count = p->GetVideoProducerCount();
    for (i = 0; i < count; i++)
        videoBus->addVideoProducer(p->GetVideoProducer(i));

    //audio producers
    count = p->GetAudioProducerCount();
    for (i = 0; i < count; i++)
        audioMixer->addAudioProducer(p->GetAudioProducer(i));

    //input consumers
    count = p->GetInputConsumerCount();
    for (i = 0; i < count; i++)
    {
        inputConsumerBus.addInputConsumer(p->GetInputConsumer(i));
    }
}

void Emulator::RemovePeripheral(Peripheral* p)
{
    UINT16 i;

    //processors
    UINT16 count = p->GetProcessorCount();
    for (i = 0; i < count; i++)
        processorBus.removeProcessor(p->GetProcessor(i));

    //RAM
    count = p->GetRAMCount();
    for (i = 0; i < count; i++)
        memoryBus.removeMemory(p->GetRAM(i));

    //ROM
    count = p->GetROMCount();
    for (i = 0; i < count; i++)
        memoryBus.removeMemory(p->GetROM(i));

    //video producers
    count = p->GetVideoProducerCount();
    for (i = 0; i < count; i++)
        videoBus->removeVideoProducer(p->GetVideoProducer(i));

    //audio producers
    count = p->GetAudioProducerCount();
    for (i = 0; i < count; i++)
        audioMixer->removeAudioProducer(p->GetAudioProducer(i));

    //input consumers
    count = p->GetInputConsumerCount();
    for (i = 0; i < count; i++)
        inputConsumerBus.removeInputConsumer(p->GetInputConsumer(i));
}

// --------------------------------------------------------------------------
// This is the main entry point to allow the emulator to run 1 frame... 
// Basically poll for inputs and then run 1 frame of CPU, PSG, STIC, etc.
// --------------------------------------------------------------------------
ITCM_CODE void Emulator::Run()
{
    inputConsumerBus.evaluateInputs();
    processorBus.run();
}

// -----------------------------------------------------
// Draw the screen - we call the NDS video render here.
// -----------------------------------------------------
ITCM_CODE void Emulator::Render()
{
    videoBus->render();
}

// -------------------------------------------------------
// Flush the audio - not much really to be done as 
// the NDS audio handler is interrupt driven from TIMER2.
// -------------------------------------------------------
ITCM_CODE void Emulator::FlushAudio()
{
    audioMixer->flushAudio();
}

// End of file
