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

#include "Peripheral.h"

void Peripheral::AddProcessor(Processor* p)
{
    processors[processorCount] = p;
    processorCount++;
}

void Peripheral::RemoveProcessor(Processor* p)
{
    for (UINT32 i = 0; i < processorCount; i++) {
        if (processors[i] == p) {
            for (UINT32 j = i; j < (processorCount-1); j++)
                processors[j] = processors[j+1];
            processorCount--;
            return;
        }
    }
}

UINT16 Peripheral::GetProcessorCount()
{
    return processorCount;
}

Processor* Peripheral::GetProcessor(UINT16 i)
{
    return processors[i];
}

void Peripheral::AddRAM(RAM* m)
{
    if (ramCount < MAX_ROMS)
    {
        rams[ramCount] = m;
        ramCount++;
    }
}

UINT16 Peripheral::GetRAMCount()
{
    return ramCount;
}

RAM* Peripheral::GetRAM(UINT16 i)
{
    return rams[i];
}

void Peripheral::AddVideoProducer(VideoProducer* vp)
{
    videoProducers[videoProducerCount] = vp;
    videoProducerCount++;
}

UINT16 Peripheral::GetVideoProducerCount()
{
    return videoProducerCount;
}

VideoProducer* Peripheral::GetVideoProducer(UINT16 i)
{
    return videoProducers[i];
}

void Peripheral::AddAudioProducer(AudioProducer* ap)
{
    audioProducers[audioProducerCount] = ap;
    audioProducerCount++;
}

UINT16 Peripheral::GetAudioProducerCount()
{
    return audioProducerCount;
}

AudioProducer* Peripheral::GetAudioProducer(UINT16 i)
{
    return audioProducers[i];
}

void Peripheral::AddInputConsumer(InputConsumer* ic)
{
    inputConsumers[inputConsumerCount] = ic;
    inputConsumerCount++;
}

UINT16 Peripheral::GetInputConsumerCount()
{
    return inputConsumerCount;
}

InputConsumer* Peripheral::GetInputConsumer(UINT16 i)
{
    return inputConsumers[i];
}

void Peripheral::AddROM(ROM* r)
{
    if (romCount < MAX_ROMS)
    {
        roms[romCount] = r;
        romCount++;
    }
}

UINT16 Peripheral::GetROMCount()
{
    return romCount;
}

ROM* Peripheral::GetROM(UINT16 i)
{
    return roms[i];
}


