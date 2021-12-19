/**
 * This library is an adaptation of https://github.com/geeksville/Micro-RTSP
 * suited for use with the AsyncTCP library https://github.com/me-no-dev/AsyncTCP
 * JPEG over RTP packet format: https://datatracker.ietf.org/doc/html/rfc2435
 * 
 */

#include <AsyncRTSP.h>
#include "JPEGHelpers.cpp"

AsyncRTSPServer::AsyncRTSPServer(uint16_t port, dimensions dim) : _server(port),
                                                                  _dim(dim)
{

  this->client = nullptr;
  this->prevMsec = millis();
  this->curMsec = this->prevMsec;

  this->RTPBuffer = new char[2048]; // Note: we assume single threaded, this large buf we keep off of the tiny stack

  _server.onClient([this](void *s, AsyncClient *c)
                   {
                     AsyncRTSPServer *rtps = (AsyncRTSPServer *)s;

                     rtps->client = new AsyncRTSPClient(c, this);
                     rtps->connectCallback(rtps->that);
                   },
                   this);

  this->loggerCallback = NULL;
  this->currentFrame = {
    nullptr,
    nullptr,
    nullptr,
    0
  };
  this->bpr = {0, 0, false};
}

void AsyncRTSPServer::writeLog(String log)
{

  if (this->loggerCallback != NULL)
  {
    this->loggerCallback(log);
  }
}

boolean AsyncRTSPServer::hasClients()
{
  return this->client != nullptr && this->client->getIsCurrentlyStreaming();
}

void AsyncRTSPServer::pushFrame(uint8_t *data, size_t length, std::shared_ptr<void> image)
{
  // only decode the JPEG if we actually have clients connected.
  if (!this->hasClients())
  {
    return;
  }

  this->currentFrameSharedPointer = image;

  printf("Pushing frame %u\n", millis());
  this->curMsec = millis();
  this->deltams = (this->curMsec >= this->prevMsec) ? this->curMsec - this->prevMsec : 100;
  this->prevMsec = this->curMsec;
  printf("Delta MS %u\n", this->deltams);
  printf("CHANGED TIMESTAMP FROM %u\n", this->m_Timestamp);
  this->m_Timestamp += (RTP_TIMESTAMP_HZ * deltams) / 1000; 
  printf("CHANGED TIMESTAMP TO %u\n" , this->m_Timestamp);


  if (!decodeJPEGfile(&data, &length, &this->currentFrame))
  {
    this->loggerCallback("Cannot decode JPEG Data");
    return;
  }
  
}

void AsyncRTSPServer::tick()
{
  if (this->currentFrame.scanDataLength  == 0 || this->currentFrame.scanData == nullptr ) {
    //this->loggerCallback("Skipping RTP PAcket prep");
    return;
  }

  if (this->hasClients() ) {
    //this->loggerCallback("prepping RTSP Packet");
    PrepareRTPBufferForClients(
        this->RTPBuffer,
        this->currentFrame.scanData,
        this->currentFrame.scanDataLength,
        &this->bpr,
        this->currentFrame.quant0tbl,
        this->currentFrame.quant1tbl);
    //this->loggerCallback("sending RTSP Packet");
    this->client->PushRTPBuffer(this->RTPBuffer, this->bpr.bufferSize);
    if (this->bpr.isLastFragment) {
      this->bpr = {0, 0, false};
      this->loggerCallback("RTSP server pushed last camera ");
      this->frameFinishedCallback();
      // free the buffer
      this->currentFrameSharedPointer = nullptr;
    }
    
  }
  else {
    // free the buffer
    this->currentFrameSharedPointer = nullptr;
  }
}

void AsyncRTSPServer::onClient(RTSPConnectHandler callback, void *that)
{
  this->connectCallback = callback;
  this->that = that;
}

void AsyncRTSPServer::onFrameFinished( std::function<void ()> callback, void *that) {
  this->frameFinishedCallback = callback;
  this->that = that;
}

void AsyncRTSPServer::setLogFunction(LogFunction callback, void *that)
{
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

void AsyncRTSPServer::PrepareRTPBufferForClients(
    char *RtpBuf,
    uint8_t *data,
    int length,
    RTPBuffferPreparationResult *bpr,
    unsigned const char *quant0tbl,
    unsigned const char *quant1tbl)
{

#define KRtpHeaderSize 12 // size of the RTP header
#define KJpegHeaderSize 8 // size of the special JPEG payload header

#define MAX_FRAGMENT_SIZE 1100 // FIXME, pick more carefully
  int fragmentLen = MAX_FRAGMENT_SIZE;
  if (fragmentLen + bpr->offset >= length)
  {                                         // Shrink last fragment if needed
    fragmentLen = length - bpr->offset - 2; // the JPEG end marker (FFD9) will be in this fragment.  drop it from the end
    bpr->isLastFragment = true;
  }

  // Do we have custom quant tables? If so include them per RFC

  bool includeQuantTbl = quant0tbl && quant1tbl && bpr->offset == 0;
  uint8_t q = includeQuantTbl ? 128 : 0x5e;

  bpr->bufferSize = 4 + fragmentLen + KRtpHeaderSize + KJpegHeaderSize + (includeQuantTbl ? (4 + 64 * 2) : 0);

  memset(RtpBuf, 0x00, sizeof(RtpBuf));
  // Prepare the first 4 byte of the packet. This is the Rtp over Rtsp header in case of TCP based transport
  RtpBuf[0] = '$'; // magic number
  RtpBuf[1] = 0;   // number of multiplexed subchannel on RTPS connection - here the RTP channel
  RtpBuf[2] = (bpr->bufferSize & 0x0000FF00) >> 8;
  RtpBuf[3] = (bpr->bufferSize & 0x000000FF);
  // Prepare the 12 byte RTP header
  RtpBuf[4] = 0x80;                                       // RTP version
  RtpBuf[5] = 0x1a | (bpr->isLastFragment ? 0x80 : 0x00); // JPEG payload (26) and marker bit
  RtpBuf[7] = m_SequenceNumber & 0x0FF;                   // each packet is counted with a sequence counter
  RtpBuf[6] = m_SequenceNumber >> 8;
  RtpBuf[8] = (this->m_Timestamp & 0xFF000000) >> 24; // each image gets a timestamp
  RtpBuf[9] = (this->m_Timestamp & 0x00FF0000) >> 16;
  RtpBuf[10] = (this->m_Timestamp & 0x0000FF00) >> 8;
  RtpBuf[11] = (this->m_Timestamp & 0x000000FF);
  RtpBuf[12] = 0x13; // 4 byte SSRC (sychronization source identifier)
  RtpBuf[13] = 0xf9; // we just an arbitrary number here to keep it simple
  RtpBuf[14] = 0x7e;
  RtpBuf[15] = 0x67;

  // Prepare the 8 byte payload JPEG header
  RtpBuf[16] = 0x00;                             // type specific
  RtpBuf[17] = (bpr->offset & 0x00FF0000) >> 16; // 3 byte fragmentation offset for fragmented images
  RtpBuf[18] = (bpr->offset & 0x0000FF00) >> 8;
  RtpBuf[19] = (bpr->offset & 0x000000FF);

  /*    These sampling factors indicate that the chrominance components of
       type 0 video is downsampled horizontally by 2 (often called 4:2:2)
       while the chrominance components of type 1 video are downsampled both
       horizontally and vertically by 2 (often called 4:2:0). */
  RtpBuf[20] = 0x00;                  // type (fixme might be wrong for camera data) https://tools.ietf.org/html/rfc2435
  RtpBuf[21] = q;                     // quality scale factor was 0x5e
  RtpBuf[22] = this->_dim.width / 8;  // width  / 8
  RtpBuf[23] = this->_dim.height / 8; // height / 8

  int headerLen = 24; // Inlcuding jpeg header but not qant table header
  if (includeQuantTbl)
  {                 // we need a quant header - but only in first packet of the frame
    RtpBuf[24] = 0; // MBZ
    RtpBuf[25] = 0; // 8 bit precision
    RtpBuf[26] = 0; // MSB of lentgh

    int numQantBytes = 64;         // Two 64 byte tables
    RtpBuf[27] = 2 * numQantBytes; // LSB of length

    headerLen += 4;

    memcpy(RtpBuf + headerLen, quant0tbl, numQantBytes);
    headerLen += numQantBytes;

    memcpy(RtpBuf + headerLen, quant1tbl, numQantBytes);
    headerLen += numQantBytes;
  }

  // append the JPEG scan data to the RTP buffer
  memcpy(RtpBuf + headerLen, data + bpr->offset, fragmentLen);
  bpr->offset += fragmentLen;

  m_SequenceNumber++; // prepare the packet counter for the next packet
}

void AsyncRTSPServer::begin()
{
  _server.setNoDelay(true);
  _server.begin();
}
