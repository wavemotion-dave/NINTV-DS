#ifndef __DS_TOOLS_H
#define __DS_TOOLS_H

#include <nds.h>

#define STELLADS_MENUINIT 0x01
#define STELLADS_MENUSHOW 0x02
#define STELLADS_PLAYINIT 0x03 
#define STELLADS_PLAYGAME 0x04 
#define STELLADS_QUITSTDS 0x05

extern unsigned int etatEmu;

typedef enum {
  EMUARM7_INIT_SND = 0x123C,
  EMUARM7_STOP_SND = 0x123D,
  EMUARM7_PLAY_SND = 0x123E,
} FifoMesType;

typedef struct FICtoLoad {
  char filename[255];
  bool directory;
} FICA2600;

extern void dsPrintValue(int x, int y, unsigned int isSelect, char *pchStr);
extern void dsMainLoop(void);
extern void dsInstallSoundEmuFIFO(void);
extern void dsInitScreenMain(void);
extern void dsInitTimer(void);
extern void dsFreeEmu(void);
extern void dsShowScreenEmu(void);
extern bool dsWaitOnQuit(void);
extern void dsChooseOptions(void);
extern unsigned int dsWaitForRom(char *chosen_filename);

#endif
