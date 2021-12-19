#pragma once
#include "Arduino.h"
// Image header bytes
#define JEPG_StartOfImage 0xd8
#define JPEG_APP_0 0xe0
#define JPEG_DefineQuantizationTable 0xdb
#define JPEG_DefineHuffmanTable 0xc4
#define JPEG_StartOfScan 0xda
#define JPEG_StartBaselineDCTFrame 0xc0
#define JPEG_EndOfImage 0xd9

typedef unsigned char* BufPtr;

struct DecodedJPEGFrame {
  unsigned char * quant0tbl;
  unsigned char * quant1tbl;
  unsigned char * scanData;
  uint32_t scanDataLength;
};