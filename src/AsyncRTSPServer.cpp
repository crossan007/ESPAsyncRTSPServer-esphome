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
    rtps->writeLog(new String("hey"));

  }, this);

}

void AsyncRTSPServer::writeLog(String* log) {
  if (this->loggerCallback != NULL) {
    this->loggerCallback(log);
  }
}

void AsyncRTSPServer::pushFrame(uint8_t* data, size_t length) {
  if (this->client != NULL) {
    this->client->pushFrame(data,length);
  }
}

void AsyncRTSPServer::onClient(RTSPConnectHandler callback, void* that) {
  this->connectCallback = callback;
  this->that = that;
}

void AsyncRTSPServer::setLogFunction(LogFunction callback, void* that) {
  this->loggerCallback = callback;
  this->thatlog = that;
}



void AsyncRTSPServer::begin(){
  _server.setNoDelay(true);
  _server.begin();
}

AsyncRTSPClient::AsyncRTSPClient(AsyncClient* c)
{
  this->client = c;
} 

void AsyncRTSPClient::pushFrame(uint8_t* data, size_t length) {
  if (this->client != NULL) {
    this->client->write((char*)data,length);
  }

}
