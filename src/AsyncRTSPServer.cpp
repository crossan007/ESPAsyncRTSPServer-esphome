/**
 * This library is an adaptation of https://github.com/geeksville/Micro-RTSP
 * suited for use with the AsyncTCP library https://github.com/me-no-dev/AsyncTCP
 * 
 */

#include <AsyncRTSP.h>

AsyncRTSPServer::AsyncRTSPServer(uint16_t port): 
  _server(port) {

  _server.onClient([this](void *s, AsyncClient* c){
  
   AsyncRTSPServer* rtps = (AsyncRTSPServer*)s;
    
    rtps->client = new AsyncRTSPClient(c,this);
    rtps->connectCallback(rtps->that);

  }, this);

  this->loggerCallback = NULL;
}

// TODO: WHY does passing a String cause a crash, but a string poitner just works
//fighting with this: https://techtutorialsx.com/2017/05/07/esp32-arduino-passing-a-variable-as-argument-of-a-freertos-task/ 
void AsyncRTSPServer::writeLog(String log) {

  if (this->loggerCallback != NULL) {
    this->loggerCallback(log);
  }
  
}

void AsyncRTSPServer::pushFrame(uint8_t* data, size_t length) {
  if (this->client != NULL) {
    //this->client->pushFrame(data,length);
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

int AsyncRTSPServer::GetRTSPServerPort()
{
  return this->RtpServerPort;
}

int AsyncRTSPServer::GetRTCPServerPort()
{
  return this->RtcpServerPort;
}



void AsyncRTSPServer::begin(){
  _server.setNoDelay(true);
  _server.begin();
}
