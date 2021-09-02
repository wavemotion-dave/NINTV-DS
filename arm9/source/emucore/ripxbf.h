/**
 *  This file contains a list of declarations useful for working with
 *  files in the XBF format.
 *
 *      Author:  Kyle Davis
 */

#include <stdio.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

//compatibility flags
typedef enum _BiosCompatibility
{
    BIOS_PREFERRED = 0,
    BIOS_COMPATIBLE = 1,
    BIOS_INCOMPATIBLE = 2
} BiosCompatibility;

typedef enum _PeripheralCompatibility
{
    PERIPH_REQUIRED = 0,
    PERIPH_OPTIONAL = 1,
    PERIPH_COMPATIBLE = 2,
    PERIPH_INCOMPATIBLE = 3
} PeripheralCompatibility;

UINT64 freadInt(FILE*, UINT32 length);
UINT16 freadUINT16(FILE*);
UINT32 freadUINT32(FILE*);
UINT64 freadUINT64(FILE*);
void freadString(FILE*, CHAR* s, UINT32 maxLength);
void fwriteUINT16(FILE*, UINT16);
void fwriteUINT32(FILE*, UINT32);
void fwriteUINT64(FILE*, UINT64);
void fwriteString(FILE*, const CHAR*);

//the RIP magic number
#define RIP_MAGIC_NUMBER 0x3F383A347651B5DAll
#define RIP_MAJOR_REVISION 1
#define RIP_MINOR_REVISION 0

//record IDs
#define ID_HEADER_RECORD            0xFC41466E
#define ID_BIOS_COMPAT_RECORD       0xC183BF3C
#define ID_NAME_RECORD              0x5ECC1C53
#define ID_PERIPH_COMPAT_RECORD     0xAE4CEDB7
#define ID_PRODUCER_RECORD          0x86596370
#define ID_YEAR_RECORD              0x9199748D
#define ID_RAM_RECORD               0xCF1DC943
#define ID_ROM_RECORD               0xA365AF69

//emulator IDs
#define ID_EMULATOR_BLISS           0xC183BF3C

//system IDs
#define ID_SYSTEM_ATARI5200         0xE4453A0B
#define ID_SYSTEM_INTELLIVISION     0x4AC771F8
#define ID_PERIPH_ECS               0x143D9A97
#define ID_PERIPH_INTELLIVOICE      0x6EFF540A

//bios numbers
#define A5200_P_MASTER_COMPONENT    0
#define A5200_BT_BIOS               0
#define A5200_B_ORIGINAL_BIOS       0

#define INTV_P_MASTER_COMPONENT     0
#define INTV_BT_EXEC                0
#define INTV_B_ORIGINAL_EXEC        0
#define INTV_B_INTY2_EXEC           1
#define INTV_B_SEARS_EXEC           2
#define INTV_B_GPL_EXEC             3
#define INTV_BT_GROM                1
#define INTV_B_ORIGINAL_GROM        0
#define INTV_B_GPL_GROM             1
#define INTV_P_ECS                  1
#define INTV_BT_ECS_BIOS            0
#define INTV_B_ORIGINAL_ECS_BIOS    0
#define INTV_P_INTELLIVOICE         2
#define INTV_BT_IVOICE_BIOS         0
#define INTV_B_ORIGINAL_IVOICE_BIOS 0

/*
typedef struct _RipBios
{
    CHAR*       biosName;
    UINT32      biosCRC32;
} RipBios;

typedef struct _RipBiosType
{
    CHAR*       biosTypeName;
    RipBios        bioses[16];
} RipBiosType;

typedef struct _RipPeripheral
{
    CHAR*       peripheralName;
    RipBiosType    biosTypes[16];
} RipPeripheral;

typedef struct _RipSystem
{
    UINT32      systemID;
    CHAR*       systemName;
    UINT8       addressByteWidth;
    UINT8       valueByteWidth;
    RipPeripheral  peripherals[16];
} RipSystem;

RipSystem* getSystem(UINT32 systemID);
*/

#ifdef __cplusplus
}
#endif

