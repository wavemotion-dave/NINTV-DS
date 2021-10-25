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
#include "highscore.h"
#include "bgMenu-Green.h"
#include "bgMenu-White.h"
#include "bgBottom.h"

#define MAX_HS_GAMES    300
#define HS_VERSION      0x0002

#define HS_OPT_SORTMASK  0x0003
#define HS_OPT_SORTLOW   0x0001
#define HS_OPT_SORTTIME  0x0002
#define HS_OPT_SORTASCII 0x0003

#pragma pack(1)

struct score_t 
{
    char    initials[4];
    char    score[7];
    char    reserved[5];
    uint16  year;
    uint8   month;
    uint8   day;
};

struct highscore_t
{
    UINT32  crc;
    char    notes[21];
    uint16  options;
    struct score_t scores[10];
};

struct highscore_full_t
{
    uint16 version;
    char   last_initials[4];
    struct highscore_t highscore_table[MAX_HS_GAMES];
    uint32 checksum;
} highscores;


uint32 highscore_checksum(void)
{
    char *ptr = (char *)&highscores;
    uint32 sum = 0;
    
    for (int i=0; i<(int)sizeof(highscores) - 4; i++)
    {
           sum = *ptr++;
    }
    return sum;
}

void highscore_init(void) 
{
    bool create_defaults = 0;
    FILE *fp;
    
    strcpy(highscores.last_initials, "   ");
    
    // --------------------------------------------------------------
    // See if the NINTV-DS high score file exists... if so, read it!
    // --------------------------------------------------------------
    fp = fopen("/data/NINTV-DS.hi", "rb");
    if (fp != NULL)
    {
        fread(&highscores, sizeof(highscores), 1, fp);
        fclose(fp);
        
        if (highscores.version != HS_VERSION) create_defaults = 1;
        if (highscore_checksum() != highscores.checksum) create_defaults = 1;
    }
    else
    {
        create_defaults = 1;
    }
    
    if (create_defaults)  // Doesn't exist yet or is invalid... create defaults and save it...
    {
        strcpy(highscores.last_initials, "   ");
        
        for (int i=0; i<MAX_HS_GAMES; i++)
        {
            highscores.highscore_table[i].crc = 0x00000000;
            strcpy(highscores.highscore_table[i].notes, "                    ");
            highscores.highscore_table[i].options = 0x0000;
            for (int j=0; j<10; j++)
            {
                strcpy(highscores.highscore_table[i].scores[j].score, "000000");
                strcpy(highscores.highscore_table[i].scores[j].initials, "   ");
                strcpy(highscores.highscore_table[i].scores[j].reserved, "     ");                
                highscores.highscore_table[i].scores[j].year = 0;
                highscores.highscore_table[i].scores[j].month = 0;
                highscores.highscore_table[i].scores[j].day = 0;
            }
        }
        highscore_save();
    }
}


void highscore_save(void) 
{
    FILE *fp;

    DIR* dir = opendir("/data");
    if (dir)
    {
        /* Directory exists. */
        closedir(dir);
    }
    else
    {
        mkdir("/data", 0777);
    }
    
    // Ensure version and checksum are right...
    highscores.version = HS_VERSION;
    highscores.checksum = highscore_checksum();

    fp = fopen("/data/NINTV-DS.hi", "wb+");
    if (fp != NULL)
    {
        fwrite(&highscores, sizeof(highscores), 1, fp);
        fclose(fp);
    }
}

struct score_t score_entry;
char hs_line[33];

void highscore_showoptions(uint16 options)
{
    if ((options & HS_OPT_SORTMASK) == HS_OPT_SORTLOW)
    {
        dsPrintValue(22,3,0, (char*)"[LOWSC]");
    }
    else if ((options & HS_OPT_SORTMASK) == HS_OPT_SORTTIME)
    {
        dsPrintValue(22,3,0, (char*)"[TIME] ");
    }
    else if ((options & HS_OPT_SORTMASK) == HS_OPT_SORTASCII)
    {
        dsPrintValue(22,3,0, (char*)"[ALPHA]");
    }
    else
    {   
        dsPrintValue(22,3,0, (char*)"       ");    
    }
}

void show_scores(short foundIdx, bool bShowLegend)
{
    dsPrintValue(3,3,0, (char*)highscores.highscore_table[foundIdx].notes);
    for (int i=0; i<10; i++)
    {
        if ((highscores.highscore_table[foundIdx].options & HS_OPT_SORTMASK) == HS_OPT_SORTTIME)
        {
            sprintf(hs_line, "%04d-%02d-%02d   %-3s   %c%c:%c%c.%c%c", highscores.highscore_table[foundIdx].scores[i].year, highscores.highscore_table[foundIdx].scores[i].month,highscores.highscore_table[foundIdx].scores[i].day, 
                                                             highscores.highscore_table[foundIdx].scores[i].initials, highscores.highscore_table[foundIdx].scores[i].score[0], highscores.highscore_table[foundIdx].scores[i].score[1],
                                                             highscores.highscore_table[foundIdx].scores[i].score[2], highscores.highscore_table[foundIdx].scores[i].score[3], highscores.highscore_table[foundIdx].scores[i].score[4],
                                                             highscores.highscore_table[foundIdx].scores[i].score[5]);
        }
        else
        {
            sprintf(hs_line, "%04d-%02d-%02d   %-3s   %-6s  ", highscores.highscore_table[foundIdx].scores[i].year, highscores.highscore_table[foundIdx].scores[i].month,highscores.highscore_table[foundIdx].scores[i].day, 
                                                               highscores.highscore_table[foundIdx].scores[i].initials, highscores.highscore_table[foundIdx].scores[i].score);
        }
        dsPrintValue(3,4+i, 0, hs_line);
    }
    
    if (bShowLegend)
    {
        dsPrintValue(3,14,0, (char*)"                             ");
        dsPrintValue(3,17,0, (char*)"PRESS X FOR NEW HI SCORE     ");
        dsPrintValue(3,18,0, (char*)"PRESS Y FOR NOTES/OPTIONS    ");
        dsPrintValue(3,19,0, (char*)"PRESS B TO EXIT              ");
        dsPrintValue(3,20,0, (char*)"SCORES AUTO SORT AFTER ENTRY ");
    }
    highscore_showoptions(highscores.highscore_table[foundIdx].options);
}

char cmp1[21];
char cmp2[21];
void highscore_sort(short foundIdx)
{
    // Bubblesort!!
    for (int i=0; i<9; i++)
    {
        for (int j=0; j<9; j++)
        {
            if (((highscores.highscore_table[foundIdx].options & HS_OPT_SORTMASK) == HS_OPT_SORTLOW) || ((highscores.highscore_table[foundIdx].options & HS_OPT_SORTMASK) == HS_OPT_SORTTIME))
            {
                if (strcmp(highscores.highscore_table[foundIdx].scores[j+1].score, "000000") == 0)
                     strcpy(cmp1, "999999");
                else 
                    strcpy(cmp1, highscores.highscore_table[foundIdx].scores[j+1].score);
                if (strcmp(highscores.highscore_table[foundIdx].scores[j].score, "000000") == 0)
                     strcpy(cmp2, "999999");
                else 
                    strcpy(cmp2, highscores.highscore_table[foundIdx].scores[j].score);
                if (strcmp(cmp1, cmp2) < 0)
                {
                    // Swap...
                    memcpy(&score_entry, &highscores.highscore_table[foundIdx].scores[j], sizeof(score_entry));
                    memcpy(&highscores.highscore_table[foundIdx].scores[j], &highscores.highscore_table[foundIdx].scores[j+1], sizeof(score_entry));
                    memcpy(&highscores.highscore_table[foundIdx].scores[j+1], &score_entry, sizeof(score_entry));
                }
            }
            else if ((highscores.highscore_table[foundIdx].options & HS_OPT_SORTMASK) == HS_OPT_SORTASCII)
            {
                if (strcmp(highscores.highscore_table[foundIdx].scores[j+1].score, "000000") == 0)
                     strcpy(cmp1, "------");
                else 
                    strcpy(cmp1, highscores.highscore_table[foundIdx].scores[j+1].score);
                if (strcmp(highscores.highscore_table[foundIdx].scores[j].score, "000000") == 0)
                     strcpy(cmp2, "------");
                else 
                    strcpy(cmp2, highscores.highscore_table[foundIdx].scores[j].score);
                
                if (strcmp(cmp1, cmp2) > 0)
                {
                    // Swap...
                    memcpy(&score_entry, &highscores.highscore_table[foundIdx].scores[j], sizeof(score_entry));
                    memcpy(&highscores.highscore_table[foundIdx].scores[j], &highscores.highscore_table[foundIdx].scores[j+1], sizeof(score_entry));
                    memcpy(&highscores.highscore_table[foundIdx].scores[j+1], &score_entry, sizeof(score_entry));
                }
            }
            else
            {
                if (strcmp(highscores.highscore_table[foundIdx].scores[j+1].score, highscores.highscore_table[foundIdx].scores[j].score) > 0)
                {
                    // Swap...
                    memcpy(&score_entry, &highscores.highscore_table[foundIdx].scores[j], sizeof(score_entry));
                    memcpy(&highscores.highscore_table[foundIdx].scores[j], &highscores.highscore_table[foundIdx].scores[j+1], sizeof(score_entry));
                    memcpy(&highscores.highscore_table[foundIdx].scores[j+1], &score_entry, sizeof(score_entry));
                }
            }
        }
    }    
}

void highscore_entry(short foundIdx, UINT32 crc)
{
    char bEntryDone = 0;
    char blink=0;
    unsigned short entry_idx=0;
    char dampen=0;
    time_t unixTime = time(NULL);
    struct tm* timeStruct = gmtime((const time_t *)&unixTime);
    
    dsPrintValue(3,17,0, (char*)"UP/DN/LEFT/RIGHT ENTER SCORE");
    dsPrintValue(3,18,0, (char*)"PRESS START TO SAVE SCORE   ");
    dsPrintValue(3,19,0, (char*)"PRESS SELECT TO CANCEL      ");
    dsPrintValue(3,20,0, (char*)"                            ");

    strcpy(score_entry.score, "000000");
    strcpy(score_entry.initials, highscores.last_initials);
    score_entry.year  = timeStruct->tm_year +1900;
    score_entry.month = timeStruct->tm_mon+1;
    score_entry.day   = timeStruct->tm_mday;
    while (!bEntryDone)
    {
        swiWaitForVBlank();
        if (keysCurrent() & KEY_SELECT) {bEntryDone=1;}

        if (keysCurrent() & KEY_START) 
        {
            strcpy(highscores.last_initials, score_entry.initials);
            memcpy(&highscores.highscore_table[foundIdx].scores[9], &score_entry, sizeof(score_entry));
            highscores.highscore_table[foundIdx].crc = crc;
            highscore_sort(foundIdx);
            highscore_save();
            bEntryDone=1;
        }

        if (dampen == 0)
        {
            if ((keysCurrent() & KEY_RIGHT) || (keysCurrent() & KEY_A))
            {
                if (entry_idx < 8) entry_idx++; 
                blink=25;
                dampen=15;
            }

            if (keysCurrent() & KEY_LEFT)  
            {
                if (entry_idx > 0) entry_idx--; 
                blink=25;
                dampen=15;
            }

            if (keysCurrent() & KEY_UP)
            {
                if (entry_idx < 3) // This is the initials
                {
                    if (score_entry.initials[entry_idx] == ' ')
                        score_entry.initials[entry_idx] = 'A';
                    else if (score_entry.initials[entry_idx] == 'Z')
                        score_entry.initials[entry_idx] = ' ';
                    else score_entry.initials[entry_idx]++;
                }
                else    // This is the score...
                {
                    if ((highscores.highscore_table[foundIdx].options & HS_OPT_SORTMASK) == HS_OPT_SORTASCII)
                    {
                        if (score_entry.score[entry_idx-3] == ' ')
                            score_entry.score[entry_idx-3] = 'A';
                        else if (score_entry.score[entry_idx-3] == 'Z')
                            score_entry.score[entry_idx-3] = '0';
                        else if (score_entry.score[entry_idx-3] == '9')
                            score_entry.score[entry_idx-3] = ' ';
                        else score_entry.score[entry_idx-3]++;
                    }
                    else
                    {
                        score_entry.score[entry_idx-3]++;
                        if (score_entry.score[entry_idx-3] > '9') score_entry.score[entry_idx-3] = '0';
                    }
                }
                blink=0;
                dampen=10;
            }

            if (keysCurrent() & KEY_DOWN)
            {
                if (entry_idx < 3) // // This is the initials
                {
                    if (score_entry.initials[entry_idx] == ' ')
                        score_entry.initials[entry_idx] = 'Z';
                    else if (score_entry.initials[entry_idx] == 'A')
                        score_entry.initials[entry_idx] = ' ';
                    else score_entry.initials[entry_idx]--;
                }
                else   // This is the score...
                {
                    if ((highscores.highscore_table[foundIdx].options & HS_OPT_SORTMASK) == HS_OPT_SORTASCII)
                    {
                        if (score_entry.score[entry_idx-3] == ' ')
                            score_entry.score[entry_idx-3] = '9';
                        else if (score_entry.score[entry_idx-3] == '0')
                            score_entry.score[entry_idx-3] = 'Z';
                        else if (score_entry.score[entry_idx-3] == 'A')
                            score_entry.score[entry_idx-3] = ' ';
                        else score_entry.score[entry_idx-3]--;
                    }
                    else
                    {
                        score_entry.score[entry_idx-3]--;
                        if (score_entry.score[entry_idx-3] < '0') score_entry.score[entry_idx-3] = '9';
                    }
                }
                blink=0;
                dampen=10;
            }
        }
        else
        {
            dampen--;
        }

        sprintf(hs_line, "%04d-%02d-%02d   %-3s   %-6s", score_entry.year, score_entry.month, score_entry.day, score_entry.initials, score_entry.score);
        if ((++blink % 60) > 30)
        {
            if (entry_idx < 3)
                hs_line[13+entry_idx] = '_';
            else
                hs_line[16+entry_idx] = '_';
        }
        dsPrintValue(3,14, 0, (char*)hs_line);
    }
    
    show_scores(foundIdx, true);
}

void highscore_options(short foundIdx, UINT32 crc)
{
    uint16 options = 0x0000;
    char notes[21];
    char bEntryDone = 0;
    char blink=0;
    unsigned short entry_idx=0;
    char dampen=0;
    
    dsPrintValue(3,14,0, (char*)"NOTE: ");
    dsPrintValue(3,17,0, (char*)"UP/DN/LEFT/RIGHT ENTER NOTES");
    dsPrintValue(3,18,0, (char*)"X=TOGGLE SORT, L+R=CLR SCORE");
    dsPrintValue(3,19,0, (char*)"PRESS START TO SAVE OPTIONS ");
    dsPrintValue(3,20,0, (char*)"PRESS SELECT TO CANCEL      ");

    strcpy(notes, highscores.highscore_table[foundIdx].notes);
    options = highscores.highscore_table[foundIdx].options;
    
    while (!bEntryDone)
    {
        swiWaitForVBlank();
        if (keysCurrent() & KEY_SELECT) {bEntryDone=1;}

        if (keysCurrent() & KEY_START) 
        {
            strcpy(highscores.highscore_table[foundIdx].notes, notes);
            highscores.highscore_table[foundIdx].options = options;
            highscores.highscore_table[foundIdx].crc = crc;
            highscore_sort(foundIdx);
            highscore_save();
            bEntryDone=1;
        }

        if (dampen == 0)
        {
            if ((keysCurrent() & KEY_RIGHT) || (keysCurrent() & KEY_A))
            {
                if (entry_idx < 19) entry_idx++; 
                blink=25;
                dampen=15;
            }

            if (keysCurrent() & KEY_LEFT)  
            {
                if (entry_idx > 0) entry_idx--; 
                blink=25;
                dampen=15;
            }

            if (keysCurrent() & KEY_UP)
            {
                if (notes[entry_idx] == ' ')
                    notes[entry_idx] = 'A';
                else if (notes[entry_idx] == 'Z')
                    notes[entry_idx] = '0';
                else if (notes[entry_idx] == '9')
                    notes[entry_idx] = ' ';
                else notes[entry_idx]++;
                blink=0;
                dampen=10;
            }

            if (keysCurrent() & KEY_DOWN)
            {
                if (notes[entry_idx] == ' ')
                    notes[entry_idx] = '9';
                else if (notes[entry_idx] == '0')
                    notes[entry_idx] = 'Z';
                else if (notes[entry_idx] == 'A')
                    notes[entry_idx] = ' ';
                else notes[entry_idx]--;
                blink=0;
                dampen=10;
            }

            if (keysCurrent() & KEY_X)  
            {
                if ((options & HS_OPT_SORTMASK) == HS_OPT_SORTLOW)
                {
                    options &= (uint16)~HS_OPT_SORTMASK;
                    options |= HS_OPT_SORTTIME;
                }
                else if ((options & HS_OPT_SORTMASK) == HS_OPT_SORTTIME)
                {
                    options &= (uint16)~HS_OPT_SORTMASK;
                    options |= HS_OPT_SORTASCII;
                }
                else if ((options & HS_OPT_SORTMASK) == HS_OPT_SORTASCII)
                {
                    options &= (uint16)~HS_OPT_SORTMASK;
                }
                else
                {
                    options |= (uint16)HS_OPT_SORTLOW;
                }
                highscore_showoptions(options);
                dampen=15;
            }
            
            // Clear the entire game of scores... 
            if ((keysCurrent() & KEY_L) && (keysCurrent() & KEY_R))
            {
                highscores.highscore_table[foundIdx].crc = 0x00000000;
                highscores.highscore_table[foundIdx].options = 0x0000;
                strcpy(highscores.highscore_table[foundIdx].notes, "                    ");
                strcpy(notes, "                    ");                
                for (int j=0; j<10; j++)
                {
                    strcpy(highscores.highscore_table[foundIdx].scores[j].score, "000000");
                    strcpy(highscores.highscore_table[foundIdx].scores[j].initials, "   ");
                    strcpy(highscores.highscore_table[foundIdx].scores[j].reserved, "    ");
                    highscores.highscore_table[foundIdx].scores[j].year = 0;
                    highscores.highscore_table[foundIdx].scores[j].month = 0;
                    highscores.highscore_table[foundIdx].scores[j].day = 0;
                }
                show_scores(foundIdx, false);
                highscore_save();                    
            }            
        }
        else
        {
            dampen--;
        }

        sprintf(hs_line, "%-20s", notes);
        if ((++blink % 60) > 30)
        {
            hs_line[entry_idx] = '_';
        }
        dsPrintValue(9,14, 0, (char*)hs_line);
    }
    
    show_scores(foundIdx, true);
}

void highscore_display(UINT32 crc) 
{
    short foundIdx = -1;
    short firstBlank = -1;
    char bDone = 0;

    dsShowMenu();
    swiWaitForVBlank();

    // ---------------------------------------------------------------------------------
    // Check if the current CRC32 is in our High Score database...
    // ---------------------------------------------------------------------------------
    for (int i=0; i<MAX_HS_GAMES; i++)
    {
        if (firstBlank == -1)
        {
            if ((highscores.highscore_table[i].crc) == 0)
            {
                firstBlank = i;
            }
        }

        if (highscores.highscore_table[i].crc == crc)
        {
            foundIdx = i;
            break;
        }
    }
    
    if (foundIdx == -1)
    {
        foundIdx = firstBlank;   
    }
    
    show_scores(foundIdx, true);

    while (!bDone)
    {
        if (keysCurrent() & KEY_A) bDone=1;
        if (keysCurrent() & KEY_B) bDone=1;
        if (keysCurrent() & KEY_X) highscore_entry(foundIdx, crc);
        if (keysCurrent() & KEY_Y) highscore_options(foundIdx, crc);
    }    
}


// End of file
