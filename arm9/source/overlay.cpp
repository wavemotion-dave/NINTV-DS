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
#include "ECSKeyboard.h"
#include "bgBottom.h"
#include "bgBottom-ECS.h"
#include "bgBottom-disc.h"
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
    
    { 10,    87,     10,    40},    // META_RESET
    { 10,    87,     41,    70},    // META_LOAD
    { 10,    87,     71,   100},    // META_CONFIG
    {255,   255,    255,   255},    // META_SCORE
    {255,   255,    255,   255},    // META_QUIT
    { 10,    87,    101,   131},    // META_STATE
    { 10,    87,    131,   160},    // META_MENU
    {255,   255,    255,   255},    // META_SWAP
    {255,   255,    255,   255},    // META_MANUAL
    { 50,     86,   161,   191},    // META_DISC
    {  8,     49,   161,   191},    // META_KEYBOARD
};

// ----------------------------------------------------------------------------------------
// This is the ECS overlay with no menu options...
// ----------------------------------------------------------------------------------------
struct Overlay_t ecsOverlay[OVL_MAX] =
{
    {255,   255,    255,   255},    // KEY_1
    {255,   255,    255,   255},    // KEY_2
    {255,   255,    255,   255},    // KEY_3
    
    {255,   255,    255,   255},    // KEY_4
    {255,   255,    255,   255},    // KEY_5
    {255,   255,    255,   255},    // KEY_6

    {255,   255,    255,   255},    // KEY_7
    {255,   255,    255,   255},    // KEY_8
    {255,   255,    255,   255},    // KEY_9

    {255,   255,    255,   255},    // KEY_CLEAR
    {255,   255,    255,   255},    // KEY_0
    {255,   255,    255,   255},    // KEY_ENTER

    {255,   255,    255,   255},    // KEY_FIRE
    {255,   255,    255,   255},    // KEY_L_ACT
    {255,   255,    255,   255},    // KEY_R_ACT
    
    {255,   255,    255,   255},    // META_RESET
    {255,   255,    255,   255},    // META_LOAD
    {255,   255,    255,   255},    // META_CONFIG
    {255,   255,    255,   255},    // META_SCORE
    {255,   255,    255,   255},    // META_QUIT
    {255,   255,    255,   255},    // META_STATE
    {255,   255,    255,   255},    // META_MENU
    {255,   255,    255,   255},    // META_SWAP
    {255,   255,    255,   255},    // META_MANUAL
    {255,   255,    255,   255},    // META_DISC
    {255,   255,    255,   255},    // META_KEYBOARD
};

// ----------------------------------------------------------------------------------------
// This is the Disc overlay with no menu options...
// ----------------------------------------------------------------------------------------
struct Overlay_t discOverlay[OVL_MAX] =
{
    {255,   255,    255,   255},    // KEY_1
    {255,   255,    255,   255},    // KEY_2
    {255,   255,    255,   255},    // KEY_3
    
    {255,   255,    255,   255},    // KEY_4
    {255,   255,    255,   255},    // KEY_5
    {255,   255,    255,   255},    // KEY_6

    {255,   255,    255,   255},    // KEY_7
    {255,   255,    255,   255},    // KEY_8
    {255,   255,    255,   255},    // KEY_9

    {255,   255,    255,   255},    // KEY_CLEAR
    {255,   255,    255,   255},    // KEY_0
    {255,   255,    255,   255},    // KEY_ENTER

    {255,   255,    255,   255},    // KEY_FIRE
    {255,   255,    255,   255},    // KEY_L_ACT
    {255,   255,    255,   255},    // KEY_R_ACT
    
    {255,   255,    255,   255},    // META_RESET
    {255,   255,    255,   255},    // META_LOAD
    {255,   255,    255,   255},    // META_CONFIG
    {255,   255,    255,   255},    // META_SCORE
    {255,   255,    255,   255},    // META_QUIT
    {255,   255,    255,   255},    // META_STATE
    {255,   255,    255,   255},    // META_MENU
    {255,   255,    255,   255},    // META_SWAP
    {255,   255,    255,   255},    // META_MANUAL
    {255,   255,    255,   255},    // META_DISC
    {255,   255,    255,   255},    // META_KEYBOARD
};

struct Overlay_t myOverlay[OVL_MAX];
struct Overlay_t myDisc[DISC_MAX];


// -------------------------------------------------------------
// Rather than take up precious RAM, we use some video memory.
// -------------------------------------------------------------
unsigned int *customTiles = (unsigned int *) 0x06880000;          //60K of video memory for the tiles (largest I've seen so far is ~48K)
unsigned short *customMap = (unsigned short *)0x0688F000;         // 4K of video memory for the map (generally about 2.5K)
unsigned short customPal[512];

char directory[192];
char filename[192];


// -----------------------------------------------------------------------
// Map of game CRC to default Overlay file... this will help so that 
// users don't need to rename every .rom file to match exactly the 
// .ovl file. Most users just want to pick game, play game, enjoy game.
// -----------------------------------------------------------------------
struct MapRomToOvl_t
{
    UINT32      crc;
    const char  *ovl_filename;
};


struct MapRomToOvl_t MapRomToOvl[] = 
{
    {0xA6E89A53 , "A Tale of Dragons and Swords.ovl"},
    {0xA60E25FC , "ABPA Backgammon.ovl"},
    {0xF8B1F2B7 , "AD&D Cloudy Mountain.ovl"},
    {0x11C3BCFA , "AD&D Cloudy Mountain.ovl"},
    {0x16C3B62F , "AD&D Treasure of Tarmin.ovl"},
    {0x6746607B , "AD&D Treasure of Tarmin.ovl"},
    {0x5A4CE519 , "AD&D Treasure of Tarmin.ovl"},
    {0xBD731E3C , "AD&D Treasure of Tarmin.ovl"},
    {0x2F9C93FC , "AD&D Treasure of Tarmin.ovl"},
    {0x9BA5A798 , "Antarctic Tales.ovl"},
    {0x5578C764 , "Astro Invaders.ovl"},
    {0x00BE8BBA , "Astrosmash.ovl"},
    {0xFAB2992C , "Astrosmash.ovl"},
    {0x13FF363C , "Atlantis.ovl"},
    {0xB35C1101 , "Auto Racing.ovl"},
    {0x8AD19AB3 , "B-17 Bomber.ovl"},
    {0xFC49B63E , "Bank Panic.ovl"},
    {0x8B2727D9 , "BeachHead.ovl"},
    {0xEAF650CC , "Beamrider.ovl"},
    {0xC047D487 , "Beauty and the Beast.ovl"},
    {0x32697B72 , "Bomb Squad.ovl"},
    {0xF8E5398D , "Buck Rogers.ovl"},
    {0x999CCEED , "Bump 'n' Jump.ovl"},
    {0x43806375 , "Burger Time.ovl"},
    {0xC92BAAE8 , "Burger Time.ovl"},
    {0xFA492BBD , "Buzz Bombers.ovl"},
    {0xFA492BBD , "Buzz Bombers.ovl"},
    {0x2A1E0C1C , "Championship Tennis.ovl"},
    {0x36E1D858 , "Checkers.ovl"},
    {0x0BF464C6 , "Chip Shot - Super Pro Golf.ovl"},
    {0x3289C8BA , "Commando.ovl"},
    {0x060E2D82 , "D1K.ovl"},
    {0x5E6A8CD8 , "Demon Attack.ovl"},
    {0x13EE56F1 , "Diner.ovl"},
    {0x84BEDCC1 , "Dracula.ovl"},
    {0xC5182457 , "Dragon Quest.ovl"},
    {0xAF8718A1 , "Dragonfire.ovl"},
    {0x3B99B889 , "Dreadnaught Factor.ovl"},
    {0x99f5783d , "Dreadnaught Factor.ovl"},
    {0xB1BFA8B8 , "FinalRound.ovl"},
    {0x3A8A4351 , "Fox Quest.ovl"},
    {0x912E7C64 , "Frankenstien.ovl"},
    {0xAC764495 , "GosubDigital.ovl"},
    {0x4B8C5932 , "Happy Trails.ovl"},
    {0x120b53a9 , "Happy Trails.ovl"},
    {0xB5C7F25D , "Horse Racing.ovl"},
    {0x94EA650B , "Hover Bovver.ovl"},
    {0xFF83FF80 , "Hover Force.ovl"},
    {0x4F3E3F69 , "Ice Trek.ovl"},
    {0x5F2607E1 , "Infiltrator.ovl"},
    {0xEE5F1BE2 , "Jetsons.ovl"},
    {0x4422868E , "King of the Mountain.ovl"},
    {0x87D95C72 , "King of the Mountain.ovl"},
    {0x8C9819A2 , "Kool-Aid Man.ovl"},
    {0x604611C0 , "Las Vegas Poker & Blackjack.ovl"},
    {0xE00D1399 , "Lock-n-Chase.ovl"},
    {0x5C7E9848 , "Lock-n-Chase.ovl"},
    {0x6B6E80EE , "Loco-Motion.ovl"},
    {0x80764301 , "Lode Runner.ovl"},
    {0x573B9B6D , "Masters of the Universe - The Power of He-Man.ovl"},
    {0xEB4383E0 , "Maxit.ovl"},
    {0xE806AD91 , "Microsurgeon.ovl"},
    {0x9D57498F , "Mind Strike.ovl"},
    {0x598662F2 , "Mouse Trap.ovl"},
    {0xE367E450 , "MrChess.ovl"},
    {0x0753544F , "Ms Pac-Man.ovl"},
    {0x7334CD44 , "Night Stalker.ovl"},
    {0x36A7711B , "Operation Cloudfire.ovl"},
    {0x169E3584 , "PBA Bowling.ovl"},
    {0xFF87FAEC , "PGA Golf.ovl"},
    {0xA21C31C3 , "Pac-Man.ovl"},
    {0x6E4E8EB4 , "Pac-Man.ovl"},
    {0x6084B48A , "Parsec.ovl"},
    {0x1A7AAC88 , "Penguin Land.ovl"},
    {0x9C75EFCC , "Pitfall!.ovl"},
    {0x0CF06519 , "Poker Risque.ovl"},
    {0x5AEF02C6 , "Rick Dynamite.ovl"},
    {0x8910C37A , "River Raid.ovl"},
    {0x95466AD3 , "River Raid.ovl"},
    {0xDCF4B15D , "Royal Dealer.ovl"},
    {0x8F959A6E , "SNAFU.ovl"},
    {0x47AA7977 , "Safecracker.ovl"},
    {0x6E0882E7 , "SameGame and Robots.ovl"},
    {0x12BA58D1 , "SameGame and Robots.ovl"},
    {0x20eb8b7c , "SameGame and Robots.ovl"},
    {0xe9e3f60d , "Scooby Doo's Maze Chase.ovl"},
    {0xBEF0B0C7 , "Scooby Doo's Maze Chase.ovl"},
    {0x99AE29A9 , "Sea Battle.ovl"},
    {0x2A4C761D , "Shark! Shark!.ovl"},
    {0xd7b8208b , "Shark! Shark!.ovl"},
    {0xFF7CB79E , "Sharp Shot.ovl"},
    {0xF093E801 , "Skiing.ovl"},
    {0x800B572F , "Slam Dunk - Super Pro Basketball.ovl"},
    {0xE8B8EBA5 , "Space Armada.ovl"},
    {0x1AAC64CA , "Space Bandits.ovl"},
    {0xF95504E0 , "Space Battle.ovl"},
    {0x39D3B895 , "Space Hawk.ovl"},
    {0xCF39471A , "Space Panic.ovl"},
    {0x3784DC52 , "Space Spartans.ovl"},
    {0xB745C1CA , "Stadium Mud Buggies.ovl"},
    {0x2DEACD15 , "Stampede.ovl"},
    {0x72E11FCA , "Star Strike.ovl"},
    {0x3D9949EA , "Sub Hunt.ovl"},
    {0xbe4d7996 , "Super NFL Football (ECS).ovl"},
    {0x15E88FCE , "Swords & Serpents.ovl"},
    {0xD6F44FA5 , "TNT Cowboy.ovl"},
    {0xCA447BBD , "TRON Deadly Discs.ovl"},
    {0x07FB9435 , "TRON Solar Sailor.ovl"},
    {0x03E9E62E , "Tennis.ovl"},
    {0xB7923858 , "The Show Must Go On.ovl"},
    {0xC1F1CA74 , "Thunder Castle.ovl"},
    {0x67CA7C0A , "Thunder Castle.ovl"},
    {0xD1D352A0 , "Tower of Doom.ovl"},
    {0x734F3260 , "Truckin.ovl"},
    {0x752FD927 , "USCF Chess.ovl"},
    {0xF9E0789E , "Utopia.ovl"},
    {0xC9624608 , "Vanguard.ovl"},
    {0xA4A20354 , "Vectron.ovl"},
    {0xF1ED7D27 , "White Water.ovl"},
    {0x10D64E48 , "World Championship Baseball.ovl"},
    {0xC2063C08 , "World Series Major League Baseball.ovl"},
    {0x24B667B9 , "Worm Whomper.ovl"},
    {0x740C9C49 , "X-Rally.ovl"},
    {0x45119649 , "Yars Revenge.ovl"},
    {0xC00CBA0D , "gorf.ovl"},
    {0x00000000 , "generic.ovl"},
};


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
    else if (myGlobalConfig.ovl_dir == 2)   // In: /ROMS/INTV/OVL
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
        else // Not found... try to find it using the RomToOvl[] table
        {
            UINT16 i=0;
            while (MapRomToOvl[i].crc != 0x00000000)
            {
                if (MapRomToOvl[i].crc == currentRip->GetCRC())
                {
                    strcpy(filename, directory);
                    strcat(filename, MapRomToOvl[i].ovl_filename);
                    fp = fopen(filename, "rb");
                    if (fp != NULL) bFound = 1;
                    break;
                }
                i++;
            }
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
      
      memcpy(&myOverlay, &discOverlay, sizeof(myOverlay)); // Start with a blank overlay...

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
void show_overlay(u8 bShowKeyboard, u8 bShowDisc)
{
    // Assume default overlay... custom can change it below...
    memcpy(&myOverlay, &defaultOverlay, sizeof(myOverlay));
    
    swiWaitForVBlank();
    
    if (bShowKeyboard) // ECS keyboard overlay
    {
      decompress(bgBottom_ECSTiles, bgGetGfxPtr(bg0b), LZ77Vram);
      decompress(bgBottom_ECSMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
      dmaCopy((void *) bgBottom_ECSPal,(u16*) BG_PALETTE_SUB,256*2);
      // ECS Overlay...  
      memcpy(&myOverlay, &ecsOverlay, sizeof(myOverlay));
    }
    else    // Default Overlay... which might be custom!!
    {
        if (bShowDisc)
        {
            decompress(bgBottom_discTiles, bgGetGfxPtr(bg0b), LZ77Vram);
            decompress(bgBottom_discMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
            dmaCopy((void *) bgBottom_discPal,(u16*) BG_PALETTE_SUB,256*2);
            // Disc Overlay...  
            memcpy(&myOverlay, &discOverlay, sizeof(myOverlay));
        }
        else
        {
            load_custom_overlay(true);  // This will try to load nintv-ds.ovl or else default to the generic background
        }
    }
    
    unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
    dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

    REG_BLDCNT=0; REG_BLDCNT_SUB=0; REG_BLDY=0; REG_BLDY_SUB=0;
    
    swiWaitForVBlank();    
    
    if (bShowKeyboard) // ECS keyboard overlay
    {
        dsPrintValue(1,22,ecs_ctrl_key?1:0,ecs_ctrl_key ? (char*)"@": (char*)" ");
        dsPrintValue(5,22,0,ecs_shift_key ? (char*)"@": (char*)" ");
    }    
}

// End of Line
