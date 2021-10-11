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
#include <nds.h>
#include <string.h>
#include "AY38900.h"
#include "AY38900_Registers.h"

UINT16   memory[0x40] __attribute__((section(".dtcm")));

AY38900_Registers::AY38900_Registers()
: RAM(0x40, 0x0000, 0xFFFF, 0x3FFF)
{}

void AY38900_Registers::init(AY38900* ay38900)
{
    this->ay38900 = ay38900;
}

void AY38900_Registers::reset()
{
    memset(memory, 0, sizeof(memory));
}

ITCM_CODE void AY38900_Registers::poke(UINT16 location, UINT16 value) {
    if (!enabled)
        return;

    //incomplete decoding on writes
    location &= 0x3F;

    switch(location) {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            value &= 0x07FF;
            {
            MOB* mob = &ay38900->mobs[location];
            mob->setDoubleWidth((value & 0x0400) != 0);
            mob->setVisible((value & 0x0200) != 0);
            mob->setFlagCollisions((value & 0x0100) != 0);
            mob->setXLocation(value & 0x00FF);
            }
            break;
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
        case 0x0F:
            value &= 0x0FFF;
            {
            MOB* mob = &ay38900->mobs[location & 0x07];
            mob->setVerticalMirror((value & 0x0800) != 0);
            mob->setHorizontalMirror((value & 0x0400) != 0);
            mob->setQuadHeight((value & 0x0200) != 0);
            mob->setDoubleHeight((value & 0x0100) != 0);
            mob->setDoubleYResolution((value & 0x0080) != 0);
            mob->setYLocation(value & 0x007F);
            }
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
            value &= 0x3FFF;
            {
            MOB* mob = &ay38900->mobs[location & 0x07];
            mob->setBehindForeground((value & 0x2000) != 0);
            mob->setGROM((value & 0x0800) == 0);
            mob->setCardNumber((value & 0x07F8) >> 3);
            mob->setForegroundColor(((value & 0x1000) >> 9) |
                    (value & 0x0007));
            }
            break;
        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
            //a MOB's own collision bit is *always* zero, even if a
            //one is poked into it
            value &= (~(1 << (location & 0x07))) & 0x03FF;
            break;
        case 0x20:
            ay38900->displayEnabled = TRUE;
            break;
        case 0x21:
            value = 0;
            if (ay38900->colorStackMode) {
                ay38900->colorStackMode = FALSE;
                ay38900->colorModeChanged = TRUE;
            }
            break;
        case 0x28:
        case 0x29:
        case 0x2A:
        case 0x2B:
            value &= 0x000F;
            ay38900->colorStackChanged = TRUE;
            break;
        case 0x2C:
            value &= 0x000F;
            ay38900->borderColor = (UINT8)value;
            ay38900->bordersChanged = TRUE;
            break;
        case 0x30:
            value &= 0x0007;
            ay38900->horizontalOffset = value;
            ay38900->offsetsChanged = TRUE;
            break;
        case 0x31:
            value &= 0x0007;
            ay38900->verticalOffset = value;
            ay38900->offsetsChanged = TRUE;
            break;
        case 0x32:
            value &= 0x0003;
            ay38900->blockLeft = (value & 0x0001) != 0;
            ay38900->blockTop = (value & 0x0002) != 0;
            ay38900->bordersChanged = TRUE;
            break;
        default:  //  0x22 - 0x27
            value = 0;
            break;
    }
    memory[location] = value;
}

ITCM_CODE UINT16 AY38900_Registers::peek(UINT16 location)
{
    if (!enabled)
        return location&0x3FFF;
    location &= 0x3F;

    switch (location) {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            return 0x3800 | memory[location];
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
        case 0x0F:
            return 0x3000 | memory[location];
        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
            //collision register bits $3C00 are always on
            return 0x3C00 | memory[location];
        case 0x20:
            return 0;
        case 0x21:
            if (location == 0x0021 && !ay38900->colorStackMode) {
                ay38900->colorStackMode = TRUE;
                ay38900->colorModeChanged = TRUE;
            }
            return memory[location];
        case 0x28:
        case 0x29:
        case 0x2A:
        case 0x2B:
        case 0x2C:
            return 0x3FF0 | memory[location];
        case 0x30:
        case 0x31:
            return 0x3FF8 | memory[location];
        case 0x32:
            return 0x3FFC | memory[location];
        default:
            return memory[location];
    }
}
