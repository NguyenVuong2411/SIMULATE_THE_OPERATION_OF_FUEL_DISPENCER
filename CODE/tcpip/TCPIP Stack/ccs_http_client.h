/*
   ccs_http_client.h
   
   To add this, #define STACK_USE_CCS_HTTP_CLIENT to your code.
   
   Library for acting as an HTTP client
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

#ifndef __CCS_HTTP_CLIENT_H__
#define __CCS_HTTP_CLIENT_H__

// append a key=value pair onto an existing buffer for holding key=value pairs.
// this will also url encode the key=value pair when going into the buffer.
// if this is the first time calling the function, make sure pDst is 
// initialized with a null terminator. maxSize is the max number of characters 
// allocated at pDst, including the null terminator. returns TRUE if there was 
// enough space in pDst to append the new key=value.  due to URL encoding, it
// may take more space to add your key=value pair to memory than expected.  you 
// don't have to use these functions, HttpClientSetCgiGet() or 
// HttpClientSetCgiPost() can be called to point to a full string if the 
// string follows the "key1=value1&key2=value2&key3=value3" format.
int1 HttpClientAppendCgi(char *pDst, char *pKey, char *pVal, unsigned int16 maxSize);
int1 HttpClientAppendCgiROM(char *pDst, ROM char *pKey, char *pVal, unsigned int16 maxSize);

// this library is not performing the required URL encoding for CGI, with the
// exception of encoding spaces (' ').  that means do not use 
// HttpClientAppendCgi() or HttpClientAppendCgiROM() to send any non-alpha
// numeric characters.
//#warning see comment above about HttpClientAppendCgi() and URL encoding

// HttpClientSetXXXX() needs to be called before HttpClientStart().
// You only need to call HttpClientSetXXXX() once, these values will
// be retained for all calls to HttpClientStart().
// Strings must be globally or statically allocated.
// Setting the hostname and url is required.
// Setting port, CGI and custom headers is not required.
// If using HttpClientSetCustomHeaders(), each header should be terminated a \r\n
// If using HttpClientSetCgiGet() or HttpClientSetCgiPost() manually, without
// using the HttpClientAppendCgi(), then user is responsible for URL encoding
// the data into the correct format.
// The library will not fix the spaces in a URL to be properly encoded with
// %20. If your remote URL does have a space you will manually need to replace
// the spaces with %20.
void HttpClientSetHostPort(unsigned int16 port);
void HttpClientSetHostName(char *hostname);           //required
void HttpClientSetHostNameROM(ROM char *hostname);    //required
void HttpClientSetUrl(char *url);                     //required
void HttpClientSetUrlROM(ROM char *url);              //required
void HttpClientSetCgiGet(char *get);
void HttpClientSetCgiGetROM(ROM char *get);
void HttpClientSetCgiPost(char *get);
void HttpClientSetCgiPostROM(ROM char *get);
void HttpClientSetCustomHeaders(char *headers);
void HttpClientSetCustomHeadersROM(ROM char *headers);
void HttpClientSetResponseBuffer(char *response, unsigned int16 maxSize);  //maxSize is entire size allocated for response, including null terminator.
void HttpClientSetReadResponseHeaders(int1 set);      //see comments above HttpClientStart()

// if you want to start downloading a file from an offset, then it can
// specified here. a value of 0 starts downloading the file at the
// beginning.  A value of 1024 will start downloading the file but skip
// the first 1024 bytes of the file.  this will be useful if you want
// to download the file in chunks.
void HttpClientSetStartOffset(unsigned int32 startOffset);

// start an HTTP client transaction.
// use HttpClientIsBusy() to determine if the stack is still working,
// and if not use HttpClientGetResult() to get result/error code.
//
// returns TRUE if started, FALSE if not.  The reason it may have not started
// is because it's busy with a previous transaction, TCP/IP isn't ready or
// there are no available TCP sockets.
//
// if HttpClientSetResponseBuffer() was called:
//   when HttpClientIsBusy() is FALSE and HttpClientGetResult() is 0 (no
//   error), the pointer given to HttpClientSetResponseBuffer() will be
//   assigned the data from the URL.  the custom http response headers
//   from the server cannot be saved using this.  a null terminator
//   will be added to the end of the buffer given to 
//   HttpClientSetResponseBuffer().  HttpClientStart() will
//   clear this buffer at the beginning of the call.
//
// if HttpClientSetResponseBuffer() wasn't called:
//   Incoming HTTP response body will be discarded, unless you call
//   HttpClientDiscard(FALSE).  Once you call HttpClientDiscard(FALSE),
//   while HttpClientIsBusy()  use HttpClientIsGetReady() to determine
//   how much received data came back from the remote server.  If
//   HttpClientIsGetReady() returned a non-zero value, use 
//   HttpClientGetc() and HttpClientGetArray() to read that data.
//   If you don't care about receiving any data from the server, then
//   call HttpClientDiscard().  if you don't get the data or discard
//   the data the client won't disconnect (except after a timeout).
//   by default, the library will skip past the servers HTTP response
//   headers, but if your application wants to read them then
//   call HttpClientSetReadResponseHeaders()
int1 HttpClientStart(void);

//see comments above HttpClientStart()
int1 HttpClientIsBusy(void);

typedef enum
{
   // OK
   HTTP_CLIENT_EC_OK = 0,
   
   // HttpClientStart() hasn't been called yet
   HTTP_CLIENT_EC_IDLE = 1,
   
   // TCPOpen() returned an invalid socket.  this will happen if there are no
   // sockets left.  increase the number of scokets in TCPSocketInitializer[].
   // use TCP_PURPOSE_GENERIC_TCP_CLIENT
   HTTP_CLIENT_EC_NO_SOCKETS = 2,
   
   // a connection could not be established with the remote host.
   // check hostname and port settings.  if using a hostname and
   // not an ip, then check that a valid DNS is configured.
   // check that arp is able resolve the MAC address.
   // if the remote host is not on the local network, make sure that gateway
   // address is configured correctly.
   HTTP_CLIENT_EC_NO_CONNECTION = 3,
   
   // the remote HTTP server terminated the connection prematurely.
   HTTP_CLIENT_EC_SERVER_TERMINATED = 4,
   
   // HTTP server didn't respond with HTTP/1.x yyy status code
   HTTP_CLIENT_EC_NO_HTTP_STATUS_CODE = 5,

   // 200 will be replaced with 0 (HTTP_CLIENT_EC_OK), so you will not actually
   // see this value (HTTP_CLIENT_EC_FILE_FOUND) retured by the library
   HTTP_SERVER_STATUS_CODE_FILE_FOUND = 200,
  
   // any number not shown here is a response code from the HTTP server.
   // for example, 500 would be a server side error, 404 would be file not
   // found.
   HTTP_SERVER_STATUS_CODE_FILE_NOT_FOUND = 404
} http_client_ec_t;

//see comments above HttpClientStart()
http_client_ec_t HttpClientGetResult(void);

// see comments above HttpClientStart()
// this returns the number of bytes in the stack's TCP receive buffer that
// is associated with the HTTP Client.
unsigned int16 HttpClientIsGetReady(void);

// see comments above HttpClientStart()
// returns one char stack's TCP receive buffer that is associated with the 
// HTTP Client.  if there was no data in that buffer than this will return
// 0x00.  you can use HttpClientIsGetReady() to see if there is data 
// available.
char HttpClientGetc(void);

// see comments above HttpClientStart()
// reads up to 'num' characters from the stack's TCP receive buffer and
// stores them to 'pDst'.  returns the number of characters that were
// actually saved to 'pDst' in the event there weren't 'num' chars
// in the receive buffer.  you can use HttpClientIsGetReady() to see if 
// there is data available.
unsigned int16 HttpClientGetArray(char *pDst, unsigned int16 num);

// see comments above HttpClientStart().
// By default the HTTP client will discard all incoming data, the same
// as calling HttpClientDiscard(TRUE).  If you call this with a FALSE
// parameter then user is responsible for using HttpClientIsGetReady(),
// HttpClientGetc(), HttpClientGetArray() and HttpClientRemaining()
// to read the response.  The HTTP client will hold the socket open
// until the user reads all the data or uses this function to discard it.
void HttpClientDiscard(int1 doDiscardAll);

// returns the number of bytes still remaining to get from the remote 
// http server for this URL.  This value is initiated by the "content-length"
// header and is decremented for each byte received.  when this reaches
// 0 the http client task will disconnect from the host.  just because
// this returns non-zero doesn't mean there is data in the receive buffer,
// HttpClientIsGetReady() needs to be used to check the receive buffer.
unsigned int32 HttpClientRemaining(void);

// returns the 'Conent-Length' valid given to us by the HTTP server.
// returns -1 if the server didn't give us one.
unsigned int32 HttpClientContentLength(void);

#if defined(HTTP_CLIENT_GRAB_LAST_MODIFIED_HEADER)
// returns the value in the Last-Modified header in a time_t format with
// a TIME_T_USES_2010 offset.
unsigned int32 HttpClientLastModified(void);
#endif

// intialization and main task call.
// the user doesn't need to call these, they are already called by
// the TCP/IP stack.
void HttpClientInit(void);
void HttpClientTask(void);

#endif
