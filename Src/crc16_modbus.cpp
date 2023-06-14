#include "crc16_modbus.hpp"


uint16_t CRC16_Modbus(uint8_t *buf,unsigned char Len)
{
  unsigned int temp = 0xffff;
  unsigned char n,i;
  
 for( n = 0; n < Len; n++)          
 {       
     temp = buf[n] ^ temp;
     for( i = 0;i < 8;i++)            
   { 
        if(temp & 0x01)
    {
             temp = temp >> 1;
             temp = temp ^ 0xa001;
        }   
        else
    {
             temp = temp >> 1;
        }   
     }   
  }   
 return temp;                          
}
