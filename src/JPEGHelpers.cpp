// References:
// https://www.fileformat.info/format/jpeg/egff.htm
// https://www.videotechnology.com/jpeg/j1.html

#include "Arduino.h"
#include "JPEGHelpers.h"

// most of the JPEG headers contain two bytes after the marker
// specifying the length of the header (including these length bits)
// this function moves the supplied pointer PAST the currently
// pointed-at header using this length description
// NOTE: not necessarily safe for all header types; use wisely.
void nextJpegBlock(BufPtr* bytes) {
    uint32_t len = (*bytes)[0] * 256 + (*bytes)[1];
    *bytes += len;
}

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

        char framing = *bytes++; // better be 0xff since all of the JPEG_ header codes start with 0xff
        if(framing != 0xff) {
            //printf("malformed jpeg, framing=%x %p\n", framing, bytes);
            return false;
        }

        char typecode = *bytes++;
        if(typecode == marker) {
            char skipped = bytes - *start;
            //printf("found marker 0x%x at %p, skipped %d\n", marker, bytes, skipped);

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
            // All of these JPEG headers contain two bytes following the marker
            // specifying the length of the segment
            // Use that defined length to skip the whole segment
            // and advance the bytes pointer to the next segment
            case JPEG_APP_0:   // app0
            case JPEG_DefineQuantizationTable:   // dqt
            case JPEG_DefineHuffmanTable:   // dht
            case JPEG_StartBaselineDCTFrame:   // sof0
            case JPEG_StartOfScan:   // sos
            {
                nextJpegBlock(&bytes);
                break;
            }
            default:
                //printf("unexpected jpeg typecode 0x%x\n", typecode);
                break;
            }
        }
    }

    //printf("failed to find jpeg marker 0x%x", marker);
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
    //printf("could not find end of scan");
}


/**
 * Decode the incoming JPEG file bytes by locating the various portions
 * of the file (quantization tables, various headers, and scan bytes)
 * 
 * Update the strcut at the location of the provided currentFrame pointer
 * to include a reference to the original shared_ptr from the DMA transfer of the camera frame
 * as well as the memory addresses of the two quantization tables and the JPEG scan data.
 * 
 */ 
bool decodeJPEGfile(BufPtr* start, uint32_t* len, DecodedJPEGFrame* currentFrame) {
    // per https://en.wikipedia.org/wiki/JPEG_File_Interchange_Format
    unsigned char *bytes = *start;

    // move the start pointer to the beginning of the image (marker FFD8); fail if we can't.
    // Updates len to the number of bytes remaining in the buffer
    if(!findJPEGheader(&bytes, len, JEPG_StartOfImage)) // better at least look like a jpeg file
        return false; // FAILED!

    // Look for quant tables if they are present
    currentFrame->quant0tbl = nullptr;
    currentFrame->quant1tbl = nullptr;
    BufPtr quantstart = *start;
    uint32_t quantlen = *len;
    // using the start pointer as a base, move the quantstart pointer to where quant tables
    // begin (marker FFDB)
    if(!findJPEGheader(&quantstart, &quantlen, JPEG_DefineQuantizationTable)) {
        //printf("error can't find quant table 0\n");
    }
    else {
        currentFrame->quant0tbl = quantstart + 3;
        nextJpegBlock(&quantstart);
        if(!findJPEGheader(&quantstart, &quantlen, JPEG_DefineQuantizationTable)) {
            //printf("error can't find quant table 1\n");
        }
        currentFrame->quant1tbl = quantstart + 3;
        nextJpegBlock(&quantstart);
    }

    // move the start pointer (which was passed to this function) to the address of the start of
    // scan data (marker FFDA))
    if(!findJPEGheader(start, len, JPEG_StartOfScan))
        return false; // FAILED!

    // Skip the header bytes of the SOS marker
    uint32_t soslen = (*start)[0] * 256 + (*start)[1];
    *start += soslen;
    *len -= soslen;

    // start scanning the data portion of the scan to find the end marker
    BufPtr endmarkerptr = *start;
    uint32_t endlen = *len;
   
    skipScanBytes(&endmarkerptr, *len);
    if(!findJPEGheader(&endmarkerptr, &endlen, JPEG_EndOfImage))
        return false; // FAILED!

    // endlen must now be the # of bytes between the start of our scan and
    // the end marker, tell the caller to ignore bytes afterwards
    *len = endmarkerptr - *start;

    currentFrame->scanDataLength = *len;
    currentFrame->scanData = *start;

    return true;
}
