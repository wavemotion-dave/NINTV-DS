
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "SignalLine.h"
#include "types.h"
#include "MemoryBus.h"
#define MAX_PINS 16

class ProcessorBus;
class ScheduleQueue;

/**
 * An abstract class representing a processor, which is a hardware component
 * that requires a clock input.
 */
class Processor
{

    friend class ProcessorBus;

    public:
        virtual ~Processor();

        const char* getName() { return name; }

        virtual void resetProcessor() = 0;

        /**
         * Describes the clock speed of this processor.  Unlike many emulation
         * codebases, this codebase allows a processor to run at any speed
         * it prefers, and the ProcessorBus is responsible for scheduling
         * execution of each processor based on all of the various processor
         * speeds.
         */
        virtual INT32 getClockSpeed() = 0;

        /**
         *
         */
        virtual INT32 tick(INT32 i) = 0;

        virtual void connectPinOut(UINT8 pinOutNum, Processor* targetProcessor,
                UINT8 targetPinInNum);
        virtual void disconnectPinOut(UINT8 pinOutNum);

        virtual BOOL isIdle() { return FALSE; };

    protected:
        Processor(const char* name);

        const char* name;
        SignalLine* pinIn[MAX_PINS];
        SignalLine* pinOut[MAX_PINS];

		ProcessorBus* processorBus;
		ScheduleQueue* scheduleQueue;

};

#endif
