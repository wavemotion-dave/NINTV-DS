// =====================================================================================
// Copyright (c) 2021-2023 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef SPRITE_H
#define SPRITE_H

#include "types.h"

class MOBRect
{
    public:
        inline BOOL intersects(MOBRect* r) 
        {
            return !((r->x + r->width <= x) || (r->y + r->height <= y) || (r->x >= x + width) || (r->y >= y + height));
        }
        INT16 x;
        INT16 y;
        UINT8 width;
        UINT8 height;
};

class AY38900;
class AY38900_Registers;

TYPEDEF_STRUCT_PACK( _MOBState
{
    INT16   xLocation;
    INT16   yLocation;
    INT16   cardNumber;
    UINT16  collisionRegister;
    UINT8   foregroundColor;
    INT8    isGrom;
    INT8    isVisible;
    INT8    doubleWidth;
    INT8    doubleYResolution;
    INT8    doubleHeight;
    INT8    quadHeight;
    INT8    flagCollisions;
    INT8    horizontalMirror;
    INT8    verticalMirror;
    INT8    behindForeground;
    INT16   mob_rect_x;
    INT16   mob_rect_y;
    UINT8   mob_rect_width;
    UINT8   mob_rect_height;
} MOBState; )
    
class MOB
{
    friend class AY38900;
    friend class AY38900_Registers;

    public:
        void getState(MOBState *state);
        void setState(MOBState *state);

    private:
        MOB() {};
        void reset();
        void setXLocation(INT16 xLocation);
        void setYLocation(INT16 yLocation);
        void setForegroundColor(UINT8 foregroundColor);
        void setCardNumber(INT16 cardNumber);
        void setVisible(BOOL isVisible);
        void setDoubleWidth(BOOL doubleWidth);
        void setDoubleYResolution(BOOL doubleYResolution);
        void setDoubleHeight(BOOL doubleHeight);
        void setQuadHeight(BOOL quadHeight);
        void setFlagCollisions(BOOL flagCollisions);
        void setHorizontalMirror(BOOL horizontalMirror);
        void setVerticalMirror(BOOL verticalMirror);
        void setBehindForeground(BOOL behindForeground);
        void setGROM(BOOL grom);
        void markClean();
        MOBRect* getBounds();
    public:
        INT16   xLocation;
        INT16   yLocation;
        UINT16  foregroundColor;
        INT16   cardNumber;
        UINT16  collisionRegister;
        UINT8   isGrom;
        UINT8   isVisible;
        UINT8   doubleWidth;
        UINT8   doubleYResolution;
        UINT8   doubleHeight;
        UINT8   quadHeight;
        UINT8   flagCollisions;
        UINT8   horizontalMirror;
        UINT8   verticalMirror;
        UINT8   behindForeground;
        UINT8   boundsChanged;
        UINT8   shapeChanged;
        UINT8   colorChanged;
        MOBRect boundingRectangle;
};

#endif
