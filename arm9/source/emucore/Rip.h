// =====================================================================================
// Copyright (c) 2021-2024 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef RIP_H
#define RIP_H

#include <string.h>
#include <vector>
#include "types.h"
#include "Peripheral.h"
#include "Memory.h"
#include "JLP.h"

using namespace std;

typedef enum _PeripheralCompatibility
{
    PERIPH_REQUIRED     = 0,
    PERIPH_OPTIONAL     = 1,
    PERIPH_COMPATIBLE   = 2,
    PERIPH_INCOMPATIBLE = 3
} PeripheralCompatibility;

//system IDs
#define ID_SYSTEM_INTELLIVISION     0x4AC771F8
#define ID_PERIPH_ECS               0x143D9A97
#define ID_PERIPH_INTELLIVOICE      0x6EFF540A

extern UINT8 tag_compatibility[];

typedef struct _CartridgeConfiguration CartridgeConfiguration;

class Rip : public Peripheral
{

public:
    virtual ~Rip();

    void SetTargetSystemID(UINT32 t) { this->targetSystemID = t; }
    UINT32 GetTargetSystemID() { return targetSystemID; }

    void SetName(const CHAR* p);

    PeripheralCompatibility GetPeripheralUsage(const CHAR* periphName);

    //load a raw binary Intellivision image of a game
    static Rip* LoadBin(const CHAR* filename);

    //load an Intellivision .rom file
    static Rip* LoadRom(const CHAR* filename);

    const CHAR* GetFileName() {
        return this->filename;
    }

    UINT32 GetCRC() { return this->crc; }
    UINT32 GetSize() { return this->mySize; }
    JLP *JLP16Bit;

private:
    Rip(UINT32 systemID);

    void AddPeripheralUsage(const CHAR* periphName, PeripheralCompatibility usage);
    static Rip* LoadBinCfg(const CHAR* cfgFile, UINT32 crc, size_t size);

    void SetFileName(const CHAR* fname) {
        strncpy(this->filename, fname, sizeof(this->filename));
    }

    UINT32 targetSystemID;

    //peripheral compatibility indicators
    CHAR peripheralNames[MAX_PERIPHERALS][32];
    PeripheralCompatibility peripheralUsages[MAX_PERIPHERALS];
    UINT32 peripheralCount;
    CHAR filename[MAX_PATH];
    UINT32 mySize;
    UINT32 crc;
};

#endif
