#ifndef _BMPSTREAM_H_
#define _BMPSTREAM_H_

#include <stdio.h>

/* Size of the palette data for 8 BPP bitmaps */
#define BMP_PALETTE_SIZE ( 256 * 4 )

/* Bitmap header */
typedef struct _bmp_header {
    /* Magic identifier: "BM" */
	unsigned short  magic               = 0x4D42;;
    /* Size of the BMP file in bytes */
	unsigned int    file_size           = 0;
    /* Reserved */
	unsigned short  reserved1           = 0;
    /* Reserved */
	unsigned short  reserved2           = 0;
	/* Offset of image data relative to the file's start */
    unsigned int    data_offset         = 0;
    /* Size of the header in bytes */
	unsigned int    header_size         = 40;
    /* Bitmap's width */
	unsigned int 	width               = 0;
    /* Bitmap's height */
	unsigned int 	height              = 0;
    /* Number of color planes in the bitmap */
	unsigned short  planes              = 1;
    /* Number of bits per pixel */
	unsigned short  bits_per_pixel      = 0;
    /* Compression type */
	unsigned int    compression_type    = 0;
    /* Size of uncompressed image's data */
	unsigned int    img_data_size       = 0;
    /* Horizontal resolution (pixels per meter) */
	unsigned int    h_pixels_per_meter  = 0;
    /* Vertical resolution (pixels per meter) */
	unsigned int    v_pixels_per_meter  = 0;
    /* Number of color indexes in the color table
       that are actually used by the bitmap */
	unsigned int    colors_used         = 0;
    /* Number of color indexes that are required for
       displaying the bitmap */
	unsigned int 	colors_required     = 0;
} bmp_header;

class bmpStream {
    public:
        bmpStream(
            unsigned int height, unsigned int width,
            unsigned short depth );
        ~bmpStream();

    public:
        bool write_file( const char* filename );
        void set_pixel_RGB(
            unsigned int x, unsigned int y,
            unsigned char r, unsigned char g, unsigned char b );

    private:
        bmp_header header;
        unsigned char *palette;
        unsigned char *data;

    private:
        bool write_header( FILE *fpt );
        bool write_uint( unsigned int x, FILE* fpt );
        bool write_ushort( unsigned short x, FILE* fpt );
};

#endif