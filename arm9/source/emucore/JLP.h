// =====================================================================================
// Copyright (c) 2021-2023 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef JLP_H
#define JLP_H

#include <string.h>
#include "RAM.h"

#define JLP_RAM_SIZE            (0x2000)
#define JLP_RAM_ADDRESS         (0x8000)
#define JLP_FLASH_SIZE          (64*1024)
#define JLP_CRC_POLY            (0xAD52)
#define JLP_NUM_ROWS            (336)
#define JLP_NUM_BYTES_PER_ROW   (192)
#define JLP_NUM_SECTORS         (42)        // 1.5K per sector. 336x129=64512 bytes (roughly 64K) divided by 1513 bytes = 42

TYPEDEF_STRUCT_PACK( _JLPState
{
    UINT16     jlp_ram[JLP_RAM_SIZE];
} JLPState; )

class JLP : public RAM
{
    public:
        JLP();

        void reset();
        UINT16 peek(UINT16 location);
        void poke(UINT16 location, UINT16 value);
        void tick_one_second(void);
        
        void getState(JLPState *state);
        void setState(JLPState *state);

    private:
        UINT8 jlp_flash[JLP_FLASH_SIZE];
        UINT32 crc16(UINT16 data, UINT16 crc);
        void RamToFlash(void);
        void FlashToRam(void);
        void EraseSector(void);
        void ReadFlashFile(void);
        void WriteFlashFile(void);
        void ScheduleWriteFlashFile(void);
        void GetFlashFilename(void);
        UINT8 flash_read;
        UINT8 flash_write_time;
};

#endif
