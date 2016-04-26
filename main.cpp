// Path tracer main.

using namespace std;
#include <stdio.h>
#include <iostream>
#include <random>
#include <ctime>
#include "integrator.h"

int main(int argc, char** argv) {
	// Load up an STL file.
	auto scene = new Scene(argv[1]);

	// Reposition the camera.
	scene->main_camera.origin = Vec(-5, 0, 2);

	// Make a light.
	scene->lights->push_back(Light({Vec(0, 0, -5), 10.0 * Vec(1, 0.5, 0.25)}));

	// Allocate an integrator, and integrate a single pass of the scene.
	auto integrator = new Integrator(1366, 768, scene);
	integrator->perform_pass();
	cout << "Passes per second: " << 1.0 / integrator->last_pass_seconds << endl;
	integrator->canvas->save("output.png");
}

