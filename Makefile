TARGET = jpegDecoder.out
OBJS = main.o \
	   jpegDecoder.o jpegStream.o jpegHeader.o jpegMCU.o

STDC++ = -std=c++14

$(TARGET) : $(OBJS)
	g++ $(STDC++) $^ -o $@

main.o: main.cpp
	g++ $(STDC++) -c $^
%.o: %.cpp jpegDecoder.h
	g++ $(STDC++) -c $^

clean:
	rm *.o *.h.gch *.out
