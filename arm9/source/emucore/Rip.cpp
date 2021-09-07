
#include <stdlib.h>
#include <stdio.h>

#include "Rip.h"
#include "RAM.h"
#include "ROM.h"
#include "ROMBanker.h"
#include "CRC32.h"

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
}

void Rip::SetName(const CHAR* n)
{
    if (this->peripheralName)
        delete[] peripheralName;

    peripheralName = new CHAR[strlen(n)+1];
    strcpy(peripheralName, n);
    printf("RIP Peripheral Name [%s]\n", peripheralName);
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


Rip* Rip::LoadBin(const CHAR* filename, const CHAR* configFile)
{
    //determine the crc of the designated file
    UINT32 crc = CRC32::getCrc(filename);
    Rip* rip = LoadCartridgeConfiguration(configFile, crc);
    if (rip == NULL)
        return NULL;
    else printf("Configuration [%s] loaded successfully\n", configFile);

    //load the data into the rip
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        delete rip;
        return NULL;
    } else printf("File [%s] read into memory successfully\n", filename);

    //obtain the file size
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    printf("The file size is [%d] bytes\n", (int)size);

    //load in the file image
    UINT8* image = new UINT8[size];
    UINT32 count = 0;
    while (count < size)
        image[count++] = (UINT8)fgetc(file);
    fclose(file);

    printf("Parsing file into RIP format\n");
    //parse the file image into the rip
    UINT32 offset = 0;
    UINT16 romCount = rip->GetROMCount();
    for (UINT16 i = 0; i < romCount; i++) 
    {
        if (offset >= size) 
        {
            //something is wrong, the file we're trying to load isn't large enough
            //getting here indicates a likely incorrect memory map in knowncarts.cfg
            delete[] image;
            delete rip;
            return NULL;
        }
        ROM* nextRom = rip->GetROM(i);
        printf("Loading ROM segment [%s %d %d]\n", nextRom->getName(), nextRom->getByteWidth(), nextRom->getReadSize());
        nextRom->load(image+offset);
        offset += nextRom->getByteWidth() * nextRom->getReadSize();
    }

    printf("Done parsing...\n");
    delete[] image;

    rip->SetFileName(filename);
    rip->crc = CRC32::getCrc(filename);

    printf("RIP loaded!\n");
    return rip;
}

Rip* Rip::LoadCartridgeConfiguration(const CHAR* configFile, UINT32 crc)
{
    CHAR scrc[9];
    sprintf(scrc, "%08X", crc);

    //find the config that matches this crc
    FILE* cfgFile = fopen(configFile, "r");
    if (cfgFile == NULL)
        return NULL;

    Rip* rip = NULL;
    BOOL parseSuccess = FALSE;
    CHAR nextLine[256];
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
            rip->AddRAM(new RAM(ramSize, ramAddress, ramBitWidth));
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

    return rip;
}

Rip* Rip::LoadRom(const CHAR* filename)
{
    FILE* infile = fopen(filename, "rb");
    if (infile == NULL) {
        return NULL;
    }

    //read the magic byte (should always be $A8)
    int read = fgetc(infile);
    if (read != 0xA8) {
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

    while ((read = fgetc(infile)) != -1) {
        int length = (read & 0x3F);
        read = (read & 0xC) >> 6;
        for (i = 0; i < read; i++)
            length = (length | (fgetc(infile) << ((8*i)+6)));

        int type = fgetc(infile);
        int crc16;
        switch (type) {
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
                for (i = 0; i < length-1; i++) {
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

    return rip;
}

Rip* Rip::LoadRip(const CHAR* filename)
{
    FILE* file = fopen(filename, "rb");
    if (file == NULL)
        return NULL;

    //first check the magic number
    unsigned int nextUINT32 = freadUINT32(file);
    if (nextUINT32 != 0x3F383A34) {
        fclose(file);
        return NULL;
    }

    nextUINT32 = freadUINT32(file);
    if (nextUINT32 != 0x7651B5DA) {
        fclose(file);
        return NULL;
    }

    //the magic number is good.  this is definitely a RIP file
    //start reading the records
    Rip* rip = NULL;
    unsigned int nextRecordID = freadUINT32(file);
    while (!feof(file)) {
        unsigned int nextRecordSize = freadUINT32(file);
        switch (nextRecordID) {
            case ID_HEADER_RECORD:
            {
                freadUINT16(file);  //major file format version
                freadUINT16(file);  //minor file format version
                UINT32 targetSystemID = freadUINT32(file);  //target system
                rip = new Rip(targetSystemID);
                break;
            }
/*
            case ID_BIOS_COMPAT_RECORD:
                tmp1 = ripFile->biosCount;
                ripFile->biosCompat[tmp1] =
                        malloc(sizeof(BiosCompatRecord));
                ripFile->biosCompat[tmp1]->periphNum = fgetc(file);
                ripFile->biosCompat[tmp1]->biosTypeNum = fgetc(file);
                ripFile->biosCompat[tmp1]->biosNum = fgetc(file);
                ripFile->biosCompat[tmp1]->compatibility = fgetc(file);
                if (ripFile->biosCompat[tmp1]->compatibility > 2)
                    ripFile->biosCompat[tmp1]->compatibility = 2;
                ripFile->biosCount++;
                break;
*/
            case ID_PERIPH_COMPAT_RECORD:
            {
                if (!rip)
                    return NULL;
                UINT32 periphID = freadUINT32(file);
                PeripheralCompatibility usage =  (PeripheralCompatibility)(fgetc(file) & 0x03);
                rip->AddPeripheralUsage((periphID == ID_PERIPH_ECS ? "ECS" : "Intellivoice"), usage);
                break;
            }
            case ID_NAME_RECORD:
            {
                if (!rip)
                    return NULL;
                CHAR name[MAX_STRING_LENGTH];
                freadString(file, name, MAX_STRING_LENGTH);
                rip->SetName(name);
                break;
            }
            case ID_PRODUCER_RECORD:
            {
                if (!rip)
                    return NULL;
                CHAR producer[MAX_STRING_LENGTH];
                freadString(file, producer, MAX_STRING_LENGTH);
                rip->SetProducer(producer);
                break;
            }
            case ID_YEAR_RECORD:
            {
                if (!rip)
                    return NULL;
                CHAR year[MAX_STRING_LENGTH];
                freadString(file, year, MAX_STRING_LENGTH);
                rip->SetProducer(year);
                break;
            }
            case ID_RAM_RECORD:
            {
                if (!rip)
                    return NULL;

                 //TODO: handle when we exceed maximum memory units

                UINT8 flags = (UINT8)fgetc(file);
                BOOL partialReads = !!(flags & 0x80);
                BOOL partialWrites = !!(flags & 0x40);
                BOOL banked = !!(flags & 0x20);
                UINT8 addressByteWidth = (flags & 0x0F)+1;

                flags = (UINT8)fgetc(file);
                UINT8 dataBitWidth = (flags & 0x7F)+1;
                UINT16 address = (UINT16)freadInt(file, addressByteWidth);
                UINT16 size = (UINT16)freadInt(file, addressByteWidth);
                //RAM reset value unused at this point, so skip it
                freadInt(file, (dataBitWidth+7)/8);

                UINT16 readMask = 0xFFFF; 
                if (partialReads)
                    readMask = (UINT16)freadInt(file, addressByteWidth);

                UINT16 writeMask = 0xFFFF; 
                if (partialWrites)
                    writeMask = (UINT16)freadInt(file, addressByteWidth);

                if (banked) {
                    //banking descriptors not yet supported, so just need to skip 'em
                    UINT32 bankingDescriptorCount = freadUINT32(file);
                    for (UINT32 k = 0; k < bankingDescriptorCount; k++) {
                        freadInt(file, addressByteWidth); //target address width
                        freadInt(file, addressByteWidth); //write decoding mask
                        freadInt(file, (dataBitWidth+7)/8); //reset value
                        UINT32 dataCount = freadUINT32(file);
                        for (UINT32 l = 0; l < dataCount; l++)
                            freadInt(file, (dataBitWidth+7)/8); //data banking match values
                    }
                }

                rip->AddRAM(new RAM(size, address, readMask, writeMask, dataBitWidth));
            }
            case ID_ROM_RECORD:
            {
                if (!rip)
                    return NULL;

                 //TODO: handle when we exceed maximum memory units

                UINT8 flags = (UINT8)fgetc(file);
                BOOL partialReads = !!(flags & 0x80);
                BOOL compressed = !!(flags & 0x40);
                BOOL banked = !!(flags & 0x20);
                UINT8 addressByteWidth = (flags & 0x0F)+1;

                flags = (UINT8)fgetc(file);
                UINT8 dataBitWidth = (flags & 0x7F)+1;
                UINT16 address = (UINT16)freadInt(file, addressByteWidth);

                UINT32 arraySize = freadUINT32(file);
                UINT8* romImage = NULL;
                if (compressed) {
                    //TODO: support zlib compressed rom images
                    for (UINT32 k = 0; k < arraySize; k++)
                        fgetc(file);
                    arraySize = 0;
                    romImage = new UINT8[0];
                }
                else {
                    UINT32 dataByteWidth = (dataBitWidth+7)/8;
                    romImage = new UINT8[arraySize*dataByteWidth];
                    for (UINT32 k = 0; k < arraySize; k++) {
                        for (UINT8 l = 0; l < dataByteWidth; l++)
                            romImage[(k*dataByteWidth)+(dataByteWidth-l-1)] = (UINT8)fgetc(file);
                    }
                }

                UINT16 readMask = 0xFFFF; 
                if (partialReads)
                    readMask = (UINT16)freadInt(file, addressByteWidth);

                if (banked) {
                    //banking descriptors not yet supported, so just need to skip 'em
                    UINT32 bankingDescriptorCount = freadUINT32(file);
                    for (UINT32 k = 0; k < bankingDescriptorCount; k++) {
                        freadInt(file, addressByteWidth); //target address width
                        freadInt(file, addressByteWidth); //write decoding mask
                        freadInt(file, (dataBitWidth+7)/8); //reset value
                        UINT32 dataCount = freadUINT32(file);
                        for (UINT32 l = 0; l < dataCount; l++)
                            freadInt(file, (dataBitWidth+7)/8); //data banking match values
                    }
                }

                rip->AddROM(new ROM("Cartridge ROM", (void*)romImage, (dataBitWidth+7)/8, (UINT16)arraySize, address, readMask));
                delete[] romImage;

                break;
            }
            default:
            {
                //unknown record; just skip it
                for (UINT32 i = 0; i < nextRecordSize; i++)
                    fgetc(file);
            }
        }
        nextRecordID = freadUINT32(file);
    }
    fclose(file);

    rip->SetFileName(filename);
    rip->crc = CRC32::getCrc(filename);

    return rip;
}


BOOL Rip::SaveRip(const CHAR* filename)
{
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Error: Unable to create file %s\n", filename);
        return 1;
    }

    //write the 64-bit magic RIP XBF number
    fwriteUINT64(file, RIP_MAGIC_NUMBER);

    //write the header record
    fwriteUINT32(file, ID_HEADER_RECORD);
    //write the header record size in bytes
    fwriteUINT32(file, 9);
    //write the major RIP revision number
    fwriteUINT16(file, RIP_MAJOR_REVISION);
    //write the minor RIP revision number
    fwriteUINT16(file, RIP_MINOR_REVISION);
    //write the system id
    fwriteUINT32(file, targetSystemID);

    //write the name record
    fwriteUINT32(file, ID_NAME_RECORD);
    fwriteUINT32(file, (UINT32)(4+strlen(GetName())));
    fwriteString(file, GetName());

    //write the year record
    fwriteUINT32(file, ID_YEAR_RECORD);
    fwriteUINT32(file, (UINT32)(4+strlen(year)));
    fwriteString(file, year);

    //write the producer record
    fwriteUINT32(file, ID_PRODUCER_RECORD);
    fwriteUINT32(file, (UINT32)(4+strlen(producer)));
    fwriteString(file, producer);

    //write the peripheral compatibility records
    for (UINT16 i = 0; i < peripheralCount; i++) {
        fwriteUINT32(file, ID_PERIPH_COMPAT_RECORD);
        fwriteUINT32(file, 5);
        //these records are hard-coded for Intellivision now, will have to work out something more
        //flexible when Atari 5200 support is re-added in the future
        fwriteUINT32(file, (strcmpi(peripheralNames[i], "Intellivoice") == 0) ? ID_PERIPH_INTELLIVOICE : ID_PERIPH_ECS);
        fputc(peripheralUsages[i], file);
    }

    //write the RAM records
    UINT16 ramCount = GetRAMCount();
    for (UINT16 i = 0; i < ramCount; i++) {
        RAM* nextMem = GetRAM(i);
        fwriteUINT32(file, ID_RAM_RECORD);
        UINT16 readMask = nextMem->getReadAddressMask();
        UINT16 writeMask = nextMem->getWriteAddressMask();
        UINT16 byteWidth = (nextMem->getBitWidth()+7)/8;

        //calculate the size of the RAM record
        int recordSize = 1 +                     //flags
                1 +                              //more flags
                2 +                              //address
                2 +                              //size
                byteWidth +                      //reset value
                (readMask != 0xFFFF ? 2 : 0) +   //read mask
                (writeMask != 0xFFFF ? 2 : 0) +  //write mask
                0;                               //banking not supported just yet
        fwriteUINT32(file, recordSize);

        //flags #1
        fputc(((readMask != 0xFFFF) ? 0x80 : 0) |
                ((writeMask != 0xFFFF) ? 0x40 : 0) |
                1, file);

        //flags #2
        fputc(nextMem->getBitWidth()-1, file);

        //address
        fwriteUINT16(file, nextMem->getReadAddress());

        //size
        fwriteUINT16(file, nextMem->getReadSize());

        //reset value, always zero
        for (UINT16 k = 0; k < byteWidth; k++)
            fputc(0, file);
        
        //read mask
        if (readMask != 0xFFFF)
            fwriteUINT16(file, readMask);

        //write mask
        if (writeMask != 0xFFFF)
            fwriteUINT16(file, writeMask);
    }

    //write the ROM records
    UINT16 romCount = GetROMCount();
    for (UINT16 i = 0; i < romCount; i++) {
        ROM* nextMem = GetROM(i);
        fwriteUINT32(file, ID_ROM_RECORD);

        //calculate the size of the ROM record
        UINT16 readMask = nextMem->getReadAddressMask();
        UINT16 byteWidth = nextMem->getByteWidth();
        UINT16 memSize = nextMem->getReadSize();
        int recordSize = 1 +                     //flags
                1 +                              //more flags
                2 +                              //address
                4 + (byteWidth*memSize) +        //ROM image
                (readMask != 0xFFFF ? 2 : 0) +   //read mask
                0;                               //banking not supported just yet
        fwriteUINT32(file, recordSize);

        //flags #1
        fputc(((readMask != 0xFFFF) ? 0x80 : 0) |
                1, file);

        //flags #2
        fputc((byteWidth*8)-1, file);

        //address
        UINT16 address = nextMem->getReadAddress();
        fwriteUINT16(file, address);

        //ROM image
        fwriteUINT32(file, memSize);
        for (UINT16 j = 0; j < memSize; j++) {
            switch (byteWidth) {
                case 1:
                    fputc(nextMem->peek(address+j), file);
                    break;
                case 2:
                    fwriteUINT16(file, nextMem->peek(address+j));
                    break;
                case 4:
                    fwriteUINT32(file, nextMem->peek(address+j));
                    break;
                default:
                    for (UINT8 k = 0; k < byteWidth; k++)
                        fputc(0, file);
            }
        }

        //read mask
        if (readMask != 0xFFFF)
            fwriteUINT16(file, readMask);
    }
    
    fclose(file);

    return 0;
}
