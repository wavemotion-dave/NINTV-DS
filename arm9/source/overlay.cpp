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
#include <string.h>
#include <ctype.h>

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
    const char  *search1;
    const char  *search2;
    const char  *ovl_filename;
};


struct MapRomToOvl_t MapRomToOvl[] = 
{
    {0xA6E89A53 , "DRAGONS",    "SWORDS",       "A Tale of Dragons and Swords.ovl"},
    {0xA60E25FC , "BACKGAMMON", "BACKGAMMON",   "ABPA Backgammon.ovl"},
    {0xFFFFFFFF , "GAMMON",     "GAMMON",       "ABPA Backgammon.ovl"},
    {0xF8B1F2B7 , "CLOUDY",     "MOUNTAIN",     "AD&D Cloudy Mountain.ovl"},
    {0x11C3BCFA , "CLOUDY",     "MOUNTAIN",     "AD&D Cloudy Mountain.ovl"},
    {0x16C3B62F , "TREASURE",   "TARMIN",       "AD&D Treasure of Tarmin.ovl"},
    {0x6746607B , "TREASURE",   "TARMIN",       "AD&D Treasure of Tarmin.ovl"},
    {0x5A4CE519 , "TREASURE",   "TARMIN",       "AD&D Treasure of Tarmin.ovl"},
    {0xBD731E3C , "TREASURE",   "TARMIN",       "AD&D Treasure of Tarmin.ovl"},
    {0x2F9C93FC , "TREASURE",   "TARMIN",       "AD&D Treasure of Tarmin.ovl"},
    {0x2F9C93FC , "MINOTAUR",   "MINOTAUR",     "AD&D Treasure of Tarmin.ovl"},
    {0x9BA5A798 , "ANTARCTIC",  "TALES",        "Antarctic Tales.ovl"},
    {0xc4f83541 , "ANTHROPOMOR","FORCE",        "Anthropomorphic Force.ovl"},    
    {0x6F91FBC1 , "ARMOR",      "BATTLE",       "Armor Battle.ovl"},    
    {0x5578C764 , "ASTRO",      "INVADERS",     "Astro Invaders.ovl"},
    {0x00BE8BBA , "ASTROSMASH", "ASTROSMASH",   "Astrosmash.ovl"},
    {0xFAB2992C , "ASTROSMASH", "ASTROSMASH",   "Astrosmash.ovl"},
    {0x13FF363C , "ATLANTIS",   "ATLANTIS",     "Atlantis.ovl"},
    {0xB35C1101 , "AUTO",       "RACING",       "Auto Racing.ovl"},
    {0x8AD19AB3 , "B-17",       "BOMBER",       "B-17 Bomber.ovl"},
    {0xFC49B63E , "BANK",       "PANIC",        "Bank Panic.ovl"},
    {0x8B2727D9 , "BEACHHEAD",  "BEACHHEAD",    "BeachHead.ovl"},
    {0xEAF650CC , "BEAMRIDER",  "BEAMRIDER",    "Beamrider.ovl"},
    {0xC047D487 , "BEAUTY",     "BEAST",        "Beauty and the Beast.ovl"},
    {0xFFFFFFFF , "BEAST",      "SP",           "BeautyBeast - SP.ovl"},    
    {0x32697B72 , "BOMB",       "SQUAD",        "Bomb Squad.ovl"},
    {0xF8E5398D , "BUCK",       "ROGERS",       "Buck Rogers.ovl"},
    {0x999CCEED , "BUMP",       "JUMP",         "Bump 'n' Jump.ovl"},
    {0x43806375 , "BURGER",     "TIME",         "Burger Time.ovl"},
    {0xC92BAAE8 , "BURGER",     "TIME",         "Burger Time.ovl"},
    {0xFFFFFFFF , "BURGRTM",    "BURGRTM",      "Burger Time.ovl"},    
    {0xFA492BBD , "BUZZ",       "BOMBERS",      "Buzz Bombers.ovl"},
    {0xFA492BBD , "BUZZ",       "BOMBERS",      "Buzz Bombers.ovl"},
    {0x2A1E0C1C , "CHAMPIONSHIP","TENNIS",      "Championship Tennis.ovl"},
    {0x43870908 , "CARNIVAL",   "CARNIVAL",     "Carnival.ovl"},
    {0x2fec6076 , "CAVERNS",    "MARS",         "Caverns of Mars.ovl"},
    {0x36E1D858 , "CHECKERS",   "CHECKERS",     "Checkers.ovl"},
    {0x0BF464C6 , "CHIP",       "SHOT",         "Chip Shot - Super Pro Golf.ovl"},
    {0x3289C8BA , "COMMANDO",   "COMMANDO",     "Commando.ovl"},
    {0xe8b99963 , "COPTER",     "COMMAND",      "Copter Command.ovl"},
    {0xFFFFFFFF , "COSMIC",     "AVENGER",      "Cosmic Avenger.ovl"},
    {0x060E2D82 , "D1K",        "D1K",          "D1K.ovl"},
    {0xFFFFFFFF , "D2K",        "D2K",          "D1K.ovl"},
    {0xFFFFFFFF , "DK",         "JR",           "DKJr.ovl"},
    {0xFFFFFFFF , "KONG",       "JR",           "DKJr.ovl"},
    {0xba346dbd , "DEATH",      "RACE",         "Death Race.ovl"},
    {0xFFFFFFFF , "DEFENDER",   "CROWN",        "Defender of the Crown.ovl"},
    {0x5E6A8CD8 , "DEMON",      "ATTACK",       "Demon Attack.ovl"},
    {0x13EE56F1 , "DINER",      "DINER",        "Diner.ovl"},
    {0x84BEDCC1 , "DRACULA",    "DRACULA",      "Dracula.ovl"},
    {0xC5182457 , "DRAGON",     "QUEST",        "Dragon Quest.ovl"},
    {0xAF8718A1 , "DRAGONFIRE", "DRAGONFIRE",   "Dragonfire.ovl"},
    {0x3B99B889 , "DREADNAUGHT","FACTOR",       "Dreadnaught Factor.ovl"},
    {0x99f5783d , "DREADNAUGHT","FACTOR",       "Dreadnaught Factor.ovl"},
    {0xB1BFA8B8 , "FINAL",      "ROUND",        "FinalRound.ovl"},
    {0x3A8A4351 , "FOX",        "QUEST",        "Fox Quest.ovl"},
    {0x912E7C64 , "FRANKENSTIEN","FRANKENSTIEN","Frankenstien.ovl"},
    {0xD27495E9 , "FROGGER",    "FROGGER",      "Frogger.ovl"},
    {0x37222762 , "FROG",       "BOG",          "Frog Bog.ovl"},
    {0xAC764495 , "GOSUB",      "GOSUB",        "GosubDigital.ovl"},
    {0x4B8C5932 , "HAPPY",      "TRAILS",       "Happy Trails.ovl"},
    {0x120b53a9 , "HAPPY",      "TRAILS",       "Happy Trails.ovl"},
    {0xB5C7F25D , "HORSE",      "RACING",       "Horse Racing.ovl"},
    {0x94EA650B , "HOVER",      "BOVVER",       "Hover Bovver.ovl"},
    {0xFF83FF80 , "HOVER",      "FORCE",        "Hover Force.ovl"},
    {0x4F3E3F69 , "ICE",        "TREK",         "Ice Trek.ovl"},
    {0x5F2607E1 , "INFILTRATOR","INFILTRATOR",  "Infiltrator.ovl"},
    {0xFFFFFFFF , "INTELLIVANIA","INTELLIVANIA","Intellivania.ovl"},
    {0xEE5F1BE2 , "JETSONS",    "JETSONS",      "Jetsons.ovl"},
    {0x4422868E , "KING",       "MOUNTAIN",     "King of the Mountain.ovl"},
    {0x87D95C72 , "KING",       "MOUNTAIN",     "King of the Mountain.ovl"},
    {0xFFFFFFFF , "SPKOTM",     "SPKOTM",       "King of the Mountain.ovl"},    
    {0x8C9819A2 , "KOOL",       "AID",          "Kool-Aid Man.ovl"},
    {0xA6840736 , "LADY",       "BUG",          "Lady Bug.ovl"},
    {0x604611C0 , "POKER",      "BLACKJACK",    "Las Vegas Poker & Blackjack.ovl"},
    {0x48D74D3C , "VEGAS",      "ROULETTE",     "Las Vegas Roulette.ovl"},
    {0x632F6ADF , "LEARNING",   "FUN II",       "Learning Fun II.ovl"},
    {0x2C5FD5FA , "LEARNING",   "FUN I",        "Learning Fun I.ovl"},    
    {0xE00D1399 , "LOCK",       "CHASE",        "Lock-n-Chase.ovl"},
    {0x5C7E9848 , "LOCK",       "CHASE",        "Lock-n-Chase.ovl"},
    {0x6B6E80EE , "LOCO",       "MOTION"        "Loco-Motion.ovl"},
    {0x80764301 , "LODE",       "RUNNER",       "Lode Runner.ovl"},
    {0x573B9B6D , "MASTERS",    "UNIVERSE",     "Masters of the Universe - The Power of He-Man.ovl"},
    {0xEB4383E0 , "MAXIT",      "MAXIT",        "Maxit.ovl"},
    {0xE806AD91 , "MICRO",      "SURGEON",      "Microsurgeon.ovl"},
    {0x9D57498F , "MIND",       "STRIKE",       "Mind Strike.ovl"},
    {0x11FB9974 , "MISSION",    "X",            "Mission X.ovl"},    
    {0x598662F2 , "MOUSE",      "TRAP",         "Mouse Trap.ovl"},
    {0xE367E450 , "MR",         "CHESS",        "MrChess.ovl"},
    {0x0753544F , "MS",         "PAC",          "Ms Pac-Man.ovl"},
    {0x7334CD44 , "NIGHT",      "STALKER",      "Night Stalker.ovl"},
    {0xFFFFFFFF , "MSTALKER",   "MSTALKER",     "Night Stalker.ovl"},    
    {0x36A7711B , "OPERATION",  "CLOUDFIRE",    "Operation Cloudfire.ovl"},
    {0xFFFFFFFF , "OREGON",     "BOUND",        "Oregon Bound.ovl"},
    {0xFFFFFFFF , "OREGON",     "TRAIL",        "Oregon Bound.ovl"},
    {0x45668011 , "PANDORA",    "INCIDENT",     "Pandora Incident.ovl"},
    {0x169E3584 , "PBA",        "BOWLING",      "PBA Bowling.ovl"},
    {0xFF87FAEC , "PGA",        "GOLF",         "PGA Golf.ovl"},
    {0xA21C31C3 , "PAC",        "MAN",          "Pac-Man.ovl"},
    {0x6E4E8EB4 , "PAC",        "MAN",          "Pac-Man.ovl"},
    {0x6084B48A , "PARSEC",     "PARSEC",       "Parsec.ovl"},
    {0x1A7AAC88 , "PENGUIN",    "LAND",         "Penguin Land.ovl"},
    {0xD7C5849C , "PINBALL",    "PINBALL",      "Pinball.ovl"},
    {0x9C75EFCC , "PITFALL!",   "PITFALL!",     "Pitfall!.ovl"},
    {0x0CF06519 , "POKER",      "RISQUE",       "Poker Risque.ovl"},
    {0xD8C9856A,  "Q-BERT",     "Q-BERT",       "Q-Bert.ovl"},
    {0x5AEF02C6 , "RICK",       "DYNAMITE",     "Rick Dynamite.ovl"},
    {0x8910C37A , "RIVER",      "RAID",         "River Raid.ovl"},
    {0x95466AD3 , "RIVER",      "RAID",         "River Raid.ovl"},
    {0xDCF4B15D , "ROYAL",      "DEALER",       "Royal Dealer.ovl"},
    {0x8F959A6E , "SNAFU",      "SNAFU",        "SNAFU.ovl"},
    {0x47AA7977 , "SAFE",       "CRACKER",      "Safecracker.ovl"},
    {0x6E0882E7 , "SAMEGAME",   "ROBOTS",       "SameGame and Robots.ovl"},
    {0x12BA58D1 , "SAMEGAME",   "ROBOTS",       "SameGame and Robots.ovl"},
    {0x20eb8b7c , "SAMEGAME",   "ROBOTS",       "SameGame and Robots.ovl"},
    {0xAB5FD8BC , "SPACE",      "PATROL",       "Space Patrol.ovl"},
    {0xe9e3f60d , "MAZE",       "CHASE",        "Scooby Doo's Maze Chase.ovl"},
    {0xBEF0B0C7 , "MAZE",       "CHASE",        "Scooby Doo's Maze Chase.ovl"},
    {0xFFFFFFFF , "SCOOBY",     "DOO",          "Scooby Doo's Maze Chase.ovl"},
    {0x99AE29A9 , "SEA",        "BATTLE",       "Sea Battle.ovl"},
    {0x2A4C761D , "SHARK!",     "SHARK!",       "Shark! Shark!.ovl"},
    {0xd7b8208b , "SHARK!",     "SHARK!",       "Shark! Shark!.ovl"},
    {0xFF7CB79E , "SHARP",      "SHOT",         "Sharp Shot.ovl"},
    {0xF093E801 , "US",         "SKIING",       "Skiing.ovl"},
    {0x800B572F , "SLAM",       "DUNK",         "Slam Dunk - Super Pro Basketball.ovl"},
    {0x0e6198a5 , "GADHLAN",    "THUR",         "Gadhlan Thur.ovl"},    
    {0xE8B8EBA5 , "SPACE",      "ARMADA",       "Space Armada.ovl"},
    {0xFFFFFFFF , "SPACE",      "INVADERS",     "Space Armada.ovl"},
    {0x1AAC64CA , "SPACE",      "BANDITS",      "Space Bandits.ovl"},
    {0xF95504E0 , "SPACE",      "BATTLE",       "Space Battle.ovl"},
    {0x39D3B895 , "SPACE",      "HAWK",         "Space Hawk.ovl"},
    {0xCF39471A , "SPACE",      "PANIC",        "Space Panic.ovl"},
    {0x3784DC52 , "SPACE",      "SPARTANS",     "Space Spartans.ovl"},
    {0xB745C1CA , "MUD",        "BUGGIES",      "Stadium Mud Buggies.ovl"},
    {0x2DEACD15 , "STAMPEDE",   "STAMPEDE",     "Stampede.ovl"},
    {0x72E11FCA , "STAR",       "STRIKE",       "Star Strike.ovl"},
    {0x3D9949EA , "SUB",        "HUNT",         "Sub Hunt.ovl"},
    {0xbe4d7996 , "SUPER",      "NFL",          "Super NFL Football (ECS).ovl"},
    {0x15E88FCE , "SWORDS",     "SERPENTS",     "Swords & Serpents.ovl"},
    {0xD6F44FA5 , "TNT",        "COWBOY",       "TNT Cowboy.ovl"},
    {0xCA447BBD , "DEADLY",     "DISCS",        "TRON Deadly Discs.ovl"},
    {0xFFFFFFFF , "DEADLIER",   "DISCS",        "TRON Deadly Discs.ovl"},
    {0x07FB9435 , "SOLAR",      "SAILOR",       "TRON Solar Sailor.ovl"},
    {0x07FB9435 , "SOLAR",      "SAILER",       "TRON Solar Sailor.ovl"},
    {0xFFFFFFFF , "TRON",       "SOLAR",        "TRON Solar Sailor.ovl"},
    {0x03E9E62E , "TENNIS",     "TENNIS",       "Tennis.ovl"},
    {0xB7923858 , "SHOW",       "MUST",         "The Show Must Go On.ovl"},
    {0xF3DF94E0 , "THIN",       "ICE",          "Thin Ice.ovl"},
    {0x975AE6DF , "THIN",       "ICE",          "Thin Ice.ovl"},
    {0xd6495910 , "THIN",       "ICE",          "Thin Ice.ovl"},
    {0xC1F1CA74 , "THUNDER",    "CASTLE",       "Thunder Castle.ovl"},
    {0x67CA7C0A , "THUNDER",    "CASTLE",       "Thunder Castle.ovl"},
    {0x67ca7c0a , "MYSTIC",     "CASTLE",       "Mystic Castle.ovl"},
    {0xD1D352A0 , "TOWER",      "DOOM",         "Tower of Doom.ovl"},
    {0x734F3260 , "TRUCKIN",    "TRUCKIN",      "Truckin.ovl"},
    {0x752FD927 , "USCF",       "CHESS",        "USCF Chess.ovl"},
    {0xF9E0789E , "UTOPIA",     "UTOPIA",       "Utopia.ovl"},
    {0xC9624608 , "VANGUARD",   "VANGUARD",     "Vanguard.ovl"},
    {0xA4A20354 , "VECTRON",    "VECTRON",      "Vectron.ovl"},
    {0xF1ED7D27 , "WHITE",      "WATER",        "White Water.ovl"},
    {0x10D64E48 , "CHAMPIONSHIP","BASEBALL",    "World Championship Baseball.ovl"},
    {0xFFFFFFFF , "WIZARD",     "OF WOR",       "Wizard of Wor.ovl"},
    {0xC2063C08 , "WORLD",      "SERIES",       "World Series Major League Baseball.ovl"},
    {0x24B667B9 , "WORM",       "WHOMPER",      "Worm Whomper.ovl"},
    {0x740C9C49 , "X-RALLY",    "X-RALLY",      "X-Rally.ovl"},
    {0x45119649 , "YAR",        "REVENGE",      "Yars Revenge.ovl"},
    {0xC00CBA0D , "GORF",       "GORF",         "gorf.ovl"},
    {0x00000000 , "xxx",        "zzz",          "generic.ovl"},
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
            
            // If still not found after searching for CRC32... try fuzzy name search...
            if (!bFound)
            {
                UINT16 i=0;
                while (MapRomToOvl[i].crc != 0x00000000)
                {
                    // It has to match both string searches to be valid... Uppercase for the compare...
                    strcpy(szName, currentRip->GetFileName());
                    for (int j=0; j<strlen(szName); j++) szName[j] = toupper(szName[j]);
                    
                    if ((strstr(szName, MapRomToOvl[i].search1) != NULL) && (strstr(szName, MapRomToOvl[i].search2) != NULL))
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
