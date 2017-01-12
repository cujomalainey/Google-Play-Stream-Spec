CXXFLAGS=-Wall -O3 -g -fno-strict-aliasing

all:
	gcc main.cpp -lm -lpthread -lrgbmatrix -lstdc++ -lbass $(CXXFLAGS) -DRASPBERRY_PI=1

osx:
	gcc main.cpp -lm -lstdc++ -lbass $(CXXFLAGS) -DRASPBERRY_PI=0 -I. -L.