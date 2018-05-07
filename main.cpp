#include <stdio.h>

#include "jpegStream.h"

using namespace std;

int main( int argc, char **argv ) {
    if ( argc == 3 ) {
        try {
            jpegDecoder jpeg( argv[1], argv[2] );
        } catch (const char err[]) {
            printf("%s\n", err);
        }   
    }
    return 0;
}
