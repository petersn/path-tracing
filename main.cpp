// Path tracer main.

using namespace std;
#include <stdio.h>
#include <iostream>
#include <random>
#include <ctime>
#include "integrator.h"

int main(int argc, char** argv) {
	// Load up an STL file.
//	auto scene = new Scene(argv[1]);
	// I'm currently trying to iron out a bug with kd trees.
	auto scene = new Scene("inputs/bug.stl");

	// Reposition the camera.
	scene->main_camera.origin = Vec(-5, 0, 2);

	// Make a light.
	scene->lights->push_back(Light({Vec(0, 0, -5), 10.0 * Vec(1, 0.5, 0.25)}));

	Vec origin(-4.94386, -0.747191, 2);
	Vec direction(0.901766, 0.302744, -0.308486);
//	Vec origin(-0.103974, 4.99892, 2);
//	Vec direction(0.201875, -0.926962, -0.316207);
	cout << origin << "\n\n" << direction << "\n\n";
	Ray ray(origin, direction);
	Real hit_parameter;
	Triangle* hit_triangle;
	bool result = scene->tree->ray_test(ray, hit_parameter, &hit_triangle);
	cout << "Hit result: " << result << endl;
	cout << "Parameter: " << hit_parameter << endl;
	cout << "Hit triangle: " << hit_triangle << endl;
	Vec hit_spot = origin + direction * hit_parameter;
	cout << "Hit at:\n" << hit_spot << endl;

//	// Allocate an integrator, and integrate a single pass of the scene.
//	auto integrator = new Integrator(1366, 768, scene);
//	integrator->perform_pass();
//	cout << "Passes per second: " << 1.0 / integrator->last_pass_seconds << endl;
//	integrator->canvas->save("output.png");
}

