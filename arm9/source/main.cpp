// =====================================================================================
// Copyright (c) 2021-2026 Dave Bernazzani (wavemotion-dave)
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
#include "config.h"

// ------------------------------------------------------------------------
// If we are being passed a file on the command line - we store it here.
// ------------------------------------------------------------------------
static char file[160];

// ---------------------------------------------------------------------------
// Pre-allocate the big ROM buffer - this is large enough to handle any size
// game ROM that we might encounter... plus a bit more for future-proofing.
// ---------------------------------------------------------------------------
UINT8 CartBuffer[MAX_ROM_FILE_SIZE];

volatile int ds_vblank_count __attribute__((section(".dtcm"))) = 0;
ITCM_CODE void irqVCount(void)
{
    ds_vblank_count++; // This is our key to DS 'True Sync' at 60Hz.
}

// ---------------------------------------------------------------------------------------
// Due to the 160 resolution of the Intellivision being mapped onto the 256 pixel DS LCD,
// we see artifacts of some 'pixles' being thicker than others. As a trick, we can shift 
// the underlying LCD rendering slightly to blur this and trick the eye into seeing more
// uniform sized pixels. It's not perfect, but helps for games like Maze-a-TRON.
// ---------------------------------------------------------------------------------------
static u8 sIndex     __attribute__((section(".dtcm"))) = 0;
static u8 jitter[4]  __attribute__((section(".dtcm"))) = {0x00, 0x40, 0x00, 0x40};
ITCM_CODE void irqVBlank(void)
{
    if (myConfig.lcd_jitter)
    {
        REG_BG3X = ((myConfig.offset_x) << 8) + jitter[sIndex++ & 0x03];
    }
    else 
    {
        REG_BG3X = ((myConfig.offset_x) << 8);
    }
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

  // Init Timer
  dsInitTimer();
  
  // ----------------------------------------------------------------
  // Trigger DS vblank somewhat near the bottom of the display 
  // rendering - we will use this trigger to start our copy from
  // the pixelBuffer[] to the LCD for a reasonably tear-free output.
  // ----------------------------------------------------------------
  SetYtrigger(180);
  irqSet(IRQ_VCOUNT, irqVCount);
  irqEnable(IRQ_VCOUNT);  
  irqSet(IRQ_VBLANK, irqVBlank);
  irqEnable(IRQ_VBLANK);  
    
  // Setup the main screen handling
  dsInitScreenMain();

  // Init the high-score tables...
  highscore_init();
  
  // Load the current Favorites
  LoadFavorites();

  // Ensure random numbers are reasonably random
  srand(time(0));
    
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
