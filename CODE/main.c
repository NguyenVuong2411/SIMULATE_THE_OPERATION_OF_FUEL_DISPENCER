#include <main.h>
#use delay(crystal=20000000)

char *SSID="";                  //ten wifi
char *PASS="";            //pass wifi
char *WEBSITE="api.thingspeak.com"; //trang web thingspeak 

#include"keypadcode.c" // code quet ban phim


#define lcd_enable_pin  PIN_E2
#define lcd_rs_pin      PIN_E0      
#define lcd_rw_pin      PIN_E1
#define lcd_data4       PIN_D4
#define lcd_data5       PIN_D5
#define lcd_data6       PIN_D6
#define lcd_data7       PIN_D7

 // KHAI BAO BIEN//

unsigned int8 CHUOI1[] = {"                "};
unsigned int16 a1, a2, m1,  m, k;
float a,b,c,d, f,  LIT, tien, tien1;
unsigned int8 CHUOI2[] = {"                "};

///// CHUONG TRINH NGAT NGOAI ////////
#INT_EXT
void  EXT_isr(void) 
{ 
   k=k+1;
   if (k<d){
   LIT = LIT + 0.00225;
   tien = tien + b*0.00225;
   output_bit(PIN_A0, 1);
   clear_interrupt(INT_EXT); 
            }
 
   else  {
   output_bit(PIN_A0, 0);
   delay_ms(200);
 
   // GUI DU LIEU LEN WEB SAU MOI LAN BOM //
   tien1 = tien/(10e2); 
   int i;
   printf("AT+CIPSTART=\"TCP\",\"%s\",80\r\n",WEBSITE);delay_ms(2000); 
   printf("AT+CIPSEND=200\r\n");delay_ms(2000);
  
   for (i=0 ; i<2 ; i++) {
   printf("GET https://api.thingspeak.com/update?api_key=KX3VVF8T35C0WFPA&field2=%0.3f&field1=%0.3f HTTP/1.1\r\nHost: api.thingspeak.com\r\nConnection: keep-alive\r\n\r\n",tien1,LIT);
   }
   delay_ms(700);
   printf("AT+CIPCLOSE\r\n");delay_ms(200);
   clear_interrupt(INT_EXT);
  }
 
}
 
 //CHUONG TRINH CHINH//~~
#include <lcd.c>
void main()
{ 
   set_tris_B(0x0F); // B input
   set_tris_D(0);    // D output
   set_tris_E(0);    // E output
   set_tris_A(0);    // A output
   output_bit(PIN_A0, 1);           // bat dau chuong trinh relay dong
   enable_interrupts(INT_EXT);      // ngat ngoai
   enable_interrupts(GLOBAL);       // ngat toan cuc
   ext_int_edge(H_TO_L);  
   lcd_init(); 

   printf("AT\r\n");delay_ms(1000);
   printf("AT+RST\r\n");delay_ms(1000);
   printf("AT+CWMODE=1\r\n");delay_ms(1000);
   printf("AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,PASS);delay_ms(4000); // thay mat khau va tai khoan tuong ung
   printf("AT+CIPMUX=0\r\n");delay_ms(1000);
   
   LAP:    
   KEY_4X4();
   if(MP!=0x0FF)
   {
     if(MP<11) 
         {   m = m++;
             f = f*(10^(m)) + MP;  // f la gia tri tien nhap vao
             if (f>100){
             c = f/b;
             d = c/(2.25e-3);}    // d la so xung can dem 
             else {d=f/0.00225;}
           
         for(a1=0; a1<16; a1++)
            {
             CHUOI1[a1]=CHUOI1[a1+1];     
             }
             CHUOI1[15]=MP+0x30;
               
               lcd_gotoxy(21,1);
               for (a1=0; a1<16; a1++)
               {lcd_putc(CHUOI1[a1]);
               }
               
               }
          else if(MP==50) 
          { 
           lcd_gotoxy(31,1);
           printf(lcd_putc,"30000");
             a = 30000;
             c = a/b;
             d = c/(2.25e-3);
                  }
          else if (MP==20)
          { 
           lcd_gotoxy(31,1);
            printf(lcd_putc,"20000");
             a = 20000;
             c = a/b;
             d = c/(2.25e-3);     
          }      
          else if (MP==100)
          { 
           lcd_gotoxy(31,1);
            printf(lcd_putc,"40000");
             a = 40000;
             c = a/b;
             d = c/(2.25e-3);  
          }
           else if (MP==22)
          { output_bit(PIN_A0, 1);
          c = 0;
          d = m = f = 0 ;
          tien = 0.00000;
          k = LIT = 0;         
          lcd_gotoxy(31,1);
          for (a1=0; a1<16; a1++)
               {CHUOI1[a1]= 0x01;
               lcd_putc(CHUOI1[a1]);         
               }
          } 
          else if (MP ==23) 
          {   
            NE:
            KEY_4X4();
               if(MP!=0x0FF)
                  {
                     if(MP<11) 
                          {    m1 = m1++;
                               b = b*(10^(m1)) + MP;
                               for(a2=0; a2<16; a2++)
                               {
                                  CHUOI2[a2]=CHUOI2[a2+1];
                                }
           
                                  CHUOI2[15]=MP+0x30;
                                  lcd_gotoxy(21,2);
                                  for (a2=0; a2<16; a2++)
                                  {lcd_putc(CHUOI2[a2]);
                                  }
             
                                }
                    else if (MP == 23)
                         { goto LAP; }
                        } 
                        
                        goto NE; }
               
                    else if (MP == 24)
                         {
                            m1 =0;
                            b = 0; 
                            lcd_gotoxy(21,2);
                            for (a2=0; a2<16; a2++)
                            {
                            CHUOI2[a2]= 0x01;
                            lcd_putc(CHUOI2[a2]);
                             }
                                }
       }
                  
      lcd_gotoxy(1,1);                          // dong 1 
      printf(lcd_putc,"SO TIEN: %0.0f", tien);  // hien thi tien phai tra
      lcd_gotoxy(1,2);                          // dong 2
      printf(lcd_putc,"SO LIT: %0.3f ", LIT);   // hien thi so lit 
      lcd_gotoxy(21,1);                         // dong 3
      printf(lcd_putc,"CHON GIA:");             // hien thi gia tien hoac lit da nhap
      lcd_gotoxy(21,2);                         // dong 4
      printf(lcd_putc,"DON GIA: ");             // hien thi don gia 1 lit
     
                   
                goto LAP;
                 
                  }
                  

