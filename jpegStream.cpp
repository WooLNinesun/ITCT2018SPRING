#include <stdio.h>

#include "jpegDecoder.h"

using namespace std;

bool jpegDecoder::jpeg_open( const char* filepath ) {
    return( (this->fpt = fopen( filepath, "r" )) == NULL )? false : true;
}

unsigned char jpegDecoder::read_byte() {
    static int  num = 1<<20;
    static char buf[1<<20];
    static char *p  = buf, *end = buf;
    if ( p == end ) {
        end = buf + fread( buf, 1, num, this->fpt);

        if ( end == buf ) {
            this->hasEOF = true; this->hasEOI = true; return EOF;
        } p = buf;
    } return (unsigned char)*p++;
}

bool jpegDecoder::read_bit() {
    static unsigned char buf, count = 0;
    if ( count == 0 ) {
        buf = read_byte();
        while ( buf == 0xFF ) {
            unsigned char marker = read_byte();
            if ( marker == 0xFF ) { continue;      }
            if ( marker == 0x00 ) { break;         }
            if ( marker != 0x00 ) { throw "error"; }
        }
    }
    bool ret = buf & ( 1 << (7 - count) );
    count = (count == 7)? 0 : count + 1;

    return ret;
}
