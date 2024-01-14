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

#include <nds.h>
#include <nds/fifomessages.h>

#include<stdio.h>

#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "nintv-ds.h"
#include "overlay.h"
#include "config.h"
#include "bgBottom.h"
#include "bgBottom-ECS.h"
#include "bgTop.h"
#include "Emulator.h"
#include "Rip.h"

// ------------------------------------------------
// Reuse the char buffer from the game load... 
// we wouldn't need to use this at the same time.
// ------------------------------------------------
extern char szName[];
extern Rip *currentRip;

// ----------------------------------------------------------------------------------------
// This is the default overlay that matches the main non-custom overlay bottom screen.
// ----------------------------------------------------------------------------------------
struct Overlay_t defaultOverlay[OVL_MAX] =
{
    {120,   155,    30,     60},    // KEY_1
    {158,   192,    30,     60},    // KEY_2
    {195,   230,    30,     60},    // KEY_3
    
    {120,   155,    65,     95},    // KEY_4
    {158,   192,    65,     95},    // KEY_5
    {195,   230,    65,     95},    // KEY_6

    {120,   155,    101,   135},    // KEY_7
    {158,   192,    101,   135},    // KEY_8
    {195,   230,    101,   135},    // KEY_9

    {120,   155,    140,   175},    // KEY_CLEAR
    {158,   192,    140,   175},    // KEY_0
    {195,   230,    140,   175},    // KEY_ENTER

    {255,   255,    255,   255},    // KEY_FIRE
    {255,   255,    255,   255},    // KEY_L_ACT
    {255,   255,    255,   255},    // KEY_R_ACT
    
    { 23,    82,     16,    36},    // META_RESET
    { 23,    82,     45,    65},    // META_LOAD
    { 23,    82,     74,    94},    // META_CONFIG
    { 23,    82,    103,   123},    // META_SCORE
    {255,   255,    255,   255},    // META_QUIT
    { 23,    82,    132,   152},    // META_STATE
    { 23,    82,    161,   181},    // META_MENU
    {255,   255,    255,   255},    // META_SWAP
    {255,   255,    255,   255},    // META_MANUAL
    {255,   255,    255,   255},    // META_STRETCH
    {255,   255,    255,   255},    // META_GCONFIG        
};

// ----------------------------------------------------------------------------------------
// This is the ECS overlay with reduced menu options...
// ----------------------------------------------------------------------------------------
struct Overlay_t ecsOverlay[OVL_MAX] =
{
    {120,   155,    30,     60},    // KEY_1
    {158,   192,    30,     60},    // KEY_2
    {195,   230,    30,     60},    // KEY_3
    
    {120,   155,    65,     95},    // KEY_4
    {158,   192,    65,     95},    // KEY_5
    {195,   230,    65,     95},    // KEY_6

    {120,   155,    101,   135},    // KEY_7
    {158,   192,    101,   135},    // KEY_8
    {195,   230,    101,   135},    // KEY_9

    {120,   155,    140,   175},    // KEY_CLEAR
    {158,   192,    140,   175},    // KEY_0
    {195,   230,    140,   175},    // KEY_ENTER

    {255,   255,    255,   255},    // KEY_FIRE
    {255,   255,    255,   255},    // KEY_L_ACT
    {255,   255,    255,   255},    // KEY_R_ACT
    
    {255,   255,    255,   255},    // META_RESET
    {255,   255,    255,   255},    // META_LOAD
    {255,   255,    255,   255},    // META_CONFIG
    {255,   255,    255,   255},    // META_SCORE
    {255,   255,    255,   255},    // META_QUIT
    {255,   255,    255,   255},    // META_STATE
    { 55,   101,      2,    20},    // META_MENU
    {255,   255,    255,   255},    // META_SWAP
    {255,   255,    255,   255},    // META_MANUAL
    {255,   255,    255,   255},    // META_STRETCH
    {255,   255,    255,   255},    // META_GCONFIG        
};


struct Overlay_t myOverlay[OVL_MAX];
struct Overlay_t myDisc[DISC_MAX];


// -------------------------------------------------------------
// Rather than take up precious RAM, we use some video memory.
// -------------------------------------------------------------
unsigned int *customTiles = (unsigned int *) 0x06880000;          //60K of video memory for the tiles (largest I've seen so far is ~48K)
unsigned short *customMap = (unsigned short *)0x0688F000;         // 4K of video memory for the map (generally about 2.5K)
unsigned short customPal[512];

char directory[128];
char filename[128];

// -----------------------------------------------------------------------------------------------
// Custom overlays are read in and must be in a very strict format. See the documenatation
// for custom overlays for details on the format this must be in. We could probably use a
// bit more error checking here... but we expect customer overlay designers to know what's up.
// -----------------------------------------------------------------------------------------------
void load_custom_overlay(bool bCustomGeneric)
{
    FILE *fp = NULL;
    
    // -------------------------------------------------------
    // Read the associated .ovl file and parse it... Start by 
    // getting the root folder where overlays are stored...
    // -------------------------------------------------------
    if (myGlobalConfig.ovl_dir == 1)        // In: /ROMS/OVL
    {
        strcpy(directory, "/roms/ovl/");
    }
    else if (myGlobalConfig.ovl_dir == 2)   // In: /ROMS/INTY/OVL
    {
        strcpy(directory, "/roms/intv/ovl/");
    }
    else if (myGlobalConfig.ovl_dir == 3)   // In: /DATA/OVL/
    {
        strcpy(directory, "/data/ovl/");
    }
    else
    {
        strcpy(directory, "./");              // In: Same DIR as ROM files
    }
    
    u8 bFound = 0;
    // If we have a game (RIP) loaded, try to find a matching overlay
    if (currentRip != NULL)
    {
        strcpy(filename, directory);
        strcat(filename, currentRip->GetFileName());
        filename[strlen(filename)-4] = 0;
        strcat(filename, ".ovl");
        fp = fopen(filename, "rb");
        if (fp != NULL) // If file found
        {
            bFound = 1;
        }
    }

    if (bFound == 0)
    {
        strcpy(filename, directory);
        strcat(filename, "generic.ovl");   
        fp = fopen(filename, "rb");
    }    
    
    // Default these to unused... 
    bUseDiscOverlay = false;
    for (UINT8 i=0; i < DISC_MAX; i++)
    {
        myDisc[i].x1 = 255;
        myDisc[i].x2 = 255;
        myDisc[i].y1 = 255;
        myDisc[i].y2 = 255;
    }
    
    if (fp != NULL)     // If overlay found, parse it and use it...
    {
      UINT8 ov_idx = 0;
      UINT8 disc_idx=0;
      UINT16 tiles_idx=0;
      UINT16 map_idx=0;
      UINT16 pal_idx=0;
      char *token;

      memset(customTiles, 0x00, 0x10000);   // Clear the 64K of video memory to prep for custom tiles...

      do
      {
        fgets(szName, 255, fp);
        // Handle Overlay Line
        if (strstr(szName, ".ovl") != NULL)
        {
            if (ov_idx < OVL_MAX)
            {
                char *ptr = strstr(szName, ".ovl");
                ptr += 5;
                myOverlay[ov_idx].x1 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myOverlay[ov_idx].x2 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myOverlay[ov_idx].y1 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myOverlay[ov_idx].y2 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                ov_idx++;                
            }
        }

        // Handle Disc Line
        if (strstr(szName, ".disc") != NULL)
        {
            bUseDiscOverlay = true;
            if (disc_idx < DISC_MAX)
            {
                char *ptr = strstr(szName, ".disc");
                ptr += 6;
                myDisc[disc_idx].x1 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myDisc[disc_idx].x2 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myDisc[disc_idx].y1 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myDisc[disc_idx].y2 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                disc_idx++;                
            }
        }
          
        // Handle HUD_x Line
        if (strstr(szName, ".hudx") != NULL)
        {
            char *ptr = strstr(szName, ".hudx");
            ptr += 6;
            hud_x = strtoul(ptr, &ptr, 10);
        }          

        // Handle HUD_y Line
        if (strstr(szName, ".hudy") != NULL)
        {
            char *ptr = strstr(szName, ".hudy");
            ptr += 6;
            hud_y = strtoul(ptr, &ptr, 10);
        }          
          
        // Handle Tile Line
        if (strstr(szName, ".tile") != NULL)
        {
            char *ptr = strstr(szName, ".tile");
            ptr += 6;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customTiles[tiles_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
        }

        // Handle Map Line
        if (strstr(szName, ".map") != NULL)
        {
            char *ptr = strstr(szName, ".map");
            ptr += 4;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customMap[map_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
        }              

        // Handle Palette Line
        if (strstr(szName, ".pal") != NULL)
        {
            char *ptr = strstr(szName, ".pal");
            ptr += 4;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
            customPal[pal_idx++] = strtoul(ptr, &ptr, 16); while (*ptr == ',' || *ptr == ' ') ptr++;
        }              
      } while (!feof(fp));
      fclose(fp);

      decompress(customTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(customMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) customPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else  // Otherwise, just use the built-in default/generic overlay...
    {
      decompress(bgBottomTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottomMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottomPal,(u16*) BG_PALETTE_SUB,256*2);
    }    
}


// ---------------------------------------------------------------------------
// This puts the overlay on the main screen. It can be one of the built-in 
// overlays or it might be a custom overlay that will be rendered...
// ---------------------------------------------------------------------------
void show_overlay(void)
{
    // Assume default overlay... custom can change it below...
    memcpy(&myOverlay, &defaultOverlay, sizeof(myOverlay));
    
    swiWaitForVBlank();
    
    if (myConfig.overlay == 1) // ECS mini keyboard overlay
    {
      decompress(bgBottom_ECSTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_ECSMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_ECSPal,(u16*) BG_PALETTE_SUB,256*2);
      // ECS Overlay...  
      memcpy(&myOverlay, &ecsOverlay, sizeof(myOverlay));
    }
    else    // Default Overlay... which might be custom!!
    {
        load_custom_overlay(true);  // This will try to load nintv-ds.ovl or else default to the generic background
    }
    
    unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
    dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

    REG_BLDCNT=0; REG_BLDCNT_SUB=0; REG_BLDY=0; REG_BLDY_SUB=0;
    
    swiWaitForVBlank();    
}

// End of Line
