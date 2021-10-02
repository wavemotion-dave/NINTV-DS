
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

    // TODO: check for an existing audioMixer processor and release it
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

void Emulator::Run()
{
    inputConsumerBus.evaluateInputs();
    processorBus.run();
}

void Emulator::Render()
{
    videoBus->render();
}

void Emulator::FlushAudio()
{
    audioMixer->flushAudio();
}
