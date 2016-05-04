
# Optionally one can include: -fstack-protector-all
CPPFLAGS=`sdl-config --cflags` -I/usr/include/eigen3 -std=c++11 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -O3 -ffast-math -g -pthread
LINKFLAGS=-lpng -lpthread

# Option: Use doubles rather than floats for the Real type.
#CPPFLAGS+=-DDOUBLE_PRECISION

# Option: Build the k-d tree/BVH in a multithreaded manner.
CPPFLAGS+=-DTHREADED_KD_BUILD

# Option: Parallelize sampling the image.
CPPFLAGS+=-DTHREADED_SAMPLING

# Option: Use OpenMP to parallelize sampling.
#CPPFLAGS+=-fopenmp
#LINKFLAGS+=-fopenmp

all: example interactive

example: kdtree.o utils.o stlreader.o canvas.o integrator.o main.o
	g++ -o $@ $^ $(LINKFLAGS)

interactive: kdtree.o utils.o stlreader.o canvas.o integrator.o interactive.o
	g++ -o $@ $^ $(LINKFLAGS) `sdl-config --libs`

animate: kdtree.o utils.o stlreader.o canvas.o integrator.o animate.o
	g++ -o $@ $^ $(LINKFLAGS) `sdl-config --libs`

.PHONY: clean
clean:
	rm -f *.o example interactive

