// =====================================================================================
// Copyright (c) 2021-2022 Dave Bernazzani (wavemotion-dave)
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
#include "CRC32.h"
#include "bgBottom.h"
#include "config.h"
#include "bgMenu-Green.h"
#include "bgMenu-White.h"
#include "Emulator.h"
#include "Rip.h"
#include "cheat.h"

struct Cheat_t myCheats[MAX_CHEATS_PER_GAME];

extern Rip *currentRip;
extern int bg0, bg0b, bg1b;

u8 bCheatChanged = false;

// ---------------------------------------------------------------------
// Display the current list of cheats. There may be none for this
// game and that's okay... we'll show an informational message instead.
// ---------------------------------------------------------------------
static char strBuf[35];
u8 display_cheat_list(bool bFullDisplay)
{
    u8 count=0;
    
    if (bFullDisplay)
    {
        for (u8 idx=0; idx < MAX_CHEATS_PER_GAME; idx++)
        {
            if (strlen(myCheats[idx].name) <= 1) break;
            count++;
            sprintf(strBuf, "%-28s : %c", myCheats[idx].name, (myCheats[idx].enabled ? 'Y':'N'));
            dsPrintValue(0,3+idx, (idx==0 ? 1:0), strBuf);
        }
    }
    
    if (count == 0)
    {
            dsPrintValue(0, 5, 0, (char *)"  NO CHEAT FOUND FOR THIS GAME  ");
            dsPrintValue(0, 7, 0, (char *)"  ENSURE NINTV-DS.CHT IN /DATA  ");
            dsPrintValue(0, 9, 0, (char *)"     SEE README FOR DETAILS     ");
    }

    dsPrintValue(0,22, 0, (char *)"   D-PAD TOGGLE CHEAT. B=EXIT   ");
    dsPrintValue(0,23, 0, (char *)"   RESET GAME TO APPLY CHEATS   ");
    return count;    
}


// ---------------------------------------------------------------------------------------
// Display the cheat menu - let the user select among the cheats to enable/disable them.
// The user will need to RESET the game to apply the cheats - we give that warning
// at the bottom of the instruction screen.
// ---------------------------------------------------------------------------------------
void CheatMenu(void)
{
    int keys_pressed = 0;
    int last_keys_pressed = 999;
    u8 bDone = false;

    // Show the Options background...
    dsShowMenu();

    while (keysCurrent()) 
        ;
    
    // Show the list of cheats 
    u8 idx = display_cheat_list(true);
    
    // Always start at the first (top) line
    int optionHighlighted = 0;
    
    // -------------------------------------------
    // And let user select the cheat to toggle...
    // -------------------------------------------
    while (!bDone)
    {
        keys_pressed = keysCurrent();
        
        if (keys_pressed != last_keys_pressed)
        {
            last_keys_pressed = keys_pressed;
            if (keys_pressed & (KEY_UP | KEY_DOWN)) // Previous option
            {
                if (idx > 0)
                {
                    sprintf(strBuf, "%-28s : %c", myCheats[optionHighlighted].name, (myCheats[optionHighlighted].enabled ? 'Y':'N'));
                    dsPrintValue(0,3+optionHighlighted, 0, strBuf);

                    if (keys_pressed & KEY_UP)
                    {
                        if (optionHighlighted > 0) optionHighlighted--; else optionHighlighted=(idx-1);
                    }
                    else
                    {
                        if (optionHighlighted < (idx-1)) optionHighlighted++;  else optionHighlighted=0;
                    }

                    sprintf(strBuf, "%-28s : %c", myCheats[optionHighlighted].name, (myCheats[optionHighlighted].enabled ? 'Y':'N'));
                    dsPrintValue(0,3+optionHighlighted, 1, strBuf);
                }
            }
            else
            if (keys_pressed & (KEY_LEFT | KEY_RIGHT))  // Toggle cheat option
            {
                if (idx > 0)
                {
                    bCheatChanged = true;
                    myCheats[optionHighlighted].enabled = (myCheats[optionHighlighted].enabled ? 0:1);
                    sprintf(strBuf, "%-28s : %c", myCheats[optionHighlighted].name, (myCheats[optionHighlighted].enabled ? 'Y':'N'));
                    dsPrintValue(0,3+optionHighlighted, 1, strBuf);
                }
            }
            else 
            if (keys_pressed & (KEY_A | KEY_B))
            {
                bDone = true;
            }
        }
        swiWaitForVBlank();
    }
    
    return;
}


// -------------------------------------------------------------------------
// Read the global nintv-ds.cht file and match it up with our current game
// -------------------------------------------------------------------------
void LoadCheats(void)
{
    u8 bFound = false;
    char filebuf[128];
    FILE *fp = NULL;
    
    // --------------------------------------------
    // Wipe it all to zeros... cheats disabled...
    // --------------------------------------------
    memset(myCheats, 0x00, sizeof(myCheats));   
    
    // Read the nintv-ds.cht file and parse it... if it doesn't exist, we've already disabled all cheats above.
    if (currentRip != NULL)
    {
        strcpy(filebuf, "/data/nintv-ds.cht");
        fp = fopen(filebuf, "rb");
        
        if (fp)
        {
            u16 buf_lines = 0; 
            s8 cheat_idx = -1;
            u8 poke_idx = 0;
            // ------------------------------------------------------------
            // Read through the entire .cht file looking for our CRC match
            // ------------------------------------------------------------
            do
            {
                if (fgets(filebuf, 127, fp) != NULL)
                {
                    if ((filebuf[0] == ';') || (filebuf[0] <= ' ')) asm("nop");  // Swallow comment lines or lines with no relevant parsing needed
                    else
                    if (filebuf[0] == '|')
                    {
                        if (bFound) 
                        {
                            if (buf_lines > 0) break;  // We're done... next CRC found
                        }
                        else 
                        {
                            // Check if this filename line is a match...
                            char *ptr = &filebuf[1];
                            u8 compSize=0;
                            while (*ptr != '|' && *ptr != 0) {ptr++; compSize++;}
                            ptr = &filebuf[1];
                            if (strncasecmp(ptr,currentRip->GetFileName(), compSize) == 0)
                            {
                                bFound = true;
                            }
                        }
                    }
                    else
                    if (filebuf[0] == '$')
                    {
                        if (bFound) 
                        {
                            if (buf_lines > 0) break;  // We're done... next CRC found
                        }
                        else 
                        {
                            // Check if this CRC line is a match...
                            char *ptr = filebuf;
                            ptr++;
                            UINT32 crc = strtoul(ptr, &ptr, 16);
                            if (crc == currentRip->GetCRC())
                            {
                                bFound = true;
                            }
                        }
                    }
                    else
                    if (filebuf[0] == '[') // New Cheat Name directive (e.g. [Invincibility])
                    {
                        // --------------------------------------------------------------------------------
                        // If we've already found our matching CRC... copy the poke into our cheat area.
                        // --------------------------------------------------------------------------------
                        if (bFound)
                        {
                            buf_lines++;
                            cheat_idx++;
                            myCheats[cheat_idx].enabled = 0;
                            for (u8 i=0; i < 30; i++)
                            {
                                if (filebuf[i+1] == ']') 
                                {
                                    myCheats[cheat_idx].name[i] = NULL;
                                    if (filebuf[i+2] == 'Y') myCheats[cheat_idx].enabled = 1;   // Force cheat ENABLED
                                    break;
                                }
                                else
                                {
                                    myCheats[cheat_idx].name[i] = filebuf[i+1];
                                }
                                                                     
                            }
                            poke_idx=0;
                        }
                    }
                    else
                    if ((filebuf[0] == 'p') || (filebuf[0] == 'P')) // Poke directive
                    {
                        // --------------------------------------------------------------------------------
                        // If we've already found our matching CRC... copy the poke into our cheat area.
                        // --------------------------------------------------------------------------------
                        if (bFound)
                        {
                            buf_lines++;
                            char *ptr = filebuf;
                            ptr++;ptr++;
                            myCheats[cheat_idx].pokes[poke_idx].addr = strtoul(ptr, &ptr, 16); ptr++;
                            myCheats[cheat_idx].pokes[poke_idx].value = strtoul(ptr, &ptr, 16);
                            poke_idx++;
                        }
                    }
                }
            } while (!feof(fp));
            fclose(fp);    
        }
    }
}

// End of Line

