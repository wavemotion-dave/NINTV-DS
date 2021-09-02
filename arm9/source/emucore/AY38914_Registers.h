
#ifndef AY38914_REGISTERS_H
#define AY38914_REGISTERS_H

#include "RAM.h"

class AY38914;

class AY38914_Registers : public RAM
{
    friend class AY38914;

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
        AY38914_Registers(UINT16 address);
        void init(AY38914* ay38914);

        AY38914* ay38914;
        UINT16   memory[0x0E];
};

#endif
