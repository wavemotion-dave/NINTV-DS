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
#include "../ds_tools.h"

#define ROM_TAG_TITLE          0x01
#define ROM_TAG_PUBLISHER      0x02
#define ROM_TAG_RELEASE_DATE   0x05
#define ROM_TAG_COMPATIBILITY  0x06


extern UINT8 *bin_image_buf;
extern UINT16 *bin_image_buf16;

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

Rip* Rip::LoadBin(const CHAR* filename)
{
    char cfgFilename[128];

    // Create the path to the .cfg file (which may or may not exist...)
    strcpy(cfgFilename, filename);
    cfgFilename[strlen(cfgFilename)-4] = 0;
    strcat(cfgFilename, ".cfg");

    //determine the crc of the designated file
    UINT32 crc = CRC32::getCrc(filename);
    
    // ----------------------------------------------------------------
    // Now go and try and load all the memory regions based on either 
    // the internal database table or the .cfg file if it exists...
    // ----------------------------------------------------------------
    Rip* rip = LoadBinCfg(cfgFilename, crc);
    if (rip == NULL)
    {
        FatalError("UNABLE TO CREATE RIP");
        return NULL;
    }

    //load the data into the rip
    FILE* file = fopen(filename, "rb");
    if (file == NULL) 
    {
        FatalError("BIN FILE DOES NOT EXIST");
        delete rip;
        return NULL;
    }

    //obtain the file size
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    if (size >= MAX_ROM_FILE_SIZE)
    {
        fclose(file);
        delete rip;
        FatalError("BIN FILE TOO LARGE");
        return NULL;
    }

    // Read the file into our memory buffer
    fread(bin_image_buf, size, 1, file);
    fclose(file);

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
    
    if (bUseIVoice) rip->AddPeripheralUsage("Intellivoice", PERIPH_REQUIRED);
    if (bUseECS & !bUseIVoice) rip->AddPeripheralUsage("ECS", (bUseECS != 3) ? PERIPH_REQUIRED:PERIPH_OPTIONAL);

    rip->SetFileName(filename);
    rip->crc = CRC32::getCrc(filename);

    return rip;
}

Rip* Rip::LoadBinCfg(const CHAR* configFile, UINT32 crc)
{
    Rip* rip = NULL;
    const struct Database_t *db_entry = FindDatabaseEntry(crc); // Try to find the CRC in our internal database...
    
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
                rip->AddPeripheralUsage("Intellivoice", PERIPH_REQUIRED);
            }
        }
        if (db_entry->bECS)
        {
            bUseECS = db_entry->bECS;
            rip->AddPeripheralUsage("ECS", (bUseECS != 3) ? PERIPH_REQUIRED:PERIPH_OPTIONAL);
        }        
        if (db_entry->bJLP)
        {
            rip->JLP16Bit = new JLP();
            rip->AddRAM(rip->JLP16Bit);
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
                                    rip->AddRAM(new RAM((UINT16)((end_addr-start_addr) + 1), start_addr, 0xFFFF, 0xFFFF, 16));
                                }
                            }
                        }
                        if (cfg_mode == 3)  // [vars] Mode
                        {
                            if (strstr(ptr, "jlp"))
                            {
                                rip->JLP16Bit = new JLP();
                                rip->AddRAM(rip->JLP16Bit);
                            }
                            if (strstr(ptr, "voice"))
                            {
                                rip->AddPeripheralUsage("Intellivoice", PERIPH_REQUIRED);
                            }
                            if (strstr(ptr, "ecs"))
                            {
                                bUseECS=1;
                                rip->AddPeripheralUsage("ECS", PERIPH_OPTIONAL);
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
        }
    }

    return rip;    
}

Rip* Rip::LoadRom(const CHAR* filename)
{
    char tmp_buf[64];
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

    //read the magic byte (should always be $A8 or $41)
    int read = fgetc(infile);
    if ((read != 0xA8) && (read != 0x41))
    {
        fclose(infile);
        FatalError("ROM MAGIC BYTE MISSING");
        return NULL;
    }

    //read the number of ROM segments
    int romSegmentCount = fgetc(infile);
    read = fgetc(infile);
    if ((read ^ 0xFF) != romSegmentCount) {
        fclose(infile);
        return NULL;
    }

    Rip* rip = new Rip(ID_SYSTEM_INTELLIVISION);
    
    int i;
    for (i = 0; i < romSegmentCount; i++) {
        UINT16 start = (UINT16)(fgetc(infile) << 8);
        UINT16 end = (UINT16)((fgetc(infile) << 8) | 0xFF);
        UINT16 size = (UINT16)((end-start)+1);

        //finally, transfer the ROM image
        UINT16* romImage = (UINT16*)bin_image_buf16;
        int j;
        for (j = 0; j < size; j++) {
            int nextbyte = fgetc(infile) << 8;
            nextbyte |= fgetc(infile);
            romImage[j] = (UINT16)nextbyte;
        }
        rip->AddROM(new ROM("Cartridge ROM", romImage, 2, size, start, 0xFFFF));

        //read the CRC
        fgetc(infile); fgetc(infile);
        //TODO: validate the CRC16 instead of just skipping it
        //int crc16 = (fgetc(infile) << 8) | fgetc(infile);
        //...
    }

    //no support currently for the access tables, so skip them
    for (i = 0; i < 16; i++)
        fgetc(infile);

    //no support currently for the fine address restriction
    //tables, so skip them, too
    for (i = 0; i < 32; i++)
        fgetc(infile);

    // Read through the rest of the .ROM and look for tags...
    while ((read = fgetc(infile)) != -1) 
    {
        int length = (read & 0x3F);
        read = (read & 0xC) >> 6;
        for (i = 0; i < read; i++)
            length = (length | (fgetc(infile) << ((8*i)+6)));

        int type = fgetc(infile);
        int crc16;
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
            case ROM_TAG_COMPATIBILITY:
            {
                read = fgetc(infile);
                BOOL requiresECS = ((read & 0xC0) != 0x80);
                if (requiresECS)
                    rip->AddPeripheralUsage("ECS", PERIPH_REQUIRED);
                
                BOOL intellivoiceSupport = ((read & 0x0C) != 0x0C);
                if (intellivoiceSupport)
                    rip->AddPeripheralUsage("Intellivoice", PERIPH_OPTIONAL);
                for (i = 0; i < length-1; i++) 
                {
                    fgetc(infile);
                    fgetc(infile);
                }
                fgetc(infile);
                fgetc(infile);
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

    // See if we have any special overrides...
    const struct SpecialRomDatabase_t *db_entry = FindRomDatabaseEntry(rip->crc); // Try to find the CRC in our internal database...    
    if (db_entry != NULL)
    {
        if (db_entry->bJLP) bUseJLP=true;       
        if (db_entry->bIntellivoice) bUseIVoice=true;       
        if (db_entry->bECS) bUseECS=true;
    }

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

    // Force Intellivoice if asked for...
    if (bUseIVoice) rip->AddPeripheralUsage("Intellivoice", PERIPH_REQUIRED);
    // Use ECS if asked for...
    if (bUseECS) rip->AddPeripheralUsage("ECS", PERIPH_REQUIRED);

    return rip;
}



