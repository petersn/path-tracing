// Monte-Carlo path tracing integrator.

#ifndef _RENDER_INTEGRATOR_H
#define _RENDER_INTEGRATOR_H

#include <string>
#include <random>
#include "kdtree.h"
#include "canvas.h"

struct Scene {
	vector<Triangle>* mesh;
	kdTree* tree;
	Ray main_camera;
	Vec scene_up;
	Real camera_image_plane_width;

	Scene(std::string path);
	~Scene();
};

class Integrator {
	Scene* scene;
	Canvas* canvas;
	int passes;
	random_device rd;
	mt19937 engine;

	Color cast_ray(const Ray& ray, int recursions);

public:
	Integrator(int width, int height, Scene* scene);
	~Integrator();
	void perform_pass();
};

#endif

