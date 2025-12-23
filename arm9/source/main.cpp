// =====================================================================================
// Copyright (c) 2021-2025 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
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
#include "nintv-ds.h"
#include "highscore.h"
#include "Memory.h"

// ------------------------------------------------------------------------
// If we are being passed a file on the command line - we store it here.
// ------------------------------------------------------------------------
char file[128];

UINT32 MAX_ROM_FILE_SIZE = 0;

UINT8 *bin_image_buf = NULL;
UINT16 *bin_image_buf16 = NULL;

volatile int ds_vblank_count __attribute__((section(".dtcm"))) = 0;
ITCM_CODE void irqVBlank(void)
{
    ds_vblank_count++; // This is our key to DS 'True Sync' at 60Hz.
}


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
  
  MAX_ROM_FILE_SIZE             = (1024 * 1024);    // Simply massive... Covers everything known to mankind.
    
  bin_image_buf = new UINT8[MAX_ROM_FILE_SIZE];
  bin_image_buf16 = (UINT16*)bin_image_buf;
    
  // Init Timer
  dsInitTimer();
  
  // ----------------------------------------------------------------
  // Trigger DS vblank somewhat near the bottom of the display 
  // rendering - we will use this trigger to start our copy from
  // the pixelBuffer[] to the LCD for a reasonably tear-free output.
  // ----------------------------------------------------------------
  SetYtrigger(180);
  irqSet(IRQ_VCOUNT, irqVBlank);
  irqEnable(IRQ_VCOUNT);  
    
  // Setup the main screen handling
  dsInitScreenMain();

  // Init the high-score tables...
  highscore_init();
  
  // Load the current Favorites
  LoadFavorites();
    
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
