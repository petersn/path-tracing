// Example main.

using namespace std;
#include <math.h>
#include <iostream>
#include "integrator.h"
#include "visualizer.h"

int main(int argc, char** argv) {
	// Load up an STL file.
	auto scene = new Scene(argv[1]);
	// Make a light.
	scene->lights->push_back(Light({Vec(0, 0, 3), 9.0 * Vec(0.8, 0.5, 0.25)}));
	scene->lights->push_back(Light({Vec(-2, 2, 4), 9.0 * Vec(0.25, 0.8, 0.25)}));
	scene->lights->push_back(Light({Vec(-2, -2, 4), 9.0 * Vec(0.25, 0.25, 0.8)}));
	scene->camera_image_plane_width = 0.5 * 1.5;
	Real angle = 20 * 0.05;
	scene->main_camera.origin = -5 * Vec(cos(angle), sin(angle), 0.0);
	scene->main_camera.direction = -scene->main_camera.origin;
	scene->main_camera.direction.normalize();
	scene->main_camera.origin += Vec(0.0, 0.0, 0.2);
	scene->plane_of_focus_distance = 4.5;
	scene->dof_dispersion = 0.1;

	start_performance_counter();

	auto engine = new RenderEngine(1366, 768, scene);
	engine->tile_width = 32;
	engine->tile_height = 32;

	auto display = new ProgressBar(engine);
//	display->init();

	engine->perform_full_passes(10);
//	engine->sync();
//	engine->rebuild_master_canvas();
	display->main_loop();

	engine->master_canvas->save("output.png");
	delete engine;

	cout << "Total time to render: ";
	print_performance_counter();
	cout << endl;

	delete scene;
}

