#include <stdio.h>

#include "jpegStream.h"

using namespace std;

int main( int argc, char **argv ) {
    if ( argc == 2 ) {
        try {
            jpegDecoder jpeg( argv[1] );
        } catch (const char err[]) {
            printf("%s\n", err);
        }   
    }
    return 0;
}
