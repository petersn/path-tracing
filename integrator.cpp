// Monte-Carlo path tracing integrator.

using namespace std;
#include <iostream>
#include <ctime>
#include "integrator.h"
#include "stlreader.h"

Scene::Scene(string path) : main_camera(Vec(-1, 0, 0), Vec(1, 0, 0)) {
	// The convention is that main_camera.cross(scene_up) is camera right.
	scene_up = Vec(0, 0, 1);
	// Field of view is 
	camera_image_plane_width = 1.0;

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

	// Allocate empty storage.
	lights = new vector<Light>();
}

Scene::~Scene() {
	delete mesh;
	delete lights;
	delete tree;
}

Color Integrator::cast_ray(const Ray& ray, int recursions) {
	Real param;
	Triangle* hit_triangle;
	bool result = scene->tree->ray_test(ray, param, &hit_triangle);
	Color energy(0, 0, 0);
	if (result) {
		Vec hit = ray.origin + param * ray.direction;
		// Lift the point off the surface.
		hit = hit_triangle->project_point_to_given_altitude(hit, 1e-3);
		// Color by lights.
		for (auto& light : *scene->lights) {
			// Cast a ray to the light.
			Ray shadow_ray(hit, light.position - hit);
			Real shadow_param;
			bool shadow_result = scene->tree->ray_test(shadow_ray, shadow_param);
			Real distance_to_light = (light.position - hit).norm();
			if ((not shadow_result) /*or shadow_param > distance_to_light*/) {
				// Light is not obscured -- apply it. */
				energy += light.color / (distance_to_light * distance_to_light);
			}
		}
	}
	return energy;
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

Ray Integrator::get_ray_for_pixel(int x, int y) {
	Vec camera_right = scene->main_camera.direction.cross(scene->scene_up);
	// A zero division on this next line indicates that camera_up is parallel to main_camera.
	camera_right.normalize();
	Vec camera_up = camera_right.cross(scene->main_camera.direction);
	camera_up.normalize();
	Real aspect_ratio = canvas->height / (Real) canvas->width;
	Real dx = scene->camera_image_plane_width * (x - canvas->width / 2) / (Real) canvas->width;
	Real dy = -scene->camera_image_plane_width * (y - canvas->height / 2) * aspect_ratio / (Real) canvas->height;
	// Compute an offset into the image plane that the camera should face.
	Vec offset = camera_right * dx + camera_up * dy;
	return Ray(scene->main_camera.origin, scene->main_camera.direction + offset);
}

void Integrator::perform_pass() {
	// Iterate over the image.
	// XXX: Use tiles instead for cache coherency.
	Vec camera_right = scene->main_camera.direction.cross(scene->scene_up);
	// A zero division on this next line indicates that camera_up is parallel to main_camera.
	camera_right.normalize();
	Vec camera_up = camera_right.cross(scene->main_camera.direction);
	camera_up.normalize();
	Real aspect_ratio = canvas->height / (Real) canvas->width;

	clock_t start_time = clock();

//	#pragma omp parallel for
	for (int y = 0; y < canvas->height; y++) {
		for (int x = 0; x < canvas->width; x++) {
			Real dx = scene->camera_image_plane_width * (x - canvas->width / 2) / (Real) canvas->width;
			Real dy = -scene->camera_image_plane_width * (y - canvas->height / 2) * aspect_ratio / (Real) canvas->height;
			// Compute an offset into the image plane that the camera should face.
			Vec offset = camera_right * dx + camera_up * dy;
//			Ray ray(scene->main_camera.origin, scene->main_camera.direction + offset);
			Ray ray = get_ray_for_pixel(x, y);
			// Do the big expensive computation.
			Color contribution = cast_ray(ray, 0);
//			Color contribution = Color(0.3, 0.6, 0.7);
			// Accumulate the energy into our buffer.
			*canvas->pixel_ptr(x, y) += contribution;
		}
	}

	last_pass_seconds = (clock() - start_time) / (double) CLOCKS_PER_SEC;

	// Track the number of passes we've performed, so we can normalize at the end.
	passes++;
}

