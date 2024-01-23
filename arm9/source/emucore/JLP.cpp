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
#include <stdio.h>
#include <fat.h>
#include <dirent.h>
#include <unistd.h>
#include "JLP.h"
#include "RAM.h"
#include "Rip.h"
#include "../config.h"
#include "../nintv-ds.h"

extern Rip      *currentRip;

JLP::JLP()
: RAM(JLP_RAM_SIZE, JLP_RAM_ADDRESS, 0xFFFF, 0xFFFF)
{}

void JLP::reset()
{
    enabled = TRUE;
    for (UINT16 i = 0; i < JLP_RAM_SIZE; i++)
        jlp_ram[i] = 0xFFFF;
    jlp_ram[JLP_RAM_SIZE-1] = 0;                    /* The last byte of jlp RAM reads back as 0 */
    
    jlp_ram[0x23] = 0;                              /* First valid flash row number */
    jlp_ram[0x24] = JLP_NUM_ROWS;                   /* Last  valid flash row number */
    jlp_ram[0x2D] = 0;                              /* Command regs read as 0   */
    jlp_ram[0x2E] = 0;                              /* Command regs read as 0   */
    jlp_ram[0x2F] = 0;                              /* Command regs read as 0   */
    
    flash_read = 1;             // Force flash to read...
    flash_write_time = 0;       // And reset the time to write...
}

// If the JLP flash needs to be written, we write it to the backing file. We do it this way so that quick-succession writes
// to the flash do not force a write to the backing file which is slow and wasteful... so we have a 2 second backing timer.
void JLP::tick_one_second(void)
{
    if (flash_write_time > 0)
    {
        if (--flash_write_time == 0)
        {
            WriteFlashFile();
        }
    }
}

UINT16 JLP::peek(UINT16 location)
{
    if (location == 0x9FFE) {return (UINT16)random();}
    return jlp_ram[location&0x1FFF];
}

UINT32 JLP::crc16(UINT16 data, UINT16 crc)
{
    crc ^= data;

    for (int i = 0; i < 16; i++)
    {
        crc = (crc >> 1) ^ (crc & 1 ? JLP_CRC_POLY : 0);
    }

    return crc;
}


char flash_filename[128];
void JLP::GetFlashFilename(void)
{
    if (myGlobalConfig.save_dir == 1) 
    {
        DIR* dir = opendir("/roms/sav");    // See if directory exists
        if (dir) closedir(dir);             // Directory exists. All good.
        else mkdir("/roms/sav", 0777);      // Doesn't exist - make it...
        strcpy(flash_filename, "/roms/sav/");
    }
    else if (myGlobalConfig.save_dir == 2)
    {
        DIR* dir = opendir("/roms/intv/sav");   // See if directory exists
        if (dir) closedir(dir);                 // Directory exists. All good.
        else mkdir("/roms/intv/sav", 0777);     // Doesn't exist - make it...
        strcpy(flash_filename, "/roms/intv/sav/");
    }
    else if (myGlobalConfig.save_dir == 3)
    {
        DIR* dir = opendir("/data/sav");    // See if directory exists
        if (dir) closedir(dir);             // Directory exists. All good.
        else mkdir("/data/sav", 0777);      // Doesn't exist - make it...
        strcpy(flash_filename, "/data/sav/");
    }
    else strcpy(flash_filename, "");
    
    strcat(flash_filename, currentRip->GetFileName());
    flash_filename[strlen(flash_filename)-4] = 0;
    strcat(flash_filename, ".jlp");
}

void JLP::ReadFlashFile(void)
{
    FILE *fp;
    
    memset(jlp_flash, 0xFF, JLP_FLASH_SIZE);    // Ensure buffer is reset to FF before we read the file...
    GetFlashFilename();
    fp = fopen(flash_filename, "rb");
    if (fp != NULL)
    {
        fread(jlp_flash, 1, JLP_FLASH_SIZE, fp);
        fclose(fp);
    }
    flash_read = 0;
}

void JLP::WriteFlashFile(void)
{
    FILE *fp;
    
    dsPrintValue(hud_x,hud_y,0, (char*)"JLP FLASH");
    GetFlashFilename();
    fp = fopen(flash_filename, "wb");
    if (fp != NULL)
    {
        fwrite(jlp_flash, 1, JLP_FLASH_SIZE, fp);
        fclose(fp);
    }
    dsPrintValue(hud_x,hud_y,0,(char*)"         ");
}

void JLP::ScheduleWriteFlashFile(void)
{
    flash_write_time = 2;
}

void JLP::RamToFlash(void)
{
    UINT32 addr = jlp_ram[(0x8025&readAddressMask) - this->location] - JLP_RAM_ADDRESS;
    UINT32 row  = (jlp_ram[(0x8026&readAddressMask) - this->location] - jlp_ram[(0x8023&readAddressMask) - this->location]) * JLP_NUM_BYTES_PER_ROW;
    int i, a;

    if (jlp_ram[(0x8026&readAddressMask) - this->location] > JLP_NUM_ROWS) return;
    
    if (flash_read)  ReadFlashFile();
    
    /* Refuse to write if row isn't empty. */
    for (i = 0; i < JLP_NUM_BYTES_PER_ROW; i++)
        if (jlp_flash[row + i] != 0xFF)
        {
            return;
        }

    /* Explicitly copy little-endian regardless of machine endian */
    for (i = a = 0; i < JLP_NUM_BYTES_PER_ROW; i += 2, a++)
    {
        jlp_flash[row + i + 0] = jlp_ram[addr + a] & 0xFF;
        jlp_flash[row + i + 1] = jlp_ram[addr + a] >> 8;
    }
    ScheduleWriteFlashFile();
}

void JLP::FlashToRam(void)
{
    UINT32 addr = jlp_ram[(0x8025&readAddressMask) - this->location] - JLP_RAM_ADDRESS;
    UINT32 row  = (jlp_ram[(0x8026&readAddressMask) - this->location] - jlp_ram[(0x8023&readAddressMask) - this->location]) * JLP_NUM_BYTES_PER_ROW;
    int i, a;
    
    if (jlp_ram[(0x8026&readAddressMask) - this->location] > JLP_NUM_ROWS) return;

    // Check to see if we need to read in the flash file...
    if (flash_read)  ReadFlashFile();
    
    /* Explicitly copy little-endian regardless of machine endian */
    for (i = a = 0; i < JLP_NUM_BYTES_PER_ROW; i += 2, a++)
    {
        UINT16 lo = jlp_flash[row + i + 0];
        UINT16 hi = jlp_flash[row + i + 1];
        jlp_ram[addr + a] = lo | (hi << 8);
    }    
}

void JLP::EraseSector(void)
{
    UINT32 row = ((jlp_ram[(0x8026&readAddressMask) - this->location] - jlp_ram[(0x8023&readAddressMask) - this->location]) & -8) * JLP_NUM_BYTES_PER_ROW;
    if (jlp_ram[(0x8026&readAddressMask) - this->location] > JLP_NUM_ROWS) return;
    if (flash_read)  ReadFlashFile();
    memset((void *)&jlp_flash[row], 0xFF, JLP_NUM_BYTES_PER_ROW * 8);
    ScheduleWriteFlashFile();
}



void JLP::poke(UINT16 location, UINT16 value)
{
    UINT32 prod=0, quot=0, rem=0;
    
    if ((location >= 0x8040))
    {
        jlp_ram[location & 0x1FFF] = value;
    }
    
    /* -------------------------------------------------------------------- */
    /*  Check for mult/div writes                                           */
    /*      $9F8(0,1):  s16($9F80) x s16($9F81) -> s32($9F8F:$9F8E)         */
    /*      $9F8(2,3):  s16($9F82) x u16($9F83) -> s32($9F8F:$9F8E)         */
    /*      $9F8(4,5):  u16($9F84) x s16($9F85) -> s32($9F8F:$9F8E)         */
    /*      $9F8(6,7):  u16($9F86) x u16($9F87) -> u32($9F8F:$9F8E)         */
    /*      $9F8(8,9):  s16($9F88) / s16($9F89) -> quot($9F8E), rem($9F8F)  */
    /*      $9F8(A,B):  u16($9F8A) / u16($9F8B) -> quot($9F8E), rem($9F8F)  */
    /* -------------------------------------------------------------------- */    
    switch(location)
    {
        case 0x9F80:
        case 0x9F81:
            prod = (UINT32)  ((INT32)(INT16)jlp_ram[(0x9F80&readAddressMask) - this->location] * (INT32)(INT16)jlp_ram[(0x9F81&readAddressMask) - this->location]);
            jlp_ram[(0x9F8E&writeAddressMask)-this->location] = prod & 0xFFFF;
            jlp_ram[(0x9F8F&writeAddressMask)-this->location] = prod >> 16;                                                                                                      
            break;    

        case 0x9F82:
        case 0x9F83:
            prod = (UINT32)  ((INT32)(INT16)jlp_ram[(0x9F82&readAddressMask) - this->location] * (INT32)(UINT16)jlp_ram[(0x9F83&readAddressMask) - this->location]);
            jlp_ram[(0x9F8E&writeAddressMask)-this->location] = prod & 0xFFFF;
            jlp_ram[(0x9F8F&writeAddressMask)-this->location] = prod >> 16;                                                                                                      
            break;    
            
        case 0x9F84:
        case 0x9F85:
            prod = (UINT32)  ((INT32)(UINT16)jlp_ram[(0x9F84&readAddressMask) - this->location] * (INT32)(INT16)jlp_ram[(0x9F85&readAddressMask) - this->location]);
            jlp_ram[(0x9F8E&writeAddressMask)-this->location] = prod & 0xFFFF;
            jlp_ram[(0x9F8F&writeAddressMask)-this->location] = prod >> 16;                                                                                                      
            break;    

        case 0x9F86:
        case 0x9F87:
            prod = (UINT32)  ((UINT32)(UINT16)jlp_ram[(0x9F86&readAddressMask) - this->location] * (UINT32)(UINT16)jlp_ram[(0x9F87&readAddressMask) - this->location]);
            jlp_ram[(0x9F8E&writeAddressMask)-this->location] = prod & 0xFFFF;
            jlp_ram[(0x9F8F&writeAddressMask)-this->location] = prod >> 16;                                                                                                      
            break;    
            
        case 0x9F88:
        case 0x9F89:
            if (jlp_ram[(0x9F89&readAddressMask) - this->location] != 0)
            {
                quot = (UINT32)  ((INT32)(INT16)jlp_ram[(0x9F88&readAddressMask) - this->location] / (INT32)(INT16)jlp_ram[(0x9F89&readAddressMask) - this->location]);
                rem  = (UINT32)  ((INT32)(INT16)jlp_ram[(0x9F88&readAddressMask) - this->location] % (INT32)(INT16)jlp_ram[(0x9F89&readAddressMask) - this->location]);
            }
            jlp_ram[(0x9F8E&writeAddressMask)-this->location] = quot;
            jlp_ram[(0x9F8F&writeAddressMask)-this->location] = rem;  
            break;    
            
        case 0x9F8A:
        case 0x9F8B:
            if (jlp_ram[(0x9F8B&readAddressMask) - this->location] != 0)
            {
                quot = (UINT32)  ((UINT32)(UINT16)jlp_ram[(0x9F8A&readAddressMask) - this->location] / (UINT32)(UINT16)jlp_ram[(0x9F8B&readAddressMask) - this->location]);
                rem  = (UINT32)  ((UINT32)(UINT16)jlp_ram[(0x9F8A&readAddressMask) - this->location] % (UINT32)(UINT16)jlp_ram[(0x9F8B&readAddressMask) - this->location]);
            }
            jlp_ram[(0x9F8E&writeAddressMask)-this->location] = quot;
            jlp_ram[(0x9F8F&writeAddressMask)-this->location] = rem;  
            break;    

        //CRC-16 accelerator at $9FFC / $9FFD (poly 0xAD52, right-shifting)            
        case 0x9FFC:
            jlp_ram[(0x9FFD&writeAddressMask)-this->location] = crc16(value, (UINT16)jlp_ram[(0x9FFD&readAddressMask) - this->location]);
            break;
            
        case 0x9FFD:
            jlp_ram[(0x9FFD&writeAddressMask)-this->location] = value;
            break;
            
        /* -------------------------------------------------------------------- */
        /*  Check for Save Game Arch V2 writes                                  */
        /*                                                                      */
        /*      $8025   JLP RAM address to operate on                           */
        /*      $8026   Flash row to operate on                                 */
        /*                                                                      */
        /*      $802D   Copy JLP RAM to flash row (unlock: $C0DE)               */
        /*      $802E   Copy flash row to JLP RAM (unlock: $DEC0)               */
        /*      $802F   Erase flash sector        (unlock: $BEEF)               */
        /* -------------------------------------------------------------------- */
        case  0x8025:
        case  0x8026:
            jlp_ram[(location & writeAddressMask)-this->location] = value;
            break;

        case 0x802D:
            if (value == 0xC0DE) RamToFlash();
            break;

        case 0x802E:
            if (value == 0xDEC0) FlashToRam();
            break;
            
        case 0x802F:
            if (value == 0xBEEF) EraseSector();
            break;
    }
    
}


void JLP::getState(JLPState *state)
{
    for (int i=0; i<JLP_RAM_SIZE; i++)   state->jlp_ram[i] = jlp_ram[i];
}

void JLP::setState(JLPState *state)
{
    for (int i=0; i<JLP_RAM_SIZE; i++)   jlp_ram[i] = state->jlp_ram[i];
    flash_read = 1;     // Force flash to read...
}
