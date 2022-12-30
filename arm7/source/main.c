// =====================================================================================
// Copyright (c) 2021-2023 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

/*---------------------------------------------------------------------------------

    default ARM7 core

        Copyright (C) 2005 - 2010
        Michael Noland (joat)
        Jason Rogers (dovoto)
        Dave Murphy (WinterMute)

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any
    damages arising from the use of this software.

    Permission is granted to anyone to use this software for any
    purpose, including commercial applications, and to alter it and
    redistribute it freely, subject to the following restrictions:

    1.  The origin of this software must not be misrepresented; you
        must not claim that you wrote the original software. If you use
        this software in a product, an acknowledgment in the product
        documentation would be appreciated but is not required.

    2.  Altered source versions must be plainly marked as such, and
        must not be misrepresented as being the original software.

    3.  This notice may not be removed or altered from any source
        distribution.

---------------------------------------------------------------------------------*/
#include <nds.h>
#include <dswifi7.h>
#include <maxmod7.h>

extern void installSoundEmuFIFO(void);


//---------------------------------------------------------------------------------
void VblankHandler(void) {
//---------------------------------------------------------------------------------
    Wifi_Update();
}


//---------------------------------------------------------------------------------
void VcountHandler() {
//---------------------------------------------------------------------------------
    inputGetAndSend();
}

volatile bool exitflag = false;

//---------------------------------------------------------------------------------
void powerButtonCB() {
//---------------------------------------------------------------------------------
    exitflag = true;
}

//---------------------------------------------------------------------------------
int main() {
//---------------------------------------------------------------------------------
    readUserSettings();

    irqInit();
    // Start the RTC tracking IRQ
    initClockIRQ();
    touchInit();
    fifoInit();

    //mmInstall(FIFO_MAXMOD);

    SetYtrigger(80);

    installWifiFIFO();
    installSoundFIFO();

    installSystemFIFO();

    installSoundEmuFIFO();

    irqSet(IRQ_VCOUNT, VcountHandler);
    irqSet(IRQ_VBLANK, VblankHandler);

    irqEnable( IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK);
    
    setPowerButtonCB(powerButtonCB);   

    // Keep the ARM7 mostly idle
    while (!exitflag) {
        if ( 0 == (REG_KEYINPUT & (KEY_SELECT | KEY_START | KEY_L | KEY_R))) {
            exitflag = true;
        }
    
        swiWaitForVBlank();
    }
    return 0;
}
