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
#include <fat.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "ds_tools.h"
#include "highscore.h"

// ------------------------------------------------------------------------
// If we are being passed a file on the command line - we store it here.
// ------------------------------------------------------------------------
char file[64];

// --------------------------------------------------------
// DSi default is 15KHz but we reduce this for DS mode...
// --------------------------------------------------------
UINT16 __attribute__((section(".dtcm"))) mySoundFrequency = 15360;

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
    
  srand(time(0));
    
  // For the DS-LITE/PHAT we need more speed so we reduce the sound quality a bit...
  if (!isDSiMode()) mySoundFrequency = 12000;
    
  // Init the high-score tables...
  highscore_init();

  // Init Timer
  dsInitTimer();
    
  // Setup the main screen handling
  dsInitScreenMain();
    
  // Handle command line argument... mostly for TWL++
  if (argc > 1) 
  {
      // We want to start in the directory where the file is being launched...
      if (strchr(argv[1], '/') != NULL)
      {
          char path[128];
          strcpy(path, argv[1]);
          char *ptr = &path[strlen(path)-1];
          while (*ptr != '/') ptr--;
          ptr++; 
          strcpy(file, ptr);
          *ptr=NULL;
          chdir(path);
      }
      else
      {
          strcpy(file, argv[1]);
      }
      
      // Main loop of emulation - with initial file
      dsMainLoop(file);
  } 
  else
  {
      // Main loop of emulation - no initial file
      dsMainLoop(NULL);
  }
  	
  // Free memory to be correct 
  dsFreeEmu();
 
  return 0;
}

// End of file
