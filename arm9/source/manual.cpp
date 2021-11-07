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
#include "config.h"
#include "manual.h"
#include "CRC32.h"
#include "bgBottom.h"
#include "bgMenu-Green.h"
#include "bgMenu-White.h"
#include "Emulator.h"
#include "Rip.h"

// -------------------------------------------------------------------
// We have a buffer to snarf in the entire text of the .man file. 
// This is plenty big - even the most complex manuals are less than 
// about 8k and we support twice that (roughly 16k max).
//
// The .man files must not exceed 32 bytes per line - they will be
// cut-off otherwise.  Such is life with a small screen!
// -------------------------------------------------------------------
UINT16 buf_lines = 0;
char man_buf[MAX_MAN_ROWS][MAX_MAN_COLS +1];

extern Rip *currentRip;
extern int bg0, bg0b,bg1b;

static void ReadManual(void)
{
    char filebuf[128];
    FILE *fp = NULL;
    // Read the associated .ovl file and parse it...
    if (currentRip != NULL)
    {
        if (myGlobalConfig.man_dir == 1)        // In: /ROMS/MAN
        {
            strcpy(filebuf, "/roms/man/");
        }
        else if (myGlobalConfig.man_dir == 2)   // In: /ROMS/INTY/MAN
        {
            strcpy(filebuf, "/roms/intv/man/");
        }
        else if (myGlobalConfig.man_dir == 3)   // In: /DATA/MAN/
        {
            strcpy(filebuf, "/data/man/");
        }
        else
        {
            strcpy(filebuf, "./");              // In: Same DIR as ROM files
        }
        strcat(filebuf, currentRip->GetFileName());
        filebuf[strlen(filebuf)-4] = 0;
        strcat(filebuf, ".man");
        fp = fopen(filebuf, "rb");
    }

    memset(man_buf, 0x00, sizeof(man_buf));
    
    // --------------------------------------------------------------
    // Now read the entire file in... up to limits of our buffers...
    // --------------------------------------------------------------
    buf_lines = 0;
    if (fp != NULL)
    {
        do
        {
            if (fgets(filebuf, 127, fp) != NULL)
            {
                filebuf[MAX_MAN_COLS] = NULL;
                strcpy((char*)man_buf[buf_lines++], filebuf);
            }
            if (buf_lines >= MAX_MAN_ROWS) break;
        } while (!feof(fp));
        fclose(fp);
    }
    else
    {
        strcpy((char*)man_buf[buf_lines++], (char*)"No .man Manual Found");
    }
}


// -------------------------------------------------------------------------
// Show one page (16 lines) of the manual starting at start_line
// -------------------------------------------------------------------------
void DisplayManualPage(UINT16 start_line)
{
    char strBuf[MAX_MAN_COLS +1];
    UINT16 idx=0;
    for (UINT16 i=start_line; i<(start_line+ROWS_PER_PAGE); i++)
    {
        sprintf(strBuf, "%-32s", (char*)man_buf[i]);
        dsPrintValue(0, 3+idx, 0, strBuf); idx++;
    }
    dsPrintValue(0, 23, 0, (char*)"UP/DN TO SCROLL, LEFT/RIGHT PAGE");
}

// -------------------------------------------------------------------------
// Show the manual on the lower screen... we show up to 16 rows at a time 
// and allow the user to scroll up/down 1 line at a time using the dpad L/R
// or a whole page at a time using the left and right buttons.
// -------------------------------------------------------------------------
void dsShowManual(void)
{
    static UINT16 top_line = 0;
    static UINT32 last_crc=0;
    bool bDone=false;
    int keys_pressed;
    static int last_keys = -1;
    
    dsShowMenu();
    swiWaitForVBlank();    
    
    if (last_crc != currentRip->GetCRC())
    {
        top_line=0;   
        ReadManual();
        last_crc = currentRip->GetCRC();
    }
    DisplayManualPage(top_line);
    while (!bDone)
    {
        keys_pressed = keysCurrent();
        if (keys_pressed & KEY_UP) 
        {
            if (top_line > 0) 
            {
                top_line--;
                DisplayManualPage(top_line);
            }
        }
        if (keys_pressed & KEY_DOWN) 
        {
            if (top_line < (MAX_MAN_ROWS-ROWS_PER_PAGE)) 
            {
                top_line++;
                DisplayManualPage(top_line);
            }
        }
        
        if (keys_pressed != last_keys)
        {
            last_keys = keys_pressed;
            
            if (keys_pressed & KEY_LEFT) 
            {
                if (top_line > (ROWS_PER_PAGE)) 
                {
                    top_line -= ROWS_PER_PAGE;
                }
                else
                {
                    top_line = 0;
                }
                DisplayManualPage(top_line);
            }
            if (keys_pressed & KEY_RIGHT) 
            {
                if (top_line < (MAX_MAN_ROWS-ROWS_PER_PAGE)) 
                {
                    top_line += ROWS_PER_PAGE;
                    DisplayManualPage(top_line);
                }
            }
            if ((keys_pressed & KEY_B) || (keys_pressed & KEY_A))
            {
                bDone=true;
            }
        }
        
        WAITVBL;
    }
}


// End of Line

