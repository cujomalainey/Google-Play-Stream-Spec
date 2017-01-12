CXXFLAGS=-Wall -O3 -g -fno-strict-aliasing

all:
	gcc main.cpp -lm -lpthread -lrgbmatrix -lstdc++ -lbass $(CXXFLAGS) -DRASPBERRY_PI=TRUE

osx:
	gcc main.cpp -lm -lstdc++ -lbass $(CXXFLAGS) -DRASPBERRY_PI=FALSE -I. -L.