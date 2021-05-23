/**
 * This library is an adaptation of https://github.com/geeksville/Micro-RTSP
 * suited for use with the AsyncTCP library https://github.com/me-no-dev/AsyncTCP
 * 
 */

#include <AsyncRTSPServer.h>

AsyncRTSPServer::AsyncRTSPServer(uint16_t port): 
  _server(port) {

  _server.onClient([](void *s, AsyncClient* c){
  
   AsyncRTSPServer* rtps = (AsyncRTSPServer*)s;
    
    rtps->client = new AsyncRTSPClient(c);
    rtps->connectCallback(rtps->that);

  }, this);

  
}

void AsyncRTSPServer::onClient(RTSPConnectHandler callback, void* that) {
  this->connectCallback = callback;
  this->that = that;
}

void AsyncRTSPServer::begin(){
  _server.setNoDelay(true);
  _server.begin();
}

AsyncRTSPClient::AsyncRTSPClient(AsyncClient* c)
{
  this->client = c;
} 