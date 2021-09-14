#include <nds.h>
#include "JLP.h"

extern UINT16 jlp_ram[];

JLP::JLP()
: RAM(RAM_JLP_SIZE, 0x8040, 0xFFFF, 0xFFFF)
{}

void JLP::reset()
{
    enabled = TRUE;
    for (UINT16 i = 0; i < RAM_JLP_SIZE; i++)
        jlp_ram[i] = 0xFFFF;
    jlp_ram[0x1FFF] = 0;
}

UINT16 JLP::peek(UINT16 location)
{
    if (location&0x1FFF == 0x1FFE) {return (UINT16)random();}
    return jlp_ram[(location&readAddressMask) - this->location];
}

void JLP::poke(UINT16 location, UINT16 value)
{
    UINT32 prod=0, quot=0, rem=0;
    jlp_ram[(location&writeAddressMask)-this->location] = value;

    /* -------------------------------------------------------------------- */
    /*  Check for mult/div writes                                           */
    /*      $9F8(0,1):  s16($9F80) x s16($9F81) -> s32($9F8F:$9F8E)         */
    /*      $9F8(2,3):  s16($9F82) x u16($9F83) -> s32($9F8F:$9F8E)         */
    /*      $9F8(4,5):  u16($9F84) x s16($9F85) -> s32($9F8F:$9F8E)         */
    /*      $9F8(6,7):  u16($9F86) x u16($9F87) -> u32($9F8F:$9F8E)         */
    /*      $9F8(8,9):  s16($9F88) / s16($9F89) -> quot($9F8E), rem($9F8F)  */
    /*      $9F8(A,B):  u16($9F8A) / u16($9F8B) -> quot($9F8E), rem($9F8F)  */
    /* -------------------------------------------------------------------- */    
    switch(location)
    {
        case 0x9F80:
        case 0x9F81:
            prod = (UINT32)  ((INT32)(INT16)jlp_ram[(0x9F80&readAddressMask) - this->location] * (INT32)(INT16)jlp_ram[(0x9F81&readAddressMask) - this->location]);
            jlp_ram[(0x9F8E&writeAddressMask)-this->location] = prod & 0xFFFF;
            jlp_ram[(0x9F8F&writeAddressMask)-this->location] = prod >> 16;                                                                                                      
            break;    

        case 0x9F82:
        case 0x9F83:
            prod = (UINT32)  ((INT32)(INT16)jlp_ram[(0x9F82&readAddressMask) - this->location] * (INT32)(UINT16)jlp_ram[(0x9F83&readAddressMask) - this->location]);
            jlp_ram[(0x9F8E&writeAddressMask)-this->location] = prod & 0xFFFF;
            jlp_ram[(0x9F8F&writeAddressMask)-this->location] = prod >> 16;                                                                                                      
            break;    
            
        case 0x9F84:
        case 0x9F85:
            prod = (UINT32)  ((INT32)(UINT16)jlp_ram[(0x9F84&readAddressMask) - this->location] * (INT32)(INT16)jlp_ram[(0x9F85&readAddressMask) - this->location]);
            jlp_ram[(0x9F8E&writeAddressMask)-this->location] = prod & 0xFFFF;
            jlp_ram[(0x9F8F&writeAddressMask)-this->location] = prod >> 16;                                                                                                      
            break;    

        case 0x9F86:
        case 0x9F87:
            prod = (UINT32)  ((UINT32)(UINT16)jlp_ram[(0x9F86&readAddressMask) - this->location] * (UINT32)(UINT16)jlp_ram[(0x9F87&readAddressMask) - this->location]);
            jlp_ram[(0x9F8E&writeAddressMask)-this->location] = prod & 0xFFFF;
            jlp_ram[(0x9F8F&writeAddressMask)-this->location] = prod >> 16;                                                                                                      
            break;    
            
        case 0x9F88:
        case 0x9F89:
            if (jlp_ram[(0x9F89&readAddressMask) - this->location] != 0)
            {
                quot = (UINT32)  ((INT32)(INT16)jlp_ram[(0x9F88&readAddressMask) - this->location] / (INT32)(INT16)jlp_ram[(0x9F89&readAddressMask) - this->location]);
                rem  = (UINT32)  ((INT32)(INT16)jlp_ram[(0x9F88&readAddressMask) - this->location] % (INT32)(INT16)jlp_ram[(0x9F89&readAddressMask) - this->location]);
            }
            jlp_ram[(0x9F8E&writeAddressMask)-this->location] = quot;
            jlp_ram[(0x9F8F&writeAddressMask)-this->location] = rem;  
            break;    
            
        case 0x9F8A:
        case 0x9F8B:
            if (jlp_ram[(0x9F8B&readAddressMask) - this->location] != 0)
            {
                quot = (UINT32)  ((UINT32)(UINT16)jlp_ram[(0x9F8A&readAddressMask) - this->location] / (UINT32)(UINT16)jlp_ram[(0x9F8B&readAddressMask) - this->location]);
                rem  = (UINT32)  ((UINT32)(UINT16)jlp_ram[(0x9F8A&readAddressMask) - this->location] % (UINT32)(UINT16)jlp_ram[(0x9F8B&readAddressMask) - this->location]);
            }
            jlp_ram[(0x9F8E&writeAddressMask)-this->location] = quot;
            jlp_ram[(0x9F8F&writeAddressMask)-this->location] = rem;  
            break;    
    }
    
}
