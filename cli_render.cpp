// CLI for renderer.

using namespace std;
#include <boost/program_options.hpp>
#include <iostream>
#include <vector>
#include <string>

#include "integrator.h"
#include "visualizer.h"

namespace po = boost::program_options;

int main(int argc, char** argv) {
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "Produce help message.")
		("stl", po::value<vector<string>>(), "Input STL file.")
		("output", po::value<string>()->default_value("output.png"), "Output PNG file.")
		("samples", po::value<int>()->default_value(10), "Number of samples.")
		("width", po::value<int>()->default_value(1920), "Width of rendered image.")
		("height", po::value<int>()->default_value(1080), "Height of rendered image.")
		("display", "Display render progress graphically.")
		("progressive", po::value<int>(), "Render passes progressively.")
		("threads", po::value<int>()->default_value(0), "Number of threads. (0 for auto)")
		("angle", po::value<double>()->default_value(1.0), "Camera angle.")
		("camera-altitude", po::value<double>()->default_value(0.2), "Camera altitude.")
		("dof-aperture", po::value<double>()->default_value(0.0), "Depth of field Gaussian aperture standard deviation. Use 0.0 to disable DoF.")
		("dof-distance", po::value<double>()->default_value(1.0), "Distance to the plane of focus.")
		("tile-width", po::value<int>()->default_value(64), "Width of a rendering tile in pixels.")
		("tile-height", po::value<int>()->default_value(64), "Height of a rendering tile in pixels.")
	;

	po::positional_options_description p;
	p.add("stl", -1);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << "\n";
		return 1;
	}

	// Try to figure out input file.
	if (not vm.count("stl")) {
		cout << "No input STL specified." << endl;
		cout << "Example usage: cli_render input.stl --output output.png" << endl;
		return 1;
	}

	auto inputs = vm["stl"].as<vector<string>>();
	assert(inputs.size() > 0);
	if (inputs.size() > 1) {
		cout << "Too many inputs specified -- currently only one input STL is supported." << endl;
		return 1;
	}
	auto path = inputs[0];

	// Print out the various arguments set.
	cout << "input        = " << path << endl;
	for (string key : {"output", "samples", "width", "height", "threads", "angle", "camera-altitude", "dof-aperture", "dof-distance", "tile-width", "tile-height"}) {
		// Skip the dof-distance if dof-aperture is zero.
		if (key == "dof-distance" and vm["dof-aperture"].as<double>() == 0)
			continue;
		cout << key;
		for (int i = key.size(); i < 13; i++)
			cout << " ";
		cout << "= ";
		const auto& value = vm[key];
		try { cout << value.as<int>(); }
		catch (...) {};
		try { cout << value.as<double>(); }
		catch (...) {};
		try { cout << value.as<string>(); }
		catch (...) {};
		cout << endl;
	}

	// Set the thread count -- zero tells override_thread_count to go back to automatic detection.
	override_thread_count(vm["threads"].as<int>());

	// Begin rendering!
	auto scene = new Scene(path);
	scene->lights->push_back(Light({Vec(0, 0, 3), 9.0 * Vec(0.8, 0.5, 0.25)}));
	scene->lights->push_back(Light({Vec(-2, 2, 4), 9.0 * Vec(0.25, 0.8, 0.25)}));
	scene->lights->push_back(Light({Vec(-2, -2, 4), 9.0 * Vec(0.25, 0.25, 0.8)}));
	scene->camera_image_plane_width = 0.5 * 1.5;
	Real angle = vm["angle"].as<double>();//22 * 0.05;
	scene->main_camera.origin = -5 * Vec(cos(angle), sin(angle), 0.0);
	scene->main_camera.direction = -scene->main_camera.origin;
	scene->main_camera.direction.normalize();
	scene->main_camera.origin += Vec(0.0, 0.0, vm["camera-altitude"].as<double>());
	scene->plane_of_focus_distance = vm["dof-distance"].as<double>();
	scene->dof_dispersion = vm["dof-aperture"].as<double>();

	auto engine = new RenderEngine(vm["width"].as<int>(), vm["height"].as<int>(), scene);
	engine->tile_width = vm["tile-width"].as<int>();
	engine->tile_height = vm["tile-height"].as<int>();

	ProgressReporter* pr;
	if (vm.count("display"))
		pr = new ProgressDisplay(engine);
	else
		pr = new ProgressBar(engine);
	pr->init();
	int samples_count = vm["samples"].as<int>();
	if (vm.count("progressive")) {
		int progressive_count = vm["progressive"].as<int>();
		for (int i = 0; i < samples_count / progressive_count; i++)
			engine->perform_full_passes(progressive_count);
		engine->perform_full_passes(samples_count % progressive_count);
	} else
		engine->perform_full_passes(samples_count);
	pr->main_loop();
	delete pr;
//	engine->sync();
	engine->rebuild_master_canvas();
	auto output_path = vm["output"].as<string>();
	engine->master_canvas->save(output_path);
	cout << "Wrote to: " << output_path << endl;
//	delete engine;
//	delete scene;
}

