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

class AsyncRTSPClient {
  public:
    AsyncRTSPClient(AsyncClient* client);
    ~AsyncRTSPClient();

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

    //void streamImage();
  protected:
    AsyncServer _server;
    void* that;

  private:
    AsyncRTSPClient* client;
    RTSPConnectHandler connectCallback;

};


