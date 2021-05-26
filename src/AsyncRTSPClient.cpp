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
  this->server->writeLog(t);

  this->_currentRequest = new AsyncRTSPRequest(this);
  //void*, AsyncClient*, void *data, size_t len
  c->onData([this](void* p, AsyncClient* c, void *data, size_t len) {
    this->_currentRequest->_onData(data,len);

  });
  
  
} 

String AsyncRTSPClient::getFriendlyName() {
  String address = this->_tcp_client->remoteIP().toString();
  return address;
}

void AsyncRTSPClient::pushFrame(uint8_t* data, size_t length) {
  if (this->_tcp_client != NULL) {
    //this->_tcp_client->write((char*)data,length);
  }

}


AsyncRTSPRequest::AsyncRTSPRequest(AsyncRTSPClient* c)
  : _client(c){

    _temp = String();

    c->server->writeLog("Receiving Data from " + c->getFriendlyName());

}

void AsyncRTSPRequest::_onData(void *buf, size_t len) {

  _client->server->writeLog("Received Data length: ");

  char *str = (char*)buf;
  // super janky way to pass a null-terminated char array to the string
  // and then just add the last char to the end of the string
  
  char last = str[len-1];
  str[len-1] = 0;
  _temp.reserve(_temp.length() + len);
  _temp.concat(str);
  _temp.concat(last);
  String q = "Received Data: '" + _temp +"'";
  this->_client->server->writeLog(q);

  // TODO: this may be naieve to assume a complete RTSP request
  if (_temp.substring(_temp.length()-2, _temp.length()-1) == "\r" ) {
    this->_client->server->writeLog("We got a full command!");
  }
}
