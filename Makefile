
# Optionally one can include: -fstack-protector-all
CPPFLAGS=`sdl-config --cflags` -I/usr/include/eigen3 -std=c++11 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -O3 -ffast-math -g -fopenmp -pthread
#CPPFLAGS+=-DDOUBLE_PRECISION
#CPPFLAGS+=-DTHREADED_KD_BUILD

all: example interactive

example: kdtree.o utils.o stlreader.o canvas.o integrator.o main.o
	g++ -o $@ $^ -lpng -fopenmp -lpthread

interactive: kdtree.o utils.o stlreader.o canvas.o integrator.o interactive.o
	g++ -o $@ $^ -lpng -fopenmp -lpthread `sdl-config --libs`

animate: kdtree.o utils.o stlreader.o canvas.o integrator.o animate.o
	g++ -o $@ $^ -lpng -fopenmp -lpthread `sdl-config --libs`

.PHONY: clean
clean:
	rm -f *.o example interactive

