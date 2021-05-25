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

// class declarations

class AsyncRTSPClient {
  friend class AsyncRTSPRequest;
  public:
    AsyncRTSPClient(AsyncClient* client, AsyncRTSPServer * server);
    ~AsyncRTSPClient();
    void pushFrame(uint8_t* data, size_t length);
    String getFriendlyName();

  private:
    AsyncClient * _tcp_client;
    AsyncRTSPServer * server;
    AsyncRTSPRequest * _currentRequest;
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

class AsyncRTSPRequest { 
  friend class AsyncRTSPServer;
  public:
    AsyncRTSPRequest(AsyncRTSPClient*);
    ~AsyncRTSPRequest();
    void _onData(void *buf, size_t len);
  private:
    AsyncRTSPClient* _client;
    String _temp;

};

