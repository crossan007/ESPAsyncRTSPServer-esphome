/**
 * This library is an adaptation of https://github.com/geeksville/Micro-RTSP
 * suited for use with the AsyncTCP library https://github.com/me-no-dev/AsyncTCP
 * 
 */

#include <AsyncRTSP.h>

/**
 * Sets up a new client connection / session
 * 
 * Creates the callback function to listen for incoming TCP data
 * and parses to RTSP messages
 * 
 */
AsyncRTSPClient::AsyncRTSPClient(AsyncClient* c, AsyncRTSPServer * server)
{
  this->_tcp_client = c;
  this->server = server;
  this->_isCurrentlyStreaming = false;
  
  String t = "Connected new RTSP Client: " + getFriendlyName();
  this->server->writeLog(t);

  //void*, AsyncClient*, void *data, size_t len
  c->onData([this](void* p, AsyncClient* c, void *data, size_t len) {
    char *str = (char*)data;
    // super janky way to pass a null-terminated char array to the string
    // and then just add the last char to the end of the string
    
    char last = str[len-1];
    str[len-1] = 0;
    this->_temp.reserve(_temp.length() + len);
    _temp.concat(str);
    _temp.concat(last);
    String q = "Received Data: '" + _temp +"'";
    this->server->writeLog(q);

    // TODO: this may be naieve to assume a complete RTSP request
    int endS = _temp.indexOf("\r\n\r\n");
    char e[10];
    char t[10];
    itoa(endS,e,10);
    itoa(_temp.length(),t,10);
    this->server->writeLog("Index of end sequence: " + String(e) + ". total len: " + String(t));


    if (endS == _temp.length()-4 ) {
      AsyncRTSPRequest* req = new AsyncRTSPRequest(_temp);
      AsyncRTSPResponse* resp = new AsyncRTSPResponse(c);
      this->handleRTSPRequest(req, resp);
    }

  });
 
} 


void AsyncRTSPClient::handleRTSPRequest(AsyncRTSPRequest* req, AsyncRTSPResponse* res){
  this->server->writeLog(req->toString());

  if(req->Method =="OPTIONS") {
    res->Send("RTSP/1.0 200 OK\r\nCSeq: " + req->Seq + "\r\nPublic: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n\r\n");
    this->server->writeLog("Responded to request");
  }
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


AsyncRTSPRequest::AsyncRTSPRequest(String rawRequest) {
  int methodEnd = rawRequest.indexOf(" ");
  this->Method = rawRequest.substring(0,methodEnd);

  int uriEnd = rawRequest.indexOf(" ",methodEnd+1);
  this->RequestURI = rawRequest.substring(methodEnd + 1, uriEnd);

  int requestLineEnd = rawRequest.indexOf("\r\n",uriEnd+1);
  this->RTSPVersion = rawRequest.substring(uriEnd+1,requestLineEnd);

  int seqStart = rawRequest.indexOf("CSeq:");
  int seqEnd = rawRequest.indexOf("\n",seqStart);
  this->Seq = rawRequest.substring(seqStart+6,seqEnd-2);
}
 
String AsyncRTSPRequest::toString() {
  return "Method: " + this->Method +"\n"
    + "URI: " + this->RequestURI + "\n"
    + "Version:" + this->RTSPVersion + "\n"
    + "Sequence: " + this->Seq;
}

AsyncRTSPResponse::AsyncRTSPResponse(AsyncClient* c)
  :_tcpClient(c)
  {

}

void AsyncRTSPResponse::Send(String data){
  this->_tcpClient->write(data.c_str());
  this->_tcpClient->send();
}