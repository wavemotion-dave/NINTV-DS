
#ifndef INTELLIVOICE_H
#define INTELLIVOICE_H

#include "Memory.h"
#include "Peripheral.h"
#include "types.h"
#include "Processor.h"
#include "SP0256.h"
#include "AudioOutputLine.h"

TYPEDEF_STRUCT_PACK( _IntellivoiceState
{
    SP0256State sp0256State;
} IntellivoiceState; )

class Intellivoice : public Peripheral
{
public:
	Intellivoice()
		: Peripheral("Intellivoice", "Intellivoice")
	{
		AddProcessor(&sp0256);
        AddAudioProducer(&sp0256);
        AddRAM(&sp0256.registers);
		AddROM(&sp0256.ivoiceROM);
	}
    UINT32 getProcessorCount();
    void getProcessor(INT32 i, Processor** p);
    UINT32 getMemoryCount();
    void getMemory(INT32 i, Memory** m);

    IntellivoiceState getState();
    void setState(IntellivoiceState state);

private:
    SP0256         sp0256;
};

#endif
