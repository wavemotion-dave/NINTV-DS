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
#include<stdio.h>
#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "ds_tools.h"
#include "savestate.h"
#include "config.h"
#include "bgBottom.h"
#include "bgTop.h"
#include "bgMenu-Green.h"
#include "bgMenu-White.h"
#include "Emulator.h"
#include "Rip.h"
#include "ROMBanker.h"
#include "CRC32.h"
#include "loadgame.h"
#include "debugger.h"

// -------------------------------------------------------------
// We support up to 512 files per directory. More than enough.
// -------------------------------------------------------------
FICA_INTV intvromlist[512];
u16 countintv=0, ucFicAct=0;
char szName[256];
char szName2[256];

extern Rip *currentRip;
extern UINT16 slow_ram_idx;

u8 bFavsOnlyMode = false;

// -------------------------------------------------------------------------------
// Load the cart from a file on disk. We support .bin/.int (raw binary) and we 
// also support .ROM (intellicart). The .bin file must have a matching CRC in
// our internal database or else must have a matching <filename>.cfg file so 
// we know how to load the binary into memory. The intellivision allows the 
// ROM to be located in many possible memory segments... the .ROM will give
// us this information and is easier to deal with as a self-contained file.
// -------------------------------------------------------------------------------
BOOL LoadCart(const CHAR* filename)
{
    if (strlen(filename) < 5)
        return FALSE;
    
    bIsFatalError = false;
    bGameLoaded = FALSE;
    
    memset(gLastBankers, 0x00, sizeof(gLastBankers));

    slow_ram_idx = 0;

    const CHAR* extStart = filename + strlen(filename) - 4;
    
    if (strcmpi(extStart, ".int") == 0 || strcmpi(extStart, ".bin") == 0)
    {
        //load the bin file as a Rip - use internal database or maybe <filename>.cfg exists... LoadBin() handles all that.
        currentRip = Rip::LoadBin(filename);
        if (currentRip == NULL)
        {
            return FALSE;   // FatalError() will have already been called
        }
    }
    else if (strcmpi(extStart, ".rom") == 0)    // .rom files contain the loading info...
    {
        //load the bin file as a Rip
        currentRip = Rip::LoadRom(filename);
        if (currentRip == NULL)
        {
            return FALSE;   // FatalError() will have already been called
        }
    }
    else
    {
        currentRip = NULL;
        FatalError("UNKNOWN FILE TYPE");
        return FALSE;
    }

    // ---------------------------------------------------------------------
    // New game is loaded... (would have returned FALSE above otherwise)
    // ---------------------------------------------------------------------
    extern UINT16 fudge_timing;
    extern UINT8 bLatched;
    fudge_timing = 0;
    bLatched = false;
    if (currentRip->GetCRC() == 0x5F6E1AF6) fudge_timing = 1000;    // Motocross needs some fudge timing to run... known race condition...
    if (currentRip->GetCRC() == 0x2DEACD15) bLatched = true;        // Stampede must have latched backtab access
    if (currentRip->GetCRC() == 0x573B9B6D) bLatched = true;        // Masters of the Universe must have latched backtab access
    
    FindAndLoadConfig();
    dsShowScreenEmu();
    dsShowScreenMain(false);
    
    bGameLoaded = TRUE;    
    bInitEmulator = true;    
    
    bStartSoundFifo = true;
    
    return TRUE;
}

// -----------------------------------------------------------------------
// If this is the first time we are being asked to load a directory of
// games, we will check the configuration to see if we should start in
// one of the common directories:  /roms or /roms/intv ... otherwise
// we will just open up in the last directory the user picked.
// -----------------------------------------------------------------------
void CheckFirstTimeLoad(void)
{
  static bool bFirstTime = true;
    
  // First time in we use the config setting to determine where we open files...
  if (bFirstTime)
  {
      bFirstTime = false;
      if (myGlobalConfig.rom_dir == 1)
      {
         chdir("/ROMS");
      }
      else if (myGlobalConfig.rom_dir == 2)
      {
         chdir("/ROMS/INTV");
      }
  }
}

// ---------------------------------------------------------------------------------------------
// If the exec.bin was not found, we will do a search for it in the current directory
// to see if we get a match via CRC32.  We keep our search as fast as possible, we 
// will only look at files that are exactly 8k in size.  This will still take a solid
// second or two on the NDS - it's better if the user has the right filename to avoid this.
// ---------------------------------------------------------------------------------------------
void FindAndLoadExec(char *szFoundName)
{
  // Look through all files in the current directory to see if we get a CRC32 match...
  DIR *pdir;
  struct dirent *pent;

  CheckFirstTimeLoad();
    
  pdir = opendir(".");

  if (pdir) 
  {
    while (((pent=readdir(pdir))!=NULL)) 
    {
      if (pent->d_type != DT_DIR)
      {
        struct stat st;
        stat(pent->d_name, &st);
        if (st.st_size == 8192)
        {
            if (CRC32::getCrc(pent->d_name) == 0xcbce86f7)
            {
                // Found it!!
                strcpy(szFoundName, pent->d_name);
                break;
            }
        }
      }
    }
    closedir(pdir);
  }    
}


// ---------------------------------------------------------------------------------------------
// If the grom.bin was not found, we will do a search for it in the current directory
// to see if we get a match via CRC32.  We keep our search as fast as possible, we 
// will only look at files that are exactly 2k in size.  This will still take a solid
// second or two on the NDS - it's better if the user has the right filename to avoid this.
// ---------------------------------------------------------------------------------------------
void FindAndLoadGrom(char *szFoundName)
{
  // Look through all files in the current directory to see if we get a CRC32 match...
  DIR *pdir;
  struct dirent *pent;

  CheckFirstTimeLoad();
    
  pdir = opendir(".");

  if (pdir) 
  {
    while (((pent=readdir(pdir))!=NULL)) 
    {
      if (pent->d_type != DT_DIR)
      {
        struct stat st;
        stat(pent->d_name, &st);
        if (st.st_size == 2048)
        {
            if (CRC32::getCrc(pent->d_name) == 0x683a4158)
            {
                // Found it!!
                strcpy(szFoundName, pent->d_name);
                break;
            }
        }
      }
    }
    closedir(pdir);
  }    
}


// ---------------------------------------------------------------------------------------------
// If the ivoice.bin was not found, we will do a search for it in the current directory
// to see if we get a match via CRC32.  We keep our search as fast as possible, we 
// will only look at files that are exactly 2k in size.  This will still take a solid
// second or two on the NDS - it's better if the user has the right filename to avoid this.
// ---------------------------------------------------------------------------------------------
void FindAndLoadIVoice(char *szFoundName)
{
  // Look through all files in the current directory to see if we get a CRC32 match...
  DIR *pdir;
  struct dirent *pent;

  CheckFirstTimeLoad();
    
  pdir = opendir(".");

  if (pdir) 
  {
    while (((pent=readdir(pdir))!=NULL)) 
    {
      if (pent->d_type != DT_DIR)
      {
        struct stat st;
        stat(pent->d_name, &st);
        if (st.st_size == 2048)
        {
            if (CRC32::getCrc(pent->d_name) == 0x0de7579d)
            {
                // Found it!!
                strcpy(szFoundName, pent->d_name);
                break;
            }
        }
      }
    }
    closedir(pdir);
  }    
}


// ---------------------------------------------------------------------------------------------
// Peripheral roms are basically our BIOS files... grom.bin, exec.bin and ivoice.bin
// ---------------------------------------------------------------------------------------------
BOOL LoadPeripheralRoms(Peripheral* peripheral)
{
    UINT16 count = peripheral->GetROMCount();
    for (UINT16 i = 0; i < count; i++) {
        ROM* r = peripheral->GetROM(i);
        if (r->isLoaded())
            continue;

        CHAR nextFile[MAX_PATH];
        
        if (myGlobalConfig.bios_dir == 1)        // In: /ROMS/BIOS
        {
            strcpy(nextFile, "/roms/bios/");
        }
        else if (myGlobalConfig.bios_dir == 2)   // In: /ROMS/INTV/BIOS
        {
            strcpy(nextFile, "/roms/intv/bios/");
        }
        else if (myGlobalConfig.bios_dir == 3)   // In: /DATA/BIOS/
        {
            strcpy(nextFile, "/data/bios/");
        }
        else
        {
            strcpy(nextFile, "./");              // In: Same DIR as ROM files
        }

        strcat(nextFile, r->getDefaultFileName());
        if (!r->load(nextFile, r->getDefaultFileOffset())) 
        {
            if (strstr(nextFile, "grom") != NULL) FindAndLoadGrom(nextFile);
            if (strstr(nextFile, "exec") != NULL) FindAndLoadExec(nextFile);
            if (strstr(nextFile, "ivoice") != NULL) FindAndLoadIVoice(nextFile);
            
            if (!r->load(nextFile, r->getDefaultFileOffset()))
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}


// ---------------------------------------------------------------------------------------------
// Let the user know what buttons they can press to load various peripheral roms...
// ---------------------------------------------------------------------------------------------
void dsDisplayLoadInstructions(void)
{
  dsPrintValue(1,22,0,(char*)"SEL=MARK, STA=SAVEFAV, L/R=FAVS");
  dsPrintValue(1,23,0,(char*)"A=LOAD, X=LOAD OPTIONS, B=BACK ");
}

// ---------------------------------------------------------------------------------------------
// This is our custom sorting compare routine. Directories always sort to the top otherwise
// we do a case-insensitive sort.
// ---------------------------------------------------------------------------------------------
int intvFilescmp (const void *c1, const void *c2) 
{
  FICA_INTV *p1 = (FICA_INTV *) c1;
  FICA_INTV *p2 = (FICA_INTV *) c2;

  if (p1->filename[0] == '.' && p2->filename[0] != '.')
      return -1;
  if (p2->filename[0] == '.' && p1->filename[0] != '.')
      return 1;
  if (p1->directory && !(p2->directory))
      return -1;
  if (p2->directory && !(p1->directory))
      return 1;
  return strcasecmp (p1->filename, p2->filename);    
}

// ----------------------------------------------------------------------------------------------
// Determine if this game is one of our 'favs'. We do a hash crc32 on the title of the game
// which is much faster than trying to get CRCs of the actual files in a directory (too slow).
// ----------------------------------------------------------------------------------------------
bool isFavorite(char *filename)
{
    for (int i=0; i<64; i++)
    {
        if (myGlobalConfig.favorites[i] != 0x00000000)
        {
            if (myGlobalConfig.favorites[i] == CRC32::getCrc((UINT8* )filename, strlen(filename)))
            {
                return true;
            }
        }
    }
    return false;
}

// ----------------------------------------------------------------------------------------
// Set the current filename as a favorite. We use the CRC32 of the filename as the hash.
// ----------------------------------------------------------------------------------------
void setFavorite(char *filename)
{    
    for (int i=0; i<64; i++)
    {
        if (myGlobalConfig.favorites[i] == 0x00000000)
        {
            myGlobalConfig.favorites[i] = CRC32::getCrc((UINT8* )filename, strlen(filename));
            break;
        }
    }
}


// ----------------------------------------------------------------------------------------
// Clear the current filename as a favorite. We use the CRC32 of the filename as the hash.
// ----------------------------------------------------------------------------------------
void clrFavorite(char *filename)
{
    for (int i=0; i<64; i++)
    {
        if (myGlobalConfig.favorites[i] != 0x00000000)
        {
            if (myGlobalConfig.favorites[i] == CRC32::getCrc((UINT8* )filename, strlen(filename)))
            {
                myGlobalConfig.favorites[i] = 0x00000000;
            }
        }
    }
}


// ----------------------------------------------------------------------------------------
// Find all .bin, .int and .rom files in the current directory (or, if this is the first
// time we are loading up files, use the global configuration to determine what directory
// we should be starting in... after the first time we just load up the current directory).
// ----------------------------------------------------------------------------------------
ITCM_CODE void intvFindFiles(void) 
{
  static bool bFirstTime = true;
  DIR *pdir;
  struct dirent *pent;

  countintv = 0;
  memset(intvromlist, 0x00, sizeof(intvromlist));

  // First time in we use the config setting to determine where we open files...
  if (bFirstTime)
  {
      bFirstTime = false;
      if (myGlobalConfig.rom_dir == 1)
      {
         chdir("/ROMS");
      }
      else if (myGlobalConfig.rom_dir == 2)
      {
         chdir("/ROMS/INTV");
      }
  }
    
  pdir = opendir(".");

  if (pdir) 
  {
    while (((pent=readdir(pdir))!=NULL)) 
    {
      strcpy(szName2,pent->d_name);
      szName2[127] = NULL;
      if (pent->d_type == DT_DIR)
      {
        if (!( (szName2[0] == '.') && (strlen(szName2) == 1))) 
        {
          intvromlist[countintv].directory = true;
          strcpy(intvromlist[countintv].filename,szName2);
          countintv++;
        }
      }
      else 
      {
        // ------------------------------------------------
        // Filter out the BIOS files from the list...
        // ------------------------------------------------
        if (strcasecmp(szName2, "grom.bin") == 0) continue;
        if (strcasecmp(szName2, "exec.bin") == 0) continue;
        if (strcasecmp(szName2, "ivoice.bin") == 0) continue;
        if (strcasecmp(szName2, "ecs.bin") == 0) continue;
        if (strstr(szName2, "[BIOS]") != NULL) continue;
        if (strstr(szName2, "[bios]") != NULL) continue;
        
        if (strlen(szName2)>4) 
        {
          if ( (strcasecmp(strrchr(szName2, '.'), ".int") == 0) ||
               (strcasecmp(strrchr(szName2, '.'), ".bin") == 0) ||
               (strcasecmp(strrchr(szName2, '.'), ".rom") == 0) )
          {
            intvromlist[countintv].favorite = isFavorite(szName2);
            if (bFavsOnlyMode && !intvromlist[countintv].favorite) 
            {
                continue;
            }
            intvromlist[countintv].directory = false;
            strcpy(intvromlist[countintv].filename,szName2);
            countintv++;
          }
        }
      }
    }
    closedir(pdir);
  }
    
  // ----------------------------------------------
  // If we found any files, go sort the list...
  // ----------------------------------------------
  if (countintv)
  {
    qsort (intvromlist, countintv, sizeof (FICA_INTV), intvFilescmp);
  }
}

// --------------------------------------------------------------------------
// Display the files - up to 18 in a page. This is carried over from the 
// oldest emulators I've worked on and there is some bug in here where
// it occasinoally gets out of sync... it's minor enough to not be an
// issue but at some point this routine should be re-written...
// --------------------------------------------------------------------------
ITCM_CODE void dsDisplayFiles(unsigned int NoDebGame,u32 ucSel)
{
  unsigned int ucBcl,ucGame;

  // Display all games if possible
  unsigned short dmaVal = *(bgGetMapPtr(bg1b) +31*32);
  dmaFillWords(dmaVal | (dmaVal<<16),(void*) (bgGetMapPtr(bg1b)),32*24*2);
  sprintf(szName,"%04d/%04d %s",(int)(1+ucSel+NoDebGame),countintv, (bFavsOnlyMode ? "FAVS":"GAMES"));
  dsPrintValue(16-strlen(szName)/2,2,0,szName);
  dsPrintValue(31,4,0,(char *) (NoDebGame>0 ? "<" : " "));
  dsPrintValue(31,20,0,(char *) (NoDebGame+14<countintv ? ">" : " "));

  dsDisplayLoadInstructions();
    
  for (ucBcl=0;ucBcl<17; ucBcl++) 
  {
    ucGame= ucBcl+NoDebGame;
    if (ucGame < countintv)
    {
      strcpy(szName,intvromlist[ucGame].filename);
      szName[29]='\0';
      if (intvromlist[ucGame].directory)
      {
        szName[27]='\0';
        sprintf(szName2,"[%s]",szName);
        dsPrintValue(0,4+ucBcl,(ucSel == ucBcl ? 1 :  0),szName2);
      }
      else
      {
          if (intvromlist[ucGame].favorite)
          {
             dsPrintValue(0,4+ucBcl,0, (char*)"@");
          }
          dsPrintValue(1,4+ucBcl,(ucSel == ucBcl ? 1 : 0),szName);
      }
    }
  }
}


#define LOAD_OPTION_MENU_ITEMS 8
const char *load_options_menu[LOAD_OPTION_MENU_ITEMS] = 
{
    "LOAD GAME AS-IS",  
    "LOAD GAME WITH JLP",  
    "LOAD GAME WITH IVOICE",  
    "LOAD GAME WITH ECS",  
    "LOAD GAME WITH JLP+IVOICE",  
    "LOAD GAME WITH ECS+IVOICE",
    "LOAD GAME WITH JLP+ECS+IV",
    "EXIT THIS MENU",  
};


UINT8 LoadWithOptions(void)
{
    UINT8 current_entry = 0;

    dsShowBannerScreen();
    swiWaitForVBlank();
    dsPrintValue(3,3,0, (char*) "  LOAD GAME OPTIONS       ");
    dsPrintValue(3,20,0, (char*)"PRESS UP/DOWN AND A=SELECT");

    for (int i=0; i<LOAD_OPTION_MENU_ITEMS; i++)
    {
           dsPrintValue(5,5+i, (i==0 ? 1:0), (char*)load_options_menu[i]);
    }
    
    int last_keys_pressed = -1;
    while (1)
    {
        int keys_pressed = keysCurrent();
        
        if (keys_pressed != last_keys_pressed)
        {
            last_keys_pressed = keys_pressed;
            if (keys_pressed & KEY_DOWN)
            {
                dsPrintValue(5,5+current_entry, 0, (char*)load_options_menu[current_entry]);
                if (current_entry < (LOAD_OPTION_MENU_ITEMS-1)) current_entry++; else current_entry=0;
                dsPrintValue(5,5+current_entry, 1, (char*)load_options_menu[current_entry]);
            }
            if (keys_pressed & KEY_UP)
            {
                dsPrintValue(5,5+current_entry, 0, (char*)load_options_menu[current_entry]);
                if (current_entry > 0) current_entry--; else current_entry=(LOAD_OPTION_MENU_ITEMS-1);
                dsPrintValue(5,5+current_entry, 1, (char*)load_options_menu[current_entry]);
            }
            if (keys_pressed & KEY_A)
            {
                return current_entry;
            }            
            swiWaitForVBlank();
        }
    }    
    return (LOAD_OPTION_MENU_ITEMS-1);
}


// --------------------------------------------------------------------------
// Let the user see all relevant ROM files and pick the one they want...
// --------------------------------------------------------------------------
unsigned int dsWaitForRom(char *chosen_filename)
{
  u8 bDone=false, bRet=false;
  u16 ucHaut=0x00, ucBas=0x00,ucSHaut=0x00, ucSBas=0x00,romSelected= 0, firstRomDisplay=0,nbRomPerPage, uNbRSPage;
  u16 uLenFic=0, ucFlip=0, ucFlop=0;

  strcpy(chosen_filename, "tmpz");
  intvFindFiles();   // Initial get of files...
    
  dsShowBannerScreen();

  nbRomPerPage = (countintv>=17 ? 17 : countintv);
  uNbRSPage = (countintv>=5 ? 5 : countintv);
  
  if (ucFicAct>countintv-nbRomPerPage)
  {
    firstRomDisplay=countintv-nbRomPerPage;
    romSelected=ucFicAct-countintv+nbRomPerPage;
  }
  else
  {
    firstRomDisplay=ucFicAct;
    romSelected=0;
  }
  dsDisplayFiles(firstRomDisplay,romSelected);
    
  // -----------------------------------------------------
  // Until the user selects a file or exits the menu...
  // -----------------------------------------------------
  while (!bDone)
  {
    if (keysCurrent() & KEY_UP)
    {
      if (!ucHaut)
      {
        ucFicAct = (ucFicAct>0 ? ucFicAct-1 : countintv-1);
        if (romSelected>uNbRSPage) { romSelected -= 1; }
        else {
          if (firstRomDisplay>0) { firstRomDisplay -= 1; }
          else {
            if (romSelected>0) { romSelected -= 1; }
            else {
              firstRomDisplay=countintv-nbRomPerPage;
              romSelected=nbRomPerPage-1;
            }
          }
        }
        ucHaut=0x01;
        dsDisplayFiles(firstRomDisplay,romSelected);
      }
      else {

        ucHaut++;
        if (ucHaut>10) ucHaut=0;
      }
      uLenFic=0; ucFlip=0; ucFlop=0;     
    }
    else
    {
      ucHaut = 0;
    }
    if (keysCurrent() & KEY_DOWN)
    {
      if (!ucBas) {
        ucFicAct = (ucFicAct< countintv-1 ? ucFicAct+1 : 0);
        if (romSelected<uNbRSPage-1) { romSelected += 1; }
        else {
          if (firstRomDisplay<countintv-nbRomPerPage) { firstRomDisplay += 1; }
          else {
            if (romSelected<nbRomPerPage-1) { romSelected += 1; }
            else {
              firstRomDisplay=0;
              romSelected=0;
            }
          }
        }
        ucBas=0x01;
        dsDisplayFiles(firstRomDisplay,romSelected);
      }
      else
      {
        ucBas++;
        if (ucBas>10) ucBas=0;
      }
      uLenFic=0; ucFlip=0; ucFlop=0;     
    }
    else {
      ucBas = 0;
    }
      
    // -------------------------------------------------------------
    // Left and Right on the D-Pad will scroll 1 page at a time...
    // -------------------------------------------------------------
    if (keysCurrent() & KEY_RIGHT)
    {
      if (!ucSBas)
      {
        ucFicAct = (ucFicAct< countintv-nbRomPerPage ? ucFicAct+nbRomPerPage : countintv-nbRomPerPage);
        if (firstRomDisplay<countintv-nbRomPerPage) { firstRomDisplay += nbRomPerPage; }
        else { firstRomDisplay = countintv-nbRomPerPage; }
        if (ucFicAct == countintv-nbRomPerPage) romSelected = 0;
        ucSBas=0x01;
        dsDisplayFiles(firstRomDisplay,romSelected);
      }
      else
      {
        ucSBas++;
        if (ucSBas>10) ucSBas=0;
      }
      uLenFic=0; ucFlip=0; ucFlop=0;     
    }
    else {
      ucSBas = 0;
    }
      
    // -------------------------------------------------------------
    // Left and Right on the D-Pad will scroll 1 page at a time...
    // -------------------------------------------------------------
    if (keysCurrent() & KEY_LEFT)
    {
      if (!ucSHaut)
      {
        ucFicAct = (ucFicAct> nbRomPerPage ? ucFicAct-nbRomPerPage : 0);
        if (firstRomDisplay>nbRomPerPage) { firstRomDisplay -= nbRomPerPage; }
        else { firstRomDisplay = 0; }
        if (ucFicAct == 0) romSelected = 0;
        if (romSelected > ucFicAct) romSelected = ucFicAct;          
        ucSHaut=0x01;
        dsDisplayFiles(firstRomDisplay,romSelected);
      }
      else
      {
        ucSHaut++;
        if (ucSHaut>10) ucSHaut=0;
      }
      uLenFic=0; ucFlip=0; ucFlop=0;     
    }
    else {
      ucSHaut = 0;
    }
    
    // --------------------------------------------------------------------
    // L/R Shoulder Buttons will toggle between all games and just favs...
    // --------------------------------------------------------------------
    if (keysCurrent() & (KEY_L | KEY_R))
    {
        bFavsOnlyMode = !bFavsOnlyMode;
        intvFindFiles();
        ucFicAct = 0;
        nbRomPerPage = (countintv>=16 ? 16 : countintv);
        uNbRSPage = (countintv>=5 ? 5 : countintv);
        if (ucFicAct>countintv-nbRomPerPage) {
          firstRomDisplay=countintv-nbRomPerPage;
          romSelected=ucFicAct-countintv+nbRomPerPage;
        }
        else {
          firstRomDisplay=ucFicAct;
          romSelected=0;
        }
        dsDisplayFiles(firstRomDisplay,romSelected);
        while (keysCurrent() & (KEY_L | KEY_R))
            ;
        WAITVBL;
    }

    // -------------------------------------------------------------------------
    // They B key will exit out of the ROM selection without picking a new game
    // -------------------------------------------------------------------------
    if ( keysCurrent() & KEY_B )
    {
      bDone=true;
      while (keysCurrent() & KEY_B);
    }
      
    // -------------------------------------------------------
    // The SELECT key will toggle this game as a 'favorite'
    // -------------------------------------------------------
    if ( keysCurrent() & KEY_SELECT )
    {
        if (!intvromlist[ucFicAct].directory)
        {
            if (intvromlist[ucFicAct].favorite)
                clrFavorite(intvromlist[ucFicAct].filename);
            else
                setFavorite(intvromlist[ucFicAct].filename);
            intvromlist[ucFicAct].favorite = isFavorite(intvromlist[ucFicAct].filename);
            dsDisplayFiles(firstRomDisplay,romSelected);
            while (keysCurrent() & KEY_SELECT);
        }
    }

    // --------------------------------------------------------------------------
    // Since the user can select favorites, we also allow saving those out here.
    // --------------------------------------------------------------------------
    if ( keysCurrent() & KEY_START )
    {
        dsPrintValue(0,23,0, (char*)"     SAVING CONFIGURATION       ");
        SaveConfig(false);
        WAITVBL;WAITVBL;WAITVBL;
        dsDisplayLoadInstructions();
        while (keysCurrent() & KEY_START);
    }
      
    // -------------------------------------------------------------------
    // Any of these keys will pick the current ROM and try to load it...
    // -------------------------------------------------------------------
    if (keysCurrent() & KEY_A || keysCurrent() & KEY_X)
    {
      if (!intvromlist[ucFicAct].directory)
      {
          UINT8 opt = 0;
          bUseJLP=false;
          bUseIVoice=false;
          bUseECS=false;
          if (keysCurrent() & KEY_X)
          {
              opt = LoadWithOptions();
              if (opt == 0) {asm("nop");}
              if (opt == 1) {bUseJLP = true;}
              if (opt == 2) {bUseIVoice = true;}
              if (opt == 3) {bUseECS = true;}
              if (opt == 4) {bUseJLP = true; bUseIVoice = true;}
              if (opt == 5) {bUseECS = true; bUseIVoice = true;}
              if (opt == 6) {bUseJLP = true; bUseECS = true; bUseIVoice = true;}
              while (keysCurrent() & KEY_A);
              WAITVBL;
          }
          if (opt != (LOAD_OPTION_MENU_ITEMS-1))
          {
              bRet=true;
              bDone=true;
              strcpy(chosen_filename,  intvromlist[ucFicAct].filename);
          }
          else
          {
              bRet = false;
              bDone = false;
              dsShowBannerScreen();
              dsDisplayFiles(firstRomDisplay,romSelected);
          }
          WAITVBL;
      }
      else
      {
        chdir(intvromlist[ucFicAct].filename);
        intvFindFiles();
        ucFicAct = 0;
        nbRomPerPage = (countintv>=16 ? 16 : countintv);
        uNbRSPage = (countintv>=5 ? 5 : countintv);
        if (ucFicAct>countintv-nbRomPerPage) {
          firstRomDisplay=countintv-nbRomPerPage;
          romSelected=ucFicAct-countintv+nbRomPerPage;
        }
        else {
          firstRomDisplay=ucFicAct;
          romSelected=0;
        }
        dsDisplayFiles(firstRomDisplay,romSelected);
        while (keysCurrent() & KEY_A);
      }
    }
    
    // --------------------------------------------
    // If the filename is too long... scroll it.
    // --------------------------------------------
    if (strlen(intvromlist[ucFicAct].filename) > 29) 
    {
      ucFlip++;
      if (ucFlip >= 15) 
      {
        ucFlip = 0;
        uLenFic++;
        if ((uLenFic+29)>strlen(intvromlist[ucFicAct].filename)) 
        {
          ucFlop++;
          if (ucFlop >= 15) 
          {
            uLenFic=0;
            ucFlop = 0;
          }
          else
            uLenFic--;
        }
        strncpy(szName,intvromlist[ucFicAct].filename+uLenFic,29);
        szName[29] = '\0';
        dsPrintValue(1,4+romSelected,1,szName);
      }
    }
      
    swiWaitForVBlank();
  }

  // ----------------------------------------------------------------------
  // We are going back to the main emulation now - restore bottom screen.
  // ----------------------------------------------------------------------
  dsShowScreenMain(false);

  return bRet;
}

// End of Line
