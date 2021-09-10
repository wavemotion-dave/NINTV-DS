
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

//#define SOUND_FREQ  (11100)
#define SOUND_FREQ  (15360)
#define SOUND_SIZE  (SOUND_FREQ/60)

typedef struct _StateHeader
{
    UINT32   emu;
    UINT32   state;
    UINT32   emuID;
    UINT32   version;
    UINT32   sys;
    UINT32   sysID;
    UINT32   cart;
    UINT32   cartID;
} StateHeader;

typedef struct _StateChunk
{
    UINT32   id;
    UINT32   size;
} StateChunk;

#if defined(DEBUG)
#define EMU_STATE_VERSION ('dev\0')
#else
#define EMU_STATE_VERSION (0x02010000)
#endif

class Intellivision;

#define MAX_PERIPHERALS    16
#define NUM_EMULATORS       1

/**
 *
 */
class Emulator : public Peripheral
{
    public:
        void AddPeripheral(Peripheral* p);
        UINT32 GetPeripheralCount();
        Peripheral* GetPeripheral(UINT32);

		UINT32 GetVideoWidth();
		UINT32 GetVideoHeight();

        void UsePeripheral(UINT32, BOOL);

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

        virtual BOOL SaveState(const CHAR* filename) = 0;
        virtual BOOL LoadState(const CHAR* filename) = 0;

		static UINT32 GetEmulatorCount();
        static Emulator* GetEmulator(UINT32 i);
		static Emulator* GetEmulatorByID(UINT32 targetSystemID);
        
    protected:
        Emulator(const char* name);

        MemoryBus          memoryBus;

        Rip*               currentRip;

        UINT32             videoWidth;
        UINT32             videoHeight;

    private:
        ProcessorBus       processorBus;
        AudioMixer         *audioMixer;
        VideoBus           *videoBus;
        InputConsumerBus   inputConsumerBus;

        void InsertPeripheral(Peripheral* p);
        void RemovePeripheral(Peripheral* p);

        Peripheral*     peripherals[MAX_PERIPHERALS];
        BOOL            usePeripheralIndicators[MAX_PERIPHERALS];
        INT32           peripheralCount;

        static UINT32 systemIDs[NUM_EMULATORS];
        static Emulator* emus[NUM_EMULATORS];
        static Intellivision inty;
};

#endif
