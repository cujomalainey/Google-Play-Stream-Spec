CXXFLAGS=-Wall -O3 -g -fno-strict-aliasing
COMMON_LIBS=-lstdc++

audio: audio.cpp
	gcc audio.cpp -lm -lbass -L. $(COMMON_LIBS) $(CXXFLAGS) -o audio

matrix: matrix.cpp
	gcc main.cpp -lrgbmatrix $(COMMON_LIBS) $(CXXFLAGS) -o matrix
