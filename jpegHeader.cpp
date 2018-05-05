#include <stdio.h>

#include "jpegDecoder.h"

using namespace std;

bool jpegDecoder::read_header_skip (
    unsigned char marker, const char name[4] ) {

    unsigned short len = (this->read_byte() << 8) + this->read_byte();
    // printf("ff%x - %s\n\tLq: %d\n", marker, name, len);

    for ( int i = 0; i < len-2; i++ ) { this->read_byte(); }    
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
    unsigned short len = (this->read_byte() << 8) + this->read_byte();

    unsigned char precision = this->read_byte();
    this->img.height = (this->read_byte() << 8) + this->read_byte();
    this->img.width  = (this->read_byte() << 8) + this->read_byte();
    if( img.height <= 0 || img.width <= 0 ) {
        throw "invalid image height or width.";
    }

    // printf("ffc0 - SOF - Baseline DCT, Huffman coding\n");
    // printf("\tLf: %d\tP: %d\tY: %d\tX: %d",
    //     len, (precision? 8 : 16), this->img.height, this->img.width);

    this->components_num = this->read_byte();
    // printf("\tNf: %d\n", this->components_num);
    for ( int i = 0; i < components_num; i++ ) {
        unsigned char component_id = this->read_byte() - 1;
        component *component = &(this->components[component_id]);
        unsigned char sample_factor = this->read_byte();
        component->hori = sample_factor >> 4;
        component->vert = sample_factor & 0x0F;
        component->qt_id = this->read_byte();

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
    unsigned short len = (this->read_byte() << 8) + this->read_byte();
    // printf("ffc4 - DHT\n\tLh: %d\n", len);

    while ( len - 2 ) {
        unsigned char ht_info = this->read_byte();
        unsigned char Tc = ht_info >> 4, Th = ht_info & 0x0F;

        huffmanTable_el* ht = this->huffmancode_tables.get(ht_info);
        if ( ht == 0 ) { throw "DHT ht_info error"; }

        unsigned char total_read_count = 16;
        for ( int i = 0; i < 16; i++ ) {
            ht[i].num = this->read_byte();
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
                ht[i].symbol[j]   = this->read_byte();
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
    unsigned short len = (this->read_byte() << 8) + this->read_byte();
    unsigned char sos_components_num = this->read_byte();

    if ( this->components_num == 0 ) {
        throw "SOF header not present or specifies 0 components.";
    } else if ( len != (this->components_num*2) + 6 ) {
        throw "Bad SOS header length.";
    } else if ( sos_components_num != this->components_num ) {
        throw "Components_num not same.";
    }

    for ( int i = 0; i < this->components_num; i++ ) {
        unsigned char component_id = this->read_byte() - 1;
        unsigned char DCAC = this->read_byte();
        this->components[component_id].ht_DC = DCAC >> 4;
        this->components[component_id].ht_AC = DCAC & 0x0F;
    }

    unsigned char zigzag_start = this->read_byte();
    unsigned char zigzag_end   = this->read_byte();
    unsigned char dummy        = this->read_byte();

    // printf("ffc4 - DHT\n\tLs: %d\tNf: %d\n", len, sos_components_num);
    // for ( int i = 0; i < this->components_num; i++ ) {
    //     component* cpt = &(this->components[i]);
    //     printf("\tCs: %d\tTd: %d\tTa: %d\n",
    //         i+1, cpt->ht_DC, cpt->ht_AC);
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
    unsigned short len = (this->read_byte() << 8) + this->read_byte();
    // printf("ffdb - DQT\n\tLq: %d\n", len);

    while ( len - 2 ) {
        unsigned char qt_info = this->read_byte();
        unsigned char qt_precision = qt_info >> 4, qt_id = qt_info & 0x0F;
        if ( len - 2 >= 1 + 64*(qt_precision? 2 : 1) ) { 
            len -= 1 + 64*(qt_precision? 2 : 1);
        } else { throw "DQT read error."; }

        unsigned short* qt = this->quantization_tables[qt_id];
        for ( int i = 0; i < 64; i++ ) {
            if ( qt_precision ) {
                qt[zigzag[i]] = (this->read_byte() << 8) + this->read_byte();
            } else { qt[ zigzag[i] ] = this->read_byte(); }
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
    unsigned short len = (this->read_byte() << 8) + this->read_byte();
    for ( int i = 0; i < len-2; i++ ) { this->read_byte(); }
    return true;
}
