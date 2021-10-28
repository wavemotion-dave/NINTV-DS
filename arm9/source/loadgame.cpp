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
#include "CRC32.h"
#include "loadgame.h"
#include "debugger.h"

FICA_INTV intvromlist[512];
unsigned short int countintv=0, ucFicAct=0;
char szName[256];
char szName2[256];

extern Rip *currentRip;

bool bFavsOnlyMode = false;

BOOL LoadCart(const CHAR* filename)
{
    if (strlen(filename) < 5)
        return FALSE;

    //convert .bin and .rom to .rip, since our emulation only knows how to run .rip
    const CHAR* extStart = filename + strlen(filename) - 4;
    if (strcmpi(extStart, ".int") == 0 || strcmpi(extStart, ".bin") == 0)
    {
        //load the bin file as a Rip - use internal database or maybe <filename>.cfg exists... LoadBin() handles all that.
        currentRip = Rip::LoadBin(filename);
        if (currentRip == NULL)
        {
            return FALSE;
        }
    }
    else if (strcmpi(extStart, ".rom") == 0)    // .rom files contain the loading info...
    {
        //load the bin file as a Rip
        currentRip = Rip::LoadRom(filename);
        if (currentRip == NULL)
        {
            return FALSE;
        }
    }
    else
    {
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


void dsDisplayLoadInstructions(void)
{
  dsPrintValue(1,22,0,(char*)"SEL=MARK, STA=SAVEFAV, L/R=FAVS");
  dsPrintValue(1,23,0,(char*)"A=LOAD, X=JLP, Y=IVOICE, B=BACK");
}

// Find files (.int) available
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
        // Filter out the BIOS files from the list...
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
  if (countintv)
    qsort (intvromlist, countintv, sizeof (FICA_INTV), intvFilescmp);
}

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



unsigned int dsWaitForRom(char *chosen_filename)
{
  bool bDone=false, bRet=false;
  u32 ucHaut=0x00, ucBas=0x00,ucSHaut=0x00, ucSBas=0x00,romSelected= 0, firstRomDisplay=0,nbRomPerPage, uNbRSPage;
  u32 uLenFic=0, ucFlip=0, ucFlop=0;

  strcpy(chosen_filename, "tmpz");
  intvFindFiles();   // Initial get of files...
    
  dsShowMenu();

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

    if ( keysCurrent() & KEY_B )
    {
      bDone=true;
      while (keysCurrent() & KEY_B);
    }
      
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

    if ( keysCurrent() & KEY_START )
    {
        dsPrintValue(0,23,0, (char*)"     SAVING CONFIGURATION       ");
        SaveConfig(false);
        WAITVBL;WAITVBL;WAITVBL;
        dsDisplayLoadInstructions();
        while (keysCurrent() & KEY_START);
    }
      
    if (keysCurrent() & KEY_A || keysCurrent() & KEY_Y || keysCurrent() & KEY_X)
    {
      if (!intvromlist[ucFicAct].directory)
      {
        bRet=true;
        bDone=true;
        WAITVBL;
        if (keysCurrent() & KEY_X) bUseJLP = true; else bUseJLP=false;
        if (keysCurrent() & KEY_Y) bForceIvoice = true; else bForceIvoice=false;          
        strcpy(chosen_filename,  intvromlist[ucFicAct].filename);
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
      
    // If the filename is too long... scroll it.
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

  dsShowScreenMain(false);

  return bRet;
}

// End of Line

