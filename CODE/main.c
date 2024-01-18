#include <main.h>
#use delay(crystal=20000000)

char *SSID="";                  //ten wifi.
char *PASS="";                  //pass wifi
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
unsigned int16 a1, a2, m1, m, k ;
float tien_Mua_macdinh, gia_Tien_1l,soLit_can_dem, soXung_can_dem, giaTri_nhap, LIT,soTien_tra, tienNghin ;
unsigned int8 CHUOI2[] = {"                "};
// KHAI BAO HAM//
void tinhXung_can_dem(float *a ,float *b );
void  EXT_isr(void) ;
///// CHUONG TRINH NGAT NGOAI ////////
#INT_EXT
void  EXT_isr(void) 
{ 
   k=k+1;
   if (k<soXung_can_dem){
   LIT = LIT + 0.00225;
   soTien_tra = soTien_tra + gia_Tien_1l*0.00225;
   output_bit(PIN_A0, 1);
   clear_interrupt(INT_EXT); 
            }
 
   else  {
   output_bit(PIN_A0, 0);
   delay_ms(200);
 
   // GUI DU LIEU LEN WEB SAU MOI LAN BOM //
   tienNghin = soTien_tra/(10e2); 
   int i;
   printf("AT+CIPSTART=\"TCP\",\"%s\",80\r\n",WEBSITE);delay_ms(2000); 
   printf("AT+CIPSEND=200\r\n");delay_ms(2000);
  
   for (i=0 ; i<2 ; i++) {
   printf("GET https://api.thingspeak.com/update?api_key=KX3VVF8T35C0WFPA&field2=%0.3f&field1=%0.3f HTTP/1.1\r\nHost: api.thingspeak.com\r\nConnection: keep-alive\r\n\r\n",tienNghin,LIT);
   }
   delay_ms(700);
   printf("AT+CIPCLOSE\r\n");delay_ms(200);
   clear_interrupt(INT_EXT);
  }
 
}

//CHUONG TRINH TINH XUNG CAN DEM//
void tinhXung_can_dem(float *tien_Mua, float *gia_Tien_1l)
 {
      soLit_can_dem = (*tien_Mua)/(*gia_Tien_1l);
      soXung_can_dem = soLit_can_dem/(2.25e-3);  
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
   KEY_4X4();                 // quet ban phim 
   if(MP!=0x0FF)              // neu co nut dc nhan
   {
     if(MP<11)                // nut nhan la so
         {   m = m++;
             giaTri_nhap = giaTri_nhap*(10^(m)) + MP;  
             if (giaTri_nhap>100){                    // neu gia tri nhap lon hon 100 thi nhan la so lit
             tinhXung_can_dem(&giaTri_nhap,&gia_Tien_1l);}   
             else {soXung_can_dem=giaTri_nhap/0.00225;}
           
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
          else if(MP==20)                 // nut mac dinh bom 30k
          { 
           lcd_gotoxy(31,1);
           printf(lcd_putc,"20000");   
             tien_Mua_macdinh= 20000;    
             tinhXung_can_dem(&tien_Mua_macdinh,&gia_Tien_1l);
                  }
          else if (MP==50)                // mut mac dinh bom 50k
          { 
           lcd_gotoxy(31,1);
            printf(lcd_putc,"50000");
             tien_Mua_macdinh = 50000;
             tinhXung_can_dem(&tien_Mua_macdinh,&gia_Tien_1l);   
          }      
          else if (MP==100)               // nut mac dinh bom 100k 
          { 
           lcd_gotoxy(31,1);
            printf(lcd_putc,"100000");
             tien_Mua_macdinh = 100000;
             tinhXung_can_dem(&tien_Mua_macdinh,&gia_Tien_1l);
          }
           else if (MP==22)                     // nut reset(xoa SO TIEN, SO LIT, DON GIA) va reset relay
          { output_bit(PIN_A0, 1);
          soLit_can_dem = 0;
          soXung_can_dem = m = giaTri_nhap = 0 ;
          soTien_tra = k = LIT = 0;         
          lcd_gotoxy(31,1);
          for (a1=0; a1<16; a1++)
               {CHUOI1[a1]= 0x01;
               lcd_putc(CHUOI1[a1]);         
               }
          } 
          else if (MP ==23)                      // nhap gia tri don gia moi
          {   
            NE:
            KEY_4X4();
               if(MP!=0x0FF)
                  {
                     if(MP<11) 
                          {    m1 = m1++;
                               gia_Tien_1l = gia_Tien_1l*(10^(m1)) + MP;
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
                         { goto LAP; }     // neu duoc nhan lan nua thi thoat ra ngoai tiep tuc quet phim 
                        } 
                        
                        goto NE; }        // tiep tuc quet ban phim de nhan gia tri don gia 
               
                    else if (MP == 24)    // xoa gia tri don gia hien tai
                         {
                            m1 =0;
                            gia_Tien_1l = 0; 
                            lcd_gotoxy(21,2);
                            for (a2=0; a2<16; a2++)
                            {
                            CHUOI2[a2]= 0x01;
                            lcd_putc(CHUOI2[a2]);
                             }
                                }
       }
                  
      lcd_gotoxy(1,1);                          // dong 1 
      printf(lcd_putc,"SO TIEN: %f", soTien_tra);  // hien thi tien phai tra
      lcd_gotoxy(1,2);                          // dong 2
      printf(lcd_putc,"SO LIT: %0.3f ", LIT);   // hien thi so lit 
      lcd_gotoxy(21,1);                         // dong 3
      printf(lcd_putc,"CHON GIA:");             // hien thi gia tien hoac lit da nhap
      lcd_gotoxy(21,2);                         // dong 4
      printf(lcd_putc,"DON GIA: ");             // hien thi don gia 1 lit
     
                goto LAP;
                 
                  }
             

