
#include "GROM.h"

GROM::GROM()
: ROM("GROM", "grom.bin", 0, 1, GROM_SIZE, GROM_ADDRESS)
{}

void GROM::reset()
{
    visible = TRUE;
}




