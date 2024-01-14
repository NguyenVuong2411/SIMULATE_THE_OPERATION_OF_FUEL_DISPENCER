// CCS PCH C Compiler to Microchip C18 Compiler compatability layer.

#ifndef __P18CXXXX_C__
#define __P18CXXXX_C__

#include "p18cxxx.h"

signed int8 memcmppgm2ram(void * s1, rom char *s2, unsigned int16 n)
{
   char *su1;
   rom char *su2;
   
   for(su1=s1, su2=s2; 0<n; ++su1, ++su2, --n)
   {
      if(*su1!=*su2)
         return ((*su1<*su2)?-1:+1);
   }
   return 0;
}

__ADDRESS__ strlenpgm(rom char *s)
{
   rom char *sc;

   for (sc = s; *sc != 0; sc++);
   return(sc - s);
}

#if 0 //this is in string.h and helpers.c
char* strupr(char *s)
{
   char *p;
   
   p=s;
   
   while(*p)
   {
      *p = toupper(*p++);
   }
   return(s);
}
#endif

void memcpypgm2ram(unsigned int8 *d, rom char* s, unsigned int16 n)
{
   read_program_memory(s, d, n);
}

void strcpypgm2ram(char *d, rom char *s)
{
   char c;
   
   do
   {
      c = *s;
      *d = c;
      d++;
      s++;
   } while(c);
}

signed int8 strcmppgm2ram(char *s1, rom char *s2)
{
   for (; *s1 == *s2; s1++, s2++)
      if (*s1 == '\0')
         return(0);
   return((*s1 < *s2) ? -1: 1);
}

rom char *strchrpgm(rom char* s, unsigned int8 c)
{
   for (; *s != c; s++)
      if (*s == '\0')
         return(0);
   return(s);
}

char *strstrrampgm(char *s1, rom char * s2)
{
   char *s;
   rom char *t;

   while (*s1)
   {
      for(s = s1, t = s2; *t && (*s == *t); ++s, ++t);

      if (*t == '\0')
         return s1;
      ++s1;
      while(*s1 != '\0' && *s1 != *s2)
         ++s1;
   }
   return 0;
}

#endif
