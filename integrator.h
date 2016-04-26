// Monte-Carlo path tracing integrator.

#ifndef _RENDER_INTEGRATOR_H
#define _RENDER_INTEGRATOR_H

#include <string>
#include <random>
#include "kdtree.h"
#include "canvas.h"

struct Light {
	Vec position;
	Color color;
};

struct Scene {
	vector<Triangle>* mesh;
	vector<Light>* lights;
	kdTree* tree;
	Ray main_camera;
	Vec scene_up;
	Real camera_image_plane_width;

	Scene(std::string path);
	~Scene();
};

struct Integrator {
	Scene* scene;
	Canvas* canvas;
	int passes;
	double last_pass_seconds;
	random_device rd;
	mt19937 engine;

	Color cast_ray(const Ray& ray, int recursions);
	Ray get_ray_for_pixel(int x, int y);

	Integrator(int width, int height, Scene* scene);
	~Integrator();
	void perform_pass();
};

#endif

