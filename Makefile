TARGET = jpegDecoder.out
OBJS = main.o huffmanTable.o jpegDecoder.o 

$(TARGET) : $(OBJS)
	g++ $^ -std=c++14 -o $@

%.o: %.cpp
	g++ -c $^ -std=c++14

%.o: %.cpp %.h
	g++ -c $^ -std=c++14

clean:
	rm *.o *.h.gch *.out
