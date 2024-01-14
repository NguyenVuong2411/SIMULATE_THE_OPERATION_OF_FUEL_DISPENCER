/*
   ccs_http_client.c
   
   To add this, #define STACK_USE_CCS_HTTP_CLIENT to your code.
   
   Library for acting as an HTTP client
   
   For documentation, see ccs_http_client.h
*/
///////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996,2014 Custom Computer Services           ////
//// This source code may only be used by licensed users of the CCS C  ////
//// compiler.  This source code may only be distributed to other      ////
//// licensed users of the CCS C compiler.  No other use, reproduction ////
//// or distribution is permitted without written permission.          ////
//// Derivative programs created using this software in object code    ////
//// form are not restricted in any way.                               ////
////                                                                   ////
//// http://www.ccsinfo.com                                            ////
///////////////////////////////////////////////////////////////////////////

#ifndef __CCS_HTTP_CLIENT_C__
#define __CCS_HTTP_CLIENT_C__

#ifndef debug_httpc
   #define debug_httpc(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)
#else
   #define __DO_DEBUG_HTTPC
#endif

typedef struct
{
   union
   {
      char * pRam;
      ROM char * pRom;
   };
   int1 romPointer;
} http_string_ptr_t;

unsigned char ptrderef_httpptr(http_string_ptr_t p)
{
   if (p.romPointer && p.pRom)
   {
      return(*p.pRom);
   }
   else if (p.pRam)
   {
      return(*p.pRam);
   }
   else
   {
      return(0);
   }
}

unsigned int16 strlen_httpptr(http_string_ptr_t p)
{
   if (p.romPointer && p.pRom)
   {
      return(strlenpgm(p.pRom));
   }
   else if (p.pRam)
   {
      return(strlen(p.pRam));
   }
   else
   {
      return(0);
   }
}

int1 HttpClientAppendCgiHTTPPTR(char *pDst, http_string_ptr_t key, http_string_ptr_t val, unsigned int16 maxSize)
{
   int1 doAmp = 0;
   size_t len;
   
   if (maxSize)
      maxSize--;  //save space for null
   
   len = strlen(pDst);
   if (len)
   {
      pDst += len;
      
      if (len > maxSize)
         maxSize = 0;
      else
         maxSize -= len;
         
      doAmp = TRUE;
      
      if (maxSize)
         maxSize--;
   }
   
   len = strlen_httpptr(key);
   if (len > maxSize)
      maxSize = 0;
   else
      maxSize -= len;
      
   len = strlen_httpptr(val);
   if (len > maxSize)
      maxSize = 0;
   else
      maxSize -= len;
   
   if (maxSize)
   {
      //there is enough room
      if (doAmp)
         *pDst++ = '&';
      
      if (key.romPointer)
      {
         sprintf(pDst, "%s=%s", key.pRom, val.pRam);
      }
      else
      {
         sprintf(pDst, "%s=%s", key.pRam, val.pRam);
      }
      
      //url encoding
      for(;;)
      {
         pDst = strchr(pDst, ' ');
         if (!pDst)
            break;
         *pDst = '+';
      }
      
      //TODO: convert other non-alpha chars
      
      return(TRUE);
   }
   
   return(FALSE);
}

// see ccs_http_client.h for documentation
int1 HttpClientAppendCgi(char *pDst, char *pKey, char *pVal, unsigned int16 maxSize)
{
   http_string_ptr_t val, key;
   
   key.pRam = pKey;
   key.romPointer = FALSE;
   
   val.pRam = pVal;
   val.romPointer = FALSE;
   
   return(HttpClientAppendCgiHTTPPTR(pDst, key, val, maxSize));
}

// see ccs_http_client.h for documentation
int1 HttpClientAppendCgiROM(char *pDst, ROM char *pKey, char *pVal, unsigned int16 maxSize)
{
   http_string_ptr_t val, key;
   
   key.pRom = pKey;
   key.romPointer = TRUE;
   
   val.pRam = pVal;
   val.romPointer = FALSE;
   
   return(HttpClientAppendCgiHTTPPTR(pDst, key, val, maxSize));
}

typedef enum
{
   HTTP_CLIENT_SM_IDLE = 0,
   HTTP_CLIENT_SM_CONNECTING,
   HTTP_CLIENT_SM_PUT_URL,
   HTTP_CLIENT_SM_PUT_GET,
   HTTP_CLIENT_SM_PUT_CONTENT_LEN,
   HTTP_CLIENT_SM_PUT_RANGE_BYTES,
   HTTP_CLIENT_SM_PUT_HOSTNAME,
   HTTP_CLIENT_SM_PUT_USER_HEADER,
   HTTP_CLIENT_SM_PUT_POST,
   HTTP_CLIENT_SM_GET_HEADERS,
   HTTP_CLIENT_SM_GET_BODY,
} http_client_sm_t;

struct
{
   TCP_SOCKET sock;  //make sure this is the first element in this structure
   http_client_sm_t sm;
   TICK t;
   unsigned int32 contentLen; // this is the active workspace saying how much content-len is left
   unsigned int32 contentLenInit;   //this is the content-len as initially told to us by the HTTP server
   int1 contentLenValid;
   unsigned int16 idx;  //index into TCP socket buffer
   int1 discardAll;
   http_client_ec_t ec;
   char buffer[48];

   unsigned int16 port;
   http_string_ptr_t hostname;
   http_string_ptr_t url;
   http_string_ptr_t get;
   http_string_ptr_t post;
   http_string_ptr_t customHeaders;
   char *response;
   unsigned int16 responseSize;
   int1 readResponseHeaders;   
   unsigned int32 startOffset;
   
  #if defined(HTTP_CLIENT_GRAB_LAST_MODIFIED_HEADER)
   unsigned int32 lastModified;
   int1 lastModifiedValid;
  #endif
} g_HttpClient = {INVALID_SOCKET};


#if defined(HTTP_CLIENT_GRAB_LAST_MODIFIED_HEADER)
unsigned int32 HttpClientLastModified(void)
{
   if (!g_HttpClient.lastModifiedValid)
      return(-1);
   return(g_HttpClient.lastModified);
}
#endif


void HttpClientSetStartOffset(unsigned int32 startOffset)
{
   g_HttpClient.startOffset = startOffset;
}

void HttpClientSetHostPort(unsigned int16 port)
{
   debug_httpc(debug_putc, "HttpClientSetHostPort(%lu)\r\n", port);
   
   g_HttpClient.port = port;
}

void HttpClientSetHostName(char *hostname)
{
   debug_httpc(debug_putc, "HttpClientSetHostName('%s')\r\n", hostname);
   
   g_HttpClient.hostname.pRam = hostname;
   g_HttpClient.hostname.romPointer = FALSE;
}

void HttpClientSetHostNameROM(ROM char *hostname)
{
   debug_httpc(debug_putc, "HttpClientSetHostNameROM('%s')\r\n", hostname);
   
   g_HttpClient.hostname.pRom = hostname;
   g_HttpClient.hostname.romPointer = TRUE;
}

void HttpClientSetUrl(char *url)
{
   debug_httpc(debug_putc, "HttpClientSetUrl('%s')\r\n", url);
   
   g_HttpClient.url.pRam = url;
   g_HttpClient.url.romPointer = FALSE;
}

void HttpClientSetUrlROM(ROM char *url)
{
   debug_httpc(debug_putc, "HttpClientSetUrlROM('%s')\r\n", url);
   
   g_HttpClient.url.pRom = url;
   g_HttpClient.url.romPointer = TRUE;
}

void HttpClientSetCgiGet(char *get)
{
   debug_httpc(debug_putc, "HttpClientSetCgiGet('%s')\r\n", get);
   
   g_HttpClient.get.pRam = get;
   g_HttpClient.get.romPointer = FALSE;
}

void HttpClientSetCgiGetROM(ROM char *get)
{
   debug_httpc(debug_putc, "HttpClientSetCgiGetROM('%s')\r\n", get);
   
   g_HttpClient.get.pRom = get;
   g_HttpClient.get.romPointer = TRUE;
}

void HttpClientSetCgiPost(char *post)
{
   debug_httpc(debug_putc, "HttpClientSetCgiPost('%s')\r\n", post);
   
   g_HttpClient.post.pRam = post;
   g_HttpClient.post.romPointer = FALSE;
}

void HttpClientSetCgiPostROM(ROM char *post)
{
   debug_httpc(debug_putc, "HttpClientSetCgiPostROM('%s')\r\n", post);
   
   g_HttpClient.post.pRom = post;
   g_HttpClient.post.romPointer = TRUE;
}

void HttpClientSetCustomHeaders(char *headers)
{
   debug_httpc(debug_putc, "HttpClientSetCustomHeaders('%s')\r\n", headers);
   
   g_HttpClient.customHeaders.pRam = headers;
   g_HttpClient.customHeaders.romPointer = FALSE;
}

void HttpClientSetCustomHeadersROM(ROM char *headers)
{
   debug_httpc(debug_putc, "HttpClientSetCustomHeadersROM('%s')\r\n", headers);
   
   g_HttpClient.customHeaders.pRom = headers;
   g_HttpClient.customHeaders.romPointer = TRUE;
}

void HttpClientSetResponseBuffer(char *response, unsigned int16 maxSize)
{
   debug_httpc(debug_putc, "HttpClientSetResponseBuffer(%LX,%LX)\r\n", response, maxSize);
   
   g_HttpClient.response = response;
   g_HttpClient.responseSize = maxSize;
 
   g_HttpClient.discardAll = FALSE;
}

void HttpClientSetReadResponseHeaders(int1 set)
{
   debug_httpc(debug_putc, "HttpClientSetReadResponseHeaders(%u)\r\n", set);
   g_HttpClient.readResponseHeaders = set;
}

static void HttpClientDisconnect(void)
{
   if (g_HttpClient.sock != INVALID_SOCKET)
   {
      debug_httpc(debug_putc, "HttpClientDisconnect()\r\n");
      
      TCPDisconnect(g_HttpClient.sock);
      TCPDisconnect(g_HttpClient.sock);
      g_HttpClient.sock = INVALID_SOCKET;
   }
   g_HttpClient.sm = HTTP_CLIENT_SM_IDLE;
}

#define HTTP_CLIENT_TASK_DONE(e)   \
   g_HttpClient.startOffset = 0; \
   g_HttpClient.ec = e;   \
   HttpClientDisconnect()
   
#define HTTP_CLIENT_TASK_STATE(s)  \
   g_HttpClient.sm = s;   \
   g_HttpClient.idx = 0;   \
   g_HttpClient.t = TickGet()

// see ccs_http_client.h for documentation
int1 HttpClientStart(void)
{
   if (!HttpClientIsBusy() && DHCPBoundOrDisabled())
   {
      g_HttpClient.ec = 0;
      g_HttpClient.contentLen = 0;
      g_HttpClient.contentLenValid = FALSE;
      
      HttpClientDisconnect();
      
      if (g_HttpClient.hostname.romPointer && g_HttpClient.hostname.pRom)
      {
         g_HttpClient.sock = TCPOpen(g_HttpClient.hostname.pRom, TCP_OPEN_ROM_HOST, g_HttpClient.port, TCP_PURPOSE_GENERIC_TCP_CLIENT);
         debug_httpc(debug_putc, "HttpClientStart() '%s':%lu sock=%u\r\n", g_HttpClient.hostname.pRom, g_HttpClient.port, g_HttpClient.sock);
      }
      else if (g_HttpClient.hostname.pRam)
      {
         g_HttpClient.sock = TCPOpen(g_HttpClient.hostname.pRam, TCP_OPEN_RAM_HOST, g_HttpClient.port, TCP_PURPOSE_GENERIC_TCP_CLIENT);
         debug_httpc(debug_putc, "HttpClientStart() '%s':%lu sock=%u\r\n", g_HttpClient.hostname.pRam, g_HttpClient.port, g_HttpClient.sock);
      }
      else
      {
         g_HttpClient.ec = HTTP_CLIENT_EC_NO_CONNECTION;
         debug_httpc(debug_putc, "HttpClientStart() ???\r\n");
      }
      
      if (!g_HttpClient.ec && (g_HttpClient.sock==INVALID_SOCKET))
      {
         g_HttpClient.ec = HTTP_CLIENT_EC_NO_SOCKETS;
      }
      
      if (!g_HttpClient.ec)
      {
         HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_CONNECTING);
         
         g_HttpClient.ec = HTTP_CLIENT_EC_NO_HTTP_STATUS_CODE;
      }
      
      if (!g_HttpClient.responseSize || !g_HttpClient.response)
      {
         debug_httpc(debug_putc, "HttpClientStart() erasing response_buffer because invalid parameters\r\n");
         g_HttpClient.response = NULL;
         g_HttpClient.responseSize = 0;
      }
      
      if (g_HttpClient.response)
      {
         // you can't do both of these
         g_HttpClient.readResponseHeaders = FALSE;
      }
      
      return(HttpClientIsBusy());
   }
  #if defined(__DO_DEBUG_HTTPC)
   else
   {
      debug_httpc(debug_putc, "HttpClientStart() BUSY %U %U\r\n", HttpClientIsBusy(), DHCPBoundOrDisabled());
   }
  #endif
   
   return(FALSE);
}

// see ccs_http_client.h for documentation
int1 HttpClientIsBusy(void)
{
   return(g_HttpClient.sm != HTTP_CLIENT_SM_IDLE);
}

// see ccs_http_client.h for documentation
http_client_ec_t HttpClientGetResult(void)
{
   return(g_HttpClient.ec);
}

// see ccs_http_client.h for documentation
unsigned int16 HttpClientIsGetReady(void)
{
   if (HttpClientIsBusy())
      return(TCPIsGetReady(g_HttpClient.sock));
   else
      return(0);
}

static unsigned int16 HttpClientIsPutReady(void)
{
   return(TCPIsPutReady(g_HttpClient.sock));
}

// see ccs_http_client.h for documentation
char HttpClientGetc(void)
{
   char c;
   
   if (!HttpClientGetArray(&c, 1))
      return(0);
      
   return(c);
}

#if defined(HTTP_CLIENT_GRAB_LAST_MODIFIED_HEADER)
#include <time.h>
#include <time.c>

// this is basically timeasc() but with data in a different order
// returns TRUE if OK, FALSE if error.
//ex: Tue, 15 Nov 1994 12:45:26
int1 _HttpClientParseLastModified(char *szTime)
{
   struct_tm tm;
   char str[9];

   if (szTime[3] != ',')   return(FALSE);
   if (szTime[4] != ' ')   return(FALSE);
   if (szTime[7] != ' ')   return(FALSE);
   if (szTime[11] != ' ')   return(FALSE);
   if (szTime[16] != ' ')   return(FALSE);
   if (szTime[19] != ':')   return(FALSE);
   if (szTime[22] != ':')   return(FALSE);
 
   szTime[3] = 0;
   szTime[11] = 0;
   
   for (tm.tm_wday=0; tm.tm_wday<7; tm.tm_wday++)
   {
      WeekdayAbbreviations(tm.tm_wday, str);
      if (!_time_stricmp(szTime, str))
         break;
   }
   if (tm.tm_wday >= 7)
      return(FALSE);

   szTime += 5;
   tm.tm_mday = atoi(szTime);

   szTime += 3;
   for (tm.tm_mon=0; tm.tm_mon<12; tm.tm_mon++)
   {
      MonthAbbreviations(tm.tm_mon, str);
      if (!_time_stricmp(szTime, str))
         break;
   }
   if (tm.tm_mon >= 12)
      return(FALSE);
      
   szTime += 4;
   tm.tm_year = atol(szTime) - 1900;
   szTime += 5;

   tm.tm_hour = atoi(szTime);
   szTime += 3;

   tm.tm_min = atoi(szTime);   
   szTime += 3;

   tm.tm_sec = atoi(szTime);

   g_HttpClient.lastModified = mktime(&tm);

   return(TRUE);
}
#endif   //if defined(HTTP_CLIENT_GRAB_LAST_MODIFIED_HEADER)

#define HTTPC_CONTENT_LENGTH_STRING "CONTENT-LENGTH: "

// check TCP receive buffer (either with a full get() or peek()) to scan
// incoming http headers for content-length field.
// returns number of bytes read from header.
static unsigned int16 HttpClientHeaderCheck(unsigned int16 maxGet, int1 isPeek)
{
   unsigned int16 idx = 0;
   char c;
   
   //debug_httpc(debug_putc, "HttpClientHeaderCheck(max=%lu, peek=%u)\r\n", maxGet, isPeek);
   
   while ((idx < maxGet) && (g_HttpClient.sm==HTTP_CLIENT_SM_GET_HEADERS))
   {
      if (isPeek)
      {
         c = TCPPeek(g_HttpClient.sock, idx);
      }
      else
      {
         TCPGet(g_HttpClient.sock, &c);
      }
      
      idx++;
      
      if (c != '\r')
      {
         if (c == '\n')
         {
            if (g_HttpClient.idx == 0)
            {
               HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_GET_BODY);
               break;
            }
            else 
            {
               char *p;
               g_HttpClient.buffer[g_HttpClient.idx] = 0;
               strupr(g_HttpClient.buffer);
               debug_httpc(debug_putc, "HttpClientHeaderCheck() '%s'\r\n", g_HttpClient.buffer);
               
               p = strstrrampgm(g_HttpClient.buffer, (rom char*)HTTPC_CONTENT_LENGTH_STRING);
               if (p)
               {
                  //p += strlenpgm((rom char*)HTTPC_CONTENT_LENGTH_STRING);
                  p += 16;
                  g_HttpClient.contentLen = atoi32(p);
                  g_HttpClient.contentLenInit = g_HttpClient.contentLen;
                  g_HttpClient.contentLenValid = TRUE;
                  debug_httpc(debug_putc, "Content-len is %lu\r\n", g_HttpClient.contentLen);
               }

              #if defined(HTTP_CLIENT_GRAB_LAST_MODIFIED_HEADER)
               p = strstrrampgm(g_HttpClient.buffer, (rom char*)"LAST-MODIFIED: ");
               if (p)
               {
                  //p += strlenpgm((rom char*)HTTPC_CONTENT_LENGTH_STRING);
                  p += 15;
                  g_HttpClient.lastModifiedValid = _HttpClientParseLastModified(p);
                  debug_httpc(debug_putc, "Last-Modified is %lu %u\r\n", g_HttpClient.lastModified, g_HttpClient.lastModifiedValid);
               }              
              #endif

               p = strstrrampgm(g_HttpClient.buffer, (rom char*)"HTTP/1.");
               if (p)
               {
                  //p += strlenpgm((rom char*)"HTTP/1.");
                  p += 9;  //strlen("HTTP/1.x ");
                  g_HttpClient.ec = atol(p);
                  debug_httpc(debug_putc, "HTTP result code is %lu\r\n", g_HttpClient.ec);
                  if (g_HttpClient.ec == HTTP_SERVER_STATUS_CODE_FILE_FOUND)
                     g_HttpClient.ec = 0;
               }

               g_HttpClient.idx = 0;
            }
         }
         else
         {
            g_HttpClient.buffer[g_HttpClient.idx] = c;
            if (g_HttpClient.idx < (sizeof(g_HttpClient.buffer)-1))
            {
               g_HttpClient.idx++;
            }
         }
      }
   }
   
   return(idx);
}

// see ccs_http_client.h for documentation
//
// before data is passed to the user, this function may peek at it
// to glean any information from the HTTP headers.
unsigned int16 HttpClientGetArray(char *pDst, unsigned int16 num)
{
   unsigned int16 max, hdrNum = 0;
   
   max = HttpClientIsGetReady();
   
   if (num > max)
      num = max;

   if
   (
      g_HttpClient.readResponseHeaders && 
      (g_HttpClient.sm==HTTP_CLIENT_SM_GET_HEADERS)
   )
   {
      hdrNum = HttpClientHeaderCheck(num, TRUE);
   }
   
   if (g_HttpClient.sm==HTTP_CLIENT_SM_GET_BODY)
   {
      hdrNum = num - hdrNum;  //don't subtract any bytes read from the header from content-length
      
      if (hdrNum > g_HttpClient.contentLen)
         g_HttpClient.contentLen = 0;
      else
         g_HttpClient.contentLen -= hdrNum;
   }
   
   num = TCPGetArray(g_HttpClient.sock, pDst, num);
   
   if (num != 0)
   {
      g_HttpClient.t = TickGet();
   }
   
   return(num);
}

// see ccs_http_client.h for documentation
void HttpClientDiscard(int1 doDiscardAll)
{
   g_HttpClient.discardAll = doDiscardAll;
}

// see ccs_http_client.h for documentation
unsigned int32 HttpClientRemaining(void)
{
   return(g_HttpClient.contentLen);
}

unsigned int32 HttpClientContentLength(void)
{
   if (!g_HttpClient.contentLenValid)
      return(-1);
   return(g_HttpClient.contentLenInit);
}

// see ccs_http_client.h for documentation
void HttpClientInit(void)
{
   debug_httpc(debug_putc, "HttpClientInit()\r\n");
   
   HttpClientDisconnect();
   
   memset(&g_HttpClient, 0x00, sizeof(g_HttpClient));

   g_HttpClient.discardAll = TRUE;
     
   g_HttpClient.port = 80;
   
   g_HttpClient.sock = INVALID_SOCKET;
}

// 'pSend' is the string to send.
// 'idx' is the starting index of 'pSend' to start sending.
// returns the number of chars sent from 'pSend'.
static unsigned int16 HttpClientPutString(http_string_ptr_t pSend, unsigned int16 idx)
{
   unsigned int16 len;
   unsigned int16 avail = 0;
   http_string_ptr_t p;
   
   p = pSend;

   if (p.romPointer)
   {
      p.pRom += idx;
   }
   else
   {
      p.pRam += idx;
   }
   
   len = strlen_httpptr(p);
   avail = TCPIsPutReady(g_HttpClient.sock);
   
   if (len > avail)
      len = avail;
   
   if (len)
   {
      if (p.romPointer)
      {
         TCPPutROMArray(g_HttpClient.sock, p.pRom, len);
      }
      else
      {
         TCPPutArray(g_HttpClient.sock, p.pRam, len);
      }
   }
   
   return(len);
}

// see ccs_http_client.h for documentation
void HttpClientTask(void)
{
   unsigned int16 avail;
   unsigned int16 len;
   char str[6];
   int1 cont;
   
  #if defined(__DO_DEBUG_HTTPC)
   static unsigned int8 debug;
  #endif

   if 
   (
      (g_HttpClient.sm == HTTP_CLIENT_SM_GET_BODY) &&
      g_HttpClient.contentLenValid &&
      (g_HttpClient.contentLen == 0)
   )
   {
      debug_httpc(debug_putc, "HttpClientTask() DONE2\r\n");
      HTTP_CLIENT_TASK_DONE(g_HttpClient.ec);
   }

   if 
   (
      (g_HttpClient.sm != HTTP_CLIENT_SM_IDLE) && 
      (g_HttpClient.sm != HTTP_CLIENT_SM_CONNECTING) && 
      !TCPIsConnected(g_HttpClient.sock) &&
      !HttpClientIsGetReady() //don't close state machine if there is still data available
   )
   {
      if (g_HttpClient.contentLenValid)
      {
         //http/1.1 - server doesn't close the connection, client does
         
         debug_httpc(debug_putc, "HttpClientTask() SERVER_ABORT r=%LU cl=%LU s=%LU\r\n", HttpClientIsGetReady(), g_HttpClient.contentLen, g_HttpClient.responseSize);
         
         HTTP_CLIENT_TASK_DONE(HTTP_CLIENT_EC_SERVER_TERMINATED);
      }
      else
      {
         //http/1.0 - server closes connection

         debug_httpc(debug_putc, "HttpClientTask() DONE3\r\n");
         
         HTTP_CLIENT_TASK_DONE(g_HttpClient.ec);
      }
   }
   
   if 
   (
      (g_HttpClient.sm >= HTTP_CLIENT_SM_CONNECTING) &&
      ((TickGet() - g_HttpClient.t) >= ((TICK)TICKS_PER_SECOND*30))
   )
   {
      debug_httpc(debug_putc, "HttpClientTask() DONE4 s=%U\r\n", g_HttpClient.sock);
      HTTP_CLIENT_TASK_DONE(HTTP_CLIENT_EC_NO_CONNECTION);   
   }
   
   //we're not expecting data in these states, so discard it
   if ((g_HttpClient.sock != INVALID_SOCKET) && (g_HttpClient.sm < HTTP_CLIENT_SM_GET_HEADERS))
   {
      //debug_httpc(debug_putc, ".");
      TCPDiscard(g_HttpClient.sock);
   }
   
   do
   {
   cont = FALSE;
  #if defined(__DO_DEBUG_HTTPC)
   static unsigned int8 debug;
   if (g_HttpClient.sm != debug)
   {
      debug_httpc(debug_putc, "HttpClientTask() %X->%X rem:%LU ", debug, g_HttpClient.sm, HttpClientIsGetReady());
      debug_httpc(debug_putc, "r:%LX rsize:%LU idx:%LU ", g_HttpClient.response, g_HttpClient.responseSize, g_HttpClient.idx);
      debug_httpc(debug_putc, "\r\n");
      debug = g_HttpClient.sm;
   }
  #endif
   switch(g_HttpClient.sm)
   {
      default:
         HttpClientInit();
      case HTTP_CLIENT_SM_IDLE:
         break;
         
      case HTTP_CLIENT_SM_CONNECTING:
         if (TCPIsConnected(g_HttpClient.sock))
         {
            HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_PUT_URL);
            cont = TRUE;
         }
         break;
         
      case HTTP_CLIENT_SM_PUT_URL:
         avail = HttpClientIsPutReady();
         if (avail >= 16)
         {
            if (g_HttpClient.idx == 0)
            {
               if (strlen_httpptr(g_HttpClient.post))
               {
                  TCPPutROMString(g_HttpClient.sock, (rom char*)"POST ");
               }
               else
               {
                  TCPPutROMString(g_HttpClient.sock, (rom char*)"GET ");
               }
            }
            len = strlen_httpptr(g_HttpClient.url);
            if (g_HttpClient.idx == 0)
            {
               if (ptrderef_httpptr(g_HttpClient.url) != '/')
               {
                  TCPPut(g_HttpClient.sock, '/');
               }
            }
            if (g_HttpClient.idx < len)
            {
               g_HttpClient.idx += HttpClientPutString(g_HttpClient.url, g_HttpClient.idx);
            }
            if (g_HttpClient.idx >= len)
            {
               HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_PUT_GET);
               cont = TRUE;
            }
         }
         break;
         
      case HTTP_CLIENT_SM_PUT_GET:
         avail = HttpClientIsPutReady();
         if (avail >= 16)
         {
            len = strlen_httpptr(g_HttpClient.get);
            if (g_HttpClient.idx < len)
            {
               if (g_HttpClient.idx == 0)
               {
                  TCPPut(g_HttpClient.sock, '?');
               }
               g_HttpClient.idx += HttpClientPutString(g_HttpClient.get, g_HttpClient.idx);
            }
            else
            {
               TCPPutROMString(g_HttpClient.sock, (rom char*)" HTTP/1.1\r\n");
               HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_PUT_HOSTNAME);
               cont = TRUE;
            }
         }
         break;

      case HTTP_CLIENT_SM_PUT_HOSTNAME:
         avail = HttpClientIsPutReady();
         if (avail >= 16)
         {      
            len = strlen_httpptr(g_HttpClient.hostname);
            if (g_HttpClient.idx == 0)
            {
               TCPPutROMString(g_HttpClient.sock, (rom char*)"Host: ");
            }            
            if (g_HttpClient.idx < len)
            {
               g_HttpClient.idx += HttpClientPutString(g_HttpClient.hostname, g_HttpClient.idx);
            }
            else
            {
               TCPPutROMString(g_HttpClient.sock, (rom char*)"\r\n");
               
               if (strlen_httpptr(g_HttpClient.customHeaders))
               {
                  HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_PUT_USER_HEADER);
               }
               else if (g_HttpClient.startOffset)
               {
                  HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_PUT_RANGE_BYTES);
               }
               else
               {
                  HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_PUT_CONTENT_LEN);
               }               
               
               cont = TRUE;
            }
         }
         break;

      case HTTP_CLIENT_SM_PUT_USER_HEADER:
         len = strlen_httpptr(g_HttpClient.customHeaders);
         if (g_HttpClient.idx < len)
         {
            g_HttpClient.idx += HttpClientPutString(g_HttpClient.customHeaders, g_HttpClient.idx);
         }
         else
         {
            TCPPutROMString(g_HttpClient.sock, (rom char*)"\r\n");

            if (g_HttpClient.startOffset)
            {
               HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_PUT_RANGE_BYTES);
            }
            else
            {
               HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_PUT_CONTENT_LEN);
            }            
            
            cont = TRUE;
         }
         break;
         
      case HTTP_CLIENT_SM_PUT_RANGE_BYTES:
         avail = HttpClientIsPutReady();
         if (avail >= 32)
         {
            sprintf(str, "%lu", g_HttpClient.startOffset);
            TCPPutROMString(g_HttpClient.sock, (rom char*)"Range: bytes=");
            TCPPutString(g_HttpClient.sock, str);
            TCPPutROMString(g_HttpClient.sock, (rom char*)"-\r\n");

            HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_PUT_CONTENT_LEN);
            cont = TRUE;
         }      
         break;


      case HTTP_CLIENT_SM_PUT_CONTENT_LEN:
         avail = HttpClientIsPutReady();
         if (avail >= 32)
         {
            sprintf(str, "%lu", strlen_httpptr(g_HttpClient.post));
            TCPPutROMString(g_HttpClient.sock, (rom char*)HTTPC_CONTENT_LENGTH_STRING);
            TCPPutString(g_HttpClient.sock, str);
            TCPPutROMString(g_HttpClient.sock, (rom char*)"\r\n\r\n");

            if (strlen_httpptr(g_HttpClient.post))
            {
               HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_PUT_POST);
            }
            else
            {
               HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_GET_HEADERS);
            }
            cont = TRUE;
         }
         break;
         
      case HTTP_CLIENT_SM_PUT_POST:
         avail = HttpClientIsPutReady();
         if (avail >= 16)
         {
            len = strlen_httpptr(g_HttpClient.post);
            if (g_HttpClient.idx < len)
            {
               g_HttpClient.idx += HttpClientPutString(g_HttpClient.post, g_HttpClient.idx);
            }
            if (g_HttpClient.idx >= len)
            {
               g_HttpClient.buffer[0] = 0;
               HTTP_CLIENT_TASK_STATE(HTTP_CLIENT_SM_GET_HEADERS);
               cont = TRUE;
            }
         }
         break;

      case HTTP_CLIENT_SM_GET_HEADERS:
         if (!g_HttpClient.readResponseHeaders || g_HttpClient.discardAll)
         {
            HttpClientHeaderCheck(HttpClientIsGetReady(), FALSE);
            if (g_HttpClient.sm == HTTP_CLIENT_SM_GET_BODY)
            {
               cont = TRUE;
            }
         }
         break;
         
      case HTTP_CLIENT_SM_GET_BODY:
         if (g_HttpClient.response)
         {
            avail = HttpClientIsGetReady();
            len = g_HttpClient.responseSize - g_HttpClient.idx;

            if (avail > len)
            {
               avail = len;
            }

            if (avail)
            {
               avail = HttpClientGetArray(&g_HttpClient.response[g_HttpClient.idx], avail);
               if (avail)
               {
                  g_HttpClient.idx += avail;
                  avail = g_HttpClient.idx;
                  if (avail >= g_HttpClient.responseSize)
                     avail = g_HttpClient.responseSize - 1;
                  g_HttpClient.response[avail] = 0;
               }
            }
         }
         if 
         (
            ((g_HttpClient.response != NULL) && (g_HttpClient.idx >= g_HttpClient.responseSize)) ||
            ((g_HttpClient.contentLen == 0) && (g_HttpClient.contentLenValid)) || 
            g_HttpClient.discardAll
         )
         {
            debug_httpc(debug_putc, "HttpClientTask() DONE idx=%lu responseSize=%lu contentLen=%lu(%u) discardAll=%u\r\n", g_HttpClient.idx, g_HttpClient.responseSize, g_HttpClient.contentLen, g_HttpClient.contentLenValid, g_HttpClient.discardAll);
            HTTP_CLIENT_TASK_DONE(g_HttpClient.ec);
         }
         break;
   }
   } while(cont);
}

#endif
