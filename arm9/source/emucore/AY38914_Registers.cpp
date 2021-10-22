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

#include <stdio.h>
#include <string.h>
#include "AY38914.h"
#include "AY38914_Registers.h"
#include "../ds_tools.h"

AY38914_Registers::AY38914_Registers(UINT16 address)
: RAM(0x10, address, 0xFFFF, 0xFFFF)
{}

void AY38914_Registers::init(AY38914* ay38914)
{
    this->ay38914 = ay38914;
}

void AY38914_Registers::reset()
{
    memset(memory, 0, sizeof(memory));
}

void AY38914_Registers::poke(UINT16 location, UINT16 value)
{
    location &= 0x0F;
    switch(location) {
        case 0x00:
            value = value & 0x00FF;
            channel0.period = (channel0.period & 0x0F00) |
                    value;
            channel0.periodValue = (channel0.period
                    ? channel0.period : 0x1000);
            memory[location] = value;
            break;

        case 0x01:
            value = value & 0x00FF;
            channel1.period = (channel1.period & 0x0F00) |
                    value;
            channel1.periodValue = (channel1.period
                    ? channel1.period : 0x1000);
            memory[location] = value;
            break;

        case 0x02:
            value = value & 0x00FF;
            channel2.period = (channel2.period & 0x0F00) |
                    value;
            channel2.periodValue = (channel2.period
                    ? channel2.period : 0x1000);
            memory[location] = value;
            break;

        case 0x03:
            value = value & 0x00FF;
            envelopePeriod = (envelopePeriod & 0xFF00) |
                    value;
            envelopePeriodValue = (envelopePeriod
                    ? (envelopePeriod << 1) : 0x20000);
            memory[location] = value;
            break;

        case 0x04:
            value = value & 0x000F;
            channel0.period = (channel0.period & 0x00FF) |
                    (value<<8);
            channel0.periodValue = (channel0.period
                    ? channel0.period : 0x1000);
            memory[location] = value;
            break;

        case 0x05:
            value = value & 0x000F;
            channel1.period = (channel1.period & 0x00FF) |
                    (value<<8);
            channel1.periodValue = (channel1.period
                    ? channel1.period : 0x1000);
            memory[location] = value;
            break;

        case 0x06:
            value = value & 0x000F;
            channel2.period = (channel2.period & 0x00FF) |
                    (value<<8);
            channel2.periodValue = (channel2.period
                    ? channel2.period : 0x1000);
            memory[location] = value;
            break;

        case 0x07:
            value = value & 0x00FF;
            envelopePeriod = (envelopePeriod & 0x00FF) |
                    (value<<8);
            envelopePeriodValue = (envelopePeriod
                    ? (envelopePeriod << 1) : 0x20000);
            memory[location] = value;
            break;

        case 0x08:
            value = value & 0x00FF;
            channel0.toneDisabled = !!(value & 0x0001);
            channel1.toneDisabled = !!(value & 0x0002);
            channel2.toneDisabled = !!(value & 0x0004);
            channel0.noiseDisabled = !!(value & 0x0008);
            channel1.noiseDisabled = !!(value & 0x0010);
            channel2.noiseDisabled = !!(value & 0x0020);
            channel0.isDirty = TRUE;
            channel1.isDirty = TRUE;
            channel2.isDirty = TRUE;
            noiseIdle = channel0.noiseDisabled &
                    channel1.noiseDisabled &
                    channel2.noiseDisabled;
            memory[location] = value;
            break;

        case 0x09:
            value = value & 0x001F;
            noisePeriod = value;
            noisePeriodValue = (noisePeriod
                    ? (noisePeriod << 1) : 0x0040);
            memory[location] = value;
            break;

        case 0x0A:
            value = value & 0x000F;
            envelopeHold = !!(value & 0x0001);
            envelopeAltr = !!(value & 0x0002);
            envelopeAtak = !!(value & 0x0004);
            envelopeCont = !!(value & 0x0008);
            envelopeVolume = (envelopeAtak ? 0 : 15);
            envelopeCounter = envelopePeriodValue;
            envelopeIdle = FALSE;
            memory[location] = value;
            break;

        case 0x0B:
            value = value & 0x003F;
            channel0.envelope = !!(value & 0x0010);
            channel0.volume = (value & 0x000F);
            channel0.isDirty = TRUE;
            memory[location] = value;
            break;

        case 0x0C:
            value = value & 0x003F;
            channel1.envelope = !!(value & 0x0010);
            channel1.volume = (value & 0x000F);
            channel1.isDirty = TRUE;
            memory[location] = value;
            break;

        case 0x0D:
            value = value & 0x003F;
            channel2.envelope = !!(value & 0x0010);
            channel2.volume = (value & 0x000F);
            channel2.isDirty = TRUE;
            memory[location] = value;
            break;

        case 0x0E:
            ay38914->psgIO1->setOutputValue(value);
            break;

        case 0x0F:
            ay38914->psgIO0->setOutputValue(value);
            break;
    }
}

UINT16 AY38914_Registers::peek(UINT16 location)
{
    location &= 0x0F;
    switch(location) {
        case 0x0E:
            return ay38914->psgIO1->getInputValue();
        case 0x0F:
            return ay38914->psgIO0->getInputValue();
        default:
            return memory[location];
    }
}

