// =====================================================================================
// Copyright (c) 2021-2025 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef CRC32_H
#define CRC32_H

#include "types.h"

class CRC32
{

    public:
        CRC32();
        void reset();
        void update(UINT8 data);
        void update(UINT8* data, UINT32 length);
        UINT32 getValue();
        static UINT32 getCrc(const CHAR* filename);
        static UINT32 getCrc(UINT8* image, UINT32 imageSize);

    private:
        //static void formInternalFilename(CHAR*, const CHAR*, const CHAR*);

        UINT32 crc;
};

#endif

