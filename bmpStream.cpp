#include <stdio.h>

#include "bmpStream.h"

using namespace std;

bmpStream::bmpStream(
    unsigned int height, unsigned int width,
    unsigned short depth ) {

    if ( depth != 8 && depth != 24 && depth != 32 ) {
		throw "BMP_FILE_NOT_SUPPORTED";
	}

    /* Calculate the number of bytes used to store a single image row.
       This is always rounded up to the next multiple of 4. */
    int bytes_per_pixel = depth >> 3;
    unsigned long int bytes_per_row = width * bytes_per_pixel;
    bytes_per_row += (bytes_per_row % 4)? (4 - bytes_per_row % 4) : 0;

	/* Set header's image specific values */
	this->header.width          = width;
	this->header.height         = height;
	this->header.bits_per_pixel   = depth;
	this->header.img_data_size  = bytes_per_row * height;
	this->header.data_offset     = 
        54 + ( depth == 8 ? BMP_PALETTE_SIZE : 0 );
	this->header.file_size       =
        this->header.img_data_size + this->header.data_offset;

	/* Allocate palette */
	if ( this->header.bits_per_pixel == 8 ) {
		this->palette = new unsigned char [ BMP_PALETTE_SIZE ];
		if ( this->palette == NULL ) {
            throw "BMP_OUT_OF_MEMORY";
		}
	} else { this->palette = NULL; }

	/* Allocate pixels */
    
	this->data = new unsigned char [ this->header.img_data_size ];
	if ( this->data == NULL ) {
		delete [] this->palette;
        throw "BMP_OUT_OF_MEMORY";
	}

}
bmpStream::~bmpStream() { 
	if ( this->palette != NULL ) { delete [] this->palette; }
	if ( this->data    != NULL ) { delete [] this->data;    }
}

bool bmpStream::write_file( const char* filename ) {
	FILE* fpt;

	/* Open file */
	if ( (fpt = fopen( filename, "wb" )) == NULL ) {
		throw "BMP_FILE_NOT_FOUND"; return false;
	}

	/* Write header */
	if ( this->write_header( fpt ) != true ) {
		throw "BMP_IO_ERROR";
        fclose( fpt ); return false;
	}

	/* Write palette */
	if ( this->palette ) {
		if ( fwrite( this->palette, 1, BMP_PALETTE_SIZE, fpt )
                != BMP_PALETTE_SIZE
        ) { throw "BMP_IO_ERROR"; fclose( fpt ); return false; }
	}

	/* Write data */
	if ( fwrite( this->data, 1, this->header.img_data_size, fpt )
            != this->header.img_data_size
    ) { throw "BMP_IO_ERROR"; fclose( fpt); return false; }

	fclose( fpt );  return true;
}

bool bmpStream::write_header( FILE* fpt ) {
	if ( fpt == NULL ) { return false; }

	/* The header's fields are written one by one, and converted to the format's
	little endian representation. */
    bmp_header *header = &this->header;
	if ( !write_ushort( header->magic,              fpt ) ) return false;
	if ( !write_uint  ( header->file_size,          fpt ) ) return false;
	if ( !write_ushort( header->reserved1,          fpt ) ) return false;
	if ( !write_ushort( header->reserved2,          fpt ) ) return false;
	if ( !write_uint  ( header->data_offset,        fpt ) ) return false;
	if ( !write_uint  ( header->header_size,        fpt ) ) return false;
	if ( !write_uint  ( header->width,              fpt ) ) return false;
	if ( !write_uint  ( header->height,             fpt ) ) return false;
	if ( !write_ushort( header->planes,             fpt ) ) return false;
	if ( !write_ushort( header->bits_per_pixel,     fpt ) ) return false;
	if ( !write_uint  ( header->compression_type,   fpt ) ) return false;
	if ( !write_uint  ( header->img_data_size,      fpt ) ) return false;
	if ( !write_uint  ( header->h_pixels_per_meter, fpt ) ) return false;
	if ( !write_uint  ( header->v_pixels_per_meter, fpt ) ) return false;
	if ( !write_uint  ( header->colors_used,        fpt ) ) return false;
	if ( !write_uint  ( header->colors_required,    fpt ) ) return false;

	return true;
}

bool bmpStream::write_uint( unsigned int x, FILE* fpt ) {
	unsigned char little[ 4 ];	/* BMPs use 32 bit ints */

	little[ 3 ] = (unsigned char)( ( x & 0xff000000 ) >> 24 );
	little[ 2 ] = (unsigned char)( ( x & 0x00ff0000 ) >> 16 );
	little[ 1 ] = (unsigned char)( ( x & 0x0000ff00 ) >> 8  );
	little[ 0 ] = (unsigned char)( ( x & 0x000000ff ) >> 0  );

	return ( fpt && fwrite( little, 4, 1, fpt ) == 1 );
}

bool bmpStream::write_ushort( unsigned short x, FILE* fpt ) {
	unsigned char little[ 2 ];	/* BMPs use 16 bit shorts */

	little[ 1 ] = (unsigned char )( ( x & 0xff00 ) >> 8 );
	little[ 0 ] = (unsigned char )( ( x & 0x00ff ) >> 0 );

	return ( fpt && fwrite( little, 2, 1, fpt ) == 1 );
}

void bmpStream::set_pixel_RGB(
    unsigned int h, unsigned int w,
    unsigned char r, unsigned char g, unsigned char b ) {

	unsigned char*	pixel;
	unsigned char	bytes_per_pixel;
	unsigned int	bytes_per_row;

	if ( w < 0 || w >= this->header.width ||
         h < 0 || h >= this->header.height   ) {
		printf("(%d, %d)\n", h, w);	
		throw "BMP_INVALID_ARGUMENT";
	}
    if ( this->header.bits_per_pixel != 24 &&
         this->header.bits_per_pixel != 32    ) {
		throw "BMP_TYPE_MISMATCH";
	}

    bytes_per_pixel = this->header.bits_per_pixel >> 3;

    /* Row's size is rounded up to the next multiple of 4 bytes */
    bytes_per_row = this->header.img_data_size / this->header.height;

    /* Calculate the location of the relevant pixel (rows are flipped) */
    pixel = this->data +
            ( this->header.height - h - 1 ) * bytes_per_row +
            ( w * bytes_per_pixel);

    /* Note: colors are stored in BGR order */
    *( pixel + 2 ) = r; *( pixel + 1 ) = g; *( pixel + 0 ) = b;
}