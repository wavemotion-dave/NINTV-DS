
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
