CXXFLAGS=-Wall -O3 -g -fno-strict-aliasing

all:
	gcc main.cpp -lrt -lm -lpthread -lrgbmatrix -lstdc++ -lbass $(CXXFLAGS)
