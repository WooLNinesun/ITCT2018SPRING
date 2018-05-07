#ifndef _JPEGDECODER_H_
#define _JPEGDECODER_H_

#include <stdio.h>

typedef struct _image { int height = 0, width = 0; } image;

typedef struct _component {
    int DC_predictor    = 0;
    unsigned char hori  = 0;
    unsigned char vert  = 0;
    unsigned char qt_id = 0;
    unsigned char ht_DC = 0;
    unsigned char ht_AC = 0;
} component;

typedef struct _huffmanTable_el {
    unsigned char num        = 0;
    unsigned char* symbol    = NULL;
    unsigned short* codeword = NULL;
} huffmanTable_el;

typedef struct _DC_code {
    int value, diff;
    unsigned char size;
} DC_code;

typedef struct _AC_code {
    int value;
    unsigned char zeros, size;
} AC_code;

typedef struct _RGB { unsigned char R, G, B; } RGB;

// define in jpegMCU.cpp
class MCU {
    public:
        MCU( component *cpts, unsigned char Vmax, unsigned char Hmax );

        double blocks[5][2][2][64];

        void deQuantize( unsigned short *qt, double *block );
        void IDCT( double *block );
        void shift128( double *block );
        RGB *toRGB();

    private:
        double get_cpt_value(
            unsigned char id, unsigned char v, unsigned char h );
        unsigned char Normalize( double x );

        // void generator_alpahcos() {
        //     double pi = 3.14159265358979323846;
        //     double div2sqrt = 0.70710678118;
        //     double alphacos[64] = { 0 };
        //     for( int i= 0; i < 8; i++ ) {
        //         for( int j= 0; j < 8; j++ ) {
        //             alphacos[i*8+j] = (j? 1:div2sqrt)
        //                               *cos((2*i+1)*j*pi / 16.0 );
        //             printf("%9f ", alphacos[i*8+j]/2);
        //         } printf("\n");
        //     }
        // }

    private:
        unsigned char Vmax = 0, Hmax = 0;
        component *cpts = NULL;

        // alpha() * cos table for idct
        const double alphacos[64] = {
             0.353554,  0.490393,  0.461940,  0.415735, 
             0.353553,  0.277785,  0.191342,  0.097545, 
             0.353554,  0.415735,  0.191342, -0.097545, 
            -0.353553, -0.490393, -0.461940, -0.277785, 
             0.353554,  0.277785, -0.191342, -0.490393, 
            -0.353554,  0.097545,  0.461940,  0.415735, 
             0.353554,  0.097545, -0.461940, -0.277785, 
             0.353553,  0.415735, -0.191341, -0.490393, 
             0.353554, -0.097545, -0.461940,  0.277785, 
             0.353554, -0.415734, -0.191343,  0.490392, 
             0.353554, -0.277785, -0.191342,  0.490393, 
            -0.353553, -0.097546,  0.461940, -0.415734, 
             0.353554, -0.415735,  0.191341,  0.097546, 
            -0.353554,  0.490393, -0.461939,  0.277784, 
             0.353554, -0.490393,  0.461940, -0.415734, 
             0.353553, -0.277784,  0.191340, -0.097543, 
        };
};

class jpegDecoder {
    public:
        jpegDecoder( const char* filepath, const char *out );
        ~jpegDecoder();

    public:
        class huffmanTables {
            public:
                huffmanTable_el DC[2][16];
                huffmanTable_el AC[2][16];

            public:    
                huffmanTable_el* get ( unsigned char ht_info ) {
                    switch(ht_info) {
                        case  0: { return this->DC[0]; }
                        case  1: { return this->DC[1]; }
                        case 16: { return this->AC[0]; }
                        case 17: { return this->AC[1]; }
                        default: { return 0;           }
                    }
                }
        };

    private:
        // define in jpegDecoder.cpp
        bool read_ctrl();
        bool read_data();
            char *output_file;
        bool read_MCU( MCU* mcu );
        DC_code read_DC( unsigned char ht_DC, int* predictor );
        AC_code read_AC( unsigned char ht_AC );
        unsigned char search_symbol( unsigned char ht_info );

        // define in jpegHeader.cpp
        bool read_header_skip(
            unsigned char marker, const char name[4] );
        bool read_header_SOF();
            image img;
            component components[5];
            unsigned char components_num = 0;
            unsigned char Hmax = 0, Vmax = 0;
        bool read_header_DHT();
            huffmanTables  huffmancode_tables;
        bool read_header_SOS();
        bool read_header_DQT();
            unsigned short quantization_tables[4][64] = { { 0 } };
        bool read_header_DRI();

        // define in jpegStream.cpp
        bool hasEOF = false, hasEOI = false;
        bool jpeg_open( const char* filepath );
            FILE *fpt = NULL; 
        unsigned char read_byte();
        bool read_bit();

        // zigzag index table
        const char zigzag[64] = {
            0,   1,  8, 16,  9,  2,  3, 10,
            17, 24, 32, 25, 18, 11,  4,  5,
            12, 19, 26, 33, 40, 48, 41, 34,
            27, 20, 13,  6,  7, 14, 21, 28,
            35, 42, 49, 56, 57, 50, 43, 36,
            29, 22, 15, 23, 30, 37, 44, 51,
            58, 59, 52, 45, 38, 31, 39, 46,
            53, 60, 61, 54, 47, 55, 62, 63
        };
};

#endif
