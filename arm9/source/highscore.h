#ifndef __HIGHSCORE_H
#define __HIGHSCORE_H

#include <nds.h>
#include "types.h"

extern void highscore_init(void);
extern void highscore_save(void);
extern void highscore_display(UINT32 crc);

#endif
