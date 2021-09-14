#include <nds.h>
#include <fat.h>
#include <filesystem.h>
#include <stdio.h>
#include <time.h>
#include "ds_tools.h"
#include "highscore.h"

UINT16 SOUND_FREQ __attribute__((section(".dtcm"))) = 15360;
    
int main(int argc, char **argv) 
{
  // Init sound
  consoleDemoInit();
  soundEnable();
  lcdMainOnTop();
    
  if(argc > 0)
  {
    // Init NitroFS
    if(!nitroFSInit(nullptr))
    {
      iprintf("Unable to initialize NitroFS!\n");
      while(1)
        swiWaitForVBlank();
    }
  }
  else
  {
    // Init Fat
    if (!fatInitDefault()) 
    {
      iprintf("Unable to initialize libfat!\n");
      while(1)
        swiWaitForVBlank();
    }
  }
    
  // For the older DS-LITE and DS-PHAT hardware, we lower the sample rate
  if (!isDSiMode())
  {
    SOUND_FREQ = 11100;
  }
    
  srand(time(0));
    
  // Init the high-score tables...
  highscore_init();

  // Init Timer
  dsInitTimer();
    
  // Setup the main screen handling
  dsInitScreenMain();
    
  // Main loop of emulation
  dsMainLoop();
  	
  // Free memory to be correct 
  dsFreeEmu();
 
  return 0;
}

