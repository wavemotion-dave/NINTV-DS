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

#ifndef __DATABASE_H
#define __DATABASE_H

#include <nds.h>
#include "types.h"

// ---------------------------
// Database handling...
// ---------------------------
#define MAX_MEM_AREAS             5   

// Memory Types...
#define DB_ROM8     0
#define DB_ROM16    1
#define DB_RAM8     2
#define DB_RAM16    3
#define DB_R16B0    4
#define DB_R16B1    5
#define DB_NONE     9

struct MemArea_t
{
    UINT8       mem_type;                // ROM8, ROM16, RAM8, etc
    UINT16      mem_addr;                // Where located in the 16-bit memory space... e.g. 0x5000 or 0xD000
    UINT16      mem_size;                // The size of the memory in 16-bit memory space. e.g. 0x1000 or 0x2000
};

struct Database_t 
{
    UINT32      game_crc;                 // CRC32 of the game itself - this is how we map .BIN/.INT to this table
    const char *name;                     // Game name (e.g. "Astrosmash (Matel 1981)")
    UINT8       bIntellivoice;            // TRUE if the game uses intellivoice
    UINT8       bJLP;                     // TRUE if the game uses JLP accelerator functions or extra RAM
    UINT8       bECS;                     // TRUE if the game uses ECS
    
    struct MemArea_t mem_areas[MAX_MEM_AREAS];
};

extern const struct Database_t database[];

struct SpecialRomDatabase_t 
{
    UINT32      game_crc;                 // CRC32 of the game itself - this is how we map .BIN/.INT to this table
    const char *name;                     // Game name (e.g. "Astrosmash (Matel 1981)")
    UINT8       bIntellivoice;            // TRUE if the game uses intellivoice
    UINT8       bJLP;                     // TRUE if the game uses JLP accelerator functions or extra RAM
    UINT8       bECS;                     // TRUE if the game uses ECS
};


const struct Database_t *FindDatabaseEntry(UINT32 crc);
const struct SpecialRomDatabase_t *FindRomDatabaseEntry(UINT32 crc);

#endif
