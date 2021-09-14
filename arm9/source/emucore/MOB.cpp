
#include "MOB.h"

void MOB::reset() {
    xLocation         = 0;
    yLocation         = 0;
    foregroundColor   = 0;
    cardNumber        = 0;
    collisionRegister = 0;
    isGrom            = FALSE;
    isVisible         = FALSE;
    doubleWidth       = FALSE;
    doubleYResolution = FALSE;
    doubleHeight      = FALSE;
    quadHeight        = FALSE;
    flagCollisions    = FALSE;
    horizontalMirror  = FALSE;
    verticalMirror    = FALSE;
    behindForeground  = FALSE;
    boundingRectangle.x = 0;
    boundingRectangle.y = 0;
    boundingRectangle.width = 0;
    boundingRectangle.height = 0;
    shapeChanged      = TRUE;
    boundsChanged     = TRUE;
    colorChanged      = TRUE;
}

void MOB::setXLocation(INT32 xLocation) {
    if (xLocation == this->xLocation)
        return;

    this->xLocation = xLocation;
    boundsChanged = TRUE;
}

void MOB::setYLocation(INT32 yLocation) {
    if (yLocation == this->yLocation)
        return;

    this->yLocation = yLocation;
    boundsChanged = TRUE;
}


void MOB::setForegroundColor(INT32 foregroundColor) {
    if (foregroundColor == this->foregroundColor)
        return;

    this->foregroundColor = foregroundColor;
    colorChanged = TRUE;
}

void MOB::setCardNumber(INT32 cardNumber) {
    if (cardNumber == this->cardNumber)
        return;

    this->cardNumber = cardNumber;
    shapeChanged = TRUE;
}

void MOB::setVisible(BOOL isVisible) {
    if (isVisible == this->isVisible)
        return;

    this->isVisible = isVisible;
}

void MOB::setDoubleWidth(BOOL doubleWidth) {
    if (doubleWidth == this->doubleWidth)
        return;

    this->doubleWidth = doubleWidth;
    boundsChanged = TRUE;
    shapeChanged = TRUE;
}

void MOB::setDoubleYResolution(BOOL doubleYResolution) {
    if (doubleYResolution == this->doubleYResolution)
        return;

    this->doubleYResolution = doubleYResolution;
    boundsChanged = TRUE;
    shapeChanged = TRUE;
}

void MOB::setDoubleHeight(BOOL doubleHeight) {
    if (doubleHeight == this->doubleHeight)
        return;

    this->doubleHeight = doubleHeight;
    boundsChanged = TRUE;
    shapeChanged = TRUE;
}

void MOB::setQuadHeight(BOOL quadHeight) {
    if (quadHeight == this->quadHeight)
        return;

    this->quadHeight = quadHeight;
    boundsChanged = TRUE;
    shapeChanged = TRUE;
}

void MOB::setFlagCollisions(BOOL flagCollisions) {
    if (flagCollisions == this->flagCollisions)
        return;

    this->flagCollisions = flagCollisions;
}

void MOB::setHorizontalMirror(BOOL horizontalMirror) {
    if (horizontalMirror == this->horizontalMirror)
        return;

    this->horizontalMirror = horizontalMirror;
    shapeChanged = TRUE;
}

void MOB::setVerticalMirror(BOOL verticalMirror) {
    if (verticalMirror == this->verticalMirror)
        return;

    this->verticalMirror = verticalMirror;
    shapeChanged = TRUE;
}

void MOB::setBehindForeground(BOOL behindForeground) {
    if (behindForeground == this->behindForeground)
        return;

    this->behindForeground = behindForeground;
}

void MOB::setGROM(BOOL grom) {
    if (grom == this->isGrom)
        return;

    this->isGrom = grom;
    shapeChanged = TRUE;
}

void MOB::markClean() {
    shapeChanged = FALSE;
    colorChanged = FALSE;
}

MOBRect* MOB::getBounds() {
    if (boundsChanged) {
        boundingRectangle.x = xLocation-8;
        boundingRectangle.y = yLocation-8;
        boundingRectangle.width = 8 * (doubleWidth ? 2 : 1);
        boundingRectangle.height = 4 * (quadHeight ? 4 : 1) *
                (doubleHeight ? 2 : 1) * (doubleYResolution ? 2 : 1);
        boundsChanged = FALSE;
    }
    return &boundingRectangle;
}
