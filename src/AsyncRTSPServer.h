/**
 * This library is an adaptation of https://github.com/geeksville/Micro-RTSP
 * suited for use with the AsyncTCP library https://github.com/me-no-dev/AsyncTCP
 * 
 */

#pragma once
#include "Arduino.h"
#include <AsyncTCP.h>
#include <stdio.h>


typedef std::function<void (void *)> RTSPConnectHandler;
typedef std::function<void (String *)> LogFunction;

class AsyncRTSPClient {
  public:
    AsyncRTSPClient(AsyncClient* client);
    ~AsyncRTSPClient();
    void pushFrame(uint8_t* data, size_t length);


  private:
    AsyncClient * client;
  
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

    //void streamImage();
  protected:
    AsyncServer _server;
    void* that;
    void* thatlog;

  private:
    AsyncRTSPClient* client;
    RTSPConnectHandler connectCallback;
    LogFunction loggerCallback;
    void writeLog(String* log);

};


