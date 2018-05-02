struct huffmanTable_el {
    unsigned char num    = 0;
    unsigned char* symbol  = 0;
    unsigned short* codeword = 0;
};

class huffmanTables {
public:
    huffmanTable_el DC[2][16];
    huffmanTable_el AC[2][16];
    
    huffmanTable_el* get ( unsigned char ht_info );
};
