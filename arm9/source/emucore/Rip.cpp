// =====================================================================================
// Copyright (c) 2021-2023 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#include <stdlib.h>
#include <stdio.h>

#include "Rip.h"
#include "RAM.h"
#include "ROM.h"
#include "ROMBanker.h"
#include "JLP.h"
#include "Memory.h"
#include "CRC32.h"
#include "../database.h"
#include "../nintv-ds.h"
#include "../loadgame.h"
#include "../config.h"

#define ROM_TAG_TITLE          0x01
#define ROM_TAG_PUBLISHER      0x02
#define ROM_TAG_RELEASE_DATE   0x05
#define ROM_TAG_COMPATIBILITY  0x06

UINT8 tag_compatibility[8] = {0};

extern UINT8 *bin_image_buf;
extern UINT16 *bin_image_buf16;

/* ======================================================================== */
/*  CRC16_TBL    -- Lookup table used for the CRC-16 code.                  */
/* ======================================================================== */
const uint16_t crc16_tbl[256] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

/* ======================================================================== */
/*  CRC16_UPDATE -- Updates a 16-bit CRC using the lookup table above.      */
/*                  Note:  The 16-bit CRC is set up as a left-shifting      */
/*                  CRC with no inversions.                                 */
/* ======================================================================== */
uint16_t crc16_update(uint16_t crc, uint8_t data)
{
    return (crc << 8) ^ crc16_tbl[(crc >> 8) ^ data];
}

Rip::Rip(UINT32 systemID)
: Peripheral("", ""),
  peripheralCount(0),
  targetSystemID(systemID),
  crc(0)
{
    memset(filename, 0, sizeof(filename));
    JLP16Bit = NULL;
}

Rip::~Rip()
{
    UINT16 count = GetROMCount();
    for (UINT16 i = 0; i < count; i++)
        delete GetROM(i);
    if (JLP16Bit) delete JLP16Bit;
}

void Rip::SetName(const CHAR* n)
{
    strcpy(peripheralName, n);
}

void Rip::AddPeripheralUsage(const CHAR* periphName, PeripheralCompatibility usage)
{
    strcpy(peripheralNames[peripheralCount], periphName);
    peripheralUsages[peripheralCount] = usage;
    peripheralCount++;
}

PeripheralCompatibility Rip::GetPeripheralUsage(const CHAR* periphName)
{
    for (UINT16 i = 0; i < peripheralCount; i++) {
        if (strcmpi(peripheralNames[i], periphName) == 0)
            return peripheralUsages[i];
    }
    return PERIPH_INCOMPATIBLE;
}

void ForceLoadOptions(void)
{
    // --------------------------------------------------------------------
    // If the user has picked some load options, we want to save those out
    // --------------------------------------------------------------------
    if ((load_options != 0x00) && (load_options != myConfig.load_options))
    {
        myConfig.load_options = (load_options == LOAD_NORMALLY ? 0x00: load_options);
        dsPrintValue(2,23,0, (char*)"    SAVING CONFIGURATION     ");
        SaveConfig(FALSE);
        WAITVBL;WAITVBL;WAITVBL;
        dsPrintValue(2,23,0, (char*)"                             ");
    }
    
    // If the user has overridden the load options for this game, use those instead...
    if (myConfig.load_options)
    {
        bUseECS = 0;
        bUseJLP = 0;
        bUseIVoice = 0;
        
        if ((myConfig.load_options & LOAD_WITH_JLP) == LOAD_WITH_JLP)  bUseJLP = 1;
        if ((myConfig.load_options & LOAD_WITH_ECS) == LOAD_WITH_ECS)  bUseECS = 1;
        if ((myConfig.load_options & LOAD_WITH_IVOICE) == LOAD_WITH_IVOICE)  bUseIVoice = 1;
    }
}

char cfgFilename[128];
Rip* Rip::LoadBin(const CHAR* filename)
{
    // Create the path to the .cfg file (which may or may not exist...)
    strcpy(cfgFilename, filename);
    cfgFilename[strlen(cfgFilename)-4] = 0;
    strcat(cfgFilename, ".cfg");

    // Open the binary file - we will read it all in...
    FILE* file = fopen(filename, "rb");
    if (file == NULL) 
    {
        FatalError("BIN FILE DOES NOT EXIST");
        return NULL;
    }
    
    //obtain the file size
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    if (size >= MAX_ROM_FILE_SIZE)
    {
        fclose(file);
        FatalError("BIN FILE TOO LARGE");
        return NULL;
    }
    
    // Read the file into our memory buffer - we can process it from there...
    fread(bin_image_buf, size, 1, file);
    fclose(file);
    
    //determine the crc of the designated file
    UINT32 crc = CRC32::getCrc(bin_image_buf, size);
    
    // ----------------------------------------------------------------
    // Now go and try and load all the memory regions based on either 
    // the internal database table or the .cfg file if it exists...
    // ----------------------------------------------------------------
    Rip* rip = LoadBinCfg(cfgFilename, crc, size);
    if (rip == NULL)
    {
        FatalError("UNABLE TO CREATE RIP");
        return NULL;
    }
    
    //parse the file bin_image_buf[] into the rip
    UINT32 offset = 0;
    UINT16 romCount = rip->GetROMCount();
    for (UINT16 i = 0; i < romCount; i++) 
    {
        if (offset >= size) 
        {
            // -------------------------------------------------------------------------------
            // Something is wrong, the file we're trying to load isn't large enough getting
            // here indicates a likely incorrect memory map in the database or the .cfg file
            // -------------------------------------------------------------------------------
            delete rip;
            FatalError("BAD MEMORY MAP");
            return NULL;
        }
        ROM* nextRom = rip->GetROM(i);
        nextRom->load(bin_image_buf+offset);
        offset += nextRom->getByteWidth() * nextRom->getReadSize();
    }

    // Save some basic info which we need if we end up force-loading options
    rip->SetFileName(filename);
    rip->crc = CRC32::getCrc(filename);
    rip->mySize = size;
    
    // --------------------------------------------------------------
    // If the user asked for a specific combination of hardware...
    // --------------------------------------------------------------
    ForceLoadOptions();
    
    // Add the JLP RAM module if required...
    if (bUseJLP)
    {
        rip->JLP16Bit = new JLP();
        rip->AddRAM(rip->JLP16Bit);
    }
    else
    {
        rip->JLP16Bit = NULL;
    }

    // Add the Intellivoice if required...
    if (bUseIVoice) rip->AddPeripheralUsage("Intellivoice", (bUseIVoice == 3) ? PERIPH_OPTIONAL:PERIPH_REQUIRED);
    
    // Add the ECS module if required...
    if (bUseECS)    rip->AddPeripheralUsage("ECS", (bUseECS == 3) ? PERIPH_OPTIONAL:PERIPH_REQUIRED);

    return rip;
}


// Check if a file exists... return 1 if exists, return 0 if it does not exist...
static u8 exists(const CHAR *filename)
{
    FILE *file;
    if (file = fopen(filename, "r"))
    {
        fclose(file);
        return 1;
    }

    return 0;
}


u16 maybeRAM_start[16];
u16 maybeRAM_end[16];
u8 maybeRAMidx = 0;


Rip* Rip::LoadBinCfg(const CHAR* configFile, UINT32 crc, size_t size)
{
    Rip* rip = NULL;
    const struct Database_t *db_entry = FindDatabaseEntry(crc); // Try to find the CRC in our internal database...
    
    // If we didn't find a game in the database and no .cfg exists but the .bin is 16K or less, we can assume it will load at 0x5000
    if ((db_entry == NULL) && !exists(configFile) && (size <= 16384))
    {
        db_entry = &database[0];    // Generic loader at 5000h for games up to 16K in size (fairly common)
    }
    
    maybeRAMidx = 0;
    
    if (db_entry != NULL)   // We found an entry... let's go!
    {
        rip = new Rip(ID_SYSTEM_INTELLIVISION);
        rip->JLP16Bit = NULL;
        rip->SetName(db_entry->name);
        for (UINT8 i=0; i<MAX_MEM_AREAS; i++)
        {
            switch (db_entry->mem_areas[i].mem_type)
            {
                case DB_ROM8:
                {
                    rip->AddROM(new ROM("Cartridge ROM", "", 0, sizeof(UINT8), db_entry->mem_areas[i].mem_size, db_entry->mem_areas[i].mem_addr));
                    break;
                }
                case DB_ROM16:
                {
                    rip->AddROM(new ROM("Cartridge ROM", "", 0, sizeof(UINT16), db_entry->mem_areas[i].mem_size, db_entry->mem_areas[i].mem_addr));
                    break;
                }
                case DB_R16B0:
                {
                    ROM *bankedROM = new ROM("Cartridge ROM", "", 0, sizeof(UINT16), db_entry->mem_areas[i].mem_size, db_entry->mem_areas[i].mem_addr);
                    bankedROM->SetEnabled(true);
                    rip->AddROM(bankedROM);
                    rip->AddRAM(new ROMBanker(bankedROM, db_entry->mem_areas[i].mem_addr+0xFFF, 0xFFF0, db_entry->mem_areas[i].mem_addr+0xA50, 0x000F, 0));
                    break;
                }
                case DB_R16B1:
                {
                    ROM *bankedROM = new ROM("Cartridge ROM", "", 0, sizeof(UINT16), db_entry->mem_areas[i].mem_size, db_entry->mem_areas[i].mem_addr);
                    bankedROM->SetEnabled(false);
                    rip->AddROM(bankedROM);
                    rip->AddRAM(new ROMBanker(bankedROM, db_entry->mem_areas[i].mem_addr+0xFFF, 0xFFF0, db_entry->mem_areas[i].mem_addr+0xA50, 0x000F, 1));
                    break;
                }
                case DB_RAM8:
                {
                    rip->AddRAM(new RAM(db_entry->mem_areas[i].mem_size, db_entry->mem_areas[i].mem_addr, 0xFFFF, 0xFFFF, 8));
                    break;
                }
                case DB_RAM16:
                {
                    rip->AddRAM(new RAM(db_entry->mem_areas[i].mem_size, db_entry->mem_areas[i].mem_addr, 0xFFFF, 0xFFFF, 16));
                    break;
                }
                default:
                    break;
            }
        }
        
        // Now add the required peripherals...
        if (db_entry->bIntellivoice)
        {
            if ((db_entry->game_crc == 0xC2063C08) && !isDSiMode())
            {
                // Skip Intellivoice for World Series of Baseball on the DS-LITE
            }
            else
            {
                bUseIVoice = db_entry->bIntellivoice;
            }
        }
        if (db_entry->bECS)
        {
            bUseECS = db_entry->bECS;
        }        
        if (db_entry->bJLP)
        {
            bUseJLP = db_entry->bJLP;
        }
    }
    else    // Didn't find it... let's see if we can read a .cfg file
    {
        FILE* cfgFile = fopen(configFile, "r");
        if (cfgFile != NULL)
        {
            rip = new Rip(ID_SYSTEM_INTELLIVISION);
            rip->JLP16Bit = NULL;
            CHAR nextLine[128];
            int cfg_mode=0;
            while (!feof(cfgFile)) 
            {
                if (fgets(nextLine, 127, cfgFile) != NULL)
                {
                    if (strstr(nextLine, "[mapping]") != NULL)          cfg_mode = 1;
                    else if (strstr(nextLine, "[memattr]") != NULL)     cfg_mode = 2;
                    else if (strstr(nextLine, "[vars]") != NULL)        cfg_mode = 3;
                    else
                    {
                        char *ptr = nextLine;
                        while ((*ptr == ' ') || (*ptr == '\t')) ptr++;
                        if (cfg_mode == 1)  // [mapping] Mode
                        {
                            if (*ptr == '$')
                            {
                                ptr++;  
                                UINT16 start_addr = strtoul(ptr, &ptr, 16);
                                while (*ptr == ' ' || *ptr == '\t' || *ptr == '-' || *ptr == '$') ptr++;
                                UINT16 end_addr = strtoul(ptr, &ptr, 16);                            
                                while (*ptr == ' ' || *ptr == '\t' || *ptr == '=' || *ptr == '$') ptr++;
                                UINT16 map_addr = strtoul(ptr, &ptr, 16);
                                ROM *newROM = new ROM("Cartridge ROM", "", 0, sizeof(UINT16), (UINT16)((end_addr-start_addr) + 1), map_addr);
                                while (*ptr == ' ' || *ptr == '\t' || *ptr == '=' || *ptr == '$') ptr++;
                                if (strstr(ptr, "PAGE") != NULL)
                                {
                                    ptr+=5;
                                    UINT16 page = (UINT16)strtoul(ptr, &ptr, 16) & 0xf;
                                    if (page == 0) newROM->SetEnabled(true); else newROM->SetEnabled(false);
                                    rip->AddROM(newROM);
                                    rip->AddRAM(new ROMBanker(newROM, (map_addr&0xF000)+0xFFF, 0xFFF0, (map_addr&0xF000)+0xA50, 0x000F, page));
                                }
                                else rip->AddROM(newROM);
                            }
                        }
                        if (cfg_mode == 2)  // [memattr] Mode
                        {
                            if (*ptr == '$')
                            {
                                ptr++;
                                UINT16 start_addr = strtoul(ptr, &ptr, 16);
                                while (*ptr == ' ' || *ptr == '\t' || *ptr == '-' || *ptr == '$') ptr++;
                                UINT16 end_addr = strtoul(ptr, &ptr, 16);                            
                                if (strstr(ptr, "RAM 8") != NULL)
                                {
                                    rip->AddRAM(new RAM((UINT16)((end_addr-start_addr) + 1), start_addr, 0xFFFF, 0xFFFF, 8));
                                }
                                else if (strstr(ptr, "RAM 16") != NULL)
                                {
                                    if ((start_addr >= 0x8000) && (start_addr <= 0x9FFF))
                                    {
                                        maybeRAM_start[maybeRAMidx] = start_addr;
                                        maybeRAM_end[maybeRAMidx] = end_addr;
                                        maybeRAMidx++;
                                    }
                                    else
                                    {
                                        rip->AddRAM(new RAM((UINT16)((end_addr-start_addr) + 1), start_addr, 0xFFFF, 0xFFFF, 16));
                                    }
                                }
                            }
                        }
                        if (cfg_mode == 3)  // [vars] Mode
                        {
                            if (strstr(ptr, "jlp"))
                            {
                                bUseJLP = 1;
                            }
                            if (strstr(ptr, "voice"))
                            {
                                bUseIVoice = 1;
                            }
                            if (strstr(ptr, "ecs"))
                            {
                                bUseECS = 1;
                            }
                        }
                        
                        // ------------------------------------------------------------------------
                        // Make sure we haven't exceeded the maximum number of mapped ROM segments
                        // ------------------------------------------------------------------------
                        if (rip->GetRAMCount() >= MAX_ROMS) break;
                        if (rip->GetROMCount() >= MAX_ROMS) break;
                    }
                }
            }
            fclose(cfgFile);

            // If we were asked to have 16-bit RAM in the 8000-9FFF region and we didn't get JLP enabled (which has its own RAM mapped there), we map new RAM
            if (maybeRAMidx && !bUseJLP)
            {
                for (u8 idx=0; idx<maybeRAMidx; idx++)
                {
                    rip->AddRAM(new RAM((UINT16)((maybeRAM_end[idx]-maybeRAM_start[idx]) + 1), maybeRAM_start[idx], 0xFFFF, 0xFFFF, 16));   
                }
            }
            
        }
    }

    return rip;    
}

Rip* Rip::LoadRom(const CHAR* filename)
{
    static char tmp_buf[64];
    u16 crc16;

    FILE* infile = fopen(filename, "rb");
    if (infile == NULL) 
    {
        FatalError("ROM FILE NOT FOUND");
        return NULL;
    }
    
    //obtain the file size
    fseek(infile, 0, SEEK_END);
    size_t size = ftell(infile);
    rewind(infile);
    if (size >= MAX_ROM_FILE_SIZE)
    {
        fclose(infile);
        FatalError("ROM FILE TOO LARGE");
        return NULL;
    }
    
    //read the magic byte (should always be $A8 or $41 or $61)
    int read = fgetc(infile);
    if ((read != 0xA8) && (read != 0x41) && (read != 0x61))
    {
        fclose(infile);
        FatalError("ROM MAGIC BYTE MISSING");
        return NULL;
    }

    //read the number of ROM segments
    int romSegmentCount = fgetc(infile);
    read = fgetc(infile);
    if ((read ^ 0xFF) != romSegmentCount) 
    {
        FatalError("ROM SEGMENT COUNT INVALID");
        fclose(infile);
        return NULL;
    }
    
    memset(tag_compatibility, 0x00, sizeof(tag_compatibility));

    Rip* rip = new Rip(ID_SYSTEM_INTELLIVISION);

    // ----------------------------------------------------------------
    // Read through the .ROM and parse out all of the rom segments...
    // ----------------------------------------------------------------
    int i;
    for (i = 0; i < romSegmentCount; i++) 
    {
        read = fgetc(infile);
        u16 calcCrc16 = crc16_update(0xFFFF, read);
        UINT16 start = (UINT16)(read << 8);
        read = fgetc(infile);
        calcCrc16 = crc16_update(calcCrc16, read);
        UINT16 end = (UINT16)((read << 8) | 0xFF);
        UINT16 size = (UINT16)((end-start)+1);

        //finally, transfer the ROM image to our binary buffer
        UINT16* romImage = (UINT16*)bin_image_buf16;
        int j;
        for (j = 0; j < size; j++)
        {
            int hi = fgetc(infile);
            int lo = fgetc(infile);
            calcCrc16 = crc16_update(calcCrc16, hi);
            calcCrc16 = crc16_update(calcCrc16, lo);
            int nextbyte = (hi << 8) | lo;
            romImage[j] = (UINT16)nextbyte;
        }
        
        //read the CRC
        crc16 = (fgetc(infile) << 8) | fgetc(infile);
        
        // Validate the CRC to make sure this ROM segment has integrity...
        if (calcCrc16 == crc16)
        {
            // And finally, add this rom segment to our RIP...
            rip->AddROM(new ROM("Cartridge ROM", romImage, 2, size, start, 0xFFFF));
        }
        else 
        {
            FatalError("ROM SEGMENT INTEGRITY FAIL");
            fclose(infile);
            return NULL;
        }
    }

    // Now skip over the enable tables.  These have a well-defined and fixed size and we don't use them.s
    for (i = 0; i < 50; i++)
    {
        fgetc(infile);
    }
    
    // Read through the rest of the .ROM and look for tags...
    while ((read = fgetc(infile)) != -1) 
    {
        u16 calcCrc16 = crc16_update(0xFFFF, read);
        u16 length = (read & 0x3F);
        read = (read & 0xC) >> 6;
        for (i = 0; i < read; i++)
        {
            u8 read2 = fgetc(infile);
            calcCrc16 = crc16_update(calcCrc16, read2);
            length = (length | (read2 << ((8*i)+6)));
        }

        u8 type = fgetc(infile);
        calcCrc16 = crc16_update(calcCrc16, type);
        switch (type) 
        {
            case ROM_TAG_TITLE:
            {
                for (i = 0; i < length; i++) {
                    read = fgetc(infile);
                    tmp_buf[i] = (char)read;
                }
                crc16 = (fgetc(infile) << 8) | fgetc(infile);
                rip->SetName(tmp_buf);
            }
            break;
            case ROM_TAG_PUBLISHER:
            {
                for (i = 0; i < length; i++) {
                    read = fgetc(infile);
                    tmp_buf[i] = (char)read;
                }
                crc16 = (fgetc(infile) << 8) | fgetc(infile);
            }
            break;
                
            //    7   6   5   4   3   2   1   0
            //  +---+---+---+---+---+---+---+---+
            //  |  ECS  | rsvd  | VOICE | KEYBD |    byte 0
            //  +---+---+---+---+---+---+---+---+
            //  00 -- Don't Care.  The cartridge works with or without the peripheral.  The peripheral is ignored.
            //  01 -- Supports.  The cartridge works with the peripheral and may provide extra functionality when used it.
            //  10 -- Requires.  The cartridge will not work without this particular peripheral.
            //  11 -- Incompatible.  The peripheral must not be used with this cartridge, because the two are not compatible.                
            case ROM_TAG_COMPATIBILITY:
            {
                read = fgetc(infile);
                tag_compatibility[0] = read;
                calcCrc16 = crc16_update(calcCrc16, read);
                
                // Check for ECS Supported or Required...
                if ((read & 0xC0) == 0x80) bUseECS = 1; // Required
                if ((read & 0xC0) == 0x40) bUseECS = 3; // Supports
                
                // Check for Intellivoice Supported or Required...
                if ((read & 0x0C) == 0x08) bUseIVoice = 1; // Required
                if ((read & 0x0C) == 0x04) bUseIVoice = 3; // Supports
                
                for (i = 0; i < length-1; i++) // -1 because we already processed the first byte above...
                {
                    read = fgetc(infile);
                    tag_compatibility[1+i] = read;
                    calcCrc16 = crc16_update(calcCrc16, read);
                    // ------------------------------------------------------------------------
                    // Optional Byte 4 is of interest to us as that  one has JLP stuff in it 
                    // +-----+-----+------+-----+-----+-----+-----+-----+
                    // | JLP Accel | LTOM |    reserved     | JLPF[9:8] |    byte 3
                    // +-----+-----+------+-----+-----+-----+-----+-----+
                    // 00  JLP accel disabled; flash disabled (i.e. no JLP features)
                    // 01  JLP accel enabled, default to "on";  flash disabled
                    // 10  JLP accel enabled, default to "off"; flash enabled
                    // 11  JLP accel enabled, default to "on";  flash enabled                        
                    // ------------------------------------------------------------------------
                    if (i == 2) // 4th byte (first byte further above... 0,1,2 = fourth byte)
                    {   
                        if ((read & 0xC0) != 0x00) bUseJLP = 1; else bUseJLP = 0;
                    }                    
                }
            }
            break;
                
            case ROM_TAG_RELEASE_DATE:
            default:
            {
                for (i = 0; i < length; i++) {
                    fgetc(infile);
                    fgetc(infile);
                }
                fgetc(infile);
                fgetc(infile);
            }
            break;
        }
    }
    fclose(infile);
    
    rip->SetFileName(filename);
    rip->crc = CRC32::getCrc(filename);
    rip->mySize = size;

    // See if we have any special overrides for .ROM files
    const struct SpecialRomDatabase_t *db_entry = FindRomDatabaseEntry(rip->crc); // Try to find the CRC in our internal database...    
    if (db_entry != NULL)
    {
        if (db_entry->bJLP)             bUseJLP     = db_entry->bJLP;
        if (db_entry->bIntellivoice)    bUseIVoice  = db_entry->bIntellivoice;
        if (db_entry->bECS)             bUseECS     = db_entry->bECS;
    }
    
    // --------------------------------------------------------------
    // If the user asked for a specific combination of hardware...
    // --------------------------------------------------------------
    ForceLoadOptions();

    // Load the JLP RAM module if required...
    if (bUseJLP)
    {
        rip->JLP16Bit = new JLP();
        rip->AddRAM(rip->JLP16Bit);
    }
    else
    {
        rip->JLP16Bit = NULL;
    }

    // Load the Intellivoice if asked for...
    if (bUseIVoice) rip->AddPeripheralUsage("Intellivoice", (bUseIVoice == 3) ? PERIPH_OPTIONAL:PERIPH_REQUIRED);
    
    // Load the ECS if asked for...
    if (bUseECS)    rip->AddPeripheralUsage("ECS", (bUseECS == 3) ? PERIPH_OPTIONAL:PERIPH_REQUIRED);

    return rip;
}



