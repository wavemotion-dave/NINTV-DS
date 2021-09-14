
#ifndef SPRITE_H
#define SPRITE_H

#include "MOBRect.h"
#include "types.h"

class AY38900;
class AY38900_Registers;

class MOB
{

    friend class AY38900;
    friend class AY38900_Registers;

    private:
        MOB() {};
        void reset();
        void setXLocation(INT32 xLocation);
        void setYLocation(INT32 yLocation);
        void setForegroundColor(INT32 foregroundColor);
        void setCardNumber(INT32 cardNumber);
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

        INT32   xLocation;
        INT32   yLocation;
        INT32   foregroundColor;
        INT32   cardNumber;
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
