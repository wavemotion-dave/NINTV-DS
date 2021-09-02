
#ifndef PROCESSORBUS_H
#define PROCESSORBUS_H

#include "Processor.h"
#include "types.h"
#include "AudioMixer.h"

const INT32 MAX_PROCESSORS = 15;

class ScheduleQueue;

class ProcessorBus
{

public:
    ProcessorBus();
    virtual ~ProcessorBus();
    void addProcessor(Processor* p);
    void removeProcessor(Processor* p);
	void removeAll();

	void reset();
	void run();
	void stop();

	void halt(Processor* p);
    void unhalt(Processor* p);
    void pause(Processor* p, int ticks);

private:
	void reschedule(ScheduleQueue*);

    UINT32      processorCount;
    Processor*  processors[MAX_PROCESSORS];
	bool running;
	ScheduleQueue* startQueue;
	ScheduleQueue* endQueue;

};

class ScheduleQueue
{
	friend class ProcessorBus;

private:
    ScheduleQueue(Processor* p)
		: tickFactor(0),
		  tick(0),
		  previous(NULL),
		  next(NULL)
	{
        this->processor = p;
    }

    Processor* processor;
    UINT64 tickFactor;
    UINT64 tick;
    ScheduleQueue* previous;
    ScheduleQueue* next;
};

UINT64 lcm(UINT64 a, UINT64 b);

#endif
