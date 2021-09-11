
#ifndef CP1610_H
#define CP1610_H

#include "Processor.h"
#include "SignalLine.h"
#include "MemoryBus.h"

#define CP1610_PIN_IN_INTRM 0
#define CP1610_PIN_IN_BUSRQ 1

#define CP1610_PIN_OUT_BUSAK 0
    
extern UINT8 interruptible;

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

        BOOL isIdle() {
            if (!pinIn[CP1610_PIN_IN_BUSRQ]->isHigh && interruptible) {
                pinOut[CP1610_PIN_OUT_BUSAK]->isHigh = FALSE;
                return TRUE;
            }
            else {
                pinOut[CP1610_PIN_OUT_BUSAK]->isHigh = TRUE;
                return FALSE;
            }
        }

    private:
        void setIndirect(UINT16 register, UINT16 value);
        UINT16 getIndirect(UINT16 register);
        INT32 HLT();
        INT32 SDBD();
        INT32 EIS();
        INT32 DIS();
        INT32 TCI();
        INT32 CLRC();
        INT32 SETC();
        INT32 J(UINT16 target);
        INT32 JSR(UINT16 register, UINT16 target);
        INT32 JE(UINT16 target);
        INT32 JSRE(UINT16 register, UINT16 target);
        INT32 JD(UINT16 target);
        INT32 JSRD(UINT16 register, UINT16 target);
        INT32 INCR(UINT16 register);
        INT32 DECR(UINT16 register);
        INT32 COMR(UINT16 register);
        INT32 NEGR(UINT16 register);
        INT32 ADCR(UINT16 register);
        INT32 RSWD(UINT16 register);
        INT32 GSWD(UINT16 register);
        INT32 NOP(UINT16 twoOption);
        INT32 SIN(UINT16 twoOption);
        INT32 SWAP_1(UINT16 register);
        INT32 SWAP_2(UINT16 register);
        INT32 SLL_1(UINT16 register);
        INT32 SLL_2(UINT16 register);
        INT32 RLC_1(UINT16 register);
        INT32 RLC_2(UINT16 register);
        INT32 SLLC_1(UINT16 register);
        INT32 SLLC_2(UINT16 register);
        INT32 SLR_1(UINT16 register);
        INT32 SLR_2(UINT16 register);
        INT32 SAR_1(UINT16 register);
        INT32 SAR_2(UINT16 register);
        INT32 RRC_1(UINT16 register);
        INT32 RRC_2(UINT16 register);
        INT32 SARC_1(UINT16 register);
        INT32 SARC_2(UINT16 register);
        INT32 MOVR(UINT16 sourceReg, UINT16 destReg);
        INT32 ADDR(UINT16 sourceReg, UINT16 destReg);
        INT32 SUBR(UINT16 sourceReg, UINT16 destReg);
        INT32 CMPR(UINT16 sourceReg, UINT16 destReg);
        INT32 ANDR(UINT16 sourceReg, UINT16 destReg);
        INT32 XORR(UINT16 sourceReg, UINT16 destReg);
        INT32 BEXT(UINT16 condition, INT16 displacement);
        INT32 B(INT16 displacement);
        INT32 NOPP(INT16 displacement);
        INT32 BC(INT16 displacement);
        INT32 BNC(INT16 displacement);
        INT32 BOV(INT16 displacement);
        INT32 BNOV(INT16 displacement);
        INT32 BPL(INT16 displacement);
        INT32 BMI(INT16 displacement);
        INT32 BEQ(INT16 displacement);
        INT32 BNEQ(INT16 displacement);
        INT32 BLT(INT16 displacement);
        INT32 BGE(INT16 displacement);
        INT32 BLE(INT16 displacement);
        INT32 BGT(INT16 displacement);
        INT32 BUSC(INT16 displacement);
        INT32 BESC(INT16 displacement);
        INT32 MVO(UINT16 register, UINT16 address);
        INT32 MVO_ind(UINT16 registerWithAddress, UINT16 registerToMove);
        INT32 MVI(UINT16 address, UINT16 register);
        INT32 MVI_ind(UINT16 registerWithAddress, UINT16 registerToReceive);
        INT32 ADD(UINT16 address, UINT16 register);
        INT32 ADD_ind(UINT16 registerWithAddress, UINT16 registerToReceive);
        INT32 SUB(UINT16 address, UINT16 register);
        INT32 SUB_ind(UINT16 registerWithAddress, UINT16 registerToReceive);
        INT32 CMP(UINT16 address, UINT16 register);
        INT32 CMP_ind(UINT16 registerWithAddress, UINT16 registerToReceive);
        INT32 AND(UINT16 address, UINT16 register);
        INT32 AND_ind(UINT16 registerWithAddress, UINT16 registerToReceive);
        INT32 XOR(UINT16 address, UINT16 register);
        INT32 XOR_ind(UINT16 registerWithAddress, UINT16 registerToReceive);
        INT32 decode(void);

        //the mory bus
        MemoryBus* memoryBus;

        //interrupt address
        UINT16 interruptAddress;

        //reset address
        UINT16 resetAddress;
};

#endif
