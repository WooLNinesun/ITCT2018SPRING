#include <string.h>

#include "jpegStream.h"

using namespace std;

MCU::MCU( component *cpts, unsigned char Vmax, unsigned char Hmax ) {
    this->cpts = cpts; this->Vmax = Vmax; this->Hmax = Hmax;
    memset ( this->blocks, 0, 5*2*2*64 );
}

void MCU::deQuantize( unsigned short *qt, double *block ) {
    for( int i = 0; i < 8; i++ ) {
        for ( int j = 0; j < 8; j++ ) {
            block[8*i+j] *= (double)qt[8*i+j];
        }
    }
}

/*
 alphacos define in jpegStream.h, div2sqrt = 1/sqrt(2), pi = 3.14..
    alphacos[i*8+j] = (j? 1:div2sqrt) * cos( (2*i+1)*j*pi / 16.0 );
 use two 1-D IDCT to calculate 2-D IDCT.
 */
void MCU::IDCT( double *block ) {
    double tmp[64] = { 0 };
    for ( int x = 0; x < 8; x++ ) {
        for ( int y = 0; y < 8; y++ ) {
            for ( int v = 0; v < 8; v++ ) {
                tmp[x*8+y] += block[x*8+v]*this->alphacos[ y*8+v ];
            }
        }
    }
    for ( int x = 0; x < 8; x++ ) {
        for ( int y = 0; y < 8; y++ ) {
            block[x*8+y] = 0;
            for ( int u = 0; u < 8; u++ ) {
                block[x*8+y] +=  tmp[u*8+y]*this->alphacos[ x*8+u ];
            }
        }
    }
}

void MCU::shift128( double *block ) {
    for (unsigned char i = 0; i < 8; i++) {
        for (unsigned char j = 0; j < 8; j++) {
            block[i*8+j] += 128;
        }
    }
}

RGB* MCU::toRGB() {
    unsigned char V = (this->Vmax * 8), H = (this->Hmax * 8);
    RGB *RBG_block = new RGB[V * H];

    for (int i = 0; i < V; i++) {
        for (int j = 0; j < H; j++) {
            double Y  = this->get_cpt_value(0, i, j);
            double Cb = this->get_cpt_value(1, i, j);
            double Cr = this->get_cpt_value(2, i, j);

            RBG_block[i*H+j].R = Normalize(Y + 1.402  *Cr);
            RBG_block[i*H+j].G = Normalize(Y - 0.34414*Cb - 0.71414*Cr);
            RBG_block[i*H+j].B = Normalize(Y + 1.772  *Cb);
        }
    }


    return RBG_block;
}

double MCU::get_cpt_value(
    unsigned char id, unsigned char v, unsigned char h ) {
    unsigned char tmp;
    tmp = v * this->cpts[id].vert / this->Vmax;
    unsigned char v_id = tmp / 8, v_index = tmp % 8;
    tmp = h * this->cpts[id].hori / this->Hmax;
    unsigned char h_id = tmp / 8, h_index = tmp % 8;

    return this->blocks[id][v_id][h_id][v_index*8+h_index];
}

unsigned char MCU::Normalize( double x ) {
    if ( x > 255.0 ) { return 255; }
    if ( x <   0.0 ) { return 0; }
    return (unsigned char) x;
}
