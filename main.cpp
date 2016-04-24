// Path tracer main.

using namespace std;
#include <stdio.h>
#include <iostream>
#include "stlreader.h"
#include "kdtree.h"

int main(int argc, char** argv) {
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

#if 0
	int deepest = 0, biggest = 0;
	cout << ":::";
//	tree->root->low_side->high_side->low_side->high_side->low_side->get_stats(deepest, biggest);
	tree->root->get_stats(deepest, biggest);
	cout << endl;
	cerr << "Depth of: " << deepest << " Size: " << biggest << endl;

	// Walk until you find a leaf that is kinda big.
	kdTreeNode* node = tree->root;
	while (node->stored_triangles == 0) {
		node = node->low_side;
		if (node->stored_triangles != 0)
			break;
		node = node->high_side;
	}
//	node = node->low_side->high_side->low_side->low_side;
	Triangle* contents = node->triangles;
	cerr << "Hit triangle count: " << node->stored_triangles << endl;
	cerr << node->reason_to_form << endl;

	// Scan the tree, and write out an STL.
	FILE* fp = fopen("debug.stl", "wb");
	char header[80] = "Debugging STL from snp's renderer.";
	fwrite(header, 80, 1, fp);
	uint32_t actual_triangle_count = 0;
	for (unsigned int i = 0; i < node->stored_triangles; i++) {
//	for (unsigned int i = 0; i < triangles->size(); i++) {
		//Triangle& t = (*triangles)[i];
		Triangle& t = contents[i];
		if (t.from_split)
			continue;
		actual_triangle_count++;
	}
	fwrite(&actual_triangle_count, 4, 1, fp);
	cout << "Triangle count: " << actual_triangle_count << endl;
	for (unsigned int i = 0; i < node->stored_triangles; i++) {
		Triangle& t = contents[i];
//		Triangle& t = (*triangles)[i];
		if (t.from_split)
			continue;
		float normal[3] = {0.0, 0.0, 0.0};
		fwrite(normal, 4, 3, fp);
		float vertex[3];
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++)
				vertex[k] = t.points[j](k);
			fwrite(vertex, 4, 3, fp);
		}
		uint16_t attribute_count = 0;
		fwrite(&attribute_count, 2, 1, fp);
	}
	fclose(fp);
#endif

	delete tree;
	delete triangles;

	return 0;
}

