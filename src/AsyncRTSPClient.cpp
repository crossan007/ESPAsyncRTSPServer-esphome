/**
 * This library is an adaptation of https://github.com/geeksville/Micro-RTSP
 * suited for use with the AsyncTCP library https://github.com/me-no-dev/AsyncTCP
 * 
 */

#include <AsyncRTSP.h>


AsyncRTSPClient::AsyncRTSPClient(AsyncClient* c, AsyncRTSPServer * server)
{
  this->_tcp_client = c;
  this->server = server;
  
  String t = "Connected new RTSP Client: " + getFriendlyName();
  String* tt = &t;
  this->server->writeLog(tt);

  //void*, AsyncClient*, void *data, size_t len
  c->onData([this](void* p, AsyncClient* c, void *data, size_t len) {
    if( this->_currentRequest == NULL) {
      this->_currentRequest = new AsyncRTSPRequest(this);
    }
    this->_currentRequest->_onData(data,len);

  });
  
  
} 

String AsyncRTSPClient::getFriendlyName() {
  String address = this->_tcp_client->remoteIP().toString();
  address += ":" + this->_tcp_client->remotePort();
  address += "=>" + this->_tcp_client->localPort();
  return address;
}

void AsyncRTSPClient::pushFrame(uint8_t* data, size_t length) {
  if (this->_tcp_client != NULL) {
    //this->_tcp_client->write((char*)data,length);
  }

}


AsyncRTSPRequest::AsyncRTSPRequest(AsyncRTSPClient* c)
  : _client(c){
    String* l = new String("Receiving Data from ");
    c->server->writeLog(l);

}

void AsyncRTSPRequest::_onData(void *buf, size_t len) {

  char* str = (char*)buf;
  _temp.reserve(_temp.length() + len);
  _temp.concat(str);
  String q = "Received Data: " + _temp;
  String* qq = &q;
  
  this->_client->server->writeLog(qq);
}