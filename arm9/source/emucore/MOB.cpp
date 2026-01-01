// =====================================================================================
// Copyright (c) 2021-2026 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

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

void MOB::setXLocation(INT16 xLocation) {
    if (xLocation == this->xLocation)
        return;

    this->xLocation = xLocation;
    boundsChanged = TRUE;
}

void MOB::setYLocation(INT16 yLocation) {
    if (yLocation == this->yLocation)
        return;

    this->yLocation = yLocation;
    boundsChanged = TRUE;
}


void MOB::setForegroundColor(UINT8 foregroundColor) {
    if (foregroundColor == this->foregroundColor)
        return;

    this->foregroundColor = foregroundColor;
    colorChanged = TRUE;
}

void MOB::setCardNumber(INT16 cardNumber) {
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

MOBRect* MOB::getBounds() 
{
    if (boundsChanged) 
    {
        boundingRectangle.x = xLocation-8;
        boundingRectangle.y = yLocation-8;
        boundingRectangle.width = 8 * (doubleWidth ? 2 : 1);
        boundingRectangle.height = 4 * (quadHeight ? 4 : 1) *
                (doubleHeight ? 2 : 1) * (doubleYResolution ? 2 : 1);
        boundsChanged = FALSE;
    }
    return &boundingRectangle;
}

void MOB::getState(MOBState *state)
{
    state->xLocation = xLocation;
    state->yLocation = yLocation;
    state->foregroundColor = foregroundColor;
    state->cardNumber = cardNumber;
    state->collisionRegister = collisionRegister;
    state->isGrom = isGrom;
    state->isVisible = isVisible;
    state->doubleWidth = doubleWidth;
    state->doubleYResolution = doubleYResolution;
    state->doubleHeight = doubleHeight;
    state->quadHeight = quadHeight;
    state->flagCollisions = flagCollisions;
    state->horizontalMirror = horizontalMirror;
    state->verticalMirror = verticalMirror;
    state->behindForeground = behindForeground;

    // Probably don't need to save the bounding rectangle since
    // it gets recalculated every frame - but it's small enough...
    state->mob_rect_x      = boundingRectangle.x;
    state->mob_rect_y      = boundingRectangle.y;
    state->mob_rect_width  = boundingRectangle.width;
    state->mob_rect_height = boundingRectangle.height;
}

void MOB::setState(MOBState *state)
{
    xLocation = state->xLocation;
    yLocation = state->yLocation;
    foregroundColor = state->foregroundColor;
    cardNumber = state->cardNumber;
    collisionRegister = state->collisionRegister;
    isGrom = state->isGrom;
    isVisible = state->isVisible;
    doubleWidth = state->doubleWidth;
    doubleYResolution = state->doubleYResolution;
    doubleHeight = state->doubleHeight;
    quadHeight = state->quadHeight;
    flagCollisions = state->flagCollisions;
    horizontalMirror = state->horizontalMirror;
    verticalMirror = state->verticalMirror;
    behindForeground = state->behindForeground;
    
    boundingRectangle.x = state->mob_rect_x;
    boundingRectangle.y = state->mob_rect_y;
    boundingRectangle.width = state->mob_rect_width;
    boundingRectangle.height = state->mob_rect_height;    

    this->boundsChanged = TRUE;
    this->shapeChanged = TRUE;
    this->colorChanged = TRUE;
}
