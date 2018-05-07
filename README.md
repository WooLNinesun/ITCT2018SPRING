## How to compile and run your code

    Makefile and use "make" command, will build in "-std=c++11".

    ```
    $ make
    g++ -std=c++11 -c main.cpp
    g++ -std=c++11 -c jpegStream.cpp jpegStream.h
    g++ -std=c++11 -c jpegRead.cpp jpegStream.h
    g++ -std=c++11 -c jpegHeader.cpp jpegStream.h
    g++ -std=c++11 -c jpegMCU.cpp jpegStream.h
    g++ -std=c++11 -c bmpStream.cpp bmpStream.h
    g++ -std=c++11 main.o jpegStream.o jpegRead.o jpegHeader.o jpegMCU.o bmpStream.o -o main
    ```

## Provide your environment, gcc/g++ version

    ```
    $ g++ --version
    g++ (Ubuntu 5.4.0-6ubuntu1~16.04.9) 5.4.0 20160609
    ```
    ```
    $ uname -a
    Linux ubuntu 4.4.0-122-generic ... x86_64 GNU/Linux
    ```

## How to use?

    ./main  input_file output_file
    
    Ex: ./main test/img/monalisa.jpg test/img/monalisa.bmp
    
## file info

1. bmpStream:
    output to .bmp

2. jpegHeader:
    read header and table
    only read SOF, DHT, SOS, DQT, other skip.

3. jpegMCU:
    handle mcu black(data unit) deQuantize, idct, shift128, to RGB.

4. jpegRead:
    read byte, bits and open file.

5. jpegStream:
    jpeg decode flow.
    read header -> read data ( for each mcu ) -> output

6. main:
    main function.
