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
#include <nds/fifomessages.h>

#include<stdio.h>

#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "ds_tools.h"
#include "overlay.h"
#include "config.h"
#include "bgBottom.h"
#include "bgBottom-treasure.h"
#include "bgBottom-cloudy.h"
#include "bgBottom-astro.h"
#include "bgBottom-spartans.h"
#include "bgBottom-b17.h"
#include "bgBottom-atlantis.h"
#include "bgBottom-bombsquad.h"
#include "bgBottom-utopia.h"
#include "bgBottom-swords.h"
#include "bgTop.h"
#include "Emulator.h"
#include "Rip.h"

extern char szName[];
extern Rip *currentRip;

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

struct Overlay_t myOverlay[OVL_MAX];
struct Overlay_t myDisc[DISC_MAX];


unsigned int *customTiles = (unsigned int *) 0x06860000;          //128K of video memory
unsigned short *customMap = (unsigned short *)0x068A0000;         //16K of video memory
unsigned short customPal[512];

char filename[128];
void load_custom_overlay(void)
{
    FILE *fp = NULL;
    // Read the associated .ovl file and parse it...
    if (currentRip != NULL)
    {
        if (myGlobalConfig.ovl_dir == 1)        // In: /ROMS/OVL
        {
            strcpy(filename, "/roms/ovl/");
        }
        else if (myGlobalConfig.ovl_dir == 2)   // In: /ROMS/INTY/OVL
        {
            strcpy(filename, "/roms/intv/ovl/");
        }
        else if (myGlobalConfig.ovl_dir == 3)   // In: /DATA/OVL/
        {
            strcpy(filename, "/data/ovl/");
        }
        else
        {
            strcpy(filename, "./");              // In: Same DIR as ROM files
        }
        strcat(filename, currentRip->GetFileName());
        filename[strlen(filename)-4] = 0;
        strcat(filename, ".ovl");
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
    
    if (fp != NULL)
    {
      UINT8 ov_idx = 0;
      UINT8 disc_idx=0;
      UINT16 tiles_idx=0;
      UINT16 map_idx=0;
      UINT16 pal_idx=0;
      char *token;

      memset(customTiles, 0x00, 24*1024*sizeof(UINT32));
      memset(customMap, 0x00, 16*1024*sizeof(UINT16));
      memset(customPal, 0x00, 512*sizeof(UINT16));

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
    else
    {
      decompress(bgBottomTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottomMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottomPal,(u16*) BG_PALETTE_SUB,256*2);
    }    
}

void show_overlay(void)
{
    // Assume default overlay... custom can change it below...
    memcpy(&myOverlay, &defaultOverlay, sizeof(myOverlay));
    
    swiWaitForVBlank();
    
    if (myConfig.overlay_selected == 1) // Custom Overlay!
    {
        load_custom_overlay();
    }
    else if (myConfig.overlay_selected == 2) // Treasure of Tarmin
    {
      decompress(bgBottom_treasureTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_treasureMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_treasurePal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 3) // Cloudy Mountain
    {
      decompress(bgBottom_cloudyTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_cloudyMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_cloudyPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 4) // Astrosmash
    {
      decompress(bgBottom_astroTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_astroMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_astroPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 5) // Space Spartans
    {
      decompress(bgBottom_spartansTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_spartansMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_spartansPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 6) // B17 Bomber
    {
      decompress(bgBottom_b17Tiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_b17Map, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_b17Pal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 7) // Atlantis
    {
      decompress(bgBottom_atlantisTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_atlantisMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_atlantisPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 8) // Bomb Squad
    {
      decompress(bgBottom_bombsquadTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_bombsquadMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_bombsquadPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 9) // Utopia
    {
      decompress(bgBottom_utopiaTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_utopiaMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_utopiaPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else if (myConfig.overlay_selected == 10) // Swords & Serpents
    {
      decompress(bgBottom_swordsTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_swordsMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_swordsPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    else    // Default Overlay...
    {
      decompress(bgBottomTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottomMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottomPal,(u16*) BG_PALETTE_SUB,256*2);
    }
    
    unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
    dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

    REG_BLDCNT=0; REG_BLDCNT_SUB=0; REG_BLDY=0; REG_BLDY_SUB=0;
    
    swiWaitForVBlank();    
}
// End of Line

