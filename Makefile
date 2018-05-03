TARGET = jpegDecoder.out
OBJS = main.o jpegDecoder.o jpegStream.o jpegHeader.o huffmanTable.o 

$(TARGET) : $(OBJS)
	g++ $^ -std=c++14 -o $@

jpegStream.o: jpegStream.cpp jpegDecoder.h
	g++ -c $^ -std=c++14
jpegHeader.o: jpegHeader.cpp jpegDecoder.h
	g++ -c $^ -std=c++14

%.o: %.cpp %.h
	g++ -c $^ -std=c++14
%.o: %.cpp
	g++ -c $^ -std=c++14

clean:
	rm *.o *.h.gch *.out
