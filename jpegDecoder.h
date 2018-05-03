#include <stdio.h>

#include "huffmanTable.h"

struct image { int height = 0, width = 0; };

struct component {
    unsigned char hori  = 0;
    unsigned char vert  = 0;
    unsigned char qt_id = 0;
    unsigned char ht_DC = 0;
    unsigned char ht_AC = 0;
    int DC_predictor = 0;
};

struct DC_code {
    int value = 0, diff = 0;
    unsigned char size = 0;
};

struct AC_code {
    unsigned char zeros = 0, size = 0;
    int value = 0;
};

class MCU {
    public:
        double blocks[5][2][2][64];
    private:
};

class jpegDecoder {
    public:
        jpegDecoder( const char* filepath ); 
        ~jpegDecoder();

    private:
        bool read_ctrl();
        bool read_data();
        MCU  read_MCU();
        DC_code read_DC( unsigned char ht_DC, int* predictor );
        AC_code read_AC( unsigned char ht_AC );
        unsigned char search_symbol( unsigned char ht_info );

        void   IDCT( double* block );
        double IDCT_el( double* f, unsigned char x, unsigned char y );
        void shift128( double* block );

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

        bool hasEOF = false, hasEOI = false;
        bool jpeg_open( const char* filepath );
            FILE *fpt = NULL; 
        unsigned char read_byte();
        bool read_bit();

        // zigzag index table
        char zigzag[64] = {
            0,   1,  8, 16,  9,  2,  3, 10,
            17, 24, 32, 25, 18, 11,  4,  5,
            12, 19, 26, 33, 40, 48, 41, 34,
            27, 20, 13,  6,  7, 14, 21, 28,
            35, 42, 49, 56, 57, 50, 43, 36,
            29, 22, 15, 23, 30, 37, 44, 51,
            58, 59, 52, 45, 38, 31, 39, 46,
            53, 60, 61, 54, 47, 55, 62, 63
        };

        // cos table
        double alphacos[64] = {
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
