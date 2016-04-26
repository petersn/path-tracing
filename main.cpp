// Path tracer main.

using namespace std;
#include <stdio.h>
#include <iostream>
#include <random>
#include <ctime>
#include "integrator.h"

#define INDENT \
	for (int xx = 0; xx < n->depth*2; xx++) cout << " ";

void dump_tree(kdTreeNode* n) {
	INDENT
	cout << "Depth: " << n->depth << " (tris: " << n->total_triangles << ") ";
	if (n->is_leaf)
		cout << " LEAF ";
	if (n->stored_triangle_count != 0) {
		for (int i = 0; i < n->stored_triangle_count; i++) {
			cout << &n->stored_triangles[i] << " ";
		}
		cout << "\n";
		return;
	}
	cout << "Split: " << n->split_axis << " height: " << n->split_height << "\n";
	if (n->low_side != nullptr) {
		INDENT
		cout << " Low:\n";
		dump_tree(n->low_side);
	}
	if (n->high_side != nullptr) {
		INDENT
		cout << " High:\n";
		dump_tree(n->high_side);
	}
}

int main(int argc, char** argv) {
	// Load up an STL file.
//	auto scene = new Scene(argv[1]);
	// I'm currently trying to iron out a bug with kd trees.
	auto scene = new Scene("inputs/bug.stl");

	// Dump the whole tree.
	dump_tree(scene->tree->root);
	cout << "\n=====\n";

	// Reposition the camera.
	scene->main_camera.origin = Vec(-5, 0, 2);

	// Make a light.
	scene->lights->push_back(Light({Vec(0, 0, -5), 10.0 * Vec(1, 0.5, 0.25)}));

	Vec origin(-4.94386, -0.747191, 2);
	Vec direction(0.901766, 0.302744, -0.308486);
//	Vec origin(-0.103974, 4.99892, 2);
//	Vec direction(0.201875, -0.926962, -0.316207);
	cout << "Origin:\n" << origin << "\n\nDirection:\n" << direction << "\n\n";
	Ray ray(origin, direction);
	Real hit_parameter;
	Triangle* hit_triangle;
	bool result = scene->tree->ray_test(ray, hit_parameter, &hit_triangle);
	cout << "\n=====\n";
	cout << "Hit result: " << result << endl;
	cout << "Parameter: " << hit_parameter << endl;
	cout << "Hit triangle: " << hit_triangle << endl;
	Vec hit_spot = origin + direction * hit_parameter;
	cout << "Hit at:\n" << hit_spot << endl;
	cout << "Hit AABB minima:\n" << hit_triangle->aabb.minima << "\nHit AABB maxima:\n" << hit_triangle->aabb.maxima << "\n";

//	// Allocate an integrator, and integrate a single pass of the scene.
//	auto integrator = new Integrator(1366, 768, scene);
//	integrator->perform_pass();
//	cout << "Passes per second: " << 1.0 / integrator->last_pass_seconds << endl;
//	integrator->canvas->save("output.png");
}

