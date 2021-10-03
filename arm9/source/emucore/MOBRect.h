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

#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "types.h"

class MOBRect
{

    public:
        BOOL intersects(MOBRect* r) {
	        return !((r->x + r->width <= x) ||
		         (r->y + r->height <= y) ||
		         (r->x >= x + width) ||
		         (r->y >= y + height));
        }

        INT32 x;
        INT32 y;
        INT32 width;
        INT32 height;

};

#endif
