
#ifndef ECS_H
#define ECS_H

#include "Peripheral.h"
#include "ECSKeyboard.h"
#include "AudioOutputLine.h"
#include "RAM.h"
#include "ROM.h"
#include "types.h"
#include "AY38914.h"

#define ECS_RAM_SIZE        0x0800
#define ECS_RAM_LOCATION    0x4000

TYPEDEF_STRUCT_PACK( _ECSState
{
    AY38914State psg2State;
} ECSState; )

class Intellivision;

class ECS : public Peripheral
{
    friend class Intellivision;

    public:
        ECS();
        
        void getState(ECSState *state);
        void setState(ECSState *state);

    private:
        ECSKeyboard         keyboard;

    public:
        ROM       bank0;
        ROM       bank1;
        ROM       bank2;
        RAM       ramBank;
        AY38914   psg2;
};

#endif
