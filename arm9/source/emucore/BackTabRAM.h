
#ifndef BACKTABRAM_H
#define BACKTABRAM_H

#include "RAM.h"

#define BACKTAB_SIZE     0xF0
#define BACKTAB_LOCATION 0x200

TYPEDEF_STRUCT_PACK(_BackTabRAMState
{
    UINT16       image[BACKTAB_SIZE];
    BOOL         dirtyBytes[BACKTAB_SIZE];
    BOOL         dirtyRAM;
    BOOL         colorAdvanceBitsDirty;
} BackTabRAMState;)

class BackTabRAM : public RAM
{
    public:
        BackTabRAM();
        void reset();

        UINT16 peek(UINT16 location);
        void poke(UINT16 location, UINT16 value);
        UINT16 peek_direct(UINT16 offset) { return image[offset]; }

        BOOL areColorAdvanceBitsDirty();
        BOOL isDirty();
        BOOL isDirty(UINT16 location);
        void markClean();
        
        void getState(BackTabRAMState *state);
        void setState(BackTabRAMState *state);

    private:
        UINT16       image[BACKTAB_SIZE];
        BOOL         dirtyBytes[BACKTAB_SIZE];
        BOOL         dirtyRAM;
        BOOL         colorAdvanceBitsDirty;
};

#endif
