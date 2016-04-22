
CPPFLAGS=-I/usr/include/eigen3 -std=c++11 -Wall -O3 -ffast-math -g -fopenmp -fstack-protector-all

all: example

example: kdtree.o utils.o stlreader.o main.o
	g++ -o $@ $^ -lpng -fopenmp

.PHONY: clean
clean:
	rm *.o

