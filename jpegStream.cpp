#include <stdio.h>

#include "jpegStream.h"
#include "bmpStream.h"

using namespace std;

jpegDecoder::jpegDecoder( const char *filepath ){
    if ( !this->jpeg_open( filepath ) ) {
        throw "File open error or isn't a JPEG file.";
    }

    while ( !(this->hasEOI) ) {
        switch( this->read_byte() ) {
            case 0xFF: {
                if ( this->read_ctrl() ) { break; }
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

    fclose( this->fpt );
}

bool jpegDecoder::read_ctrl() {
    unsigned char marker = this->read_byte();

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
    unsigned int mcu_height = 8 * this->Vmax,
                 mcu_width  = 8 * this->Hmax;
    int mcu_height_num = (this->img.height - 1) / mcu_height + 1,
        mcu_width_num  = (this->img.width  - 1) / mcu_width  + 1;

    bmpStream bmp(
        mcu_height * mcu_height_num, mcu_width * mcu_width_num, 24);

    for ( int i = 0; i < mcu_height_num; i++ ) {
        for ( int j = 0; j < mcu_width_num; j++ ) {
            // printf("MCU (%d,%d)\n", i, j);
            MCU mcu( components, this->Vmax, this->Hmax );
            read_MCU( &mcu );
            RGB *RGB_mcu = mcu.toRGB();

            for (int h = i*mcu_height; h < (i+1)*mcu_height; h++) {
                for (int w = j*mcu_width; w < (j+1)*mcu_width; w++) {
                    int dh = h - i*mcu_height, dw = w - j*mcu_width;
                    bmp.set_pixel_RGB(
                        h, w,
                        RGB_mcu[dh*mcu_width+dw].R,
                        RGB_mcu[dh*mcu_width+dw].G,
                        RGB_mcu[dh*mcu_width+dw].B
                    );
                }
            }

            delete [] RGB_mcu;
        }
    }
    bmp.write_file( "out.bmp" );

    return true;
}

bool jpegDecoder::read_MCU( MCU *mcu ) {
    for ( unsigned char id = 0; id < this->components_num; id++ ) {
        component *cpt = &(this->components[id]);
        for ( unsigned char h = 0; h < cpt->vert; h++ ) {
            for ( unsigned char w = 0; w < cpt->hori; w++ ) {
                double *block = mcu->blocks[id][h][w];

                // printf("\tDataUnit: (%d,%d,%d)\n", id, h, w);
                // printf("\t\tDC Predictor: %d\n\t\tDC:\n",
                //     this->components[i].DC_predictor);
                DC_code DC = this->read_DC(
                    cpt->ht_DC, &(cpt->DC_predictor) );
                block[0] = (double)DC.value;

                // printf("\t\tAC:\n");
                for( int count = 1; count < 64; ) {
                    AC_code AC = read_AC( cpt->ht_AC );
                    if (AC.size == 0 && AC.zeros == 0) {
                        while ( count < 64 ) {
                            block[zigzag[count++]] = 0;
                        } break;
                    }
                    for (int i = 0; i < AC.zeros; i++) {
                        block[zigzag[count++]] = 0;
                    } block[zigzag[count++]] = (double)AC.value;
                }

                // printf("\t\t*Before DeQuantize*\n");
                // for( unsigned char j = 0; j < 8; j++ ) {
                //     printf("\t\t");
                //     for ( unsigned char  = 0; k < 8; k++ ) {
                //         printf("%4.0f ",block[8*j+k]);
                //     } printf("\n");
                // }

                mcu->deQuantize(
                    this->quantization_tables[cpt->qt_id], block);

                // printf("\t\t*Before IDCT*\n");
                // for( int j = 0; j < 8; j++ ) {
                //     printf("\t\t");
                //     for ( int k = 0; k < 8; k++ ) {
                //         printf("%4.0f ",block[8*j+k]);
                //     } printf("\n");
                // }

                mcu->IDCT( block );

                // printf("\t\t*After IDCT*\n");
                // for( int j = 0; j < 8; j++ ) {
                //     printf("\t\t");
                //     for ( int k = 0; k < 8; k++ ) {
                //         printf("%4.0f ",block[8*j+k]);
                //     } printf("\n");
                // }

                // for y block, shift +128
                if ( id == 0 ) {
                    mcu->shift128( block );

                    // printf("\t\t*After level shift (+128)*\n");
                    // for( int j = 0; j < 8; j++ ) {
                    //     printf("\t\t");
                    //     for ( int k = 0; k < 8; k++ ) {
                    //         printf("%4.0f ",block[8*j+k]);
                    //     } printf("\n");
                    // }
                }
            }
        }
    }
}

unsigned char jpegDecoder::search_symbol( unsigned char ht_info ) {
    huffmanTable_el *ht = this->huffmancode_tables.get(ht_info);
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

DC_code jpegDecoder::read_DC( unsigned char ht_DC, int *predictor ) {
    unsigned char ht_info = 0x00 + (ht_DC);
    unsigned char codelen = this->search_symbol( ht_info );
    
    int diff = 0;
    if ( codelen != 0 ) {
        bool first = this->read_bit(); diff = 1;
        for ( int i = 1; i < codelen; i++ ) {
            bool b = this->read_bit();
            diff <<= 1; diff += first ? b : !b;
        } diff = (first)? diff : -diff;
    } *predictor += diff;

    // printf("\t\t\tT: %d\tDIFF: 0\tEXTEND(DIFF,T): %d\n",
    //     codelen, ret);

    return DC_code { .value = *predictor, .diff = diff, .size = codelen };;
}

AC_code jpegDecoder::read_AC( unsigned char ht_AC ) {
    unsigned char ht_info = 0x10 + (ht_AC);
    unsigned char zeros_codelen = this->search_symbol( ht_info );

    unsigned char zeros   = zeros_codelen >> 4;
    unsigned char codelen = zeros_codelen & 0x0F;
    
    int ret = 0;
    if ( codelen != 0 ) {
        bool first = this->read_bit(); ret = 1;
        for ( unsigned char i = 1; i < codelen; i++ ) {
            bool b = this->read_bit();
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

    return AC_code { .value = ret, .zeros = zeros, .size = codelen };
}
