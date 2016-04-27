// Interactive model renderer.

using namespace std;
#include <math.h>
#include <iostream>
#include "integrator.h"

int main(int argc, char** argv) {
	// Load up an STL file.
	auto scene = new Scene(argv[1]);

	// Configure the field of view.
	scene->camera_image_plane_width = 0.5;

	// Make a light.
	scene->lights->push_back(Light({Vec(0, 0, 3), 10.0 * Vec(1, 0.5, 0.25)}));

	// Allocate an integrator for the scene.
	auto integrator = new Integrator(1366, 768, scene);

	triangle_tests = 0;
	for (int frame = 0; frame < 500; frame++) {
		Real angle = frame * 0.01;
		scene->main_camera.origin = -5 * Vec(cos(angle), sin(angle), 0.0);
		scene->main_camera.direction = -scene->main_camera.origin;
		scene->main_camera.direction.normalize();
		scene->main_camera.origin += Vec(0.0, 0.0, 0.2);
		integrator->canvas->zero();
		integrator->perform_pass();
		char output_path[80];
		sprintf(output_path, "output/frame%04i.png", frame);
		integrator->canvas->save(output_path);
		cout << "Saved to: " << output_path << "\n";
	}
	cout << "Total triangle tests: " << triangle_tests << "\n";

	return 0;
}

