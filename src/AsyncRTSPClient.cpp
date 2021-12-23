/**
 * This library is an adaptation of https://github.com/geeksville/Micro-RTSP
 * suited for use with the AsyncTCP library https://github.com/me-no-dev/AsyncTCP
 * 
 */

#include <AsyncRTSP.h>


#define getRandom() random(65536)

/**
 * Sets up a new client connection / session
 * 
 * Creates the callback function to listen for incoming TCP data
 * and parses to RTSP messages
 * 
 */
AsyncRTSPClient::AsyncRTSPClient(AsyncClient* c, AsyncRTSPServer * server)
{
  udp.begin(8830);
  this->_tcp_client = c;
  this->server = server;
  this->_isCurrentlyStreaming = false;
  this->RtspSessionID = getRandom();
  this->RtspSessionID |= 0x80000000;
  
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

    // TODO: this may be naieve to assume a complete RTSP request
    int endS = _temp.indexOf("\r\n\r\n");
    char e[10];
    char t[10];
    itoa(endS,e,10);
    itoa(_temp.length(),t,10);

    if (endS == _temp.length()-4 ) {
      AsyncRTSPRequest* req = new AsyncRTSPRequest(_temp);
      AsyncRTSPResponse* resp = new AsyncRTSPResponse(c, req);
      this->handleRTSPRequest(req, resp);
      _temp="";
    }

  });
 
} 


void AsyncRTSPClient::handleRTSPRequest(AsyncRTSPRequest* req, AsyncRTSPResponse* res){

  if(req->Method =="OPTIONS") {
    res->Status = 200;
    res->Headers = "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n";
    res->Send();
    
  }
  else if(req->Method=="DESCRIBE") {
    res->Status = 200;
    res->Body = RTSPMediaLevelAttributes::toString();
    res->Send();
  }
  else if(req->Method=="SETUP") {
    String transport = req->GetHeaderValue("Transport");
    int client_port = transport.indexOf("client_port=") + 12;
    int dash = transport.indexOf("-",client_port);
    int end = transport.indexOf(";",dash);
    this->_RTPPort = transport.substring(client_port, dash);
    this->_RTPPortInt = this->_RTPPort.toInt();
    this->_RTCPPort = transport.substring(dash+1, end);
    this->server->writeLog("RTP Port: " + this->_RTPPort+"; RTCP Port: " + this->_RTCPPort);
    res->Status = 200;

    char transportResponse[255];
    sprintf(
      transportResponse,
      "Transport: RTP/AVP/UDP;unicast;destination=%s;client_port=%s-%s;server_port=%u-%u;mode=play\r\n",
      this->_tcp_client->remoteIP().toString().c_str(),
      this->_RTPPort.c_str(),
      this->_RTCPPort.c_str(),
      this->server->GetRTSPServerPort(),
      this->server->GetRTCPServerPort()
      );

    char sess[20];
    sprintf(sess,"%u",this->RtspSessionID);



    res->Headers = String(transportResponse) + "Session: " + String(sess)+"\r\n";
    res->Send();
  }
  else if(req->Method=="PLAY") {
    this->_isCurrentlyStreaming = true;
    res->Status = 200;
    res->Send();
  }
  else if(req->Method=="TEARDOWN") {
    this->_isCurrentlyStreaming = false;
    res->Status = 200;
    res->Send();
  }

  
  else {
    this->server->writeLog("Could not handle " + req->Method +  " request: \n\n" + req->rawRequest);
    return;
  }
  this->server->writeLog("Handled " + req->Method +  " request from " + this->getFriendlyName() + ". seq: " + req->GetHeaderValue("CSeq"));
}

String AsyncRTSPClient::getFriendlyName() {
  String address = this->_tcp_client->remoteIP().toString();
  return address;
}

void AsyncRTSPClient::PushRTPBuffer(const char* RTPBuffer, size_t length) {
  udp.beginPacket(this->_tcp_client->remoteIP(),this->_RTPPortInt);

  const uint8_t* udpBuffer = (uint8_t*)(RTPBuffer+4);
  udp.write(udpBuffer,length-4);
  udp.endPacket();
  //this->server->writeLog("Wrote UDP Packet to " + String(this->_RTPPortInt));
}

boolean AsyncRTSPClient::getIsCurrentlyStreaming() {
  return this->_isCurrentlyStreaming;
}

void AsyncRTSPClient::stopStreaming() {
  this->_isCurrentlyStreaming = false;
}


AsyncRTSPRequest::AsyncRTSPRequest(String rawRequest) {
  this->rawRequest = rawRequest;
  int methodEnd = rawRequest.indexOf(" ");
  this->Method = rawRequest.substring(0,methodEnd);

  int uriEnd = rawRequest.indexOf(" ",methodEnd+1);
  this->RequestURI = rawRequest.substring(methodEnd + 1, uriEnd);

  int requestLineEnd = rawRequest.indexOf("\r\n",uriEnd+1);
  this->RTSPVersion = rawRequest.substring(uriEnd+1,requestLineEnd);

  this->Headers = rawRequest.substring(requestLineEnd+2,rawRequest.length()-2);
}
 
String AsyncRTSPRequest::toString() {
  return "Method: " + this->Method +"\n"
    + "URI: " + this->RequestURI + "\n"
    + "Version:" + this->RTSPVersion + "\n"
    + "Sequence: " + this->GetHeaderValue("CSeq");
}

String AsyncRTSPRequest::GetHeaderValue(String headerName) {
  int headerLineStart = this->Headers.indexOf(headerName);
  int headerNameEnd = this->Headers.indexOf(":",headerLineStart);
  int headerLineEnd = this->Headers.indexOf("\r\n",headerLineStart);
  return this->Headers.substring(headerNameEnd+2,headerLineEnd); // TODO, make sure we ignore whitespace after the colon
}

AsyncRTSPResponse::AsyncRTSPResponse(AsyncClient* c, AsyncRTSPRequest* r)
  :_tcpClient(c), _request(r)
  {

}

void AsyncRTSPResponse::Send(){
  String sendBody = String();
  if (this->Status == 200) {
    sendBody += "RTSP/1.0 200 OK\r\nCSeq: " + _request->GetHeaderValue("CSeq") + "\r\n";
  }
  sendBody += this->Headers;
  if (this->Body.length() > 0) {
    sendBody += this->DateHeader();
    sendBody += this->ContentLengthHeader();
    sendBody += "\r\n";
    sendBody += this->Body;
  }
  // TODO make sure we always end this packet with a fully blank line
  sendBody += "\r\n";
  this->_tcpClient->write(sendBody.c_str());
  this->_tcpClient->send();
}

String AsyncRTSPResponse::ContentLengthHeader() {
  char t[10];
  itoa(this->Body.length()+2,t,10);
  return String("Content-Length: ") + t + "\r\n";
}

char * AsyncRTSPResponse::DateHeader() {
  static char buf[200];
  time_t tt = time(NULL);
  strftime(buf, sizeof buf, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
  return buf;
}

// https://www.ietf.org/rfc/rfc4566.txt page 21/22
String RTSPMediaLevelAttributes::toString() {
  return "v=0\r\n"
        "o=d 1  1 IN IP4 0.0.0.0\r\n"
        "s=ESPHome RTSP Stream\r\n"
        // If the stop time is 0 then the session is unbounded. If the start time is also zero then the session is considered permanent. Unbounded and permanent sessions are discouraged but not prohibited.
        "t=0 0\r\n" 
        "m=video 0 RTP/AVP 26\r\n"
        "c=IN IP4 0.0.0.0\r\n";
}

