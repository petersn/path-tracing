// Path tracer main.

using namespace std;
#include <iostream>
#include "stlreader.h"
#include "kdtree.h"

int main(int argc, char** argv) {
	cout << "=== snp's path tracing renderer ===" << endl;

	// Read in the input.
	vector<Triangle>* triangles = read_stl("suzanne.stl");
	cout << "Read in " << triangles->size() << " triangles." << endl;

	// Build the kdTree.
	auto tree = new kdTree(*triangles);

	return 0;
}

