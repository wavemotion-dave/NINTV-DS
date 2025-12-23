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
#include "inty-synth.h"
#include "bgTop.h"
#include "Emulator.h"
#include "Rip.h"
#include "loadgame.h"
#include "printf.h"

// ------------------------------------------------
// Reuse the char buffer from the game load...
// we wouldn't need to use this at the same time.
// ------------------------------------------------
extern char szName[];
extern Rip *currentRip;

char tile_str[16];
char map_str[16];
char pal_str[16];
char ovl_str[16];
char disc_str[16];
char hudx_str[16];
char hudy_str[16];

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
    {255,   255,    255,   255},    // META_SWAPOVL
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
    {255,   255,    255,   255},    // META_SWAPOVL
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
    {255,   255,    255,   255},    // META_SWAPOVL
};

struct Overlay_t myOverlay[OVL_MAX];
struct Overlay_t myDisc[DISC_MAX];


// -------------------------------------------------------------
// Rather than take up precious RAM, we use some video memory.
// -------------------------------------------------------------
unsigned int *customTiles = (unsigned int *) 0x06880000;          //60K of video memory for the tiles (largest I've seen so far is ~48K)
unsigned short *customMap = (unsigned short *)0x0688F000;         // 4K of video memory for the map (generally about 2.5K)
unsigned short customPal[512];

char directory[192];    // Overlay directory
char filename[192];     // Overlay filename


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
    {0x9415dad4 , "2048",       "2048",         "2048.ovl"},
    {0xD7C78754 , "4-TRIS",     "4-TRIS",       "4-TRIS.ovl"},
    {0xB91488E2 , "4TRIS",      "4TRIS",        "4-TRIS.ovl"},
    {0xA6E89A53 , "DRAGONS",    "SWORDS",       "A Tale of Dragons and Swords.ovl"},
    {0x5a144835 , "ADVENTURES", "TRON",         "Adventures of TRON.ovl"},
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
    {0xc36d63be , "ASTROSMASH", "ANTHOLOGY",    "Astrosmash Anthology.ovl"},
    {0xc36d63be , "MENUASTRO",  "SMASH",        "Astrosmash Anthology.ovl"},
    {0xFFFFFFFF , "ASTROSMASH", "SUPER",        "Astrosmash-SuperPro.ovl"},
    {0xFFFFFFFF , "ASTROSMASH", "COMPETITION",  "Astrosmash-SuperPro.ovl"},
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
    {0xB03F739B , "BLOCKADE",   "RUNNER",       "Blockade Runner.ovl"},
    {0x515E1D7E , "BODY",       "SLAM",         "Body Slam - Super Pro Wrestling.ovl"},
    {0x32697B72 , "BOMB",       "SQUAD",        "Bomb Squad.ovl"},
    {0xFFFFFFFF , "ROCK",       "SOCK",         "Rock-Em-Sock-Em.ovl"}, // Keep before Boxing
    {0xAB87C16F , "BOXING",     "BOXING",       "Boxing.ovl"},
    {0xF8E5398D , "BUCK",       "ROGERS",       "Buck Rogers.ovl"},
    {0x999CCEED , "BUMP",       "JUMP",         "Bump 'n' Jump.ovl"},
    {0x43806375 , "BURGER",     "TIME",         "Burger Time.ovl"},
    {0xC92BAAE8 , "BURGER",     "TIME",         "Burger Time.ovl"},
    {0xFFFFFFFF , "BURGRTM",    "BURGRTM",      "Burger Time.ovl"},
    {0xFFFFFFFF , "SUPER",      "CHEF",         "Burger Time.ovl"},
    {0xFFFFFFFF , "MASTER",     "CHEF",         "Burger Time.ovl"},
    {0xFA492BBD , "BUZZ",       "BOMBERS",      "Buzz Bombers.ovl"},
    {0xFA492BBD , "BUZZ",       "BOMBERS",      "Buzz Bombers.ovl"},
    {0xFFFFFFFF , "CAT",        "ATTACK",       "Cat Attack.ovl"},
    {0xD5363B8C , "CENTIPEDE",  "CENTIPEDE",    "Centipede.ovl"},
    {0x2A1E0C1C , "CHAMPIONSHIP","TENNIS",      "Championship Tennis.ovl"},
    {0xa1ad74af , "CHOPLIFTER", "CHOPLIFTER",   "Choplifter.ovl"},
    {0x43870908 , "CARNIVAL",   "CARNIVAL",     "Carnival.ovl"},
    {0x5504f202 , "CASTLE",     "DEATH",        "Castle of Death.ovl"},
    {0x2fec6076 , "CAVERNS",    "MARS",         "Caverns of Mars.ovl"},
    {0x36E1D858 , "CHECKERS",   "CHECKERS",     "Checkers.ovl"},
    {0x0BF464C6 , "CHIP",       "SHOT",         "Chip Shot - Super Pro Golf.ovl"},
    {0xFFFFFFFF , "CHRISTMAS",  "CAROL",        "Christmas Carol.ovl"},
    {0x3289C8BA , "COMMANDO",   "COMMANDO",     "Commando.ovl"},
    {0x4B23A757 , "CONGO",      "BONGO",        "Congo Bongo.ovl"},
    {0xe8b99963 , "COPTER",     "COMMAND",      "Copter Command.ovl"},
    {0xFFFFFFFF , "COSMIC",     "AVENGER",      "Cosmic Avenger.ovl"},
    {0x060E2D82 , "D1K",        "D1K",          "D1K.ovl"},
    {0xFFFFFFFF , "D2K",        "D2K",          "D1K.ovl"},
    {0xFFFFFFFF , "DK",         "JR",           "DKJr.ovl"},
    {0xFFFFFFFF , "Donkey",     "Junior",       "DKJr.ovl"},
    {0xFFFFFFFF , "KONG",       "JR",           "DKJr.ovl"},
    {0x6802B191 , "DEEP",       "POCKETS",      "Deep Pockets.ovl"},
    {0xba346dbd , "DEATH",      "RACE",         "Death Race.ovl"},
    {0xFFFFFFFF , "DEFENDER",   "CROWN",        "Defender of the Crown.ovl"},
    {0xD8F99AA2 , "DEFENDER",   "ATARISOFT",    "Defender.ovl"},
    {0x5E6A8CD8 , "DEMON",      "ATTACK",       "Demon Attack.ovl"},
    {0xFFFFFFFF , "DEEP",       "ZONE",         "Deep Zone.ovl"},
    {0x159AF7F7 , "DIG",        "DUG",          "Dig Dug.ovl"},
    {0x13EE56F1 , "DINER",      "DINER",        "Diner.ovl"},
    {0x84BEDCC1 , "DRACULA",    "DRACULA",      "Dracula.ovl"},
    {0xC5182457 , "DRAGON",     "QUEST",        "Dragon Quest.ovl"},
    {0xAF8718A1 , "DRAGONFIRE", "DRAGONFIRE",   "Dragonfire.ovl"},
    {0x3B99B889 , "DREADNAUGHT","FACTOR",       "Dreadnaught Factor.ovl"},
    {0x99f5783d , "DREADNAUGHT","FACTOR",       "Dreadnaught Factor.ovl"},
    {0x6DF61A9F , "KONG JR",    "COLECO",       "Donkey Kong.ovl"},
    {0xC30F61C0 , "DONKEY KONG","COLECO",       "Donkey Kong.ovl"},
    {0x54A3FC11 , "ELECTRIC",   "MATH",         "Electric Company - Math Fun.ovl"},
    {0x1b850d3d , "EXTRA",      "TERRESTRIAL",  "ET.ovl"},
    {0xf6db8f7c , "EXTRA",      "TERRESTRIAL",  "ET.ovl"},
    {0x291106bc , "FANTASY",    "2020",         "Fantasy.ovl"},
    {0xd3f14a9d , "FANTASY",    "PUZZLE",       "Fantasy Puzzle.ovl"},
    {0x4221EDE7 , "FATHOM",     "FATHOM",       "Fathom.ovl"},
    {0xB1BFA8B8 , "FINAL",      "ROUND",        "FinalRound.ovl"},
    {0x3A8A4351 , "FOX",        "QUEST",        "Fox Quest.ovl"},
    {0x912E7C64 , "FRANKENSTIEN","FRANKENSTIEN","Frankenstien.ovl"},
    {0xD27495E9 , "FROGGER",    "FROGGER",      "Frogger.ovl"},
    {0x37222762 , "FROG",       "BOG",          "Frog Bog.ovl"},
    {0x71c2c0bf , "GOONINUFF",  "GOONINUFF",    "Gooninuff.ovl"},
    {0xAC764495 , "GOSUB",      "GOSUB",        "GosubDigital.ovl"},
    {0x2c520121 , "GORILLAS",   "GORILLAS",     "Gorillas.ovl"},
    {0x4B8C5932 , "HAPPY",      "TRAILS",       "Happy Trails.ovl"},
    {0x120b53a9 , "HAPPY",      "TRAILS",       "Happy Trails.ovl"},
    {0xB5C7F25D , "HORSE",      "RACING",       "Horse Racing.ovl"},
    {0xe55546fb , "HOTEL",      "BUNNY",        "Hotel Bunny.ovl"},
    {0x94EA650B , "HOVER",      "BOVVER",       "Hover Bovver.ovl"},
    {0xFF83FF80 , "HOVER",      "FORCE",        "Hover Force.ovl"},
    {0x4F3E3F69 , "ICE",        "TREK",         "Ice Trek.ovl"},
    {0x5F2607E1 , "INFILTRATOR","INFILTRATOR",  "Infiltrator.ovl"},
    {0xFFFFFFFF , "INTELLIVANIA","INTELLIVANIA","Intellivania.ovl"},
    {0xFFFFFFFF , "JAWCRUSHER", "JAWCRUSHER",   "Jawcrusher.ovl"},
    {0xEE5F1BE2 , "JETSONS",    "JETSONS",      "Jetsons.ovl"},
    {0xc412dcde , "JUMPKING",   "JUNIOR",       "Jumpking Junior.ovl"},
    {0x4422868E , "KING",       "MOUNTAIN",     "King of the Mountain.ovl"},
    {0x87D95C72 , "KING",       "MOUNTAIN",     "King of the Mountain.ovl"},
    {0xFFFFFFFF , "SPKOTM",     "SPKOTM",       "King of the Mountain.ovl"},
    {0x30e2819b , "KEYBOARD",   "FUN",          "Keyboard Fun.ovl"},
    {0x8C9819A2 , "KOOL",       "AID",          "Kool-Aid Man.ovl"},
    {0xFFFFFFFF , "CAVES",      "KROZ",         "Kroz.ovl"},
    {0xA6840736 , "LADY",       "BUG",          "Lady Bug.ovl"},
    {0x604611C0 , "POKER",      "BLACKJACK",    "Las Vegas Poker & Blackjack.ovl"},
    {0x48D74D3C , "VEGAS",      "ROULETTE",     "Las Vegas Roulette.ovl"},
    {0xd2d1ad9e , "LASER",      "SHARKS",       "Laser Sharks.ovl"},
    {0x6fa5cff8 , "LASER",      "SHARKS",       "Laser Sharks.ovl"},    
    {0x632F6ADF , "LEARNING",   "FUN II",       "Learning Fun II.ovl"},
    {0x2C5FD5FA , "LEARNING",   "FUN I",        "Learning Fun I.ovl"},
    {0xE00D1399 , "LOCK",       "CHASE",        "Lock-n-Chase.ovl"},
    {0x5C7E9848 , "LOCK",       "CHASE",        "Lock-n-Chase.ovl"},
    {0x6B6E80EE , "LOCO",       "MOTION"        "Loco-Motion.ovl"},
    {0x80764301 , "LODE",       "RUNNER",       "Lode Runner.ovl"},
    {0x573B9B6D , "MASTERS",    "UNIVERSE",     "Masters of the Universe - The Power of He-Man.ovl"},
    {0xEB4383E0 , "MAXIT",      "MAXIT",        "Maxit.ovl"},
    {0x7A558CF5 , "MAZE",       "TRON",         "Maze-A-Tron.ovl"},
    {0xFF68AA22,  "MELODY",     "BLASTER",      "Melody Blaster.ovl"},
    {0xd0380fc0,  "METEORS",    "METEORS",      "Meteors.ovl"},    
    {0xE806AD91 , "MICRO",      "SURGEON",      "Microsurgeon.ovl"},
    {0x9D57498F , "MIND",       "STRIKE",       "Mind Strike.ovl"},
    {0x64d5cb31,  "MISSILE",    "DX",           "Missile Domination DX.ovl"},
    {0xec2e2320 , "MISSILE",    "DOMINATION",   "Missile Domination.ovl"},
    {0x11FB9974 , "MISSION",    "X",            "Mission X.ovl"},
    {0xb229d5c7 , "MOON",       "BLAST",        "Moon Blast.ovl"},
    {0x5F6E1AF6 , "MOTOCROSS",  "MOTOCROSS",    "Motocross.ovl"},
    {0x598662F2 , "MOUSE",      "TRAP",         "Mouse Trap.ovl"},
    {0xE367E450 , "MR",         "CHESS",        "MrChess.ovl"},
    {0xDBAB54CA , "NASL",       "SOCCER",       "NASL Soccer.ovl"},
    {0x09dc0db2 , "NINJA",      "ODYSSEY",      "Ninja Odyssey.ovl"},
    {0x4B91CF16 , "NFL",        "FOOTBALL",     "NFL Football.ovl"},
    {0x76564A13 , "NHL",        "HOCKEY",       "NHL Hockey.ovl"},
    {0x613e109b , "JR",         "PAC",          "Jr Pac-Man.ovl"},
    {0xBEF0B0C7 , "MR",         "BASIC",        "MrBASIC.ovl"},
    {0x0753544F , "MS",         "PAC",          "Ms Pac-Man.ovl"},
    {0x7334CD44 , "NIGHT",      "STALKER",      "Night Stalker.ovl"},
    {0x6B5EA9C4 , "MOUNTAIN",   "MADNESS",      "Mountain Madness - Super Pro Skiing.ovl"},
    {0xFFFFFFFF , "MSTALKER",   "MSTALKER",     "Night Stalker.ovl"},
    {0x5EE2CC2A , "NOVA",       "BLAST",        "Nova Blast.ovl"},
    {0xFFFFFFFF , "OLD",        "SCHOOL",       "Old School.ovl"},
    {0xFFFFFFFF , "OLD",        "SKOOL",        "Old School.ovl"},
    {0xFFFFFFFF , "OMEGA",      "RACE",         "Omega Race.ovl"},
    {0x36A7711B , "OPERATION",  "CLOUDFIRE",    "Operation Cloudfire.ovl"},
    {0xFFFFFFFF , "OREGON",     "BOUND",        "Oregon Bound.ovl"},
    {0xFFFFFFFF , "OREGON",     "TRAIL",        "Oregon Bound.ovl"},
    {0x45668011 , "PANDORA",    "INCIDENT",     "Pandora Incident.ovl"},
    {0x169E3584 , "PBA",        "BOWLING",      "PBA Bowling.ovl"},
    {0x800B572F , "SLAM",       "BASKETBALL",   "Slam Dunk Basketball.ovl"},
    {0xed664866 , "SUPER",      "BOWLING",      "Striker Super Pro Bowling.ovl"},
    {0xed664866 , "STRIKER",    "BOWLING",      "Striker Super Pro Bowling.ovl"},
    {0xFF87FAEC , "PGA",        "GOLF",         "PGA Golf.ovl"},
    {0xA21C31C3 , "PAC",        "MAN",          "Pac-Man.ovl"},
    {0x6E4E8EB4 , "PAC",        "MAN",          "Pac-Man.ovl"},
    {0x6084B48A , "PARSEC",     "PARSEC",       "Parsec.ovl"},
    {0x1A7AAC88 , "PENGUIN",    "LAND",         "Penguin Land.ovl"},
    {0xFFFFFFFF , "PIGGY",      "PANK",         "Piggy Bank.ovl"},
    {0xD7C5849C , "PINBALL",    "PINBALL",      "Pinball.ovl"},
    {0x9C75EFCC , "PITFALL!",   "PITFALL!",     "Pitfall!.ovl"},
    {0x1045e742 , "PITFALL3",   "PITFALL3",     "Pitfall III.ovl"},
    {0x1045e742 , "PITFALL 3",  "PITFALL 3",    "Pitfall III.ovl"},
    {0x1045e742 , "PITFALL III","PITFALL III",  "Pitfall III.ovl"}, // Must come before II below
    {0x3c1d37df , "PITFALL2",   "PITFALL2",     "Pitfall II.ovl"},
    {0x3c1d37df , "PITFALL 2",  "PITFALL 2",    "Pitfall II.ovl"},
    {0x3c1d37df , "PITFALL II", "PITFALL II",   "Pitfall II.ovl"},
    {0xBB939881 , "POLE",       "POSITION",     "Pole Position.ovl"},
    {0x0CF06519 , "POKER",      "RISQUE",       "Poker Risque.ovl"},
    {0xC51464E0 , "POPEYE",     "POPEYE",       "Popeye.ovl"},
    {0x38e9ef48 , "PRINCESS",   "QUEST",        "Princess Quest.ovl"},
    {0xFFFFFFFF , "PUMPKIN",    "TRILOGY",      "Pumpkin Trilogy.ovl"},
    {0x3ed7e397 , "PUMPKIN",    "BAKERY",       "Pumpkin Bakery.ovl"},
    {0xD8C9856A , "Q-BERT",     "Q-BERT",       "Q-Bert.ovl"},
    {0xbea60a31 , "RAIDERS",    "ARK",          "Raiders of the Lost Ark.ovl"},
    {0xC7BB1B0E , "REVERSI",    "REVERSI",      "Reversi.ovl"},
    {0x5AEF02C6 , "RICK",       "DYNAMITE",     "Rick Dynamite.ovl"},
    {0x8910C37A , "RIVER",      "RAID",         "River Raid.ovl"},
    {0x95466AD3 , "RIVER",      "RAID",         "River Raid.ovl"},
    {0x2ed0a8a4 , "ROAD",       "FIGHTER",      "Road Fighter.ovl"},
    {0xDCF4B15D , "ROYAL",      "DEALER",       "Royal Dealer.ovl"},
    {0xFFFFFFFF , "ROBOT",      "ARMY",         "Robot Army.ovl"},
    {0x8F959A6E , "SNAFU",      "SNAFU",        "SNAFU.ovl"},
    {0x47AA7977 , "SAFE",       "CRACKER",      "Safecracker.ovl"},
    {0x6E0882E7 , "SAMEGAME",   "ROBOTS",       "SameGame and Robots.ovl"},
    {0x12BA58D1 , "SAMEGAME",   "ROBOTS",       "SameGame and Robots.ovl"},
    {0x20eb8b7c , "SAMEGAME",   "ROBOTS",       "SameGame and Robots.ovl"},
    {0xE0F0D3DA , "SEWER",      "SAM",          "Sewer Sam.ovl"},
    {0xe47a0407 , "SHRINE",     "PERIL",        "Shrine of Peril.ovl"},
    {0xBA68FF28 , "SLAP",       "HOCKEY",       "Slap Shot Hockey.ovl"},
    {0x2B549528 , "SMURF",      "RESCUE",       "Smurf Rescue.ovl"},
    {0xAB5FD8BC , "SPACE",      "PATROL",       "Space Patrol.ovl"},
    {0x8F7D3069 , "SUPER",      "COBRA",        "Super Cobra.ovl"},
    {0x7C32C9B8 , "SUPER",      "COBRA",        "Super Cobra.ovl"},
    {0xe9e3f60d , "MAZE",       "CHASE",        "Scooby Doo's Maze Chase.ovl"},
    {0xFFFFFFFF , "SCOOBY",     "DOO",          "Scooby Doo's Maze Chase.ovl"},
    {0x99AE29A9 , "SEA",        "BATTLE",       "Sea Battle.ovl"},
    {0x2A4C761D , "SHARK!",     "SHARK!",       "Shark! Shark!.ovl"},
    {0xd7b8208b , "SHARK!",     "SHARK!",       "Shark! Shark!.ovl"},
    {0xFFFFFFFF , "SHARK SHARK","SHARK SHARK",  "Shark! Shark!.ovl"},
    {0xFF7CB79E , "SHARP",      "SHOT",         "Sharp Shot.ovl"},
    {0xF093E801 , "US",         "SKIING",       "Skiing.ovl"},
    {0x0e6198a5 , "GADHLAN",    "THUR",         "Gadhlan Thur.ovl"},
    {0xbe70300b , "GRAIL",      "GODS",         "Grail of the Gods.ovl"},    
    {0xFFFFFFFF , "SACRED",     "TRIBE",        "Sacred Tribe.ovl"},
    {0x6071dca7 , "SEA",        "VENTURE",      "Sea Venture.ovl"},    
    {0xE8B8EBA5 , "SPACE",      "ARMADA",       "Space Armada.ovl"},
    {0xFFFFFFFF , "SPACE",      "INVADERS",     "Space Invaders.ovl"},
    {0x1AAC64CA , "SPACE",      "BANDITS",      "Space Bandits.ovl"},
    {0xF95504E0 , "SPACE",      "BATTLE",       "Space Battle.ovl"},
    {0x39D3B895 , "SPACE",      "HAWK",         "Space Hawk.ovl"},
    {0xCF39471A , "SPACE",      "PANIC",        "Space Panic.ovl"},
    {0x3784DC52 , "SPACE",      "SPARTANS",     "Space Spartans.ovl"},
    {0xef7c4e8e , "SPACE",      "RAID",         "Space Raid.ovl"},
    {0xA95021FC , "SPIKER",     "VOLLEYBALL",   "Spiker.ovl"},
    {0xFFFFFFFF , "STAR",       "MERCENARY",    "Star Mercenary.ovl"},
    {0xB745C1CA , "MUD",        "BUGGIES",      "Stadium Mud Buggies.ovl"},
    {0x2DEACD15 , "STAMPEDE",   "STAMPEDE",     "Stampede.ovl"},
    {0xD5B0135A , "EMPIRE",     "STRIKES",      "Empire Strikes Back.ovl"},
    {0x72E11FCA , "STAR",       "STRIKE",       "Star Strike.ovl"},
    {0xFFFFFFFF , "STEAMROLLER","STEAMROLLER",  "Steamroller.ovl"},
    {0x3D9949EA , "SUB",        "HUNT",         "Sub Hunt.ovl"},
    {0xbe4d7996 , "SUPER",      "NFL",          "Super NFL Football (ECS).ovl"},
    {0x15E88FCE , "SWORDS",     "SERPENTS",     "Swords & Serpents.ovl"},
    {0xD6F44FA5 , "TNT",        "COWBOY",       "TNT Cowboy.ovl"},
    {0xCA447BBD , "DEADLY",     "DISCS",        "TRON Deadly Discs.ovl"},
    {0xFFFFFFFF , "DEADLIER",   "DISCS",        "TRON Deadly Discs.ovl"},
    {0xFFFFFFFF , "DEADLY",     "DOGS",         "Deadly Dogs.ovl"},
    {0x07FB9435 , "SOLAR",      "SAILOR",       "TRON Solar Sailer.ovl"},
    {0xbb759a58 , "SOLAR",      "SAILER",       "TRON Solar Sailer.ovl"},
    {0xFFFFFFFF , "TRON",       "SOLAR",        "TRON Solar Sailer.ovl"},
    {0x4c963cb2 , "TENNIS2",    "TENNIS2",      "Super Pro Tennis.ovl"},
    {0x4c963cb2 , "SUPER",      "TENNIS",       "Super Pro Tennis.ovl"},
    {0x16BFB8EB , "SUPER",      "DECATHLON",    "Super Pro Decathlon.ovl"},
    {0x03E9E62E , "TENNIS",     "TENNIS",       "Tennis.ovl"},
    {0xB7923858 , "SHOW",       "MUST",         "The Show Must Go On.ovl"},
    {0xF3DF94E0 , "THIN",       "ICE",          "Thin Ice.ovl"},
    {0x975AE6DF , "THIN",       "ICE",          "Thin Ice.ovl"},
    {0xd6495910 , "THIN",       "ICE",          "Thin Ice.ovl"},
    {0xC1F1CA74 , "THUNDER",    "CASTLE",       "Thunder Castle.ovl"},
    {0x67CA7C0A , "THUNDER",    "CASTLE",       "Thunder Castle.ovl"},
    {0x6F23A741 , "TROPICAL",   "TROUBLE",      "Tropical Trouble.ovl"},
    {0x67ca7c0a , "MYSTIC",     "CASTLE",       "Mystic Castle.ovl"},
    {0xD1D352A0 , "TOWER",      "DOOM",         "Tower of Doom.ovl"},
    {0x734F3260 , "TRUCKIN",    "TRUCKIN",      "Truckin.ovl"},
    {0x1AC989E2 , "TRIPLE",     "ACTION",       "Triple Action.ovl"},
    {0x095638c0 , "TRIPLE",     "CHALLENGE",    "Triple Challenge.ovl"},
    {0x6FA698B3 , "TUTANKHAM",  "TUTANKHAM",    "Tutankham.ovl"},
    {0x275F3512 , "TURBO",      "COLECO",       "Turbo.ovl"},
    {0x6b8e956c , "ULTIMATE",   "PONG",         "Ultimate Pong.ovl"},
    {0x752FD927 , "USCF",       "CHESS",        "USCF Chess.ovl"},
    {0xF9E0789E , "UTOPIA",     "UTOPIA",       "Utopia.ovl"},
    {0xC9624608 , "VANGUARD",   "VANGUARD",     "Vanguard.ovl"},
    {0xA4A20354 , "VECTRON",    "VECTRON",      "Vectron.ovl"},
    {0xde45a589 , "VOOCHKO",    "VOOCHKO",      "Voochko.ovl"},
    {0xF1ED7D27 , "WHITE",      "WATER",        "White Water.ovl"},
    {0xC2063C08 , "WORLD",      "SERIES",       "World Series Major League Baseball.ovl"},
    {0x15d9d27a , "WORLD CUP",  "FOOTBALL",     "World Cup Soccer.ovl"},
    {0x15d9d27a , "WORLD CUP",  "SOCCER",       "World Cup Soccer.ovl"},
    {0x24B667B9 , "WORM",       "WHOMPER",      "Worm Whomper.ovl"},
    {0x10D64E48 , "CHAMPIONSHIP","BASEBALL",    "Baseball.ovl"},
    {0xDAB36628 , "MLB",        "BASEBALL",     "Baseball.ovl"},
    {0x650fc1b4 , "PRO",        "BASEBALL",     "Baseball.ovl"},
    {0xFFFFFFFF , "HOME",       "RUN",          "Baseball.ovl"},
    {0x6EFA67B2 , "VENTURE",    "COLECO",       "Venture.ovl"},
    {0xFFFFFFFF , "WIZARD",     "OF WOR",       "Wizard of Wor.ovl"},
    {0x740C9C49 , "X-RALLY",    "X-RALLY",      "X-Rally.ovl"},
    {0x45119649 , "YAR",        "REVENGE",      "Yars Revenge.ovl"},
    {0x15C65DC5 , "ZAXXON",     "ZAXXON",       "Zaxxon.ovl"},
    {0xC00CBA0D , "GORF",       "GORF",         "gorf.ovl"},
    {0xFFFFFFFF , "ZOMBIE",     "MADNESS",      "Zombie Madness.ovl"},
    {0xFFFFFFFF , "STOP",       "EXPRESS",      "Stop the Express.ovl"},
    {0xFFFFFFFF , "FUBAR",      "FUBAR",        "FUBAR.ovl"},
    {0xFFFFFFFF , "JUNGLE",     "HUNT",         "Jungle Hunt.ovl"},
    {0xFFFFFFFF , "THUNDER",    "SOLDIER",      "Thunder Soldier.ovl"},
    {0x2711dcbe , "#@!^aZ",     "()##b-+",      "Maria.ovl"},
    {0x906989e2 , "HELI",       "HELI",         "HELI.ovl"},
    {0xFFFFFFFF , "KEYSTONE",   "KOPPS",        "Keystone Kopps.ovl"},
    {0xFFFFFFFF , "MR",         "TURTLE",       "Mr Turtle.ovl"},
    {0xFFFFFFFF , "QUEST",      "TIRES",        "BC Quest for Tires.ovl"},
    {0xFFFFFFFF , "KLAX",       "KLAX",         "Klax.ovl"},
    {0xFFFFFFFF , "AARDVARK",   "AARDVARK",     "Aardvark.ovl"},
    {0x51870e4a , "UPMONSTER",  "UPMONSTER",    "Upmonsters.ovl"},
    {0x63aad383 , "KVADER",     "KVADER",       "kvader.ovl"},
    {0xd6f7b4d0 , "OH",         "MUMMY",        "Oh Mummy!.ovl"},
    {0x7fd5d202 , "DEATH",      "STRIKE",       "Death Star Strike.ovl"},
    {0x8ae91ade , "ISTAR",      "ISTAR",        "istar.ovl"},
    {0xa1b83fdb , "ONION",      "ONION",        "Onion.ovl"},
    {0x3ccc255f , "SHOWCASE",   "VOL 1",        "Inty-BASIC-Showcase1.ovl"},
    {0xde8eac39 , "SHOWCASE",   "VOL 2",        "Inty-BASIC-Showcase2.ovl"},
    {0xFFFFFFFF , "MINER",      "2049",         "Miner 2049er.ovl"},
    {0xFFFFFFFF , "MINEHUNT",   "MINEHUNT",     "Minehunter.ovl"},
    {0xe08f0192 , "BEAT-EM",    "EAT-EM",       "Beat-Em-Eat-Em.ovl"},
    {0x832bb760 , "UNLUCKY",    "PONY",         "Unlucky Pony.ovl"},
    {0xFFFFFFFF , "DESERT",     "BUS",          "Desert Bus.ovl"},
    {0xFFFFFFFF , "PAULISTA",   "PAULISTA",     "Paulista Avenue Redux.ovl"},
    {0xFFFFFFFF , "MONTEZUMA",  "REVEMGE",      "Montezuma.ovl"},
    {0xFFFFFFFF , "ROAD",       "FIGHTER",      "Road Fighter.ovl"},    
    {0x00000000 , "xxx",        "zzz",          "generic.ovl"},
};


// --------------------------------------------------------------------------------------
// Get the directory in which the .ovl files are located... this is set in Global Config
// --------------------------------------------------------------------------------------
void get_ovl_directory(void)
{
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
}

// -----------------------------------------------------------------------------------------------
// Custom overlays are read in and must be in a very strict format. See the documentation
// for custom overlays for details on the format this must be in. We could probably use a
// bit more error checking here... but we expect customer overlay designers to know what's up.
// -----------------------------------------------------------------------------------------------
void load_custom_overlay(bool bCustomGeneric)
{
    FILE *fp = NULL;

    get_ovl_directory();
    
    // Is this a multi-overlay? If so, we setup for special handling
    if (multi_ovl_idx)
    {
        sprintf(tile_str, ".til%d", multi_ovl_idx);
        sprintf(map_str,  ".ma%d",  multi_ovl_idx);
        sprintf(pal_str,  ".pa%d",  multi_ovl_idx);
        sprintf(ovl_str,  ".ov%d",  multi_ovl_idx);
        sprintf(disc_str, ".dis%d", multi_ovl_idx);
        sprintf(hudx_str, ".hu%dx", multi_ovl_idx);
        sprintf(hudy_str, ".hu%dy", multi_ovl_idx);
    }
    else
    {
        strcpy(tile_str, ".tile");
        strcpy(map_str,  ".map");
        strcpy(pal_str,  ".pal");
        strcpy(ovl_str,  ".ovl");
        strcpy(disc_str, ".disc");
        strcpy(hudx_str, ".hudx");
        strcpy(hudy_str, ".hudy");
    }
    
    u8 bFound = 0;
    // If we have a game (RIP) loaded, try to find a matching overlay
    if (currentRip != NULL)
    {
        if (currentRip->GetCRC() == 0xFF68AA22) // Melody Blaster is the only game to want the Music Synth keyboard
        {
            bShowSynthesizer = 1;
        }
        
        // If user picked the special 'Generic Overlay' we force the generic overlay to be used instead...
        if (strcmp(currentRip->GetOverlayName(), "--Generic Overlay--") != 0)
        {
            strcpy(filename, directory);
            strcat(filename, currentRip->GetOverlayName());
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
                        if (fp != NULL) {bFound = 1;}
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
                            if (fp != NULL) {bFound = 1;}
                            break;
                        }
                        i++;
                    }
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
        // Get one line of the .ovl file and try to parse it...
        fgets(szName, 255, fp);
        
        // Handle Overlay Line
        if (strstr(szName, ovl_str) != NULL)
        {
            if (ov_idx < OVL_MAX)
            {
                char *ptr = strstr(szName, ovl_str);
                ptr += 5;
                myOverlay[ov_idx].x1 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myOverlay[ov_idx].x2 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myOverlay[ov_idx].y1 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myOverlay[ov_idx].y2 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                ov_idx++;
            }
        }

        // Handle Disc Line
        if (strstr(szName, disc_str) != NULL)
        {
            bUseDiscOverlay = true;
            if (disc_idx < DISC_MAX)
            {
                char *ptr = strstr(szName, disc_str);
                ptr += 6;
                myDisc[disc_idx].x1 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myDisc[disc_idx].x2 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myDisc[disc_idx].y1 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                myDisc[disc_idx].y2 = strtoul(ptr, &ptr, 10); while (*ptr == ',' || *ptr == ' ') ptr++;
                disc_idx++;
            }
        }

        // Handle HUD_x Line
        if (strstr(szName, hudx_str) != NULL)
        {
            char *ptr = strstr(szName, hudx_str);
            ptr += 6;
            hud_x = strtoul(ptr, &ptr, 10);
        }

        // Handle HUD_y Line
        if (strstr(szName, hudy_str) != NULL)
        {
            char *ptr = strstr(szName, hudy_str);
            ptr += 6;
            hud_y = strtoul(ptr, &ptr, 10);
        }
        
        // Handle MULTI Line
        if (strstr(szName, ".multi") != NULL)
        {
            char *ptr = strstr(szName, ".multi");
            ptr += 7;
            if (strstr(ptr, "LR") != NULL)
            {
                multi_ovls = 2;
                bmulti_LR = 1;
            }
            else
            {
                bmulti_LR = 0;
                multi_ovls = strtoul(ptr, &ptr, 10);
                if (multi_ovls > 8) multi_ovls = 8; // Limit to something reasonable...
            }
        }
        
        // Handle Tile Line
        if (strstr(szName, tile_str) != NULL)
        {
            char *ptr = strstr(szName, tile_str);
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
        if (strstr(szName, map_str) != NULL)
        {
            char *ptr = strstr(szName, map_str);
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
        if (strstr(szName, pal_str) != NULL)
        {
            char *ptr = strstr(szName, pal_str);
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
        if (bShowSynthesizer)
        {
            decompress(inty_synthTiles, bgGetGfxPtr(bg0b), LZ77Vram);
            decompress(inty_synthMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
            dmaCopy((void *) inty_synthPal,(u16*) BG_PALETTE_SUB,256*2);
        }
        else
        {
            decompress(bgBottom_ECSTiles, bgGetGfxPtr(bg0b), LZ77Vram);
            decompress(bgBottom_ECSMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
            dmaCopy((void *) bgBottom_ECSPal,(u16*) BG_PALETTE_SUB,256*2);
        }
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
            load_custom_overlay(true);  // This will try to load the correct overlay for this game or else default to the generic keyboard overlay
        }
    }

    unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
    dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);

    REG_BLDCNT=0; REG_BLDCNT_SUB=0; REG_BLDY=0; REG_BLDY_SUB=0;

    swiWaitForVBlank();

    if (bShowKeyboard) // ECS keyboard overlay
    {
        if (!bShowSynthesizer)
        {
            dsPrintValue(1,22,ecs_ctrl_key?1:0,ecs_ctrl_key ? (char*)"@": (char*)" ");
            dsPrintValue(5,22,0,ecs_shift_key ? (char*)"@": (char*)" ");
        }
    }
}

// --------------------------------------------------------------------------
// Allow the user to pick an overlay file for the current game. This overlay
// will override the normal overlay for the game until a new game is loaded.
// --------------------------------------------------------------------------
FICA_INTV overlayfiles[MAX_ROMS];
u16 countovls=0;
static u16 ucFicAct=0;
extern char szName[];
extern char szName2[];
char originalPath[MAX_PATH];

// ----------------------------------------------------------------------------------------
// Find all .bin, .int and .rom files in the current directory (or, if this is the first
// time we are loading up files, use the global configuration to determine what directory
// we should be starting in... after the first time we just load up the current directory).
// ----------------------------------------------------------------------------------------
void ovlFindFiles(void)
{
  DIR *pdir;
  struct dirent *pent;

  countovls = 0;
  memset(overlayfiles, 0x00, sizeof(overlayfiles));
  
  strcpy(overlayfiles[countovls++].filename, "--Generic Overlay--");

  get_ovl_directory();          // Find the overlay directory from global config selection
  pdir = opendir(directory);    // We only allow selection of overlays from the overlay directory...

  if (pdir)
  {
    while (((pent=readdir(pdir))!=NULL))
    {
        strcpy(szName2,pent->d_name);
        szName2[MAX_PATH-1] = NULL;
        if (strlen(szName2)>4)
        {
          if ( (strcasecmp(strrchr(szName2, '.'), ".ovl") == 0) )
          {
            overlayfiles[countovls].directory = false;
            strcpy(overlayfiles[countovls].filename,szName2);
            countovls++;
          }
        }
    }
    closedir(pdir);
  }

  // ----------------------------------------------
  // If we found any files, go sort the list...
  // ----------------------------------------------
  if (countovls)
  {
    extern int intvFilescmp (const void *c1, const void *c2);
    qsort(overlayfiles, countovls, sizeof (FICA_INTV), intvFilescmp);
  }
}

// --------------------------------------------------------------------------
// Display the files - up to 18 in a page. This is carried over from the
// oldest emulators I've worked on and there is some bug in here where
// it occasionally gets out of sync... it's minor enough to not be an
// issue but at some point this routine should be re-written...
// --------------------------------------------------------------------------
void OvlDisplayFiles(unsigned int NoDebGame,u32 ucSel)
{
  unsigned short ucBcl,ucGame;

  // Display all games if possible
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) (bgGetMapPtr(bg1b)),32*24*2);
  sprintf(szName,"%04d/%04d %s",(int)(1+ucSel+NoDebGame),countovls, "OVERLAYS");
  dsPrintValue(16-strlen(szName)/2,2,0,szName);
  dsPrintValue(31,4,0,(char *) (NoDebGame>0 ? "<" : " "));
  dsPrintValue(31,20,0,(char *) (NoDebGame+14<countovls ? ">" : " "));

  dsPrintValue(4,23,0,(char*)"A=PICK OVERLAY, B=BACK");

  for (ucBcl=0;ucBcl<17; ucBcl++)
  {
    ucGame= ucBcl+NoDebGame;
    if (ucGame < countovls)
    {
      strcpy(szName,overlayfiles[ucGame].filename);
      szName[29]='\0';
      if (overlayfiles[ucGame].directory)
      {
        szName[27]='\0';
        sprintf(szName2,"[%s]",szName);
        dsPrintValue(0,4+ucBcl,(ucSel == ucBcl ? 1 :  0),szName2);
      }
      else
      {
          if (overlayfiles[ucGame].favorite)
          {
             dsPrintValue(0,4+ucBcl,0, (char*)"@");
          }
          dsPrintValue(1,4+ucBcl,(ucSel == ucBcl ? 1 : 0),szName);
      }
    }
  }
}


u8 pick_overlay(void)
{
    u8 bDone=false, bRet=false;
    u16 ucHaut=0x00, ucBas=0x00,ucSHaut=0x00, ucSBas=0x00,ovlSelected= 0, firstOvlDisplay=0,nbRomPerPage, uNbRSPage;
    u16 uLenFic=0, ucFlip=0, ucFlop=0;

    getcwd(originalPath, MAX_PATH); // Save where we started from... we will restore on the way out

    // Find and display all of the .ovl files in a directory
    ovlFindFiles();

    nbRomPerPage = (countovls>=17 ? 17 : countovls);
    uNbRSPage = (countovls>=5 ? 5 : countovls);

    if (ucFicAct>countovls-nbRomPerPage)
    {
        firstOvlDisplay=countovls-nbRomPerPage;
        ovlSelected=ucFicAct-countovls+nbRomPerPage;
    }
    else
    {
        firstOvlDisplay=ucFicAct;
        ovlSelected=0;
    }
    OvlDisplayFiles(firstOvlDisplay,ovlSelected);

    // -----------------------------------------------------
    // Until the user selects a file or exits the menu...
    // -----------------------------------------------------
    while (!bDone)
    {
        if (keysCurrent() & KEY_UP)
        {
          if (!ucHaut)
          {
            ucFicAct = (ucFicAct>0 ? ucFicAct-1 : countovls-1);
            if (ovlSelected>uNbRSPage) { ovlSelected -= 1; }
            else {
              if (firstOvlDisplay>0) { firstOvlDisplay -= 1; }
              else {
                if (ovlSelected>0) { ovlSelected -= 1; }
                else {
                  firstOvlDisplay=countovls-nbRomPerPage;
                  ovlSelected=nbRomPerPage-1;
                }
              }
            }
            ucHaut=0x01;
            OvlDisplayFiles(firstOvlDisplay,ovlSelected);
          }
          else {

            ucHaut++;
            if (ucHaut>10) ucHaut=0;
          }
          uLenFic=0; ucFlip=0; ucFlop=0;
        }
        else
        {
          ucHaut = 0;
        }
        if (keysCurrent() & KEY_DOWN)
        {
          if (!ucBas) {
            ucFicAct = (ucFicAct< countovls-1 ? ucFicAct+1 : 0);
            if (ovlSelected<uNbRSPage-1) { ovlSelected += 1; }
            else {
              if (firstOvlDisplay<countovls-nbRomPerPage) { firstOvlDisplay += 1; }
              else {
                if (ovlSelected<nbRomPerPage-1) { ovlSelected += 1; }
                else {
                  firstOvlDisplay=0;
                  ovlSelected=0;
                }
              }
            }
            ucBas=0x01;
            OvlDisplayFiles(firstOvlDisplay,ovlSelected);
          }
          else
          {
            ucBas++;
            if (ucBas>10) ucBas=0;
          }
          uLenFic=0; ucFlip=0; ucFlop=0;
        }
        else {
          ucBas = 0;
        }

        // -------------------------------------------------------------
        // Left and Right on the D-Pad will scroll 1 page at a time...
        // -------------------------------------------------------------
        if (keysCurrent() & KEY_RIGHT)
        {
          if (!ucSBas)
          {
            ucFicAct = (ucFicAct< countovls-nbRomPerPage ? ucFicAct+nbRomPerPage : countovls-nbRomPerPage);
            if (firstOvlDisplay<countovls-nbRomPerPage) { firstOvlDisplay += nbRomPerPage; }
            else { firstOvlDisplay = countovls-nbRomPerPage; }
            if (ucFicAct == countovls-nbRomPerPage) ovlSelected = 0;
            ucSBas=0x01;
            OvlDisplayFiles(firstOvlDisplay,ovlSelected);
          }
          else
          {
            ucSBas++;
            if (ucSBas>10) ucSBas=0;
          }
          uLenFic=0; ucFlip=0; ucFlop=0;
        }
        else {ucSBas = 0;}

        // -------------------------------------------------------------
        // Left and Right on the D-Pad will scroll 1 page at a time...
        // -------------------------------------------------------------
        if (keysCurrent() & KEY_LEFT)
        {
          if (!ucSHaut)
          {
            ucFicAct = (ucFicAct> nbRomPerPage ? ucFicAct-nbRomPerPage : 0);
            if (firstOvlDisplay>nbRomPerPage) { firstOvlDisplay -= nbRomPerPage; }
            else { firstOvlDisplay = 0; }
            if (ucFicAct == 0) ovlSelected = 0;
            if (ovlSelected > ucFicAct) ovlSelected = ucFicAct;
            ucSHaut=0x01;
            OvlDisplayFiles(firstOvlDisplay,ovlSelected);
          }
          else
          {
            ucSHaut++;
            if (ucSHaut>10) ucSHaut=0;
          }
          uLenFic=0; ucFlip=0; ucFlop=0;
        }
        else {
          ucSHaut = 0;
        }

        // -------------------------------------------------------------------------
        // They B key will exit out of the ROM selection without picking a new game
        // -------------------------------------------------------------------------
        if ( keysCurrent() & KEY_B )
        {
          bDone=true;
          while (keysCurrent() & KEY_B);
        }

        // ------------------------------------------------------------
        // The DS 'A' key will pick the overlay and try to load it...
        // ------------------------------------------------------------
        if (keysCurrent() & KEY_A)
        {
            // If the user picked an actual .ovl filename... use it!!
            if (!overlayfiles[ucFicAct].directory)
            {
                currentRip->SetOverlayName(overlayfiles[ucFicAct].filename);
                bDone = 1;
            }
            else // It's a directory - switch into it
            {
                chdir(overlayfiles[ucFicAct].filename);
                ovlFindFiles();
                ucFicAct = 0;
                nbRomPerPage = (countovls>=16 ? 16 : countovls);
                uNbRSPage = (countovls>=5 ? 5 : countovls);
                if (ucFicAct>countovls-nbRomPerPage)
                {
                    firstOvlDisplay=countovls-nbRomPerPage;
                    ovlSelected=ucFicAct-countovls+nbRomPerPage;
                }
                else
                {
                    firstOvlDisplay=ucFicAct;
                    ovlSelected=0;
                }
                OvlDisplayFiles(firstOvlDisplay,ovlSelected);
                while (keysCurrent() & KEY_A);
            }
        }

        // --------------------------------------------
        // If the filename is too long... scroll it.
        // --------------------------------------------
        if (strlen(overlayfiles[ucFicAct].filename) > 29)
        {
            if (++ucFlip >= 15)
            {
                ucFlip = 0;
                if ((++uLenFic+29)>strlen(overlayfiles[ucFicAct].filename))
                {
                    if (++ucFlop >= 15) {uLenFic=0;ucFlop = 0;}
                    else uLenFic--;
                }
                strncpy(szName,overlayfiles[ucFicAct].filename+uLenFic,29);
                szName[29] = '\0';
                dsPrintValue(1,4+ovlSelected,1,szName);
            }
        }
        swiWaitForVBlank();
    }

    // ----------------------------------------------------------------------
    // We are going back to the main emulation now - restore bottom screen.
    // ----------------------------------------------------------------------
    chdir(originalPath);
    dsShowScreenMain(false, false);

    return 1; // Tell the caller to load the new overlay (even if it's just the old overlay)
}

// -------------------------------------------------------------------------------
// Does this overlay support multiple images/maps?  If so, this is a multi-overlay
// and  we can move to the next overlay image - this will re-read the .ovl file 
// and load up the right graphics and key-map hotspots... woot!
// -------------------------------------------------------------------------------
void swap_overlay(void)
{
    if (multi_ovls > 1)
    {
        multi_ovl_idx = (multi_ovl_idx + 1) % multi_ovls;
        if (bmulti_LR)
        {
            myConfig.controller_type = multi_ovl_idx;
        }        
        load_custom_overlay(true);  // This will try to load the correct overlay for this game or else default to the generic keyboard overlay    
    }
}

// End of Line
