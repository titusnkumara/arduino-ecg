CXX=g++
CXXFLAGS=-I. -Wall -O3
DEPS=eventECG.h
OBJ=eventECG.o main.o

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

ArduinoECG: $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJ) ArduinoECG.exe
