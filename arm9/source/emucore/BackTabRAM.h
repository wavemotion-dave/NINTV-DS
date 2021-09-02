
#ifndef BACKTABRAM_H
#define BACKTABRAM_H

#include "RAM.h"

#define BACKTAB_SIZE     0xF0
#define BACKTAB_LOCATION 0x200

TYPEDEF_STRUCT_PACK(_BackTabRAMState
{
    _RAMState     RAMState;
    UINT16       image[BACKTAB_SIZE];
} BackTabRAMState;)

class BackTabRAM : public RAM
{

    public:
        BackTabRAM();
        void reset();

        UINT16 peek(UINT16 location);
        void poke(UINT16 location, UINT16 value);

        BOOL areColorAdvanceBitsDirty();
        BOOL isDirty();
        BOOL isDirty(UINT16 location);
        void markClean();

        inline size_t getImageByteSize() {
            return size * sizeof(UINT16);
        }
        void getImage(void* dst, UINT16 offset, UINT16 size) {
            memcpy(dst, image + offset, size);
        }
        void setImage(void* src, UINT16 offset, UINT16 size) {
            memcpy(image + offset, src, size);
        }

        BackTabRAMState getState();
        void setState(BackTabRAMState state, UINT16* image);

    private:
        UINT16       image[BACKTAB_SIZE];
        BOOL         dirtyBytes[BACKTAB_SIZE];
        BOOL         dirtyRAM;
        BOOL         colorAdvanceBitsDirty;

};

#endif
