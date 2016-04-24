// Path tracer main.

using namespace std;
#include <stdio.h>
#include <iostream>
#include <random>
#include <ctime>
#include "stlreader.h"
#include "kdtree.h"
#include "canvas.h"

#define TOTAL_RAY_CASTS 100000

int main(int argc, char** argv) {
	random_device rd;
	mt19937 engine(rd());
	uniform_real_distribution<> dist(-10, 10);

	cout << "=== snp's path tracing renderer ===" << endl;

	// Read in the input.
	vector<Triangle>* triangles = read_stl(argv[1]);
	if (triangles == nullptr) {
		cerr << "Couldn't read input file." << endl;
		return 1;
	}
	cout << "Read in " << triangles->size() << " triangles." << endl;

	// Build the kdTree.
	auto tree = new kdTree(triangles);

	// Print some stats.
	int deepest = 0, biggest = 0;
	tree->root->get_stats(deepest, biggest);
	cerr << "Depth of: " << deepest << " Size: " << biggest << endl;

	// Make a canvas.
	auto canv = new Canvas(1920, 1080);
	canv->save("output.png");

	delete tree;
	delete triangles;

	return 0;
}

