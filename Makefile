
# Optionally one can include: -fstack-protector-all
CPPFLAGS=`sdl-config --cflags` -I/usr/include/eigen3 -std=c++11 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -O3 -ffast-math -g -fopenmp
CPPFLAGS+=-DDOUBLE_PRECISION

all: example interactive

example: kdtree.o utils.o stlreader.o canvas.o integrator.o main.o
	g++ -o $@ $^ -lpng -fopenmp

interactive: kdtree.o utils.o stlreader.o canvas.o integrator.o interactive.o
	g++ -o $@ $^ -lpng -fopenmp `sdl-config --libs`

animate: kdtree.o utils.o stlreader.o canvas.o integrator.o animate.o
	g++ -o $@ $^ -lpng -fopenmp `sdl-config --libs`

.PHONY: clean
clean:
	rm -f *.o example interactive

