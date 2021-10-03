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

typedef enum _PeripheralCompatibility
{
    PERIPH_REQUIRED = 0,
    PERIPH_OPTIONAL = 1,
    PERIPH_COMPATIBLE = 2,
    PERIPH_INCOMPATIBLE = 3
} PeripheralCompatibility;

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

#ifdef __cplusplus
}
#endif

