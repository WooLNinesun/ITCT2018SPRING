---
typora-copy-images-to: Readme
typora-root-url: ./
---

# JPEG-decoder

OS: ubuntu1 16.04.9, Complier: g++, std: c++11

## Usage

```shell
$ make
g++ -std=c++11 -c main.cpp
g++ -std=c++11 -c jpegStream.cpp jpegStream.h
g++ -std=c++11 -c jpegRead.cpp jpegStream.h
g++ -std=c++11 -c jpegHeader.cpp jpegStream.h
g++ -std=c++11 -c jpegMCU.cpp jpegStream.h
g++ -std=c++11 -c bmpStream.cpp bmpStream.h
g++ -std=c++11 main.o jpegStream.o jpegRead.o jpegHeader.o jpegMCU.o bmpStream.o -o jpegDecoder.out
```

## jpegStream

![1525519710461](/Readme/1525519710461.png)



## jpegRead

對檔案的相關操作，包含 jpeg_open(), read_byte(), read_bit()

`read_byte()` -> read 1 byte

1.  用來讀入 header + table 段

`read_bit()`  -> read 1 bit

1.  一旦開始用就不能再使用 read_byte()，否則會出錯，正常來說開始讀 data 段就不會再用 read_byte()
2.  用來讀入 data 段

## jpegHeader

讀入 header 和 table 段的內容，`0xFF` 是跳脫字元，後面會接 marker，表示在哪個 header 段

```c++
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
        case 0xDD: { return this->read_header_skip( marker, "DRI" ); }
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
```

1.  SOS 段之後，就是 data 段，所以可以直接進入 `read_data()`
2.  只記錄 SOF0, DHT, SOS, DQT 的內容，其他通通跳過，另外 SOI, EOI 表示整份圖檔的開始和結束

