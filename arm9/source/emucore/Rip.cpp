
#include <stdlib.h>
#include <stdio.h>

#include "Rip.h"
#include "RAM.h"
#include "ROM.h"
#include "JLP.h"
#include "ROMBanker.h"
#include "CRC32.h"
#include "../database.h"

#define MAX_MEMORY_UNITS  16
#define MAX_STRING_LENGTH 256

#define ROM_TAG_TITLE          0x01
#define ROM_TAG_PUBLISHER      0x02
#define ROM_TAG_RELEASE_DATE   0x05
#define ROM_TAG_COMPATIBILITY  0x06

Rip::Rip(UINT32 systemID)
: Peripheral("", ""),
  peripheralCount(0),
  targetSystemID(systemID),
  crc(0)
{
    producer = new CHAR[1];
    strcpy(producer, "");
    year = new CHAR[1];
    strcpy(year, "");
    memset(filename, 0, sizeof(filename));
    JLP16Bit = NULL;
}

Rip::~Rip()
{
    for (UINT16 i = 0; i < peripheralCount; i++)
        delete[] peripheralNames[i];
    UINT16 count = GetROMCount();
    for (UINT16 i = 0; i < count; i++)
        delete GetROM(i);
    delete[] producer;
    delete[] year;
    if (JLP16Bit) delete JLP16Bit;
}

void Rip::SetName(const CHAR* n)
{
    if (this->peripheralName)
        delete[] peripheralName;

    peripheralName = new CHAR[strlen(n)+1];
    strcpy(peripheralName, n);
}

void Rip::SetProducer(const CHAR* p)
{
    if (this->producer)
        delete[] producer;

    producer = new CHAR[strlen(p)+1];
    strcpy(producer, p);
}

void Rip::SetYear(const CHAR* y)
{
    if (this->year)
        delete[] year;

    year = new CHAR[strlen(y)+1];
    strcpy(year, y);
}

void Rip::AddPeripheralUsage(const CHAR* periphName, PeripheralCompatibility usage)
{
    peripheralNames[peripheralCount] = new CHAR[strlen(periphName)+1];
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
    Rip* rip = LoadCartridgeConfiguration(cfgFilename, crc);
    if (rip == NULL)
        return NULL;

    //load the data into the rip
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        delete rip;
        return NULL;
    }

    //obtain the file size
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    //load in the file image
    UINT8* image = new UINT8[size];
    UINT32 count = 0;
    while (count < size)
        image[count++] = (UINT8)fgetc(file);
    fclose(file);

    //parse the file image into the rip
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
            delete[] image;
            delete rip;
            return NULL;
        }
        ROM* nextRom = rip->GetROM(i);
        nextRom->load(image+offset);
        offset += nextRom->getByteWidth() * nextRom->getReadSize();
    }

    delete[] image;

    // Add the JLP RAM module if required...
    extern bool bUseJLP;
    if (bUseJLP)
    {
        rip->JLP16Bit = new JLP();
        rip->AddRAM(rip->JLP16Bit);
    }
    else
    {
        rip->JLP16Bit = NULL;
    }
    
    extern bool bForceIvoice;
    if (bForceIvoice) rip->AddPeripheralUsage("Intellivoice", PERIPH_REQUIRED);

    rip->SetFileName(filename);
    rip->crc = CRC32::getCrc(filename);

    return rip;
}

Rip* Rip::LoadCartridgeConfiguration(const CHAR* configFile, UINT32 crc)
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
            rip->AddPeripheralUsage("Intellivoice", PERIPH_REQUIRED);
        }
        if (db_entry->bJLP)
        {
            rip->JLP16Bit = new JLP();
            rip->AddRAM(rip->JLP16Bit);
        }
        if (db_entry->bECS)
        {
            // TODO - give warning that ECS is not supported...
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
                fgets(nextLine, 127, cfgFile);
                if (feof(cfgFile)) continue;
                if (strstr(nextLine, "[mapping]") != NULL)          cfg_mode = 1;
                else if (strstr(nextLine, "[memattr]") != NULL)     cfg_mode = 2;
                else if (strstr(nextLine, "[vars]") != NULL)        cfg_mode = 3;
                else
                {
                    char *ptr = nextLine;
                    while (*ptr == ' ') ptr++;
                    if (cfg_mode == 1)  // [mapping] Mode
                    {
                        if (*ptr == '$')
                        {
                            ptr++;
                            UINT16 start_addr = strtoul(ptr, &ptr, 16);
                            while (*ptr == ' ' || *ptr == '-' || *ptr == '$') ptr++;
                            UINT16 end_addr = strtoul(ptr, &ptr, 16);                            
                            while (*ptr == ' ' || *ptr == '=' || *ptr == '$') ptr++;
                            UINT16 map_addr = strtoul(ptr, &ptr, 16);
                            rip->AddROM(new ROM("Cartridge ROM", "", 0, sizeof(UINT16), (UINT16)((end_addr-start_addr) + 1), map_addr));
                        }
                    }
                    if (cfg_mode == 2)  // [memattr] Mode
                    {
                        if (*ptr == '$')
                        {
                            ptr++;
                            UINT16 start_addr = strtoul(ptr, &ptr, 16);
                            while (*ptr == ' ' || *ptr == '-' || *ptr == '$') ptr++;
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
                    }
                }                
            }
            fclose(cfgFile);
        }
    }
#if 0    
    CHAR scrc[9];
    sprintf(scrc, "%08X", crc);

    //find the config that matches this crc
    FILE* cfgFile = fopen(configFile, "r");
    if (cfgFile == NULL)
        return NULL;

    BOOL parseSuccess = FALSE;
    while (!feof(cfgFile)) {
        fgets(nextLine, 256, cfgFile);
        //size_t length = strlen(nextLine);
        if (strstr(nextLine, scrc) != nextLine) {
            if (parseSuccess)
                break;
            else
                continue;
        }

        CHAR* nextToken;
        if ((nextToken = strstr(nextLine, "Info")) != NULL) {
            if ((nextToken = strrchr(nextLine, ':')) == NULL)
                continue;

            if (rip == NULL)
                rip = new Rip(ID_SYSTEM_INTELLIVISION);
            rip->SetYear(nextToken+1);

            *nextToken = NULL;
            if ((nextToken = strrchr(nextLine, ':')) == NULL)
                continue;
            rip->SetProducer(nextToken+1);

            *nextToken = NULL;
            if ((nextToken = strrchr(nextLine, ':')) == NULL)
                continue;
            rip->SetName(nextToken+1);

            parseSuccess = TRUE;
        }
        else if ((nextToken = strstr(nextLine, "ROM")) != NULL) {
            //TODO: check to make sure we don't exceed the maximum number of roms for a Rip

            //parse rom bit width
            if ((nextToken = strrchr(nextLine, ':')) == NULL)
                continue;

            //first check to see if this is a banked rom; if there is an equals (=) sign in
            //the last field, then we assume we should parse banking
            UINT16 triggerAddress, triggerMask, triggerValue, onMask, onValue;
            triggerAddress = triggerMask = triggerValue = onMask = onValue = 0;
            CHAR* nextEqual = strrchr(nextLine, '=');
            if (nextEqual != NULL && nextEqual > nextToken) {
                onValue = (UINT16)strtol(nextEqual+1, NULL, 16);
                *nextEqual = NULL;
                onMask = (UINT16)strtol(nextToken+1, NULL, 16);
                *nextToken = NULL;
                if ((nextToken = strrchr(nextLine, ':')) == NULL)
                    continue;

                nextEqual = strrchr(nextLine, '=');
                if (nextEqual == NULL || nextEqual < nextToken)
                    continue;

                CHAR* nextComma = strrchr(nextLine, ',');
                if (nextComma == NULL || nextComma < nextToken || nextComma > nextEqual)
                    continue;

                triggerValue = (UINT16)strtol(nextEqual+1, NULL, 16);
                *nextEqual = NULL;
                triggerMask = (UINT16)strtol(nextComma+1, NULL, 16);
                *nextComma = NULL;
                triggerAddress = (UINT16)strtol(nextToken+1, NULL, 16);
                *nextToken = NULL;

                if ((nextToken = strrchr(nextLine, ':')) == NULL)
                    continue;
            }

            UINT8 romBitWidth = (UINT8)atoi(nextToken+1);

            //parse rom size
            *nextToken = NULL;
            if ((nextToken = strrchr(nextLine, ':')) == NULL)
                continue;
            UINT16 romSize = (UINT16)strtol(nextToken+1, NULL, 16);

            *nextToken = NULL;
            if ((nextToken = strrchr(nextLine, ':')) == NULL)
                continue;
            UINT16 romAddress = (UINT16)strtol(nextToken+1, NULL, 16);

            if (rip == NULL)
                rip = new Rip(ID_SYSTEM_INTELLIVISION);
            ROM* nextRom = new ROM("Cartridge ROM", "", 0, (romBitWidth+7)/8, romSize, romAddress);
            rip->AddROM(nextRom);
            if (triggerAddress != 0)
                rip->AddRAM(new ROMBanker(nextRom, triggerAddress, triggerMask, triggerValue, onMask, onValue));

            parseSuccess = TRUE;
        }
        else if ((nextToken = strstr(nextLine, "RAM")) != NULL) {
            //TODO: check to make sure we don't exceed the maximum number of rams for a Rip

            //parse ram width
            if ((nextToken = strrchr(nextLine, ':')) == NULL)
                continue;
            UINT8 ramBitWidth = (UINT8)atoi(nextToken+1);

            //parse ram size
            *nextToken = NULL;
            if ((nextToken = strrchr(nextLine, ':')) == NULL)
                continue;
            UINT16 ramSize = (UINT16)strtol(nextToken+1, NULL, 16);

            //parse ram address
            *nextToken = NULL;
            if ((nextToken = strrchr(nextLine, ':')) == NULL)
                continue;
            UINT16 ramAddress = (UINT16)strtol(nextToken+1, NULL, 16);

            if (rip == NULL)
                rip = new Rip(ID_SYSTEM_INTELLIVISION);
            rip->AddRAM(new RAM(ramSize, ramAddress, 0xFFFF, 0xFFFF, ramBitWidth));
            parseSuccess = TRUE;
        }
        else if ((nextToken = strstr(nextLine, "Peripheral")) != NULL) {
            //TODO: check to make sure we don't exceed the maximum number of peripherals for a Rip

            if ((nextToken = strrchr(nextLine, ':')) == NULL)
                continue;

            PeripheralCompatibility pc;
            if (strcmpi(nextToken+1, "required") != 0)
                pc = PERIPH_REQUIRED;
            else if (strcmpi(nextToken+1, "optional") != 0)
                pc = PERIPH_OPTIONAL;
            else
                continue;

            *nextToken = NULL;
            if ((nextToken = strrchr(nextLine, ':')) == NULL)
                continue;

            if (rip == NULL)
                rip = new Rip(ID_SYSTEM_INTELLIVISION);
            rip->AddPeripheralUsage(nextToken+1, pc);
            parseSuccess = TRUE;
        }
    }
    fclose(cfgFile);
 
    if (rip != NULL && !parseSuccess) {
        delete rip;
        rip = NULL;
    }
#endif
    return rip;    
}

Rip* Rip::LoadRom(const CHAR* filename)
{
    FILE* infile = fopen(filename, "rb");
    if (infile == NULL) {
        return NULL;
    }

    //read the magic byte (should always be $A8 or $41)
    int read = fgetc(infile);
    if ((read != 0xA8) && (read != 0x41))
    {
        fclose(infile);
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
        UINT16* romImage = new UINT16[size];
        int j;
        for (j = 0; j < size; j++) {
            int nextbyte = fgetc(infile) << 8;
            nextbyte |= fgetc(infile);
            romImage[j] = (UINT16)nextbyte;
        }
        rip->AddROM(new ROM("Cartridge ROM", romImage, 2, size, start, 0xFFFF));
        delete[] romImage;

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
                CHAR* title = new char[length*sizeof(char)];
                for (i = 0; i < length; i++) {
                    read = fgetc(infile);
                    title[i] = (char)read;
                }
                crc16 = (fgetc(infile) << 8) | fgetc(infile);
                rip->SetName(title);
                delete[] title;
            }
            break;
            case ROM_TAG_PUBLISHER:
            {
                CHAR* publisher = new char[length*sizeof(char)];
                for (i = 0; i < length; i++) {
                    read = fgetc(infile);
                    publisher[i] = (char)read;
                }
                crc16 = (fgetc(infile) << 8) | fgetc(infile);
                rip->SetProducer(publisher);

                delete[] publisher;
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

    extern bool bUseJLP;
    extern bool bForceIvoice;
    
    // See if we have any special overrides...
    const struct SpecialRomDatabase_t *db_entry = FindRomDatabaseEntry(rip->crc); // Try to find the CRC in our internal database...    
    if (db_entry != NULL)
    {
        if (db_entry->bJLP) bUseJLP=true;       
        if (db_entry->bIntellivoice) bForceIvoice=true;       
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
    if (bForceIvoice) rip->AddPeripheralUsage("Intellivoice", PERIPH_REQUIRED);

    return rip;
}



