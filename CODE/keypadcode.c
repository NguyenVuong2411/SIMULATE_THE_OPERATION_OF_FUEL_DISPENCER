
#include <keypadcode.h>

unsigned int8 HANG,COT,MP,MPT;
/////////////////////////////////////////////////////
void TIMHANG ()
{  HANG=0XFF;
   if(INPUT(PIN_C0)==0)  {HANG=0;}
   else if (INPUT(PIN_C1)==0) {HANG=1;}
   else if (INPUT(PIN_C2)==0) {HANG=2;}
   else if (INPUT(PIN_C3)==0) {HANG=3;}
}
void KEYPRESS()
{
   MP=0XFF;
   
   for (COT=0;COT<4;COT++)
   {
      switch(COT)
      {  case 0:  OUTPUT_LOW(PIN_A5);
                  TIMHANG();
                  OUTPUT_HIGH(PIN_A5);
                  break;
         case 1:  OUTPUT_LOW(PIN_A4);
                  TIMHANG();
                  OUTPUT_HIGH(PIN_A4);
                  break;
         case 2:  OUTPUT_LOW(PIN_A3);
                  TIMHANG();
                  OUTPUT_HIGH(PIN_A3);
                  break;
         case 3:  OUTPUT_LOW(PIN_A2);
                  TIMHANG();
                  OUTPUT_HIGH(PIN_A2);
                  break;
      }
      if (HANG!=0XFF) {
      
      MP=(HANG*4)+COT; 
         if (MP==1) 
         {MP=MP*0;
         break;}
         else if (MP>3 && MP<7)
         {MP=MP-3;
         break;}
         else if (MP==7)
         {MP=20;
         break;}
         else if (MP>7 && MP<11)
         {MP=MP-4;
         break;}
         else if (MP==11)
         {MP=50;
         break;}
         else if (MP>11 && MP<15)
         {MP=MP-5;
         BREAK;}
         else if (MP==15)
         {MP=100;
         break;}
         else if (MP==0)
         {MP=22;}
         else if (MP==2)
         {MP =23;}
         else if (MP==3)
         {MP = 24;}
     
      break;}
   }
   
  
}

void KEY_4X4()
{
   KEYPRESS();
   if (MP!=0XFF)
   {
      delay_ms(10);
      KEYPRESS();
      if(MP!=0XFF)
      {
         MPT=MP;
         
         while(MP!=0XFF) {KEYPRESS();}
         MP=MPT;
      }
   }
 }
