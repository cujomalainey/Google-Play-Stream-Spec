CXXFLAGS=-Wall -O3 -g -fno-strict-aliasing

all:
	gcc main.cpp -lm -lpthread -lrgbmatrix -lstdc++ -lbass $(CXXFLAGS)
