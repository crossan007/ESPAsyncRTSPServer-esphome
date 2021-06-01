/**
 * This library is an adaptation of https://github.com/geeksville/Micro-RTSP
 * suited for use with the AsyncTCP library https://github.com/me-no-dev/AsyncTCP
 * Tutorial: https://www.medialan.de/usecase0001.html
 * 
 * RTSP Resources
 *   RFC: https://datatracker.ietf.org/doc/html/rfc2326#page-33
 *   Wiki: https://en.wikipedia.org/wiki/Real_Time_Streaming_Protocol
 *   RTP/RTPC Packet Structure: https://datatracker.ietf.org/doc/html/rfc3550
 */

#pragma once
#include "Arduino.h"
#include <AsyncTCP.h>
#include <WiFiUdp.h>
#include <stdio.h>

typedef std::function<void (void *)> RTSPConnectHandler;
typedef std::function<void (String ) > LogFunction;

struct dimensions {
  uint width;
  uint height;
};

// Forward declaration to get around circular dependency, since
// the client only references a pointer to the server
class AsyncRTSPServer;
class AsyncRTSPRequest;
class AsyncRTSPResponse;


struct RTPBuffferPreparationResult {
  int offset;
  int bufferSize;
  bool isLastFragment;
};

// class declarations

class AsyncRTSPClient {
  friend class AsyncRTSPRequest;
  public:
    AsyncRTSPClient(AsyncClient* client, AsyncRTSPServer * server);
    ~AsyncRTSPClient();
    void PushRTPBuffer(const char* RTPBuffer, size_t length);
    String getFriendlyName();
    boolean getIsCurrentlyStreaming();
    WiFiUDP udp;

  private:
    void handleRTSPRequest(AsyncRTSPRequest*, AsyncRTSPResponse*);
    AsyncClient * _tcp_client;
    AsyncRTSPServer * server;
    boolean _isCurrentlyStreaming;
    String _temp;
    int _RTPPortInt;
    String _RTPPort;
    String _RTCPPort;

    uint RtspSessionID;
  
};



class AsyncRTSPServer {
  public:
    AsyncRTSPServer(uint16_t port, dimensions dim);
    ~AsyncRTSPServer();

    void begin();
    void end();
    void onClient(RTSPConnectHandler callback, void* arg);
    void pushFrame(uint8_t* data, size_t length);
    void setLogFunction(LogFunction logger, void* arg);
    void writeLog(String log);
    int GetRTSPServerPort();
    int GetRTCPServerPort();
    boolean hasClients();

    //void streamImage();
  protected:
    AsyncServer _server;
    void* that;
    void* thatlog;
    

  private:
    AsyncRTSPClient* client;
    RTSPConnectHandler connectCallback;
    LogFunction loggerCallback;
    int RtpServerPort;
    int RtcpServerPort;
    char* RTPBuffer; // Note: we assume single threaded, this large buf we keep off of the tiny stack
    void PrepareRTPBufferForClients(
      char* RTPBuffer, 
      uint8_t* data, 
      int length, 
      RTPBuffferPreparationResult* bpr, 
      unsigned const char * quant0tbl, 
      unsigned const char * quant1tbl);
    u_short m_SequenceNumber;
    uint32_t m_Timestamp;
    dimensions _dim;
   
    

};

/**
 * Implementation of https://datatracker.ietf.org/doc/html/rfc2326#section-6
 */
class AsyncRTSPRequest { 
  public:
    AsyncRTSPRequest(String rawRequest);
    ~AsyncRTSPRequest();
    String rawRequest;
    String Method;
    String RequestURI;
    String RTSPVersion;
    String Body;
    String Headers;
    String toString();

    String GetHeaderValue(String Header);


  private:

};

class AsyncRTSPResponse { 
  public:
    AsyncRTSPResponse(AsyncClient* c, AsyncRTSPRequest* r);
    ~AsyncRTSPResponse();
    int Status;
    void Send();
    String Body;
    String Headers;

  private:
    AsyncClient* _tcpClient;
    AsyncRTSPRequest* _request;
    char * DateHeader();
    String ContentLengthHeader();

};


/**
 * Handles modifying/stringifying the key-value attributes for the RTSP 
 * stream as defined at https://datatracker.ietf.org/doc/html/rfc2326#appendix-C.1.1
 * and full session description here: https://datatracker.ietf.org/doc/html/rfc2327#section-6
 * used for responding to the DESCRIBE request 
 * https://datatracker.ietf.org/doc/html/rfc2326#section-10.2
 */
class RTSPMediaLevelAttributes {

/*
Session description
        v=  (protocol version)
        o=  (owner/creator and session identifier).
        s=  (session name)
        i=* (session information)
        u=* (URI of description)
        e=* (email address)
        p=* (phone number)
        c=* (connection information - not required if included in all media)
        b=* (bandwidth information)
        One or more time descriptions (see below)
        z=* (time zone adjustments)
        k=* (encryption key)
        a=* (zero or more session attribute lines)
        Zero or more media descriptions (see below)

Time description
        t=  (time the session is active)
        r=* (zero or more repeat times)

Media description
        m=  (media name and transport address)
        i=* (media title)
        c=* (connection information - optional if included at session-level)
        b=* (bandwidth information)
        k=* (encryption key)
        a=* (zero or more media attribute lines)
*/
  public:
    static String toString();
};