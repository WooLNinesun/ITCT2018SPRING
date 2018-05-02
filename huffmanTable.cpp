#include "huffmanTable.h"

using namespace std;

huffmanTable_el* huffmanTables::get( unsigned char ht_info ) {
    switch(ht_info) {
        case  0: { return this->DC[0]; }
        case  1: { return this->DC[1]; }
        case 16: { return this->AC[0]; }
        case 17: { return this->AC[1]; }
        default: { return 0;           }
    }
}
