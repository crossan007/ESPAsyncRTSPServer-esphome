// References:
// https://www.fileformat.info/format/jpeg/egff.htm
// https://www.videotechnology.com/jpeg/j1.html

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


// TODO: Replace this function with one that returns a struct containing the details of the 
// header it just found (including things like length, code, etc) so that
// we can remove duplicate logic later on (for things like figuring out the length of the SOS markers)

// search for a particular JPEG marker, moves *start to just after that marker
// This function fixes up the provided start ptr to point to the
// actual JPEG stream data and returns the number of bytes skipped

// AHHHAHHHHHH - BufPtr (which I replaced with unsigned char *) was used as a pointer-to-a-pointer here
// So this is not actually modifying the incoming pointer
bool findJPEGheader(BufPtr* start, uint32_t *len, char marker) {

    // per https://en.wikipedia.org/wiki/JPEG_File_Interchange_Format
    BufPtr bytes = *start;

    // kinda skanky, will break if unlucky and the headers inxlucde 0xffda
    // might fall off array if jpeg is invalid
    // FIXME - return false instead
    while(bytes - *start < *len) {

        //printf("looking for framing (0xff) and marker (%x) at %x: %h\n", marker, bytes, *bytes);
        char framing = *bytes++; // better be 0xff since all of the JPEG_ header codes start with 0xff
        if(framing != 0xff) {
            printf("malformed jpeg, framing=%x %p\n", framing, bytes);
            return false;
        }

        char typecode = *bytes++;
        if(typecode == marker) {
            char skipped = bytes - *start;
            printf("found marker 0x%x at %p, skipped %d\n", marker, bytes, skipped);

            *start = bytes;

            // shrink len for the bytes we just skipped
            *len -= skipped;

            return true;
        }
        else {
            // not the section we were looking for, skip the entire section
            switch(typecode) {
            case JEPG_StartOfImage:     // start of image
            {
                break;   // no data to skip
            }
            case JPEG_APP_0:   // app0
            case JPEG_DefineQuantizationTable:   // dqt
            case JPEG_DefineHuffmanTable:   // dht
            case JPEG_StartBaselineDCTFrame:   // sof0
            case JPEG_StartOfScan:   // sos
            {
                // standard format section with 2 bytes for len.  skip that many bytes
                uint32_t len = (*bytes) * 256 + *(bytes+1);
                //printf("skipping section 0x%x, %d bytes\n", typecode, len);
                bytes += len;
                break;
            }
            default:
                printf("unexpected jpeg typecode 0x%x\n", typecode);
                break;
            }
        }
    }

    printf("failed to find jpeg marker 0x%x", marker);
    return false;
}

// the scan data uses byte stuffing to guarantee anything that starts with 0xff
// followed by something not zero, is a new section.  Look for that marker and return the ptr
// pointing there
void skipScanBytes(BufPtr* start, uint32_t len) {
    unsigned char* bytes = *start;

    while(bytes - *start < len) {
        while(*bytes++ != 0xff);
        if(*bytes++ == 0xd9) {
            *start = bytes - 2; // back up to the 0xff marker we just found
            return;
        }
    }
    printf("could not find end of scan");
}
void  nextJpegBlock(BufPtr* bytes) {
    uint32_t len = (*bytes)[0] * 256 + (*bytes)[1];
    //printf("going to next jpeg block %d bytes\n", len);
    *bytes += len;
}

// When JPEG is stored as a file it is wrapped in a container
// This function fixes up the provided start ptr to point to the
// actual JPEG stream data and returns the number of bytes skipped
bool decodeJPEGfile(BufPtr* start, uint32_t* len, BufPtr* qtable0, BufPtr* qtable1) {
    // per https://en.wikipedia.org/wiki/JPEG_File_Interchange_Format

    printf("parsing JPEG data at %p\n",start);
    unsigned char *bytes = *start;

    if(!findJPEGheader(&bytes, len, JEPG_StartOfImage)) // better at least look like a jpeg file
        return false; // FAILED!

    // Look for quant tables if they are present
    *qtable0 = NULL;
    *qtable1 = NULL;
    BufPtr quantstart = *start;
    uint32_t quantlen = *len;
    if(!findJPEGheader(&quantstart, &quantlen, JPEG_DefineQuantizationTable)) {
        printf("error can't find quant table 0\n");
    }
    else {
         printf("found quant table at %p\n", quantstart);

        *qtable0 = quantstart + 3;     // 3 bytes of header skipped
        nextJpegBlock(&quantstart);
        if(!findJPEGheader(&quantstart, &quantlen, JPEG_DefineQuantizationTable)) {
            printf("error can't find quant table 1\n");
        }
        else {
            // printf("found quant table %x\n", quantstart[2]);
        }
        *qtable1 = quantstart + 3;
        nextJpegBlock(&quantstart);
    }

    // try creating a temp startOfScan char pointer here.
    // this makes no sense; it seems like the startOfScan pointer
    // is reverting to the beinning of the data bloc
//     JPEG first byte: ff, last two bytes (starting at 0x3ffe9fbb) ffd9
    // parsing JPEG data at 0x3ffe4374
    // found marker 0xd8 at 0x3ffe4376, skipped 2
    // found marker 0xdb at 0x3ffe438a, skipped 22
    // found marker 0xdb at 0x3ffe438a, skipped 22
    // found marker 0xda at 0x3ffe45d7, skipped 99
    // StartofScan at: 0x3ffe4374. Bytes:
    // 0100ffd8ffe000104a4649460001010100000000
    // adjusted StartofScan at: 0x3fff434c. Bytes:
    // 0000000000000000000000000000000000000000
    // EndofScan at 0x3fff434c.  Bytes: 01. should find EOI
    // malformed jpeg, framing=0 0x3fff434d


    if(!findJPEGheader(start, len, JPEG_StartOfScan))
        return false; // FAILED!

    printf("StartofScan at: %p. Bytes:\n", *start);
    for(char i=0;i<20;i++)
    {
        printf("%02x",*(*start+i));
    }
    printf("\n");

    // Skip the header bytes of the SOS marker FIXME why doesn't this work?
    uint32_t soslen = (*start)[0] * 256 + (*start)[1];
    *start += soslen;
    *len -= soslen;

    printf("adjusted StartofScan at: %p. Bytes:\n", *start);
    for(char i=0;i<20;i++)
    {
        printf("%02x",*(*start+i));
    }
    printf("\n");

    // start scanning the data portion of the scan to find the end marker
    BufPtr endmarkerptr = *start;
    uint32_t endlen = *len;

   

   
    skipScanBytes(&endmarkerptr, *len);
    printf("EndofScan at %p.  Bytes: %02x%02x. should find EOI\n", endmarkerptr, *endmarkerptr, *(endmarkerptr+1));
    if(!findJPEGheader(&endmarkerptr, &endlen, JPEG_EndOfImage))
        return false; // FAILED!

    // endlen must now be the # of bytes between the start of our scan and
    // the end marker, tell the caller to ignore bytes afterwards
    *len = endmarkerptr - *start;

    return true;
}
