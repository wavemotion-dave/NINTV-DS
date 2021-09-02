
#include "ProcessorBus.h"

ProcessorBus::ProcessorBus()
: processorCount(0),
  startQueue(NULL),
  endQueue(NULL)
{}

ProcessorBus::~ProcessorBus()
{
    for (UINT32 i = 0; i < processorCount; i++) {
        if (processors[i]->scheduleQueue)
            delete processors[i]->scheduleQueue;
    }
}

void ProcessorBus::addProcessor(Processor* p)
{
    processors[processorCount] = p;
	processorCount++;
    p->processorBus = this;
    p->scheduleQueue = new ScheduleQueue(p);
}

void ProcessorBus::removeProcessor(Processor* p)
{
    for (UINT32 i = 0; i < processorCount; i++) {
        if (processors[i] == p) {
            for (UINT32 j = i; j < (processorCount-1); j++)
                processors[j] = processors[j+1];
            processorCount--;
            delete p->scheduleQueue;
            p->scheduleQueue = NULL;
            return;
        }
    }
}

void ProcessorBus::removeAll()
{
    while (processorCount)
        removeProcessor(processors[0]);
}


void ProcessorBus::reset()
{
    if (processorCount == 0)
        return;
    
    UINT64 totalClockSpeed = 1;
    startQueue = endQueue = NULL;

    //reorder the processor queue so that it is in the natural (starting) order
    for (UINT32 i = 0; i < processorCount; i++) {
		Processor* p = processors[i];
        totalClockSpeed = lcm(totalClockSpeed, ((UINT64)p->getClockSpeed()));

        ScheduleQueue* nextQueue = p->scheduleQueue;
        nextQueue->tick = 0;
        if (startQueue == NULL) {
            startQueue = nextQueue;
            nextQueue->previous = NULL;
            nextQueue->next = NULL;
        }
        else {
            endQueue->next = nextQueue;
            nextQueue->previous = endQueue;
            nextQueue->next = NULL;
        }
        endQueue = nextQueue;
    }
    
    //pre-cache the multiplication factor required to convert each processor's clock speed to
    //the common clock speed, and reset each processor
	for (UINT32 i = 0; i < processorCount; i++) {
		Processor* p = processors[i];
        p->scheduleQueue->tickFactor = (totalClockSpeed / ((UINT64)p->getClockSpeed()));
        p->resetProcessor();
    }
}

void ProcessorBus::run()
{
    running = true;
    while (running) {
		// TODO: jeremiah sypult, saw crash when NULL
		if (startQueue->next == NULL) {
			break;
		}
        //tick the processor that is at the head of the queue
        int minTicks = (int)((startQueue->next->tick / startQueue->tickFactor) + 1);
        startQueue->tick = ((UINT64)startQueue->processor->tick(minTicks)) * startQueue->tickFactor;

        //now reschedule the processor for later processing
        ScheduleQueue* tmp1 = startQueue;
        while (tmp1->next != NULL && startQueue->tick > tmp1->next->tick) {
            startQueue->tick -= tmp1->next->tick;
            tmp1 = tmp1->next;
        }

        //reorganize the scheduling queue
        ScheduleQueue* queueToShuffle = startQueue;
        startQueue = startQueue->next;
        queueToShuffle->previous = tmp1;
        queueToShuffle->next = tmp1->next;
        tmp1->next = queueToShuffle;
        if (queueToShuffle->next != NULL) {
            queueToShuffle->next->tick -= queueToShuffle->tick;
            queueToShuffle->next->previous = queueToShuffle;
        }
        else
            endQueue = queueToShuffle;
    }
}

void ProcessorBus::stop()
{
    running = false;
}

void ProcessorBus::halt(Processor*)
{
}

void ProcessorBus::unhalt(Processor*)
{
}

void ProcessorBus::pause(Processor* p, int ticks)
{
    ScheduleQueue* q = p->scheduleQueue;
    UINT64 commonTicks = ((UINT64)ticks) * q->tickFactor;
    if (q->next == NULL)
        q->tick += commonTicks;
    else if (commonTicks <= q->next->tick) {
        q->tick += commonTicks;
        q->next->tick -= commonTicks;
    }
    else {
        //pull this processor out of the scheduling stream and reschedule it
        //add my current ticks to the next guy's delta ticks
        q->next->tick += q->tick;
        //add the common ticks to my ticks
        q->tick += commonTicks;
        //have the previous q point to the next q
        if (q->previous != NULL) {
            q->previous->next = q->next;
            q->next->previous = q->previous;
        }
        reschedule(p->scheduleQueue);
    }
}

void ProcessorBus::reschedule(ScheduleQueue* queueToShuffle)
{
    //look for where to put this processor in the scheduling queue
    ScheduleQueue* tmp1 = queueToShuffle;
    while (tmp1->next != NULL && queueToShuffle->tick > tmp1->next->tick) {
        queueToShuffle->tick -= tmp1->next->tick;
        tmp1 = tmp1->next;
    }

    //reorganize the scheduling queue
    if (queueToShuffle == startQueue)
        startQueue = queueToShuffle->next;
    queueToShuffle->previous = tmp1;
    queueToShuffle->next = tmp1->next;
    tmp1->next = queueToShuffle;
    if (queueToShuffle->next != NULL) {
        queueToShuffle->next->tick -= queueToShuffle->tick;
        queueToShuffle->next->previous = queueToShuffle;
    }
    else
        endQueue = queueToShuffle;
}

UINT64 lcm(UINT64 a, UINT64 b) {
    //This is an implementation of Euclid's Algorithm for determining
    //the greatest common denominator (gcd) of two numbers.
    UINT64 m = a;
    UINT64 n = b;
    UINT64 r = m % n;
    while (r != 0) {
        m = n;
        n = r;
        r = m % n;
    }
    UINT64 gcd = n;

    //lcm is determined from gcd through the following equation
    return (a/gcd)*b;
}

/*
ProcessorBus::ProcessorBus()
{
    processorDebugOn = FALSE;
    processorCount     = 0;
    memset(processors, 0, sizeof(Processor*) * MAX_PROCESSORS);
    memset(processorTickFactors, 0, sizeof(INT64) * MAX_PROCESSORS);
    memset(processorTicks, 0, sizeof(INT64) * MAX_PROCESSORS);
    memset(processorsIdle, 0, sizeof(BOOL) * MAX_PROCESSORS);
}

void ProcessorBus::setAudioMixer(AudioMixer* am)
{
    this->audioMixer = am;
}

void ProcessorBus::init()
{
    UINT64 totalClockSpeed = 1;
    UINT32 i;
    for (i = 0; i < processorCount; i++) {
        totalClockSpeed = lcm(totalClockSpeed,
                ((UINT64)processors[i]->getClockSpeed()));
    }

    for (i = 0; i < processorCount; i++) {
        processorsIdle[i] = FALSE;
        processorTicks[i] = 0;
        processorTickFactors[i] = (totalClockSpeed /
                ((UINT64)processors[i]->getClockSpeed()));

        processors[i]->initProcessor();
    }
}

void ProcessorBus::release()
{
   for (UINT32 i = 0; i < processorCount; i++)
        processors[i]->releaseProcessor();
}

void ProcessorBus::addProcessor(Processor* p)
{
    processors[processorCount] = p;
    processorCount++;
}

void ProcessorBus::removeProcessor(Processor* p)
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

void ProcessorBus::removeAll()
{
    processorCount = 0;
}

void ProcessorBus::tick()
{
    //determine the next tick delta
#ifdef _MSC_VER
	UINT64 tickDelta = 0xFFFFFFFFFFFFFFFF;
#else
	UINT64 tickDelta = 0xFFFFFFFFFFFFFFFFll;
#endif

    UINT32 i;
    for (i = 0; i < processorCount; i++) {
        //wake up any processors that want to wake up
        processorsIdle[i] = (processorsIdle[i] && processors[i]->isIdle());

        //if the processor is not idle by this point, use it
        //to calculate the tick delta
        if (!processorsIdle[i] && (UINT64)processorTicks[i] < tickDelta)
            tickDelta = processorTicks[i];
    }

    //move the audio mixer clock forward by the tick delta amount
    audioMixer->clock += tickDelta;

    for (i = 0; i < processorCount; i++) {
        //skip this processor if it has been idled
        if (!processorsIdle[i]) {
            processorTicks[i] -= tickDelta;
 
            //if the clock just caught up to the processor, and
            //if the processor is not about to go idle, then run it
            if (processorTicks[i] == 0) {
                if (processors[i]->isIdle())
                    processorsIdle[i] = TRUE;
                else {
                    processorTicks[i] = (((UINT64)processors[i]->tick()) *
                            processorTickFactors[i]);
                }
            }
        }
    }
}

UINT64 lcm(UINT64 a, UINT64 b) {
    //This is an implementation of Euclid's Algorithm for determining
    //the greatest common denominator (gcd) of two numbers.
    UINT64 m = a;
    UINT64 n = b;
    UINT64 r = m % n;
    while (r != 0) {
        m = n;
        n = r;
        r = m % n;
    }
    UINT64 gcd = n;

    //lcm is determined from gcd through the following equation
    return (a/gcd)*b;
}
*/