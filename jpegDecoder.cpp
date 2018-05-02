#include <stdio.h>

#include "jpegDecoder.h"

using namespace std;

jpegDecoder::jpegDecoder( const char* filepath ){
    if ( !jpeg_open( filepath ) ) {
        throw "File open error or isn't a JPEG file.";
    }

    while ( !(this->hasEOI) ) {
        switch( read_byte() ) {
            case 0xFF: {
                if ( read_ctrl() ) { break; }
            }
            default: {
                throw "File format error.";
            }
        }
    }
}

jpegDecoder::~jpegDecoder() {
    unsigned char ht_info[4] = { 0x00, 0x01, 0x10, 0x11 };
    for ( unsigned char i = 0; i < 4; i++ ) {
        huffmanTable_el* ht = this->huffmancode_tables.get( ht_info[i] );
        for ( unsigned char j = 0; j < 16; j++ ) {
            if ( ht[j].num == 0 ) { continue; }
            delete [] ht[j].symbol;
            delete [] ht[j].codeword;
        }
    }
}

bool jpegDecoder::read_ctrl() {
    unsigned char marker = read_byte();

    switch( marker ) {
        // SOF0 = Start Of Frame (baseline DCT)
        case 0xC0: { return this->read_header_SOF(); }
        // DHT = Define Huffman Tables
        case 0xC4: { return this->read_header_DHT(); }
        // SOI = start of image
        case 0xD8: { return true; }
        // EOI = end of image
        case 0xD9: { this->hasEOI = true; return true; }
        // SOS = Start Of Scan
        case 0xDA: { return this->read_header_SOS() && this->read_data(); }
        // DQT = Define Quantization Tables
        case 0xDB: { return this->read_header_DQT(); }
        // DNL = Define Number of Lines. skip it
        case 0xDC: { return this->read_header_skip( marker, "DNL" ); }
        // DRI = Define Restart Interval. skip it
        case 0xDD: { return this->read_header_DRI(); }
        // DHP = Define Hierarchical Progression
        case 0xDE: { return this->read_header_skip( marker, "DHP" ); }
        // EXP = Expand Reference Components
        case 0xDF: { return this->read_header_skip( marker, "EXP" ); }
        // APPn = Application specific segments
        case 0xE0: case 0xE1: case 0xE2: case 0xE3:
        case 0xE4: case 0xE5: case 0xE6: case 0xE7:
        case 0xE8: case 0xE9: case 0xEA: case 0xEB:
        case 0xEC: case 0xED: case 0xEE: case 0xEF: {
            return this->read_header_skip( marker, "APP" );
        }
        // COM = Comment
        case 0xFE: { return this->read_header_skip( marker, "COM" ); }
        default:{ return false; }
    }
}

bool jpegDecoder::read_data() {
    int w = (this->img.width - 1)  / (8 * this->Hmax) + 1;
    int h = (this->img.height - 1) / (8 * this->Vmax) + 1;
    for ( int i = 0; i < h; i++ ) {
        for ( int j = 0; j < w; j++ ) {
            // printf("MCU (%d,%d)\n", i, j);
            read_MCU();
        }
    } return true;
}

bool jpegDecoder::read_MCU() {
    for ( unsigned char i = 0; i < this->components_num; i++ ) {
        component* cpt = &(this->components[i]);
        for ( unsigned char h = 0; h < cpt->hori; h++ ) {
            for ( unsigned char w = 0; w < cpt->vert; w++ ) {
                // printf("\tDataUnit: (%d,%d,%d)\n", i, h, w);
                // printf("\t\tDC Predictor: %d\n\t\tDC:\n",
                //     this->components[i].DC_predictor);
                el_code DC = this->read_DC(
                    cpt->ht_DC, &this->components[i].DC_predictor );

                // printf("\t\tAC:\n");
                for( int count = 0; count < 63; ) {
                    el_code AC = read_AC( cpt->ht_AC );
                    if (AC.size == 0 && AC.zeros == 0) { break; }
                    for (int j = 0; j < AC.zeros; j++) {
                        count++;
                    } count++;
                }
            }
        }
    }
}

unsigned char jpegDecoder::match_huffman_tables( unsigned char ht_info ) {
    huffmanTable_el* ht = this->huffmancode_tables.get(ht_info);
    if ( ht == 0 ) { throw "huffman_table get error"; }

    unsigned short codeword = 0;
    for ( int i = 0; i < 16; i++ ) {
        codeword <<= 1; codeword += (unsigned short)this->read_bit();
        for ( int j = 0; j < ht[i].num; j++ ) {
            if ( ht[i].codeword[j] == codeword ) {
                return ht[i].symbol[j];
            }
        }
    } throw "key not found.";
}

el_code jpegDecoder::read_DC( unsigned char ht_DC, int* predictor ) {
    unsigned char ht_info = 0x00 + (ht_DC);
    unsigned char codelen = this->match_huffman_tables( ht_info );
    
    int ret = 0;
    if ( codelen != 0 ) {
        bool first = read_bit(); ret = 1;
        for ( int i = 1; i < codelen; i++ ) {
            bool b = read_bit();
            ret <<= 1; ret += first ? b : !b;
        } ret = (first)? ret : -ret;
    } *predictor += ret;

    // printf("\t\t\tT: %d\tDIFF: 0\tEXTEND(DIFF,T): %d\n", codelen, ret);

    return el_code { 0, codelen, ret };
}

el_code jpegDecoder::read_AC( unsigned char ht_AC ) {
    unsigned char ht_info = 0x10 + (ht_AC);
    unsigned char zeros_codelen = this->match_huffman_tables( ht_info );

    unsigned char zeros   = zeros_codelen >> 4;
    unsigned char codelen = zeros_codelen & 0x0F;
    
    int ret = 0;
    if ( codelen != 0 ) {
        bool first = read_bit(); ret = 1;
        for ( int i = 1; i < codelen; i++ ) {
            bool b = read_bit();
            ret <<= 1; ret += first ? b : !b;
        } ret = (first)? ret : -ret;
    } else if ( zeros != 0 && zeros != 0x0F ) {
        throw "Read AC code error.";
    }

    // if ( codelen == 0 && zeros == 0) {
    //     printf("\t\t\tRS: 0x00 -- EOB\n");
    // } else {
    //     printf( "\t\t\tRS: %.2x RR:%4d SS:%4d",
    //         zeros_codelen, zeros, codelen);
    //     if ( codelen != 0 ) {
    //         printf( " ZZ(K):%2d EXTEND(ZZ(K),SS):%4d", 0, ret);
    //     } printf("\n");
    // }

    return el_code { zeros, codelen, ret };
}

bool jpegDecoder::read_header_skip (
    unsigned char marker, const char name[4] ) {

    unsigned short len = (read_byte() << 8) + read_byte();
    // printf("ff%x - %s\n\tLq: %d\n", marker, name, len);

    for ( int i = 0; i < len-2; i++ ) { read_byte(); }    
    return true;
}

/* read_SOF
 (2) len : data len, include itself
 (1) precision
 (4) img height and width
 (1*3n) number of components, for every component i is 3 bytes
        for every component:
            component_id + sampling factor ( hor + vert ) + qt_num
 */
bool jpegDecoder::read_header_SOF() {
    unsigned short len = (read_byte() << 8) + read_byte();

    unsigned char precision = read_byte();
    this->img.height = (read_byte() << 8) + read_byte();
    this->img.width  = (read_byte() << 8) + read_byte();

    // printf("ffc0 - SOF - Baseline DCT, Huffman coding\n");
    // printf("\tLf: %d\tP: %d\tY: %d\tX: %d",
    //     len, (precision? 8 : 16), this->img.height, this->img.width);

    this->components_num = read_byte();
    // printf("\tNf: %d\n", this->components_num);
    for ( int i = 0; i < components_num; i++ ) {
        unsigned char component_id = read_byte() - 1;
        component* component = &(this->components[component_id]);
        unsigned char sample_factor = read_byte();
        component->hori = sample_factor >> 4;
        component->vert = sample_factor & 0x0F;
        component->qt_id = read_byte();

        if ( this->Hmax < component->hori ) { this->Hmax = component->hori; }
        if ( this->Vmax < component->vert ) { this->Vmax = component->vert; }

        // printf("\tComponent: %d\tH: %d\tV: %d\tTq: %d\n",
        //     component_id+1, component->hori, component->vert,
        //     component->qt_id );
    }
    return true;
}

/* read_DHT
 (2) len : data len, include itself
 (1) ht_info: class and number
 (16) Number of Huffman codes of length i
 (m) Value associated with each Huffman code
 */
bool jpegDecoder::read_header_DHT() {
    unsigned short len = (read_byte() << 8) + read_byte();
    // printf("ffc4 - DHT\n\tLh: %d\n", len);

    while ( len - 2 ) {
        unsigned char ht_info = read_byte();
        unsigned char Tc = ht_info >> 4, Th = ht_info & 0x0F;

        huffmanTable_el* ht = this->huffmancode_tables.get(ht_info);
        if ( ht == 0 ) { throw "DHT ht_info error"; }

        unsigned char total_read_count = 16;
        for ( int i = 0; i < 16; i++ ) {
            ht[i].num = read_byte();
            total_read_count += ht[i].num;
        }

        if ( len - 2 >= 1 + total_read_count ) { 
            len -= 1 + total_read_count;
        } else { throw "DQT read error."; }

        unsigned short cur_codeword = 0;
        for ( unsigned char i = 0; i < 16; i++, cur_codeword <<= 1 ) {
            if ( ht[i].num  == 0 ) { continue; }
            if ( ht[i].symbol   == 0 ) { delete [] ht[i].symbol;   }
            if ( ht[i].codeword == 0 ) { delete [] ht[i].codeword; }

            ht[i].symbol =   new unsigned char [ht[i].num];
            ht[i].codeword = new unsigned short[ht[i].num];
            for ( unsigned char j = 0; j < ht[i].num; j++ ) {
                ht[i].symbol[j]   = read_byte();
                ht[i].codeword[j] = cur_codeword++;
            }
        }

        // printf("\tTc: %d\tTh: %d\n", Tc, Th);
        // for ( unsigned char i = 0; i < 16; i++ ) {
        //     if ( ht[i].num == 0 ) { continue; }
        //     for ( unsigned char j = 0; j < ht[i].num; j++ ) {
        //         printf("\t%.2d\t%.2x\t%d\n",
        //             i+1, ht[i].symbol[j], ht[i].codeword[j]);
        //     }
        // }
    }
    
    return true;
}

/* read_SOS
 (2) len : data len, include itself
 */ 
bool jpegDecoder::read_header_SOS() {
    unsigned short len = (read_byte() << 8) + read_byte();
    unsigned char sos_components_num = read_byte();

    if ( this->components_num == 0 ) {
        throw "SOF header not present or specifies 0 components.";
    } else if ( len != (this->components_num*2) + 6 ) {
        throw "Bad SOS header length.";
    } else if ( sos_components_num != this->components_num ) {
        throw "Components_num not same.";
    }

    for ( int i = 0; i < this->components_num; i++ ) {
        unsigned char component_id = read_byte() - 1;
        unsigned char DCAC = read_byte();
        this->components[component_id].ht_DC = DCAC >> 4;
        this->components[component_id].ht_AC = DCAC & 0x0F;
    }

    unsigned char zigzag_start = read_byte();
    unsigned char zigzag_end   = read_byte();
    unsigned char dummy        = read_byte();

    // printf("ffc4 - DHT\n\tLs: %d\tNf: %d\n", len, sos_components_num);
    // for ( int i = 0; i < this->components_num; i++ ) {
    //     component* cpt = &(this->components[i]);
    //     printf("\tCs: %d\tTd: %d\tTa: %d\n", i+1, cpt->ht_DC, cpt->ht_AC);
    // } printf("\tSs: %d\tSe: %d\tAh: %d\tAl: %d\n",
    //     zigzag_start, zigzag_end, dummy >> 4, dummy & 0x0F);

    return true;
}

/* read_DQT
 (2) len : data len, include itself
 (1) qt_info :
        qt_precision = hight 4 bits, qt_id = low 4 bits
        qt_precision = 0         ==> data with 1 bytes,
        qt_precision = otherwise ==> data with 2 bytes

        qt_id = 0 ==> luminance,
        qt_id = 1 ==> chrominance
 */
bool jpegDecoder::read_header_DQT() {
    unsigned short len = (read_byte() << 8) + read_byte();
    // printf("ffdb - DQT\n\tLq: %d\n", len);

    while ( len - 2 ) {
        unsigned char qt_info = read_byte();
        unsigned char qt_precision = qt_info >> 4, qt_id = qt_info & 0x0F;
        if ( len - 2 >= 1 + 64*(qt_precision? 2 : 1) ) { 
            len -= 1 + 64*(qt_precision? 2 : 1);
        } else { throw "DQT read error."; }

        unsigned short* qt = this->quantization_tables[qt_id];
        for ( int i = 0; i < 64; i++ ) {
            if ( qt_precision ) {
                qt[ zigzag[i] ] = (read_byte() << 8) + read_byte();
            } else { qt[ zigzag[i] ] = read_byte(); }
        }

        // printf("\tPq: %d\tTq: %d\tQk: %d\n",
        //     qt_precision, qt_id, (qt_precision? 16 : 8) );
        // printf("\tIn Block order:\n");
        // for ( int i = 0; i < 8; i++ ) {
        //     printf("\t");
        //     for ( int j = 0; j < 8; j++ ) { printf("%4d ", qt[ i*8+j ]); }
        //     printf("\n");
        // }
    }
    
    return true;
}

/* read_DRI
 (2) len : data len, include itself
 */
bool jpegDecoder::read_header_DRI() {
    printf("DRI\n");
    unsigned short len = (read_byte() << 8) + read_byte();
    for ( int i = 0; i < len-2; i++ ) { read_byte(); }
    return true;
}

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
