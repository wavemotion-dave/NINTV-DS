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
#include "nintv-ds.h"
#include "savestate.h"
#include "bgMenu-Green.h"
#include "bgMenu-White.h"
#include "config.h"
#include "printf.h"

extern Emulator *currentEmu;
extern Rip      *currentRip;
extern UINT16 global_frames;

#define CURRENT_SAVE_FILE_VER   0x000C

// ------------------------------------------------------
// We allow up to 3 saves per game. More than enough.
// ------------------------------------------------------
struct 
{
    UINT16                  saveFileVersion;
    UINT8                   bSlotUsed[3];
    char                    slotTimestamp[3][20];
    struct _stateStruct     slot[3];
} saveState;

// Only for the games that require this... it's larger than all of the other saveState stuff above...
JLPState jlpState[3];
SlowRAM16State slowRAM16State[3];
SlowRAM8State slowRAM8State[3];

extern UINT16 gLastBankers[];

char savefilename[128];

// ----------------------------------------------------------------------------------------------------
// Write a new save file to disc. We stamp out the slot that is being written using the NDS timestamp.
// ----------------------------------------------------------------------------------------------------
BOOL do_save(const CHAR* filename, UINT8 slot)
{
    BOOL didSave = FALSE;
    time_t unixTime = time(NULL);
    struct tm* timeStruct = gmtime((const time_t *)&unixTime);

    saveState.saveFileVersion = CURRENT_SAVE_FILE_VER; 
    saveState.bSlotUsed[slot] = TRUE;
    sprintf(saveState.slotTimestamp[slot], "%02d-%02d-%04d %02d:%02d:%02d", timeStruct->tm_mday, timeStruct->tm_mon+1, timeStruct->tm_year+1900, timeStruct->tm_hour, timeStruct->tm_min, timeStruct->tm_sec);

    // Ask the emulator to save it's state...
    currentEmu->SaveState(&saveState.slot[slot]);
    saveState.slot[slot].global_frames = global_frames;
    saveState.slot[slot].emu_frames = emu_frames;
    
    // Save the 16 possible ROM Bankers so we can put the system back to the right state
    for (UINT8 i=0; i<16; i++)
    {
        saveState.slot[slot].lastBankers[i] = gLastBankers[i];
    }

    // Save off the ECS RAM. Most games don't use it, but it's only 2K and we have dedicated space for it...
    for (int i=0; i<ECS_RAM_SIZE; i++) saveState.slot[slot].ecsRAM[i] = (UINT8)ecs_ram8[i];
    
    // Only a few games utilize JLP RAM...
    if (currentRip->JLP16Bit) currentRip->JLP16Bit->getState(&jlpState[slot]);
    
    // And even fewer games utilize extra RAM... this significantly increases the size of the save file
    if (slow_ram16_idx != 0) memcpy(slowRAM16State[slot].image, slow_ram16, 0x4000*sizeof(UINT16));
    if (slow_ram8_idx != 0)  memcpy(slowRAM8State[slot].image, slow_ram8, 0x1800*sizeof(UINT16));   // Technically we could spill 2K into the "ECS 8-bit" area but that's already preserved above.

    // Write the entire save states as a single file... overwrite if it exists.
    FILE* file = fopen(filename, "wb+");

    if (file != NULL) 
    {
        fwrite(&saveState, 1, sizeof(saveState), file);
        if (currentRip->JLP16Bit) fwrite(&jlpState, 1, sizeof(jlpState), file);            // A few gaems utilize the JLP RAM
        if (slow_ram16_idx != 0)  fwrite(slowRAM16State, 1, sizeof(slowRAM16State), file); // A tiny fraction of games need even MORE ram... we have a large "slow" buffer for those...
        if (slow_ram8_idx != 0)   fwrite(slowRAM8State,  1, sizeof(slowRAM8State), file);  // A tiny fraction of games need even MORE ram... we have a large "slow" buffer for those...
        didSave = TRUE;
        fclose(file);
    } 
    
    return didSave;
}

// ----------------------------------------------------------------------------------
// Load a save state from a slot... caller should check that the slot is populated.
// ----------------------------------------------------------------------------------
BOOL do_load(const CHAR* filename, UINT8 slot)
{
    BOOL didLoadState = FALSE;

    memset(&saveState, 0x00, sizeof(saveState));
    FILE* file = fopen(filename, "rb");

    if (file != NULL) 
    {
        int size = fread(&saveState, 1, sizeof(saveState), file);
        
        if ((size != sizeof(saveState)) || (saveState.saveFileVersion != CURRENT_SAVE_FILE_VER))
        {
            memset(&saveState, 0x00, sizeof(saveState));
            memset(&jlpState, 0x00, sizeof(jlpState));
        }
        else
        {
            if (currentRip->JLP16Bit) fread(&jlpState, 1, sizeof(jlpState), file);            // A few games utilize the JLP RAM
            if (slow_ram16_idx != 0) fread(slowRAM16State, 1, sizeof(slowRAM16State), file);  // A tiny fraction of games need even MORE ram... we have a large "slow" buffer for those...
            if (slow_ram8_idx != 0)  fread(slowRAM8State,  1, sizeof(slowRAM8State), file);   // A tiny fraction of games need even MORE ram... we have a large "slow" buffer for those...
            
            // Ask the emulator to restore it's state...
            currentEmu->LoadState(&saveState.slot[slot]);
            
            //Restore ECS RAM. Most games don't use it, but it's only 2K and we have dedicated space for it...
            for (int i=0; i<ECS_RAM_SIZE; i++) ecs_ram8[i] = saveState.slot[slot].ecsRAM[i];
            
            if (currentRip->JLP16Bit) currentRip->JLP16Bit->setState(&jlpState[slot]);
            if (slow_ram16_idx != 0) memcpy(slow_ram16, slowRAM16State[slot].image, 0x4000*sizeof(UINT16));
            if (slow_ram8_idx != 0) memcpy(slow_ram8, slowRAM8State[slot].image, 0x1800*sizeof(UINT16));  // Technically we could spill 2K into the "ECS 8-bit" area but that's already restored above.
            
            global_frames = saveState.slot[slot].global_frames;
            emu_frames = saveState.slot[slot].emu_frames;

            // We need to run through all the last known banking writes and poke those back into the system
            for (UINT8 i=0; i<16; i++)
            {
                gLastBankers[i] = saveState.slot[slot].lastBankers[i];
                if (gLastBankers[i] != 0x0000)
                {
                    // ----------------------------------------------------------------------------------------------------
                    // Sometimes poking values back in at xFFF will trigger a write to the last WORD in the GRAM memory
                    // due to aliases. We want to make sure we don't disturb that when we restore state as the original 
                    // game programmer may have done this in a very careful way and here we're being very brute-force.
                    // ----------------------------------------------------------------------------------------------------
                    UINT16 save_gram = gram_image[GRAM_SIZE-1];
                    currentEmu->memoryBus.poke((i<<12)|0xFFF, gLastBankers[i]);
                    gram_image[GRAM_SIZE-1] = save_gram;
                }
            }
            // Refresh the fast memory now that we've got it all loaded in...
            currentEmu->RefreshFastMemory();
            
            if (myGlobalConfig.erase_saves)
            {
                remove(savefilename);
            }
            didLoadState = TRUE;
        }
        fclose(file);
    }

    return didLoadState;
}


// -------------------------------------------------------------------------
// Store the save state file - use global config to determine where to 
// place the .sav file (or, by default, it goes in the same directory 
// as the game .rom or .bin file that was loaded).
// -------------------------------------------------------------------------
void get_save_filename(void)
{
    if (myGlobalConfig.save_dir == 1) 
    {
        DIR* dir = opendir("/roms/sav");    // See if directory exists
        if (dir) closedir(dir);             // Directory exists. All good.
        else mkdir("/roms/sav", 0777);      // Doesn't exist - make it...
        strcpy(savefilename, "/roms/sav/");
    }
    else if (myGlobalConfig.save_dir == 2)
    {
        DIR* dir = opendir("/roms/intv/sav");   // See if directory exists
        if (dir) closedir(dir);                 // Directory exists. All good.
        else mkdir("/roms/intv/sav", 0777);     // Doesn't exist - make it...
        strcpy(savefilename, "/roms/intv/sav/");
    }
    else if (myGlobalConfig.save_dir == 3)
    {
        DIR* dir = opendir("/data/sav");    // See if directory exists
        if (dir) closedir(dir);             // Directory exists. All good.
        else mkdir("/data/sav", 0777);      // Doesn't exist - make it...
        strcpy(savefilename, "/data/sav/");
    }
    else strcpy(savefilename, "");
    
    strcat(savefilename, currentRip->GetFileName());
    savefilename[strlen(savefilename)-4] = 0;
    strcat(savefilename, ".sav");
}

// -----------------------------------------------------------------------------
// Read the save file into memory... we purosely keep this fairly small (less
// than 32k) for 2 reasons - it takes up valuable RAM and we would prefer that
// this take up less than 1 sector on the SD card (default sector size is 32k).
// -----------------------------------------------------------------------------
void just_read_save_file(void)
{
    if (currentRip != NULL)
    {
        get_save_filename();
        memset(&saveState, 0x00, sizeof(saveState));
        FILE* file = fopen(savefilename, "rb");

        if (file != NULL) 
        {
            int size = fread(&saveState, 1, sizeof(saveState), file);
            fclose(file);
            if ((size != sizeof(saveState)) || (saveState.saveFileVersion != CURRENT_SAVE_FILE_VER))
            {
                memset(&saveState, 0x00, sizeof(saveState));
            }
        }
    }
}


// -----------------------------------------------------------------------------
// Call into all the objects in the system to gather the save state information
// and then write that set of data out to the disc...
// -----------------------------------------------------------------------------
void state_save(UINT8 slot)
{
    if (currentRip != NULL)
    {
        get_save_filename();
        dsPrintValue(10,23,0, (char*)"STATE SAVED");
        do_save(savefilename, slot);
        WAITVBL;WAITVBL;WAITVBL;
        dsPrintValue(10,23,0, (char*)"           ");   
    }
}

// ------------------------------------------------------------------------------
// Pull back state information and call into all of the objects to restore
// the saved state and get us back to the save point... 
// ------------------------------------------------------------------------------
BOOL state_restore(UINT8 slot)
{
    if (currentRip != NULL)
    {
        if (saveState.bSlotUsed[slot] == TRUE)
        {
            get_save_filename();
            dsPrintValue(10,23,0, (char*)"STATE RESTORED");
            do_load(savefilename, slot);
            WAITVBL;WAITVBL;WAITVBL;WAITVBL;
            dsPrintValue(10,23,0, (char*)"              ");       
            return TRUE;
        }
    }
    return FALSE;
}


// -----------------------------------
// Wipe the save file from disc... 
// -----------------------------------
void clear_save_file(void)
{
    if (currentRip != NULL)
    {
        get_save_filename();
        remove(savefilename);
        memset(&saveState, 0x00, sizeof(saveState));
        WAITVBL;WAITVBL;WAITVBL;WAITVBL;
        dsPrintValue(10,23,0, (char*)"                ");       
    }
}


// ------------------------------------------------------------------
// A mini state machine / menu so the user can pick which of the
// three possible save slots to use for save / restore. They can
// also erase the save file if they choose...
// ------------------------------------------------------------------
#define SAVE_MENU_ITEMS 8
const char *savestate_menu[SAVE_MENU_ITEMS] = 
{
    "SAVE TO SLOT 1",  
    "SAVE TO SLOT 2",  
    "SAVE TO SLOT 3",  
    "RESTORE FROM SLOT 1",  
    "RESTORE FROM SLOT 2",  
    "RESTORE FROM SLOT 3",  
    "ERASE SAVE FILE", 
    "EXIT THIS MENU",  
};

// ------------------------------------------------------------------------
// Show to the user if this slot has any data -- if it has data, we also
// show the date and time at which the game was saved... we use the NDS
// time/date for the saved timestamp.
// ------------------------------------------------------------------------
void show_slot_info(UINT8 slot)
{
    if (slot == 255)
    {
        dsPrintValue(8,15, 0, (char*)"                     ");
        dsPrintValue(8,16, 0, (char*)"                     ");
    }
    else
    {
        if (saveState.bSlotUsed[slot] == FALSE)
        {
            dsPrintValue(8,15, 0, (char*)"THIS SLOT IS EMPTY   ");
            dsPrintValue(8,16, 0, (char*)"                     ");
        }
        else
        {
            dsPrintValue(8,15, 0, (char*)"THIS SLOT LAST SAVED ");
            dsPrintValue(8,16, 0, (char*)saveState.slotTimestamp[slot]);
        }
    }
}

// Quick load from slot 1
void quick_load(void)
{
    just_read_save_file();
    state_restore(0);
}

// ------------------------------------------------------------------------
// Show the save/restore menu and let the user pick an option (or exit).
// ------------------------------------------------------------------------
void savestate_entry(void)
{
    UINT8 current_entry = 0;
    char bDone = 0;

    just_read_save_file();

    dsShowBannerScreen();
    swiWaitForVBlank();
    dsPrintValue(8,3,0, (char*)"SAVE/RESTORE STATE");
    dsPrintValue(4,20,0, (char*)"PRESS UP/DOWN AND A=SELECT");

    for (int i=0; i<SAVE_MENU_ITEMS; i++)
    {
           dsPrintValue(8,5+i, (i==0 ? 1:0), (char*)savestate_menu[i]);
    }
    show_slot_info(0);
    
    int last_keys_pressed = -1;
    while (!bDone)
    {
        int keys_pressed = keysCurrent();
        
        if (keys_pressed != last_keys_pressed)
        {
            last_keys_pressed = keys_pressed;
            if (keys_pressed & KEY_DOWN)
            {
                dsPrintValue(8,5+current_entry, 0, (char*)savestate_menu[current_entry]);
                if (current_entry < (SAVE_MENU_ITEMS-1)) current_entry++; else current_entry=0;
                dsPrintValue(8,5+current_entry, 1, (char*)savestate_menu[current_entry]);
                if (current_entry == 0 || current_entry == 3) show_slot_info(0);
                else if (current_entry == 1 || current_entry == 4) show_slot_info(1);
                else if (current_entry == 2 || current_entry == 5) show_slot_info(2);
                else show_slot_info(255);
            }
            if (keys_pressed & KEY_UP)
            {
                dsPrintValue(8,5+current_entry, 0, (char*)savestate_menu[current_entry]);
                if (current_entry > 0) current_entry--; else current_entry=(SAVE_MENU_ITEMS-1);
                dsPrintValue(8,5+current_entry, 1, (char*)savestate_menu[current_entry]);
                if (current_entry == 0 || current_entry == 3) show_slot_info(0);
                else if (current_entry == 1 || current_entry == 4) show_slot_info(1);
                else if (current_entry == 2 || current_entry == 5) show_slot_info(2);
                else show_slot_info(255);
            }
            if (keys_pressed & KEY_A)
            {
                switch (current_entry)
                {
                    case 0:
                        state_save(0);
                        show_slot_info(0);
                        break;
                    case 1:
                        state_save(1);
                        show_slot_info(1);
                        break;
                    case 2:
                        state_save(2);
                        show_slot_info(2);
                        break;
                    case 3:
                        if (state_restore(0)) bDone=1;
                        break;
                    case 4:
                        if (state_restore(1)) bDone=1;
                        break;
                    case 5:
                        if (state_restore(2)) bDone=1;
                        break;
                    case 6:
                        clear_save_file();
                        show_slot_info(255);
                        break;
                    case 7:
                        bDone=1;
                        break;
                }
            }
            
            if (keys_pressed & KEY_B)
            {
                bDone = 1;
            }
            swiWaitForVBlank();
        }
    }    
}

// End of file
