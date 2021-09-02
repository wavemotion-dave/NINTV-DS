
#include <string.h>
#include "ripxbf.h"

UINT64 freadInt(FILE* file, UINT32 size)
{
    unsigned int i;
    unsigned int ret = 0;
    for (i = 0; i < size; i++) {
        ret <<= 8;
        ret |= fgetc(file);
    }
    return ret;
}

UINT16 freadUINT16(FILE* file)
{
    return (UINT16)freadInt(file, 2);
}

UINT32 freadUINT32(FILE* file)
{
    return (UINT32)freadInt(file, 4);
}

UINT64 freadUINT64(FILE* file)
{
    return freadInt(file, 8);
}

void freadString(FILE* file, CHAR* out, UINT32 maxLength)
{
    unsigned int i;
    unsigned char next;
    unsigned int completeLength = freadUINT32(file);
    if (completeLength+1 < maxLength)
        maxLength = completeLength+1;

    for (i = 0; i < completeLength; i++) {
        next = (unsigned char)fgetc(file);
        if (i < (maxLength-1))
            out[i] = next;
    }
    out[maxLength-1] = '\0';
}

void fwriteUINT16(FILE* file, UINT16 num)
{
    fputc((num>>8)&0xFF, file);
    fputc(num&0xFF, file);
}
    
void fwriteUINT32(FILE* file, UINT32 num)
{
    fputc((num>>24)&0xFF, file);
    fputc((num>>16)&0xFF, file);
    fputc((num>>8)&0xFF, file);
    fputc(num&0xFF, file);
}
    
void fwriteUINT64(FILE* file, UINT64 num)
{
    fputc((int)(num>>56)&0xFF, file);
    fputc((int)(num>>48)&0xFF, file);
    fputc((int)(num>>40)&0xFF, file);
    fputc((int)(num>>32)&0xFF, file);
    fputc((int)(num>>24)&0xFF, file);
    fputc((int)(num>>16)&0xFF, file);
    fputc((int)(num>>8)&0xFF, file);
    fputc((int)num&0xFF, file);
}

void fwriteString(FILE* file, const CHAR* s)
{
    int i;
    int length = (int)strlen(s);
    fwriteUINT32(file, length);
    for (i = 0; i < length; i++)
        fputc(s[i], file);
}
    
/*
RipSystem SYSTEMS[] =
{
  { ID_SYSTEM_INTELLIVISION,
    "Intellivision",
    2, 2,
    { { "Master Component",
        { { "Executive ROM",
            { { "Original Executive ROM", 0xCBCE86F7 },
              { "Intellivision 2 Executive ROM", 0 },
              { "Sears Executive ROM", 0 },
              { "GPL Executive ROM", 0 },
              0,
          },},
          { "Graphics ROM",
            { { "Original GROM", 0x683A4158 },
              { "GPL GROM", 0 },
              0,
          },},
          0,
      },},
      { "Entertainment Computer System",
        { { "ECS BIOS",
            { { "Original ECS BIOS", 0xEA790A06 },
          },},
          0,
      },},
      { "Intellivoice",
        { { "Intellivoice BIOS",
            { { "Original Intellivoice BIOS", 0x0DE7579D },
              0,
          },},
          0,
      },},
      0,
  },},
  { ID_SYSTEM_ATARI5200,
    "Atari 5200",
    2, 1,
    { { "Main Console",
        { { "BIOS",
            { { "Original BIOS", 0x4248D3E3 },
              0,
          },},
          0,
      },},
      0,
  },},
  0,
};


RipSystem* getSystem(UINT32 systemID)
{
    int i = 0;
    while (*((int*)(&SYSTEMS[i])) != 0) {
        if (SYSTEMS[i].systemID == systemID)
            return &SYSTEMS[i];
        i++;
    }
    return NULL;
}
*/
