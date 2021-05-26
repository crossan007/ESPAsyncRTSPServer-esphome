/**
 * This library is an adaptation of https://github.com/geeksville/Micro-RTSP
 * suited for use with the AsyncTCP library https://github.com/me-no-dev/AsyncTCP
 * 
 * RTSP Resources
 *   RFC: https://datatracker.ietf.org/doc/html/rfc2326#page-33
 *   Wiki: https://en.wikipedia.org/wiki/Real_Time_Streaming_Protocol
 */

#pragma once
#include "Arduino.h"
#include <AsyncTCP.h>
#include <stdio.h>


typedef std::function<void (void *)> RTSPConnectHandler;
typedef std::function<void (String ) > LogFunction;

// Forward declaration to get around circular dependency, since
// the client only references a pointer to the server
class AsyncRTSPServer;
class AsyncRTSPRequest;
class AsyncRTSPResponse;

// class declarations

class AsyncRTSPClient {
  friend class AsyncRTSPRequest;
  public:
    AsyncRTSPClient(AsyncClient* client, AsyncRTSPServer * server);
    ~AsyncRTSPClient();
    void pushFrame(uint8_t* data, size_t length);
    String getFriendlyName();

  private:
    void handleRTSPRequest(AsyncRTSPRequest*, AsyncRTSPResponse*);
    AsyncClient * _tcp_client;
    AsyncRTSPServer * server;
    boolean _isCurrentlyStreaming;
    String _temp;
};

class AsyncRTSPServer {
  public:
    AsyncRTSPServer(uint16_t port);
    ~AsyncRTSPServer();

    void begin();
    void end();
    void onClient(RTSPConnectHandler callback, void* arg);
    void pushFrame(uint8_t* data, size_t length);
    void setLogFunction(LogFunction logger, void* arg);
    void writeLog(String log);

    //void streamImage();
  protected:
    AsyncServer _server;
    void* that;
    void* thatlog;

  private:
    AsyncRTSPClient* client;
    RTSPConnectHandler connectCallback;
    LogFunction loggerCallback;
   
    

};


/**
 * Implementation of https://datatracker.ietf.org/doc/html/rfc2326#section-6
 */
class AsyncRTSPRequest { 
  public:
    AsyncRTSPRequest(String rawRequest);
    ~AsyncRTSPRequest();
    String Method;
    String RequestURI;
    String RTSPVersion;
    String Body;
    String Seq;
    String toString();


  private:
    

};

class AsyncRTSPResponse { 
  public:
    AsyncRTSPResponse(AsyncClient* c);
    ~AsyncRTSPResponse();
    void Send(String data);

  private:
    AsyncClient* _tcpClient;

};


