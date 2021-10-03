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

#ifndef PERIPHERAL_H
#define PERIPHERAL_H

#include "Processor.h"
#include "RAM.h"
#include "ROM.h"
#include "AudioProducer.h"
#include "VideoProducer.h"
#include "InputConsumer.h"

#define MAX_COMPONENTS 16

/**
 * A peripheral represents a device that contains emulated hardware components, including
 * processors and memory units. Each processor or memory unit may optionally also be a
 * video producer, audio producer, and/or input consumer. However, some input consumers are
 * neither processors or memory units.
 */
class Peripheral
{
    public:
        /**
         * Gets the name of this peripheral.
         *
         * @return the peripheral name
         */
        const char* GetName() { return peripheralName; }

        /**
         * Gets the short name of this peripheral.
         *
         * @return the short peripheral name
         */
        const char* GetShortName() { return peripheralShortName; }

        /**
         * Adds a processor to this peripheral.
         *
         * @param processor the processor to add
         */
        void AddProcessor(Processor* processor);

        /**
         * Removes a processor from this peripheral.
         *
         * @param processor the processor to remove
         */
        void RemoveProcessor(Processor* processor);

        /**
         * Gets the number of processors in this peripheral.
         *
         * @return the number of processors
         */
        UINT16 GetProcessorCount();

        /**
         * Gets a pointer to the processor indicated by an index.
         *
         * @param index the index of the processor to return
         * @return a pointer to the processor
         */
        Processor* GetProcessor(UINT16 index);

        /**
         * Adds a RAM unit to this peripheral.
         *
         * @param ram the RAM unit to add
         */
        void AddRAM(RAM* ram);

        /**
         * Gets the number of RAM units in this peripheral.
         *
         * @return the number of RAM units
         */
        UINT16 GetRAMCount();

        /**
         * Gets a pointer to the RAM unit indicated by an index.
         *
         * @param index the index of the RAM unit to return
         * @return a pointer to the RAM unit
         */
        RAM* GetRAM(UINT16 index);

        /**
         * Adds a ROM unit to this peripheral.
         *
         * @param rom the ROM unit to add
         */
        void AddROM(ROM* rom);

        /**
         * Gets the number of ROM units in this peripheral.
         *
         * @return the number of ROM units
         */
		UINT16 GetROMCount();

        /**
         * Gets a pointer to the ROM unit indicated by an index.
         *
         * @param index the index of the ROM unit to return
         * @return a pointer to the ROM unit
         */
		ROM* GetROM(UINT16 index);

        /**
         * Adds a video producer to this peripheral.
         *
         * @param vp the video producer to add
         */
        void AddVideoProducer(VideoProducer* vp);

        /**
         * Gets the number of video producers in this peripheral.
         *
         * @return the number of video producers
         */
        UINT16 GetVideoProducerCount();

        /**
         * Gets a pointer to the video producer indicated by an index.
         *
         * @param index the index of the video producer to return
         * @return a pointer to the video producer
         */
        VideoProducer* GetVideoProducer(UINT16 index);

        /**
         * Adds an audio producer to this peripheral.
         *
         * @param ap the audio producer to add
         */
        void AddAudioProducer(AudioProducer* ap);

        /**
         * Gets the number of audio producers in this peripheral.
         *
         * @return the number of audio producers
         */
        UINT16 GetAudioProducerCount();

        /**
         * Gets a pointer to the audio producer indicated by an index.
         *
         * @param index the index of the audio producer to return
         * @return a pointer to the audio producer
         */
        AudioProducer* GetAudioProducer(UINT16 index);

        /**
         * Adds an input consumer to this peripheral.
         *
         * @param ic the input consumer to add
         */
        void AddInputConsumer(InputConsumer*);

        /**
         * Gets the number of input consumers in this peripheral.
         *
         * @return the number of input consumers
         */
        UINT16 GetInputConsumerCount();

        /**
         * Gets a pointer to the input consumer indicated by an index.
         *
         * @param index the index of the input consumer to return
         * @return a pointer to the input consumer
         */
        InputConsumer* GetInputConsumer(UINT16 index);

    protected:
        /**
         * Constructs a peripheral.
         *
         * @param name the name of the peripheral
         * @param shortName the short name of the peripheral
         */
        Peripheral(const CHAR* name, const CHAR* shortName)
            : processorCount(0),
              videoProducerCount(0),
              audioProducerCount(0),
              inputConsumerCount(0),
              ramCount(0),
			  romCount(0)
        {
            if (name) strcpy(peripheralName, name);
            else strcpy(peripheralName, "");

            if (shortName) strcpy(peripheralShortName, shortName);
            else strcpy(peripheralShortName, "");
        }

        /**
         * Destroys the peripheral.
         */
        ~Peripheral()
        {
        }

        /**
         * The peripheral name. The Rip class exposes the periphal name as mutable, so we leave
         * it exposed as protected access here. This will probably change in the future.
         */
        char              peripheralName[32];

    private:
        char              peripheralShortName[16];
        Processor*        processors[MAX_COMPONENTS];
        UINT16            processorCount;
        VideoProducer*    videoProducers[MAX_COMPONENTS];
        UINT16            videoProducerCount;
        AudioProducer*    audioProducers[MAX_COMPONENTS];
        UINT16            audioProducerCount;
        InputConsumer*    inputConsumers[MAX_COMPONENTS];
        UINT16            inputConsumerCount;
        RAM*			  rams[MAX_COMPONENTS];
        UINT16            ramCount;
        ROM*		      roms[MAX_COMPONENTS];
        UINT16            romCount;

};

#endif

