// Path tracer main.

using namespace std;
#include <stdio.h>
#include <iostream>
#include <random>
#include <ctime>
#include "stlreader.h"
#include "kdtree.h"

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

	// Perform some random ray tests.
	triangle_tests = 0;
	Ray ray(Vec(-10.0, 0.0, 0.0), Vec(1.0, +0.04, -0.005));
	Real hit_parameter;
	bool result;
	int hits = 0;
	clock_t start = clock();
	for (int i = 0; i < TOTAL_RAY_CASTS; i++) {
		for (int j = 0; j < 3; j++)
			ray.origin(j) = dist(engine);
		ray.direction = - ray.origin;
		ray.direction.normalize();
		result = tree->ray_test(ray, hit_parameter);
		hits += result;
	}
	double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
	cout << "Casts per second: " << TOTAL_RAY_CASTS / elapsed << endl;
//	cout << "Hit: " << result << endl;
//	if (result)
//		cout << "Parameter: " << hit_parameter << endl;
	cout << "Average triangle intersections: " << triangle_tests / (double) TOTAL_RAY_CASTS << endl;
	cout << "Hit proportion: " << hits / (double) TOTAL_RAY_CASTS << endl;

	delete tree;
	delete triangles;

	return 0;
}

