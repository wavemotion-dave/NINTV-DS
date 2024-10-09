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

#include <stdio.h>
#include <string.h>
#include "AY38914.h"
#include "AY38914_Registers.h"
#include "../nintv-ds.h"

AY38914_Registers::AY38914_Registers(UINT16 address)
: RAM(0x10, address, 0xFFFF, 0xFFFF)
{}

void AY38914_Registers::init(AY38914* ay38914)
{
    this->ay38914 = ay38914;
}

void AY38914_Registers::reset()
{
    memset(psg_memory, 0, sizeof(psg_memory));
}

ITCM_CODE void AY38914_Registers::poke(UINT16 location, UINT16 value)
{
    location &= 0x0F;
    switch(location) {
        case 0x00:
            value = value & 0x00FF;
            ay38914->channel0.period = (ay38914->channel0.period & 0x0F00) |
                    value;
            ay38914->channel0.periodValue = (ay38914->channel0.period
                    ? ay38914->channel0.period : 0x1000);
            psg_memory[location] = value;
            break;

        case 0x01:
            value = value & 0x00FF;
            ay38914->channel1.period = (ay38914->channel1.period & 0x0F00) |
                    value;
            ay38914->channel1.periodValue = (ay38914->channel1.period
                    ? ay38914->channel1.period : 0x1000);
            psg_memory[location] = value;
            break;

        case 0x02:
            value = value & 0x00FF;
            ay38914->channel2.period = (ay38914->channel2.period & 0x0F00) |
                    value;
            ay38914->channel2.periodValue = (ay38914->channel2.period
                    ? ay38914->channel2.period : 0x1000);
            psg_memory[location] = value;
            break;

        case 0x03:
            value = value & 0x00FF;
            ay38914->envelopePeriod = (ay38914->envelopePeriod & 0xFF00) |
                    value;
            ay38914->envelopePeriodValue = (ay38914->envelopePeriod
                    ? (ay38914->envelopePeriod << 1) : 0x20000);
            psg_memory[location] = value;
            break;

        case 0x04:
            value = value & 0x000F;
            ay38914->channel0.period = (ay38914->channel0.period & 0x00FF) |
                    (value<<8);
            ay38914->channel0.periodValue = (ay38914->channel0.period
                    ? ay38914->channel0.period : 0x1000);
            psg_memory[location] = value;
            break;

        case 0x05:
            value = value & 0x000F;
            ay38914->channel1.period = (ay38914->channel1.period & 0x00FF) |
                    (value<<8);
            ay38914->channel1.periodValue = (ay38914->channel1.period
                    ? ay38914->channel1.period : 0x1000);
            psg_memory[location] = value;
            break;

        case 0x06:
            value = value & 0x000F;
            ay38914->channel2.period = (ay38914->channel2.period & 0x00FF) |
                    (value<<8);
            ay38914->channel2.periodValue = (ay38914->channel2.period
                    ? ay38914->channel2.period : 0x1000);
            psg_memory[location] = value;
            break;

        case 0x07:
            value = value & 0x00FF;
            ay38914->envelopePeriod = (ay38914->envelopePeriod & 0x00FF) |
                    (value<<8);
            ay38914->envelopePeriodValue = (ay38914->envelopePeriod
                    ? (ay38914->envelopePeriod << 1) : 0x20000);
            psg_memory[location] = value;
            break;

        case 0x08:
            psg_memory[location] = value;
            value = value & 0x00FF;
            ay38914->channel0.toneDisabled = !!(value & 0x0001);
            ay38914->channel1.toneDisabled = !!(value & 0x0002);
            ay38914->channel2.toneDisabled = !!(value & 0x0004);
            ay38914->channel0.noiseDisabled = !!(value & 0x0008);
            ay38914->channel1.noiseDisabled = !!(value & 0x0010);
            ay38914->channel2.noiseDisabled = !!(value & 0x0020);
            ay38914->channel0.isDirty = TRUE;
            ay38914->channel1.isDirty = TRUE;
            ay38914->channel2.isDirty = TRUE;
            ay38914->noiseIdle = ay38914->channel0.noiseDisabled &
                    ay38914->channel1.noiseDisabled &
                    ay38914->channel2.noiseDisabled;
            ay38914->psgIO0->setDirectionIO(value);
            ay38914->psgIO1->setDirectionIO(value);
            break;

        case 0x09:
            value = value & 0x001F;
            ay38914->noisePeriod = value;
            ay38914->noisePeriodValue = (ay38914->noisePeriod ? (ay38914->noisePeriod << 1) : 0x0040);
            psg_memory[location] = value;
            break;

        case 0x0A:
            value = value & 0x000F;
            ay38914->envelopeHold = !!(value & 0x0001);
            ay38914->envelopeAltr = !!(value & 0x0002);
            ay38914->envelopeAtak = !!(value & 0x0004);
            ay38914->envelopeCont = !!(value & 0x0008);
            ay38914->envelopeVolume = (ay38914->envelopeAtak ? 0 : 15);
            ay38914->envelopeCounter = ay38914->envelopePeriodValue;
            ay38914->envelopeIdle = FALSE;
            psg_memory[location] = value;
            break;

        case 0x0B:
            value = value & 0x003F;
            ay38914->channel0.envelope = !!(value & 0x0010);
            ay38914->channel0.volume = (value & 0x000F);
            ay38914->channel0.isDirty = TRUE;
            psg_memory[location] = value;
            break;

        case 0x0C:
            value = value & 0x003F;
            ay38914->channel1.envelope = !!(value & 0x0010);
            ay38914->channel1.volume = (value & 0x000F);
            ay38914->channel1.isDirty = TRUE;
            psg_memory[location] = value;
            break;

        case 0x0D:
            value = value & 0x003F;
            ay38914->channel2.envelope = !!(value & 0x0010);
            ay38914->channel2.volume = (value & 0x000F);
            ay38914->channel2.isDirty = TRUE;
            psg_memory[location] = value;
            break;

        case 0x0E:
            // Make sure we are configured for output...
            ay38914->psgIO1->setOutputValue(value);
            break;

        case 0x0F:
            // Make sure we are configured for output...
            ay38914->psgIO0->setOutputValue(value);
            break;
    }
}

ITCM_CODE UINT16 AY38914_Registers::peek(UINT16 location)
{
    location &= 0x0F;
    switch(location) {
        case 0x0E:
            return ay38914->psgIO1->getInputValue((UINT8)location);
        case 0x0F:
            return ay38914->psgIO0->getInputValue((UINT8)location);
        default:
            return psg_memory[location];
    }
}

