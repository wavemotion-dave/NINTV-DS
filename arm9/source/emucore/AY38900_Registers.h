
#ifndef AY38900_REGISTERS_H
#define AY38900_REGISTERS_H

#include "RAM.h"

class AY38900;

extern UINT16   memory[0x40];
class AY38900_Registers : public RAM
{
    friend class AY38900;

    public:
        void reset();

        void poke(UINT16 location, UINT16 value);
        UINT16 peek(UINT16 location);

        inline size_t getMemoryByteSize() {
            return sizeof(memory);
        }
        void getMemory(void* dst, UINT16 offset, UINT16 size) {
            memcpy(dst, memory + offset, size);
        }
        void setMemory(void* src, UINT16 offset, UINT16 size) {
            memcpy(memory + offset, src, size);
        }

    private:
        AY38900_Registers();
        void init(AY38900* ay38900);

        AY38900* ay38900;
};

#endif
