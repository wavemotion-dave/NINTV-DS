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
#include <stdio.h>
#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "nintv-ds.h"
#include "database.h"
#include "CRC32.h"
#include "Rip.h"

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
// This is our internal database for .bin and .int files. Since intellivision ROMs can be mapped to various segments in the 16-bit memory, we need to know
// where to load the various chunks of data in the .int or .bin file.  We have a smaller table below this for .ROM files... those don't need a memory map
// as the .ROM contains that information... but .ROMs are really bad at setting things like iVoice or JLP flags so we force that if needed...
//
// Note: there is nothing magical about 5 mapped segments... we just limit to that so the table isn't huge. It can be expanded if needed in the future.
// ------------------------------------------------------------------------------------------------------------------------------------------------------------

const struct Database_t database[] =
{
    {0xDEADBEEF, "Generic Loader at 5000h",                     0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xD7C78754, "4-TRIS (Joseph Zbiciak 2001)",                0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xB91488E2, "4-TRIS (Joseph Zbiciak 2001)",                0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xA60E25FC, "ABPA Backgammon (Mattel 1978)",               0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xF8B1F2B7, "AD&D Cloudy Mountain (Mattel 1982)",          0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x11C3BCFA, "AD&D Cloudy Mountain (Mattel 1982)",          0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x16C3B62F, "AD&D Treasure of Tarmin (Mattel 1982)",       0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x2C668249, "Air Strike (Mattel 1982)",                    0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xB45633CF, "All-Stars MLB Baseball (Mattel 1980)",        0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x10D64E48, "World Championship Baseball (INTV 1987)",     0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x6F91FBC1, "Armor Battle (Mattel 1978)",                  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x00BE8BBA, "Astrosmash - Meteor (Mattel 1981)",           0,  0,  0,  3,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xFAB2992C, "Astrosmash (Mattel 1981)",                    0,  0,  0,  3,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x13FF363C, "Atlantis (Imagic 1981)",                      0,  0,  0,  1,  {{DB_ROM16, 0x4800, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xB35C1101, "Auto Racing (Mattel 1979)",                   0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x8AD19AB3, "B-17 Bomber (Mattel 1981)",                   1,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xDAB36628, "Baseball (Mattel 1978)",                      0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xEAF650CC, "Beamrider (Activision 1983)",                 0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xC047D487, "Beauty and the Beast (Imagic 1982)",          0,  0,  0,  1,  {{DB_ROM16, 0x4800, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x44519F2D, "Biplanes (Matel 1979)",                       0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xB03F739B, "Blockade Runner (Interphase 1983)",           0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xA63AA3D8, "Blow Out (Mattel 1983)",                      0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},    
    {0x515E1D7E, "Body Slam Super Pro Wrestling (Mattel 1988)", 0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x32697B72, "Bomb Squad (Mattel 1982)",                    1,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x18E08520, "Bouncing Pixels (JRMZ 1999)",                 0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x0202},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xAB87C16F, "Boxing (Mattel 1980)",                        0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x9F85015B, "Brickout! (Mattel 1981)",                     0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x999CCEED, "Bump 'n' Jump (Mattel 1983)",                 0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x43806375, "BurgerTime! (Mattel 1982)",                   0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xC92BAAE8, "BurgerTime! New Levels (Harley 2002)",        0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xFA492BBD, "Buzz Bombers (Mattel 1982)",                  0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x2A1E0C1C, "Buzz Bombers (Mattel 1982)",                  0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x43870908, "Carnival (Coleco 1982)",                      0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xD5363B8C, "Centipede (Atarisoft 1983)",                  0,  0,  0,  0,  {{DB_ROM16, 0x6000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x4CC46A04, "Championship Tennis (Nice Ideas 1985)",       0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x36E1D858, "Checkers (Matel 1979)",                       0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x0BF464C6, "Chip Shot Super Pro Golf (Mattel 1987)",      0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x47FDD8A8, "Choplifter (Mattel 1983)",                    0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x3289C8BA, "Commando (INTV 1987)",                        0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x4B23A757, "Congo Bongo (Sega 1983)",                     0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x3000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xE1EE408F, "Crazy Clones (Matel 1981)",                   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xCDC14ED8, "Deadly Dogs! (Unknown 1987)",                 0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x6802B191, "Deep Pockets- Super Pro Pool (Realtime 1990)",0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xD8F99AA2, "Defender (Atarisoft 1983)",                   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x3000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x5E6A8CD8, "Demon Attack (Imagic 1982)",                  0,  0,  0,  1,  {{DB_ROM16, 0x4800, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xA3ACD160, "Demonstration Cart (1982)",                   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x159AF7F7, "Dig Dug (INTV 1987)",                         0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x3000},   {DB_ROM16, 0x9000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x13EE56F1, "Diner (INTV 1987)",                           0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xC30F61C0, "Donkey Kong (Coleco 1982)",                   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x6DF61A9F, "Donkey Kong Jr (Coleco 1982)",                0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x84BEDCC1, "Dracula (Imagic 1982)",                       0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xAF8718A1, "Dragonfire (Imagic 1982)",                    0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x3B99B889, "Dreadnaught Factor (Activision 1983)",        0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x99f5783d, "Dreadnaught Factor [a1] (Activision 1983)",   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},    
    {0x20ACE89D, "Easter Eggs (Matel 1981)",                    0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x54A3FC11, "Electric Company - Math Fun (CTW 1978)",      0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xC9EAACAB, "Electric Company - Word Fun (CTW 1980)",      0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x4221EDE7, "Fathom (Imagic 1983)",                        0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x30e2819b, "Flinstones Keyboard Fun",                     0,  0,  1,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x37222762, "Frog Bog (Matel 1982)",                       0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xD27495E9, "Frogger (Parker Bros 1983)",                  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xDBCA82C5, "Go For The Gold (Matel 1981)",                0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x291AC826, "Grid Shock (Matel 1982)",                     0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xE573863A, "Groovy! (JRMZ 1999)",                         0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1E48},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x4B8C5932, "Happy Trails (Activision 1983)",              0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x120b53a9, "Happy Trails (Activision 1983)",              0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xB6A3D4DE, "Hard Hat (Matel 1979)",                       0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xB5C7F25D, "Horse Racing (Matel 1980)",                   0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xFF83FF80, "Hover Force (INTV 1986)",                     0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x3000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xA3147630, "Hypnotic Lights (Matel 1981)",                0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x4F3E3F69, "Ice Trek (Imagic 1983)",                      0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x7217ca7b, "IMI Test Cart (Matel 1979)",                  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x02919024, "Intelligent Television Demo (Matel 1978)",    0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xC83EEA4C, "Intellivision Test Cartridge (Matel 1978)",   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_ROM16, 0x7000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xEE5F1BE2, "Jetsons - Ways With Words (Matel 1983)",      0,  0,  1,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x4422868E, "King of the Mountain (Matel 1982)",           0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x8C9819A2, "Kool-Aid Man (Matel 1983)",                   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xA6840736, "Lady Bug (Coleco 1983)",                      0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x3825C25B, "Land Battle (Matel 1982)",                    0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_RAM8,  0xD000, 0x0400},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x604611C0, "Las Vegas Blackjack and Poker (Matel 1979)",  0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x48D74D3C, "Las Vegas  Roulette (Matel 1979)",            0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x19360442, "Leage of Light (Activision 1983)",            0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x75EE64F6, "Leage of Light (Activision 1983)",            0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xB4287B95, "Leage of Light (Activision 1983)",            0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x2C5FD5FA, "Learning Fun I - Math Master (INTV 1987)",    0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x632F6ADF, "Learning Fun II - Word Wizard (INTV 1987)",   0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xE00D1399, "Lock 'n' Chase (Matel 1982)",                 0,  0,  0,  3,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x5C7E9848, "Lock 'n' Chase Improved (Matel 1982)",        0,  0,  0,  3,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x6B6E80EE, "Locomotion (Matel 1982)",                     0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xF3B0C759, "Magic Carousel (Matel 1982)",                 0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x573B9B6D, "Masters of the Universe-He Man! (Matel 1983)",0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xFF68AA22, "Melody Blaster (Matel 1983)",                 0,  0,  1,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xE806AD91, "Microsurgeon (Imagic 1982)",                  0,  0,  0,  0,  {{DB_ROM16, 0x4800, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x94096229, "Minehunter (Ryan Kinnen 2004)",               0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x59898803, "Minehunter (Ryan Kinnen 2004)",               0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x9D57498F, "Mind Strike! (Matel 1982)",                   0,  0,  2,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x6746607B, "Minotaur v1.1 (Matel 1981)",                  0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x5A4CE519, "Minotaur v2 (Matel 1981)",                    0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xBD731E3C, "Minotaur (Matel 1981)",                       0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x2F9C93FC, "Minotaur Hack (Matel 1981)",                  0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x11FB9974, "Mission X (Matel 1982)",                      0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x5F6E1AF6, "Motocross (Matel 1982)",                      0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x6B5EA9C4, "Mountain Madness Super Pro Ski (INTV 1987)",  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x598662F2, "Mouse Trap (Coleco 1982)",                    0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xBEF0B0C7, "Mr. Basic Meets Bits 'N Bytes (Matel 1983)",  0,  0,  1,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xDBAB54CA, "NASL Soccer (Matel 1979)",                    0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x81E7FB8C, "NBA Basketball (Matel 1978)",                 0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x4B91CF16, "NFL Football (Matel 1978)",                   0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x76564A13, "NHL Hockey (Matel 1979)",                     0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x7334CD44, "Night Stalker (Matel 1982)",                  0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x5EE2CC2A, "Nova Blast (Imagic 1983)",                    0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xE5D1A8D2, "Number Jumble (Matel 1983)",                  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xA21C31C3, "Pac-Man (Atarisoft 1983)",                    0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x3000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x6E4E8EB4, "Pac-Man (INTV 1983)",                         0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x3000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x169E3584, "PBA Bowling (Matel 1980)",                    0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xFF87FAEC, "PGA Golf (Matel 1979)",                       0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xD7C5849C, "Pinball (Matel 1981)",                        0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x9C75EFCC, "Pitfall! (Activision 1982)",                  0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xBB939881, "Pole Position (INTV 1986)",                   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xA982E8D5, "Pong (Unknown 1999)",                         0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x0800},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xC51464E0, "Popeye (Parker Bros 1983)",                   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xD8C9856A, "Q-Bert (Parker Bros 1983)",                   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xC7BB1B0E, "Reversi (Matel 1984)",                        0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x8910C37A, "River Raid (Activision 1983)",                0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x95466AD3, "River Raid (Activision 1983)",                0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x1682D0B4, "Robot Rubble - Prototype (Activision 1983)",  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x7473916D, "Robot Rubble - Prototype (Activision 1983)",  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xA5E28783, "Robot Rubble - Prototype (Activision 1983)",  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x243B0812, "Robot Rubble - Prototype (Activision 1983)",  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xDCF4B15D, "Royal Dealer (Matel 1981)",                   0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xbdf0ccb0, "RPN Calculator (Chevallier 2003)",            0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x47AA7977, "Safecracker (Imagic 1983)",                   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x6E0882E7, "SameGame and Robots (IntelligentVision 2005)",1,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x12BA58D1, "SameGame and Robots (IntelligentVision 2005)",1,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x20eb8b7c, "SameGame and Robots (IntelligentVision 2014)",1,  0,  0,  0,  {{DB_ROM16, 0x4800, 0x2800},   {DB_ROM16, 0x8800, 0x3000},   {DB_ROM16, 0xC800, 0x3800},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},    
    {0xE221808C, "Santa's Helper (Matel 1983)",                 0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xe9e3f60d, "Scooby Doo's Maze Chase:Mattel (Matel 1983)", 0,  0,  1,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xBEF0B0C7, "Scooby Doo's Maze Chase:Mattel (Matel 1983)", 0,  0,  1,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x99AE29A9, "Sea Battle (Matel 1980)",                     0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xE0F0D3DA, "Sewer Sam (Interphase 1983)",                 0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xA610406E, "Shape Escape (John Doherty 2005)",            0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x2A4C761D, "Shark! Shark! [a1] (Matel 1982)",             0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xd7b8208b, "Shark! Shark! (Matel 1982)",                  0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xFF7CB79E, "Sharp Shot (Matel 1982)",                     0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x800B572F, "Slam Dunk Super Pro Basketball (INTV 1987)",  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xBA68FF28, "Slap Shot Super Pro Hockey (INTV 1987)",      0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x8F959A6E, "Snafu (Matel 1981)",                          0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xE8B8EBA5, "Space Armada (Matel 1981)",                   0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xF95504E0, "Space Battle (Matel 1979)",                   0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xF8EF3E5A, "Space Cadet (Matel 1982)",                    0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x39D3B895, "Space Hawk (Matel 1981)",                     0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xE98B9163, "Space Shuttle (Matel 1983)",                  1,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x3000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x3784DC52, "Space Spartans (Matel 1981)",                 1,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xA95021FC, "Spiker! Super Pro Volleyball (INTV 1988)",    0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xB745C1CA, "Stadium Mud Buggies (INTV 1988)",             0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x2DEACD15, "Stampeed (Activision 1982)",                  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x72E11FCA, "Star Strike (Matel 1981)",                    0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xD5B0135A, "Star Wars - Empire Strike Back (Parker 1983)",0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xA03EDF73, "Stack Em (Chevallier 2004)",                  0,  0,  0,  0,  {{DB_ROM16, 0x4800, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x66D396C0, "Stonix (Chevallier 2004)",                    0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x4830F720, "Street (Matel 1981)",                         0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x3D9949EA, "Sub Hunt (Matel 1981)",                       0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x8F7D3069, "Super Cobra (Konami 1981)",                   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x7C32C9B8, "Super Cobra (Konami 1981)",                   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xBAB638F2, "Super Masters! (Matel 1982)",                 0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x16BFB8EB, "Super Pro Decathlon (INTV 1987)",             0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x32076E9D, "Super Pro Football (INTV 1986)",              0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xbe4d7996, "Super NFL Football (ECS)",                    0,  0,  1,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x51B82EB7, "Super Soccer (Matel 1983)",                   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x15E88FCE, "Swords and Serpents (Imagic 1982)",           0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x1F584A69, "Takeover (Matel 1982)",                       0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x03E9E62E, "Tennis (Matel 1980)",                         0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xD43FD410, "Tetris (Joseph Zbiciak 2000)",                0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xF3DF94E0, "Thin Ice (Matel 1983)",                       0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x975AE6DF, "Thin Ice (Matel 1983)",                       0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x0000, 0x0000},   {DB_ROM16, 0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xd6495910, "Thin Ice (Matel 1983) Prototype",             0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},   
    {0xC1F1CA74, "Thunder Castle (Matel 1982)",                 0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xD1D352A0, "Tower of Doom (INTV 1986)",                   0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x1AC989E2, "Triple Action (Matel 1981)",                  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x095638C0, "Triple Challenge (INTV 1986)",                0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_ROM16, 0xC000, 0x0800},   {DB_RAM8,  0xD000, 0x0400},   {DB_NONE,  0x0000, 0x0000}}},
    {0x7A558CF5, "Tron Maze-A-Tron (Matel 1981)",               0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xCA447BBD, "Tron Deadly Discs (Matel 1981)",              0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x07FB9435, "Tron Solar Sailer (Matel 1982)",              1,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x6F23A741, "Tropical Trouble (Imagic 1982)",              0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x734F3260, "Truckin' (Imagic 1983)",                      0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x275F3512, "Turbo (Coleco 1983)",                         0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x6FA698B3, "Tutankham (Parker Bros 1983)",                0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xF093E801, "Ski Team Skiing (Matel 1980)",                0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x752FD927, "USCF Chess (Matel 1981)",                     0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_RAM8,  0xD000, 0x0400},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xF9E0789E, "Utopia (Matel 1981)",                         0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xA4A20354, "Vectron (Matel 1982)",                        0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x6EFA67B2, "Venture (Coleco 1982)",                       0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xF1ED7D27, "White Water! (Imagic 1983)",                  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xb8ae7e3b, "Word Hunt (INTV 1987)",                       0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x4b35d11d, "Word Rockets (INTV 1987)",                    0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x15D9D27A, "World Cup Football (Nice Ideas 1985)",        0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xC2063C08, "World Series Major League Baseball (Matel)",  1,  0,  1,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_R16B0, 0xE000, 0x1000},   {DB_R16B0, 0xF000, 0x1000},   {DB_R16B1, 0xF000, 0x1000}}},
    {0x24B667B9, "Worm Whomper (Activision 1983)",              0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x752b73dd, "Yogi's Frustration (Proto)",                  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x15C65DC5, "Zaxxon (Coleco 1982)",                        0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xD89AEC27, "Zombie Marbles (John Doherty 2004)",          0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xEF1BEC41, "DK Arcade (Carl Muller 2010)",                0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xFF7BB941, "D2K Arcade (Carl Muller 2011)",               0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x3000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x3B636837, "Ms Pac-Man (Carl Muller 2012)",               0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x8800, 0x3000},   {DB_ROM16, 0xD000, 0x3000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xAB5FD8BC, "Space Patrol",                                0,  0,  3,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x2B549528, "Smurf's Rescue in Gargamel's Castle",         0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x0700},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x4728C3BD, "Blowout",                                     0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x1000},   {DB_ROM16, 0xD000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xD0F83698, "Astrosmash Competition (Matel 1981)",         0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x9DB7197E, "King of the Mountain",                        0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x2100},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x6E0882E7, "Samegame & Robots",                           0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x23DC808D, "Scarfinger",                                  0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x5A144835, "Adventures of Tron",                          0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x243b0812, "Robot Rumble",                                0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x2A77A2FA, "Mystic Castle (Early Version)",               0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x2000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xAB748E96, "Mystic Castle",                               0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0x9000, 0x1000},   {DB_ROM16, 0xD000, 0x1000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xAFB30336, "Robot Finds Kitten",                          0,  0,  0,  0,  {{DB_ROM16, 0x5000, 0x2000},   {DB_ROM16, 0xA000, 0x4000},   {DB_ROM16, 0xF000, 0x1000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0xa4b2afe8, "MOB Collision Test",                          0,  0,  0,  1,  {{DB_ROM16, 0x5000, 0x0400},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}},
    {0x00000000, "xxxxxxxxxxxxxxxxxxxxxxx",                     0,  0,  0,  0,  {{DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000},   {DB_NONE,  0x0000, 0x0000}}}
};


// ------------------------------------------------------------------------------------------------------------------------
// For .ROM files we can force the user of JLP or Intellivoice if needed... sometimes the .ROM files don't get this right.
// ------------------------------------------------------------------------------------------------------------------------
const struct SpecialRomDatabase_t rom_database[] =
{                                                           // IV  JLP  ECS
    {0xef662b2b, "Grail of the Gods (JLP)",                     0,  1,  0},    
    {0xec2e2320, "Missile Domination (JLP + IV)",               1,  1,  0},    
    {0x5d6ac5a9, "Super Pixel Bros (JLP)",                      0,  1,  0},    
    {0xd63c8ad0, "JLP Test (JLP)",                              0,  1,  0},        
    {0xf0c5f311, "DK Jr (JLP)",                                 0,  1,  0},    
    {0x613e109b, "Jr Pac-Man (JLP)",                            0,  1,  0},    
    {0xc412dcde, "Jumpking Junior (JLP)",                       0,  1,  0},    
    {0xd3f14a9d, "Fantasy Puzzle (JLP)",                        0,  1,  0},    
    
    {0x69adbba6, "FW Diagnostics (IVoice)",                     1,  0,  0},
    {0x99e7b4fc, "Goatnom (IVoice)",                            1,  0,  0},
    {0x53bc0939, "Killer Bees (IVoice)",                        1,  0,  0},
    {0xbb759a58, "Tron Solar Sailor Revisited (IVoice)",        1,  0,  0},
    {0x074ce00c, "Tron Solar Sailor Revisited (IVoice)",        1,  0,  0},    
    {0x239bd45e, "Same Game and Robots (IVoice)",               1,  0,  0},
    
    // intydave's hacks for the "No ECS keyboard"
    {0x9555dc00, "Jetsons_Way_With_Words (Controller)",         0,  0,  1},
    {0x1be6fc51, "MindStrike.rom (Controller)",                 0,  0,  1},
    {0xceb02406, "Scooby_Doo.rom (Controller)",                 0,  0,  1},
    {0xbbe412bf, "Super_NFL.rom (Controller)",                  0,  0,  1},
    {0xb07e407e, "Super_Series_Baseball.rom (Controller)",      1,  0,  1},
    
    {0x8ddd534e, "FreeCont (ECS)",                              0,  0,  1},
    {0x00000000, "xxxxxxxxxxxxxxxxxxxxxxx",                     0,  0,  0}
};

// --------------------------------------------------------------------------------------------------------------------------
// See if the current .bin or .rom file is in the database. We use the file CRC to look that up as it's a good unique index.
// --------------------------------------------------------------------------------------------------------------------------
const struct Database_t *FindDatabaseEntry(UINT32 crc)
{
    int idx=0;
    while (database[idx].game_crc != 0x00000000)
    {
        if (database[idx].game_crc == crc)
        {
            return &database[idx];
        }
        idx++;
    }
    return NULL;
}

// ---------------------------------------------------------
// Look up a .ROM by CRC in the internal rom database...
// ---------------------------------------------------------
const struct SpecialRomDatabase_t *FindRomDatabaseEntry(UINT32 crc)
{
    int idx=0;
    while (rom_database[idx].game_crc != 0x00000000)
    {
        if (rom_database[idx].game_crc == crc)
        {
            return &rom_database[idx];
        }
        idx++;
    }
    return NULL;   
}

// End of Line
