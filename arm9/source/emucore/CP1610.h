// =====================================================================================
// Copyright (c) 2021-2026 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef CP1610_H
#define CP1610_H

#include "Processor.h"
#include "MemoryBus.h"

#define CP1610_PIN_IN_INTRM 0
#define CP1610_PIN_IN_BUSRQ 1

#define CP1610_PIN_OUT_BUSAK 0
    
extern UINT8 interruptible;

extern UINT8 bCP1610_PIN_IN_BUSRQ;
extern UINT8 bCP1610_PIN_IN_INTRM; 
extern UINT8 bCP1610_PIN_OUT_BUSAK;

#define MAX(v1, v2) (v1 > v2 ? v1 : v2)

TYPEDEF_STRUCT_PACK( _CP1610State
{
    UINT8     S;
    UINT8     Z;
    UINT8     O;
    UINT8     C;
    UINT8     I;
    UINT8     D;
    UINT8     interruptible;
    UINT8     bCP1610_PIN_IN_BUSRQ;
    UINT8     bCP1610_PIN_IN_INTRM; 
    UINT8     bCP1610_PIN_OUT_BUSAK;
    INT8      ext;
    UINT16   interruptAddress;
    UINT16   resetAddress;
    UINT16   r[8];
} CP1610State; )

class CP1610 : public Processor
{

    public:
        CP1610(MemoryBus* m, UINT16 resetAddress,
                UINT16 interruptAddress);

        //PowerConsumer functions
        void resetProcessor();

        //Processor functions
        INT32 getClockSpeed();
        INT32 tick(INT32);
        
        void getState(CP1610State *state);
        void setState(CP1610State *state);

        BOOL isIdle() {
            if (!bCP1610_PIN_IN_BUSRQ && interruptible) {
                bCP1610_PIN_OUT_BUSAK = FALSE;
                return TRUE;
            }
            else {
                bCP1610_PIN_OUT_BUSAK = TRUE;
                return FALSE;
            }
        }

    private:
        void setIndirect(UINT16 regNum, UINT16 value);
        UINT16 getIndirect(UINT16 regNum);
        UINT16 HLT();
        UINT16 SDBD();
        UINT16 EIS();
        UINT16 DIS();
        UINT16 TCI();
        UINT16 CLRC();
        UINT16 SETC();
        UINT16 J(UINT16 target);
        UINT16 JSR(UINT16 regNum, UINT16 target);
        UINT16 JE(UINT16 target);
        UINT16 JSRE(UINT16 regNum, UINT16 target);
        UINT16 JD(UINT16 target);
        UINT16 JSRD(UINT16 regNum, UINT16 target);
        UINT16 INCR(UINT16 regNum);
        UINT16 DECR(UINT16 regNum);
        UINT16 COMR(UINT16 regNum);
        UINT16 NEGR(UINT16 regNum);
        UINT16 ADCR(UINT16 regNum);
        UINT16 RSWD(UINT16 regNum);
        UINT16 GSWD(UINT16 regNum);
        UINT16 NOP(UINT16 twoOption);
        UINT16 SIN(UINT16 twoOption);
        UINT16 SWAP_1(UINT16 regNum);
        UINT16 SWAP_2(UINT16 regNum);
        UINT16 SLL_1(UINT16 regNum);
        UINT16 SLL_2(UINT16 regNum);
        UINT16 RLC_1(UINT16 regNum);
        UINT16 RLC_2(UINT16 regNum);
        UINT16 SLLC_1(UINT16 regNum);
        UINT16 SLLC_2(UINT16 regNum);
        UINT16 SLR_1(UINT16 regNum);
        UINT16 SLR_2(UINT16 regNum);
        UINT16 SAR_1(UINT16 regNum);
        UINT16 SAR_2(UINT16 regNum);
        UINT16 RRC_1(UINT16 regNum);
        UINT16 RRC_2(UINT16 regNum);
        UINT16 SARC_1(UINT16 regNum);
        UINT16 SARC_2(UINT16 regNum);
        UINT16 MOVR(UINT16 sourceReg, UINT16 destReg);
        UINT16 ADDR(UINT16 sourceReg, UINT16 destReg);
        UINT16 SUBR(UINT16 sourceReg, UINT16 destReg);
        UINT16 CMPR(UINT16 sourceReg, UINT16 destReg);
        UINT16 ANDR(UINT16 sourceReg, UINT16 destReg);
        UINT16 XORR(UINT16 sourceReg, UINT16 destReg);
        UINT16 BEXT(UINT16 condition, INT16 displacement);
        UINT16 B(INT16 displacement);
        UINT16 NOPP(INT16 displacement);
        UINT16 BC(INT16 displacement);
        UINT16 BNC(INT16 displacement);
        UINT16 BOV(INT16 displacement);
        UINT16 BNOV(INT16 displacement);
        UINT16 BPL(INT16 displacement);
        UINT16 BMI(INT16 displacement);
        UINT16 BEQ(INT16 displacement);
        UINT16 BNEQ(INT16 displacement);
        UINT16 BLT(INT16 displacement);
        UINT16 BGE(INT16 displacement);
        UINT16 BLE(INT16 displacement);
        UINT16 BGT(INT16 displacement);
        UINT16 BUSC(INT16 displacement);
        UINT16 BESC(INT16 displacement);
        UINT16 MVO(UINT16 regNum, UINT16 address);
        UINT16 MVO_ind(UINT16 registerWithAddress, UINT16 registerToMove);
        UINT16 MVI(UINT16 address, UINT16 regNum);
        UINT16 MVI_ind(UINT16 registerWithAddress, UINT16 registerToReceive);
        UINT16 ADD(UINT16 address, UINT16 regNum);
        UINT16 ADD_ind(UINT16 registerWithAddress, UINT16 registerToReceive);
        UINT16 SUB(UINT16 address, UINT16 regNum);
        UINT16 SUB_ind(UINT16 registerWithAddress, UINT16 registerToReceive);
        UINT16 CMP(UINT16 address, UINT16 regNum);
        UINT16 CMP_ind(UINT16 registerWithAddress, UINT16 registerToReceive);
        UINT16 AND(UINT16 address, UINT16 regNum);
        UINT16 AND_ind(UINT16 registerWithAddress, UINT16 registerToReceive);
        UINT16 XOR(UINT16 address, UINT16 regNum);
        UINT16 XOR_ind(UINT16 registerWithAddress, UINT16 registerToReceive);
        UINT16 decode(void);

        //the memory bus
        MemoryBus* memoryBus;

        //interrupt address
        UINT16 interruptAddress;

        //reset address
        UINT16 resetAddress;
};

#endif
