
OBJECTS=kdtree.o utils.o stlreader.o canvas.o integrator.o visualizer.o

# Optionally one can include: -fstack-protector-all
CPPFLAGS=`sdl-config --cflags` -I/usr/include/eigen3 -std=c++11 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -O3 -ffast-math -g -pthread
LINKFLAGS=-lpng -lpthread `sdl-config --libs`

# Option: Use doubles rather than floats for the Real type.
#CPPFLAGS+=-DDOUBLE_PRECISION

# Option: Build the k-d tree/BVH in a multithreaded manner.
CPPFLAGS+=-DTHREADED_KD_BUILD

# Option: Parallelize sampling the image.
CPPFLAGS+=-DTHREADED_SAMPLING

# Option: Use precise BVHs for super-duper long renders.
#CPPFLAGS+=-DPRECISE_BVH

# Option: Use link-time optimization.
CPPFLAGS+=-flto
LINKFLAGS+=-O3 -ffast-math -flto -g

# Option: Use OpenMP to parallelize sampling.
#CPPFLAGS+=-fopenmp
#LINKFLAGS+=-fopenmp

all: example interactive

example: $(OBJECTS) main.o
	g++ -o $@ $^ $(LINKFLAGS)

interactive: $(OBJECTS) interactive.o
	g++ -o $@ $^ $(LINKFLAGS)

animate: $(OBJECTS) animate.o
	g++ -o $@ $^ $(LINKFLAGS)

cli_render: $(OBJECTS) cli_render.o
	g++ -o $@ $^ $(LINKFLAGS) -lboost_program_options

.PHONY: clean
clean:
	rm -f *.o example interactive

