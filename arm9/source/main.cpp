#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include "ds_tools.h"
#include "highscore.h"

#include "clickNoQuit_wav.h"
#include "clickQuit_wav.h"

int main(int argc, char **argv) 
{
  // Init sound
  consoleDemoInit();
  soundEnable();
  lcdMainOnTop();
    
  // Init Fat
  if (!fatInitDefault()) 
  {
      iprintf("Unable to initialize libfat!\n");
	  return -1;
  }
    
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

