
#ifndef EMULATOR_H
#define EMULATOR_H

#include "Peripheral.h"
#include "types.h"
#include "Rip.h"
#include "ProcessorBus.h"
#include "Processor.h"
#include "AudioMixer.h"
#include "AudioProducer.h"
#include "InputConsumerBus.h"
#include "InputConsumer.h"
#include "VideoBus.h"
#include "VideoProducer.h"
#include "MemoryBus.h"
#include "Memory.h"

extern UINT16 SOUND_FREQ;
#define SOUND_SIZE  (SOUND_FREQ/60)

typedef struct _StateChunk
{
    UINT32   id;
    UINT32   size;
} StateChunk;


class Intellivision;

#define MAX_PERIPHERALS     4
#define NUM_EMULATORS       1

/**
 *
 */
class Emulator : public Peripheral
{
    public:
        void AddPeripheral(Peripheral* p);
        UINT8 GetPeripheralCount();
        Peripheral* GetPeripheral(UINT8);

		UINT16 GetVideoWidth();
		UINT16 GetVideoHeight();

        void UsePeripheral(UINT8, BOOL);

        void SetRip(Rip* rip);

		void InitVideo(VideoBus* video, UINT32 width, UINT32 height);
		void ReleaseVideo();
		void InitAudio(AudioMixer* audio, UINT32 sampleRate);
		void ReleaseAudio();
        void LoadFastMemory();

        void Reset();
        void Run();
        void FlushAudio();
		void Render();

		static UINT8 GetEmulatorCount();
        static Emulator* GetEmulator(UINT32 i);
		static Emulator* GetEmulatorByID(UINT32 targetSystemID);
        
    protected:
        Emulator(const char* name);

        MemoryBus          memoryBus;

        Rip*               currentRip;

        UINT16             videoWidth;
        UINT16             videoHeight;

    private:
        ProcessorBus       processorBus;
        AudioMixer         *audioMixer;
        VideoBus           *videoBus;
        InputConsumerBus   inputConsumerBus;

        void InsertPeripheral(Peripheral* p);
        void RemovePeripheral(Peripheral* p);

        Peripheral*     peripherals[MAX_PERIPHERALS];
        BOOL            usePeripheralIndicators[MAX_PERIPHERALS];
        UINT8           peripheralCount;

        static UINT32 systemIDs[NUM_EMULATORS];
        static Emulator* emus[NUM_EMULATORS];
        static Intellivision inty;
};

#endif
