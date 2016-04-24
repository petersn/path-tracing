
# Optionally one can include: -fstack-protector-all
CPPFLAGS=-I/usr/include/eigen3 -std=c++11 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -O3 -ffast-math -g -fopenmp
#CPPFLAGS+=-DDOUBLE_PRECISION

all: example

example: kdtree.o utils.o stlreader.o canvas.o integrator.o main.o
	g++ -o $@ $^ -lpng -fopenmp

.PHONY: clean
clean:
	rm *.o

