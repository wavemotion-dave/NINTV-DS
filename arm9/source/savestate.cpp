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
#include <nds.h>
#include <stdio.h>
#include <fat.h>
#include <dirent.h>
#include <unistd.h>
#include "ds_tools.h"
#include "savestate.h"
#include "bgHighScore.h"
#include "config.h"

extern Emulator *currentEmu;
extern Rip      *currentRip;

#define CURRENT_SAVE_FILE_VER   0x0005

struct 
{
    UINT16                  saveFileVersion;
    UINT8                   bSlotUsed[3];
    char                    slotTimestamp[3][20];
    struct _stateStruct     slot[3];
} saveState;

// Only for the games that require this... it's larger than all of the other saveState stuff above...
JLPState jlpState[3];

char savefilename[128];

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
    saveState.slot[slot].frames = frames;
    saveState.slot[slot].emu_frames = emu_frames;
    if (currentRip->JLP16Bit) currentRip->JLP16Bit->getState(&jlpState[slot]);
    
	FILE* file = fopen(filename, "wb+");

	if (file != NULL) 
    {
        fwrite(&saveState, 1, sizeof(saveState), file);
        if (currentRip->JLP16Bit) fwrite(&jlpState, 1, sizeof(jlpState), file);
        didSave = TRUE;
        fclose(file);
	} 
    
	return didSave;
}

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
            if (currentRip->JLP16Bit) fread(&jlpState, 1, sizeof(jlpState), file);            
            // Ask the emulator to restore it's state...
            currentEmu->LoadState(&saveState.slot[slot]);
            if (currentRip->JLP16Bit) currentRip->JLP16Bit->setState(&jlpState[slot]);
            frames = saveState.slot[slot].frames;
            emu_frames = saveState.slot[slot].emu_frames;

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

void savestate_entry(void)
{
    UINT8 current_entry = 0;
    extern int bg0, bg0b, bg1b;
    char bDone = 0;

    just_read_save_file();
    
    decompress(bgHighScoreTiles, bgGetGfxPtr(bg0b), LZ77Vram);
    decompress(bgHighScoreMap, (void*) bgGetMapPtr(bg0b), LZ77Vram);
    dmaCopy((void *) bgHighScorePal,(u16*) BG_PALETTE_SUB,256*2);
    unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
    dmaFillWords(dmaVal | (dmaVal<<16),(void*) bgGetMapPtr(bg1b),32*24*2);
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
