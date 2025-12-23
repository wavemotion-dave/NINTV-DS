// =====================================================================================
// Copyright (c) 2021-2025 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "types.h"
#include "MemoryBus.h"

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

        virtual BOOL isIdle() { return FALSE; };

    protected:
        Processor(const char* name);

        const char* name;

        ProcessorBus* processorBus;
        ScheduleQueue* scheduleQueue;
};

#endif
