// Monte-Carlo path tracing integrator.

using namespace std;
#include <iostream>
#include "integrator.h"
#include "stlreader.h"

Scene::Scene(string path) {
	// The convention is that main_camera.cross(scene_up) is camera right.
	scene_up = Vec(0, 0, 1);
	camera_image_plane_width = 0.5;

	// Read in the input.
	mesh = read_stl(path);
	if (mesh == nullptr) {
		cout << "Couldn't read input file." << endl;
		return;
	}
	cout << "Read in " << mesh->size() << " triangles." << endl;

	// Build the k-d tree.
	tree = new kdTree(mesh);

	// Print some stats.
	int deepest = 0, biggest = 0;
	tree->root->get_stats(deepest, biggest);
	cout << "kdTree depth = " << deepest << " max leaf size = " << biggest << endl;
}

Scene::~Scene() {
	delete mesh;
	delete tree;
}

Color Integrator::cast_ray(const Ray& ray, int recursions) {
	return Color(0.5, 0.5, 0.5);
}

Integrator::Integrator(int width, int height, Scene* scene) : scene(scene), engine(rd()) {
	passes = 0;
	// Allocate a canvas.
	canvas = new Canvas(width, height);
	canvas->zero();
}

Integrator::~Integrator() {
	delete canvas;
}

void Integrator::perform_pass() {
	// Iterate over the image.
	// XXX: Use tiles instead for cache coherency.
	Vec camera_right = scene->main_camera.direction.cross(scene->scene_up);
	// A zero division on this next line indicates that camera_up is parallel to main_camera.
	camera_right.normalize();
	Vec camera_up = camera_right.cross(scene->main_camera.direction);
	camera_up.normalize();
	for (int y = 0; y < canvas->height; y++) {
		for (int x = 0; x < canvas->width; x++) {
			Real dx = (x - canvas->width / 2) / (Real) canvas->width;
			Real dy = (x - canvas->height / 2) / (Real) canvas->height;
			// Compute an offset into the image plane that the camera should face.
			Vec offset = camera_right * dx + camera_up * dy;
			Ray ray(scene->main_camera.origin, scene->main_camera.direction + offset);
			// Do the big expensive computation.
			Color contribution = cast_ray(ray, 0);
			// Accumulate the energy into our buffer.
			*canvas->pixel_ptr(x, y) += contribution;
		}
	}
	// Remember the number of passes we've performed, so we can normalize at the end.
	passes++;
}

