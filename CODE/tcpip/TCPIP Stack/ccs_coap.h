/*
   ccs_coap.h
   
   CoAP implementation.
   
   Work in progress.
   
   Can TX a message with a payload larger than max UDP datagram 
   (message will be split over several blocks).
   
   Can only RX a message with a payload that fits in one UDP datagram.
*/
//// OPTIONS                                                           ////
//// -------------------------------------------------------------     ////
////  COAP_MAX_CONNECTIONS - If defined, the max number of CoAP
////   transactions that can be perfomed at once.  If not defined,
////   uses default value of 1.  If greater than 1 than it adds
////   the CoapClientUseSocket() to change sockets.
////
////  COAP_USES_MALLOC - If defined, memory needed by Coap is
////   allocated dynamically.  If using this option, CoapMalloc()
////   must be called to allocate the memory and CoapFree() must be
////   called to free the memory.
////
////  COAP_TX_BLOCK_SIZE - The maximum size that will be transmitted
////     per block.  Default value is 64.  Valid values are 16, 32, 
////     64, 128, 256, 512, 1024.  Larger values give less 
////     fragmentation for large messages but require more free
////     space available on the PHY for TX.
////                                                                   ////
////  COAP_RETRY_TICKS - Normally, a CoAP retry should have a hold-off //// 
////     range that is random - with a range of _COAP_ACK_TIMEOUT to   ////
////     _COAP_ACK_TIMEOUT+(_COAP_ACK_TIMEOUT*_COAP_ACK_RANDOM_FACTOR),////
////     in seconds.  Instead of deploying a random generator on the   ////
////     MCU, this define option is provided to specify the retry      ////
////     time.  If not specified it uses 3 seconds.  This value        ////
////     must be in ticks (TICKS_PER_SECOND), not ticks.  This         ////
////     value only really has importance in multi-cast situations.    ////
////                                                                   ////
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

#ifndef __CCS_COAP_H__
#define __CCS_COAP_H__

#include <stdint.h>

#if defined(COAP_USES_MALLOC)
   // allocate memory needed for coap usage.  returns FALSE if it couldn't
   // allocate the memory.  if malloc is used, CoapMalloc() must be called
   // before any other functions.
   int1 CoapMalloc(void);
   
   void CoapFree(void);
   
   int1 CoapIsMalloced(void);
#else
   #define CoapMalloc() (TRUE)
   #define CoapFree()
   #define CoapIsMalloced()   (TRUE)
#endif

#ifndef COAP_MAX_CONNECTIONS
#define COAP_MAX_CONNECTIONS  1
#endif

#if (COAP_MAX_CONNECTIONS > 1)
// Specify which CoAP transaction socket to use for future CoapClientXXXX()
// functions.  Range of 'which' is 0 to (COAP_MAX_CONNECTIONS-1).
void CoapClientUseSocket(int8 which);

// Copies CoAP settings (host port, hostname, uri, etc) from 'from' and
// copies them to the currently selected transaction socket (see
// CoapClientUseSocket()).
// Calling this function after a UDP connection has already been established
// to the server will cause the next CoapClientSend****Request() to close
// and open a new UDP socket.
void CoapClientCopySettings(int8 from);
#endif

// Set UDP port of remote host.
// If not used, default value of 5683 is used.
// Calling this function after a UDP connection has already been established
// to the server will cause the next CoapClientSend****Request() to close
// and open a new UDP socket.
void CoapClientSetHostPort(unsigned int16 port);

// set hostname/IP of remote host.  this is required.
// pointer contents are not copied, so it must be passed a globally/statically
// allocated string.
// Calling this function after a UDP connection has already been established
// to the server will cause the next CoapClientSend****Request() to close
// and open a new UDP socket.
void CoapClientSetHostName(char *hostname);
void CoapClientSetHostNameROM(ROM char *hostname);

// set Uri path of GET/POST/PUT/DELETE/OBSERVE request.
// Uri path may contain several slashes ('/').
// if not called then it will not send a Uri path in the request.
// pointer contents are not copied, so it must be passed a globally/statically
// allocated string.
void CoapClientSetUriPath(char *uriPath);
void CoapClientSetUriPathROM(ROM char *uriPath);

// set Uri query of GET/POST/PUT/DELETE/OBSERVE request.
// if not called then it will not send a Uri query in the request.
// pointer contents are not copied, so it must be passed a globally/statically
// allocated string.
void CoapClientSetUriQuery(char *uriQuery);
void CoapClientSetUriQueryROM(ROM char *uriQuery);

// this the data put into the payload of the POST request.
// pointer contents are not copied, so it must be passed a globally/statically
// allocated string.
void CoapClientSetPostString(char *data);
void CoapClientSetPostStringROM(ROM char *data);
void CoapClientSetPostBytes(uint8_t *data, size_t num);
void CoapClientSetPostBytesROM(ROM uint8_t *data, size_t num);

// if you want to save the payload to response from the server,
// use this to set the pointer to where to save the data and the
// max size to save to at pointer.
void CoapClientSetResponsePointer(uint8_t *p, size_t max);

// Allows setting a complex Uri that contains hostname, path, query and data.
// Format is:
//    host/path?query:data
// Uri path may contain several slashes ('/').
// If you don't want to send query or data then those parts don't have 
// to be sent.  An example:
//    host/path
// data must be a string, and is passed to CoapClientSetPostString().
// pointer contents are not copied, so it must be passed a globally/statically
// allocated string.  the contents of the data pointed to by data may be used
// as a workspace and altered.
void CoapClientSetUri(char *data);

// undo all CoapClientSetXXXXX() functions and go back to defaults.
void CoapClientClearParams();

typedef enum
{
   COAP_METHOD_GET = 1,
   COAP_METHOD_POST = 2,
   COAP_METHOD_PUT = 3,
   COAP_METHOD_DELETE = 4,
} coap_hdr_method_codes_t;

// Returns TRUE if request was succesfully sent, FALSE if an error or CoAP
// is already busy handling a request.
// If returns TRUE, call CoapClientGetResponse() until it doesnt' return
// COAP_RESP_CODE_BUSY.
// Once CoapClientGetResponse() doesn't return COAP_RESP_CODE_BUSY, then the
// response can be used to determine the result of the transaction.
// Once CoapClientGetResponse() doesn't return COAP_RESP_CODE_BUSY, then the
// the coap transaction is considered complete and future calls to 
// CoapClientGetResponse() will return COAP_RESP_CODE_NOT_STARTED.
// These are all sent as confirmable.
int1 CoapCliendSendRequest(coap_hdr_method_codes_t method);
#define CoapClientSendGetRequest()     CoapCliendSendRequest(COAP_METHOD_GET)
#define CoapClientSendPutRequest()     CoapCliendSendRequest(COAP_METHOD_PUT)
#define CoapClientSendPostRequest()    CoapCliendSendRequest(COAP_METHOD_POST)
#define CoapClientSendDeleteRequest()  CoapCliendSendRequest(COAP_METHOD_DELETE)

// If 'enable' is TRUE (1), perform a GET with the observer set indicating to the
// server that we want to observe this resource.
// If 'enable' is 2, it's similar to sending a 1 except that it won't
// re-randomize the token number.  this is useful for restarting and
// existing observe.
// If 'enable' is FALSE (0), perform a GET with the observer cleared indicated
// to the server that we do not want to observe this resource anymore.  this
// is the graceful way to tell the server that we are done with this resource.
// If any other CoapClientSend***Request() is sent while an observe is busy,
// we will forget about this observe request.  This is an ungraceful way
// to stop observing, as the server will try at least once to push us new
// data.
// Since any other CoapClientSend***Request() will cancel an observe, only
// one resource can be observed at once.
// Returns TRUE if request was succesfully sent, FALSE if an error or CoAP
// is already busy handling a request.
// Returns TRUE if request was succesfully sent, FALSE if an error or CoAP
// is already busy handling a request.
// If returns TRUE, call CoapClientGetResponse() until it doesnt' return
// COAP_RESP_CODE_BUSY.
// Once CoapClientGetResponse() doesn't return COAP_RESP_CODE_BUSY, then the
// response can be used to determine the result of the transaction.
// Once CoapClientGetResponse() doesn't return COAP_RESP_CODE_BUSY, and it
// doesn't signify an error response, then CoapClientGetResponse() can be
// called again to listen for any push updates from the server we are 
// observing.
int1 CoapClientSendObserveRequest(uint8_t enable);

// returns TRUE if Coap is busy processing a request.
int1 CoapClientIsBusy(void);

// if Coap was busy, cancel transaction and free UDP socket.
void CoapCancel(void);

typedef enum
{
   // values 0x00 - 0xFF map directly to the 8bit code as per the CoAP RFC specs
   COAP_RESP_CODE_CREATED = 0x41,   //2.01 | Created | [RFC7252] |
   COAP_RESP_CODE_DELETED = 0x42,   //2.02 | Deleted | [RFC7252] |
   COAP_RESP_CODE_VALID = 0x43,     //2.03 | Valid | [RFC7252] |
   COAP_RESP_CODE_CHANGED = 0x44,   //2.04 | Changed | [RFC7252] |
   COAP_RESP_CODE_CONTENT = 0x45,   //2.05 | Content | [RFC7252] |
   COAP_RESP_CODE_CONTINUE = 0x5F,  //2.31 | Continue | [RFCXXXX] |  //see coap-18 (block tranfsers, RFC not assigned)
   COAP_RESP_CODE_BAD_REQUEST = 0x80,     //4.00 | Bad Request | [RFC7252] |
   COAP_RESP_CODE_UNAUTHORIZED = 0x81,    //4.01 | Unauthorized | [RFC7252] |
   COAP_RESP_CODE_BAD_OPTION = 0x82,      //4.02 | Bad Option | [RFC7252] |
   COAP_RESP_CODE_FORBIDDEN = 0x83,       //4.03 | Forbidden | [RFC7252] |
   COAP_RESP_CODE_NOT_FOUND = 0x84,       //4.04 | Not Found | [RFC7252] |
   COAP_RESP_CODE_NOT_ALLOWED = 0x85,     //4.05 | Method Not Allowed | [RFC7252] |
   COAP_RESP_CODE_NOT_ACCEPTABLE = 0x86,  //4.06 | Not Acceptable | [RFC7252] |
   COAP_RESP_CODE_REQUEST_INCOMPLETE = 0x88, //4.08 | Request Entity Incomplete | [RFCXXXX] |  //see coap-18 (block tranfsers, RFC not assigned)
   COAP_RESP_CODE_PRECONDITION_FAILED = 0x8C,   //4.12 | Precondition Failed | [RFC7252] |
   COAP_RESP_CODE_TOO_LARGE = 0x8D,       //4.13 | Request Entity Too Large | [RFC7252] |
   COAP_RESP_CODE_UNSUPPORTED_CONTENT_FORMAT = 0x8F,   //4.15 | Unsupported Content-Format | [RFC7252] |
   COAP_RESP_CODE_SERVER_ERROR = 0xA0,    //5.00 | Internal Server Error | [RFC7252] |
   COAP_RESP_CODE_NOT_IMPLEMENTED = 0xA1, //5.01 | Not Implemented | [RFC7252] |
   COAP_RESP_CODE_BAD_GATEWAY = 0xA2,     //5.02 | Bad Gateway | [RFC7252] |
   COAP_RESP_CODE_SERVICE_UNAVAILABLE = 0xA3,   //5.03 | Service Unavailable | [RFC7252] |
   COAP_RESP_CODE_GATEWAY_TIMTOUT = 0xA4, //5.04 | Gateway Timeout | [RFC7252] |
   COAP_RESP_CODE_PROXY_NOT_SUPPORTED = 0xA5,   //5.05 | Proxying Not Supported | [RFC7252]
   
   // values 0x100 or greater are internal errors
   COAP_RESP_CODE_NOT_STARTED = 0xFFFF, // you called CoapClientGetResponse() without a CoapClientSend*****()
   COAP_RESP_CODE_BUSY = 0xFFFE,    //coap is still busy sending the request, call again.
   COAP_RESP_CODE_DNS_LOOKUP_FAIL = 0xFFFD,  //failed resolving hostname
   COAP_RESP_CODE_TIMEOUT = 0xFFFC,  //timeout waiting for an ack
   COAP_RESP_PHY_ERR = 0xFFFB //some kind of hardware error preventing us from transmitting a UDP datagram
} coap_response_code_t;

// If the return is COAP_RESP_CODE_BUSY then it's still busy and you need to 
// call this again.
coap_response_code_t CoapClientGetResponse(void);

// if CoapClientGetResponse() returns success, you can use this function
// to determine the number of bytes saved to the pointer set by 
// CoapClientSetResponsePointer().
size_t CoapClientGetResponseSize(void);

// 'code' is the result from CoapClientGetResponse(), and this simply looks
// to see if it was a success.
#define COAP_CODE_IS_SUCCESS(code)  ((code < 0x100) && ((code & 0xE0)==0x40))

// DO NOT CALL THESE - THESE ARE ALREADY CALLED AND HANDLED BY THE STACK!
void CoapClientInit(void);
void CoapClientTask(void);

#endif   //__CCS_COAP_H__
