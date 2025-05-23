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

#ifndef __OVERLAY_H
#define __OVERLAY_H

#include <nds.h>
#include "types.h"

// -------------------------------------------------
// Overlay handling... This defines a 'hot spot' 
// which can be presed on the touch screen.
// -------------------------------------------------
struct Overlay_t
{
    UINT8   x1;
    UINT8   x2;
    UINT8   y1;
    UINT8   y2;
};

#define OVL_KEY_1        0
#define OVL_KEY_2        1
#define OVL_KEY_3        2
#define OVL_KEY_4        3
#define OVL_KEY_5        4
#define OVL_KEY_6        5
#define OVL_KEY_7        6
#define OVL_KEY_8        7
#define OVL_KEY_9        8
#define OVL_KEY_CLEAR    9
#define OVL_KEY_0        10
#define OVL_KEY_ENTER    11
 
#define OVL_BTN_FIRE     12
#define OVL_BTN_L_ACT    13
#define OVL_BTN_R_ACT    14
 
#define OVL_META_RESET    15
#define OVL_META_LOAD     16
#define OVL_META_CONFIG   17
#define OVL_META_SCORES   18
#define OVL_META_QUIT     19
#define OVL_META_STATE    20
#define OVL_META_MENU     21
#define OVL_META_SWITCH   22
#define OVL_META_MANUAL   23
#define OVL_META_DISC     24
#define OVL_META_KEYBOARD 25
#define OVL_META_SWAPOVL  26    // Used for multi-overlay games (Triple Challenge, Mr BASIC, etc)

// These are not likely mapped anywhere... keep them near the back end
#define OVL_META_DISC_UP  27
#define OVL_META_DISC_DN  28
#define OVL_META_SPEEDUP  29
#define OVL_META_FASTLOAD 30
#define OVL_META_STRETCH  31
#define OVL_META_GCONFIG  32
#define OVL_META_CHEATS   33
#define OVL_META_EMUINFO  34
#define OVL_META_PICKOVL  35    // Let the user pick any .ovl file

#define OVL_MAX           36
#define DISC_MAX          16


// For the various controller types...
#define CONTROLLER_P1               0
#define CONTROLLER_P2               1
#define CONTROLLER_DUAL_ACTION_A    2
#define CONTROLLER_DUAL_ACTION_B    3

extern UINT8 bShowSynthesizer;

extern struct Overlay_t myOverlay[OVL_MAX];
extern struct Overlay_t myDisc[DISC_MAX];

extern void load_custom_overlay(void);
extern void show_overlay(u8 bShowKeyboard, u8 bShowDisc);
extern u8   pick_overlay(void);
extern void swap_overlay(void);

#endif
