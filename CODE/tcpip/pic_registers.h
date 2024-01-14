#byte INDF = 0x000

#byte TMR0 = 0x001

#byte PCL = 0x002

typedef struct 
 {
   unsigned int C:1;
   unsigned int DC:1;
   unsigned int Z:1;
   unsigned int PD:1;
   unsigned int TO:1;
   unsigned int RP:2;
   unsigned int IRP:1;
} STATUSBITS;
STATUSBITS STATUSbits;
#byte STATUSbits = 0x003
#byte STATUS = 0x003

#byte FSR = 0x004

typedef struct 
 {
   unsigned int RA0:1;
   unsigned int RA1:1;
   unsigned int RA2:1;
   unsigned int RA3:1;
   unsigned int RA4:1;
   unsigned int RA5:1;
   unsigned int RA6:1;
   unsigned int RA7:1;
} PORTABITS;
PORTABITS PORTAbits;
#byte PORTAbits = 0x005
#byte PORTA = 0x005

typedef struct 
 {
   unsigned int RB0:1;
   unsigned int RB1:1;
   unsigned int RB2:1;
   unsigned int RB3:1;
   unsigned int RB4:1;
   unsigned int RB5:1;
   unsigned int RB6:1;
   unsigned int RB7:1;
} PORTBBITS;
PORTBBITS PORTBbits;
#byte PORTBbits = 0x006
#byte PORTB = 0x006

typedef struct 
 {
   unsigned int RC0:1;
   unsigned int RC1:1;
   unsigned int RC2:1;
   unsigned int RC3:1;
   unsigned int RC4:1;
   unsigned int RC5:1;
   unsigned int RC6:1;
   unsigned int RC7:1;
} PORTCBITS;
PORTCBITS PORTCbits;
#byte PORTCbits = 0x007
#byte PORTC = 0x007

typedef struct 
 {
   unsigned int RD0:1;
   unsigned int RD1:1;
   unsigned int RD2:1;
   unsigned int RD3:1;
   unsigned int RD4:1;
   unsigned int RD5:1;
   unsigned int RD6:1;
   unsigned int RD7:1;
} PORTDBITS;
PORTDBITS PORTDbits;
#byte PORTDbits = 0x008
#byte PORTD = 0x008

typedef struct 
 {
   unsigned int RE0:1;
   unsigned int RE1:1;
   unsigned int RE2:1;
   unsigned int RE3:1;
   unsigned int RE4:1;
   unsigned int RE5:1;
   unsigned int RE6:1;
   unsigned int RE7:1;
} PORTEBITS;
PORTEBITS PORTEbits;
#byte PORTEbits = 0x009
#byte PORTE = 0x009

#byte PCLATH = 0x00A

typedef struct 
 {
   union {
      struct {
         unsigned int RBIF:1;
         unsigned int INTF:1;
         unsigned int TMR0IF:1;
         unsigned int RBIE:1;
         unsigned int INTE:1;
         unsigned int TMR0IE:1;
         unsigned int PEIE:1;
         unsigned int GIE:1;
      };

      struct {
         unsigned int :2;
         unsigned int T0IF:1;
         unsigned int :2;
         unsigned int T0IE:1;
      };
   };
} INTCONBITS;
INTCONBITS INTCONbits;
#byte INTCONbits = 0x00B
#byte INTCON = 0x00B

typedef struct 
 {
   unsigned int TMR1IF:1;
   unsigned int TMR2IF:1;
   unsigned int CCP1IF:1;
   unsigned int SSPIF:1;
   unsigned int TXIF:1;
   unsigned int RCIF:1;
   unsigned int ADIF:1;
   unsigned int PSPIF:1;
} PIR1BITS;
PIR1BITS PIR1bits;
#byte PIR1bits = 0x00C
#byte PIR1 = 0x00C

typedef struct 
 {
   unsigned int CCP2IF:1;
   unsigned int :2;
   unsigned int BCLIF:1;
   unsigned int EEIF:1;
   unsigned int :1;
   unsigned int CMIF:1;
} PIR2BITS;
PIR2BITS PIR2bits;
#byte PIR2bits = 0x00D
#byte PIR2 = 0x00D

#word TMR1 = 0x00E
#byte TMR1H = 0x00F
#byte TMR1L = 0x00F

typedef struct 
 {
   union {
      struct {
         unsigned int TMR1ON:1;
         unsigned int TMR1CS:1;
         unsigned int T1SYNC:1;
         unsigned int T1OSCEN:1;
         unsigned int T1CKPS:2;
      };

      struct {
         unsigned int :2;
         unsigned int T1INSYNC:1;
      };
   };
} T1CONBITS;
T1CONBITS T1CONbits;
#byte T1CONbits = 0x010
#byte T1CON = 0x010

#byte TMR2 = 0x011

typedef struct 
 {
   unsigned int T2CKPS:2;
   unsigned int TMR2ON:1;
   unsigned int TOUTPS:4;
} T2CONBITS;
T2CONBITS T2CONbits;
#byte T2CONbits = 0x012
#byte T2CON = 0x012

#byte SSPBUF = 0x013

typedef struct 
 {
   unsigned int SSPM:4;
   unsigned int CKP:1;
   unsigned int SSPEN:1;
   unsigned int SSPOV:1;
   unsigned int WCOL:1;
} SSPCONBITS;
SSPCONBITS SSPCONbits;
#byte SSPCONbits = 0x014
#byte SSPCON = 0x014

#word CCPR1 = 0x015
#byte CCPR1H = 0x016
#byte CCPR1L = 0x016

typedef struct 
 {
   unsigned int CCP1M:4;
   unsigned int CCP1Y:1;
   unsigned int CCP1X:1;
} CCP1CONBITS;
CCP1CONBITS CCP1CONbits;
#byte CCP1CONbits = 0x017
#byte CCP1CON = 0x017

typedef struct 
 {
   union {
      struct {
         unsigned int RX9D:1;
         unsigned int OERR:1;
         unsigned int FERR:1;
         unsigned int ADDEN:1;
         unsigned int CREN:1;
         unsigned int SREN:1;
         unsigned int RX9:1;
         unsigned int SPEN:1;
      };

   union {
      struct {
         unsigned int RCD8:1;
         unsigned int :5;
         unsigned int RC9:1;
      };
   };

   union {
      struct {
         unsigned int :6;
         unsigned int RC8:1;
      };
   };

      struct {
         unsigned int :6;
         unsigned int RC8_9:1;
      };
   };
} RCSTABITS;
RCSTABITS RCSTAbits;
#byte RCSTAbits = 0x018
#byte RCSTA = 0x018

#byte TXREG = 0x019

#byte RCREG = 0x01A

#word CCPR2 = 0x01B
#byte CCPR2H = 0x01C
#byte CCPR2L = 0x01C

typedef struct 
 {
   unsigned int CCP2M:4;
   unsigned int CCP2Y:1;
   unsigned int CCP2X:1;
} CCP2CONBITS;
CCP2CONBITS CCP2CONbits;
#byte CCP2CONbits = 0x01D
#byte CCP2CON = 0x01D

#byte ADRESH = 0x01E

typedef struct 
 {
   union {
      struct {
         unsigned int ADON:1;
         unsigned int :1;
         unsigned int GO:1;
         unsigned int CHS:3;
         unsigned int ADCS:2;
      };

   union {
      struct {
         unsigned int :2;
         unsigned int DONE:1;
      };
   };

      struct {
         unsigned int :2;
         unsigned int GO_DONE:1;
      };
   };
} ADCON0BITS;
ADCON0BITS ADCON0bits;
#byte ADCON0bits = 0x01F
#byte ADCON0 = 0x01F

typedef struct 
 {
   unsigned int PS:3;
   unsigned int PSA:1;
   unsigned int T0SE:1;
   unsigned int T0CS:1;
   unsigned int INTEDG:1;
   unsigned int RBPU:1;
} OPTION_REGBITS;
OPTION_REGBITS OPTION_REGbits;
#byte OPTION_REGbits = 0x081
#byte OPTION_REG = 0x081

typedef struct 
 {
   unsigned int TRISA0:1;
   unsigned int TRISA1:1;
   unsigned int TRISA2:1;
   unsigned int TRISA3:1;
   unsigned int TRISA4:1;
   unsigned int TRISA5:1;
   unsigned int TRISA6:1;
   unsigned int TRISA7:1;
} TRISABITS;
TRISABITS TRISAbits;
#byte TRISAbits = 0x085
#byte TRISA = 0x085

typedef struct 
 {
   unsigned int TRISB0:1;
   unsigned int TRISB1:1;
   unsigned int TRISB2:1;
   unsigned int TRISB3:1;
   unsigned int TRISB4:1;
   unsigned int TRISB5:1;
   unsigned int TRISB6:1;
   unsigned int TRISB7:1;
} TRISBBITS;
TRISBBITS TRISBbits;
#byte TRISBbits = 0x086
#byte TRISB = 0x086

typedef struct 
 {
   unsigned int TRISC0:1;
   unsigned int TRISC1:1;
   unsigned int TRISC2:1;
   unsigned int TRISC3:1;
   unsigned int TRISC4:1;
   unsigned int TRISC5:1;
   unsigned int TRISC6:1;
   unsigned int TRISC7:1;
} TRISCBITS;
TRISCBITS TRISCbits;
#byte TRISCbits = 0x087
#byte TRISC = 0x087

typedef struct 
 {
   unsigned int TRISD0:1;
   unsigned int TRISD1:1;
   unsigned int TRISD2:1;
   unsigned int TRISD3:1;
   unsigned int TRISD4:1;
   unsigned int TRISD5:1;
   unsigned int TRISD6:1;
   unsigned int TRISD7:1;
} TRISDBITS;
TRISDBITS TRISDbits;
#byte TRISDbits = 0x088
#byte TRISD = 0x088

typedef struct 
 {
   unsigned int TRISE0:1;
   unsigned int TRISE1:1;
   unsigned int TRISE2:1;
   unsigned int TRISE3:1;
   unsigned int TRISE4:1;
   unsigned int TRISE5:1;
   unsigned int TRISE6:1;
   unsigned int TRISE7:1;
} TRISEBITS;
TRISEBITS TRISEbits;
#byte TRISEbits = 0x089
#byte TRISE = 0x089

typedef struct 
 {
   unsigned int TMR1IE:1;
   unsigned int TMR2IE:1;
   unsigned int CCP1IE:1;
   unsigned int SSPIE:1;
   unsigned int TXIE:1;
   unsigned int RCIE:1;
   unsigned int ADIE:1;
   unsigned int PSPIE:1;
} PIE1BITS;
PIE1BITS PIE1bits;
#byte PIE1bits = 0x08C
#byte PIE1 = 0x08C

typedef struct 
 {
   unsigned int CCP2IE:1;
   unsigned int :2;
   unsigned int BCLIE:1;
   unsigned int EEIE:1;
   unsigned int :1;
   unsigned int CMIE:1;
} PIE2BITS;
PIE2BITS PIE2bits;
#byte PIE2bits = 0x08D
#byte PIE2 = 0x08D

typedef struct 
 {
   union {
      struct {
         unsigned int BOR:1;
         unsigned int POR:1;
      };

      struct {
         unsigned int BO:1;
      };
   };
} PCONBITS;
PCONBITS PCONbits;
#byte PCONbits = 0x08E
#byte PCON = 0x08E

typedef struct 
 {
   unsigned int SEN:1;
   unsigned int RSEN:1;
   unsigned int PEN:1;
   unsigned int RCEN:1;
   unsigned int ACKEN:1;
   unsigned int ACKDT:1;
   unsigned int ACKSTAT:1;
   unsigned int GCEN:1;
} SSPCON2BITS;
SSPCON2BITS SSPCON2bits;
#byte SSPCON2bits = 0x091
#byte SSPCON2 = 0x091

#byte PR2 = 0x092

#byte SSPADD = 0x093

typedef struct 
 {
   union {
      struct {
         unsigned int BF:1;
         unsigned int UA:1;
         unsigned int R:1;
         unsigned int S:1;
         unsigned int P:1;
         unsigned int D:1;
         unsigned int CKE:1;
         unsigned int SMP:1;
      };

   union {
      struct {
         unsigned int :2;
         unsigned int I2C_READ:1;
         unsigned int I2C_START:1;
         unsigned int I2C_STOP:1;
         unsigned int I2C_DATA:1;
      };
   };

   union {
      struct {
         unsigned int :2;
         unsigned int W:1;
         unsigned int :2;
         unsigned int A:1;
      };
   };

   union {
      struct {
         unsigned int :2;
         unsigned int WRITE:1;
         unsigned int :2;
         unsigned int ADDRESS:1;
      };
   };

   union {
      struct {
         unsigned int :2;
         unsigned int R_W:1;
         unsigned int :2;
         unsigned int D_A:1;
      };
   };

      struct {
         unsigned int :2;
         unsigned int READ_WRITE:1;
         unsigned int :2;
         unsigned int DATA_ADDRESS:1;
      };
   };
} SSPSTATBITS;
SSPSTATBITS SSPSTATbits;
#byte SSPSTATbits = 0x094
#byte SSPSTAT = 0x094

typedef struct 
 {
   union {
      struct {
         unsigned int TX9D:1;
         unsigned int TRMT:1;
         unsigned int BRGH:1;
         unsigned int :1;
         unsigned int SYNC:1;
         unsigned int TXEN:1;
         unsigned int TX9:1;
         unsigned int CSRC:1;
      };

   union {
      struct {
         unsigned int TXD8:1;
         unsigned int :5;
         unsigned int TX8:1;
      };
   };

      struct {
         unsigned int :6;
         unsigned int TX8_9:1;
      };
   };
} TXSTABITS;
TXSTABITS TXSTAbits;
#byte TXSTAbits = 0x098
#byte TXSTA = 0x098

#byte SPBRG = 0x099

typedef struct 
 {
   unsigned int CM:3;
   unsigned int CIS:1;
   unsigned int C1INV:1;
   unsigned int C2INV:1;
   unsigned int C1OUT:1;
   unsigned int C2OUT:1;
} CMCONBITS;
CMCONBITS CMCONbits;
#byte CMCONbits = 0x09C
#byte CMCON = 0x09C

typedef struct 
 {
   unsigned int CVR:4;
   unsigned int :1;
   unsigned int CVRR:1;
   unsigned int CVROE:1;
   unsigned int CVREN:1;
} CVRCONBITS;
CVRCONBITS CVRCONbits;
#byte CVRCONbits = 0x09D
#byte CVRCON = 0x09D

#byte ADRESL = 0x09E

typedef struct 
 {
   unsigned int PCFG:4;
   unsigned int :2;
   unsigned int ADCS2:1;
   unsigned int ADFM:1;
} ADCON1BITS;
ADCON1BITS ADCON1bits;
#byte ADCON1bits = 0x09F
#byte ADCON1 = 0x09F

#byte EEDATA = 0x10C

#byte EEADR = 0x10D

#byte EEDATH = 0x10E

#byte EEADRH = 0x10F

typedef struct 
 {
   unsigned int RD:1;
   unsigned int WR:1;
   unsigned int WREN:1;
   unsigned int WRERR:1;
   unsigned int :3;
   unsigned int EEPGD:1;
} EECON1BITS;
EECON1BITS EECON1bits;
#byte EECON1bits = 0x18C
#byte EECON1 = 0x18C

#byte EECON2 = 0x18D

