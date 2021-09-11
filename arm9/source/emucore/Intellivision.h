
#ifndef INTELLIVISION_H
#define INTELLIVISION_H

#include "Emulator.h"
#include "HandController.h"
#include "Intellivoice.h"
#include "Emulator.h"
#include "MemoryBus.h"
#include "RAM.h"
#include "ROM.h"
#include "GROM.h"
#include "JLP.h"
#include "CP1610.h"
#include "AY38900.h"
#include "AY38914.h"

#define RAM8BIT_SIZE    0x00F0
#define RAM16BIT_SIZE   0x0160

class Intellivision : public Emulator
{
    public:
        Intellivision();

    private:
        //core processors
        CP1610            cpu;
        AY38900           stic;
        AY38914           psg;
    
        //core memories
        RAM         RAM8bit;
        RAM         RAM16bit;
        ROM         execROM;
        GROM        grom;
        GRAM        gram;

        //hand controllers
        HandController    player1Controller;
        HandController    player2Controller;

        //the Intellivoice peripheral
        Intellivoice      intellivoice;
};

#endif
