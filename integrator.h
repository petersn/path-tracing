// Monte-Carlo path tracing integrator.

#ifndef _RENDER_INTEGRATOR_H
#define _RENDER_INTEGRATOR_H

#include <string>
#include <random>
#include <vector>
#include "kdtree.h"
#include "canvas.h"

// Forward declaration.
struct RenderEngine;

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
	Real plane_of_focus_distance;
	Real dof_dispersion;

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

	Color cast_ray(const Ray& ray, int recursions, int branches);
	Ray get_ray_for_pixel(int x, int y);

	Integrator(int width, int height, Scene* scene);
	~Integrator();
	void perform_pass();
};

struct RenderMessage {
	bool do_die;
};

struct RenderThread {
	pthread_t thread;
	RenderEngine* parent;

	sem_t messages_semaphore;
	pthread_mutex_t messages_lock;
	std::list<RenderMessage> messages;

	pthread_mutex_t integrator_lock;
	Integrator* integrator;

	RenderThread(RenderEngine* parent);
	~RenderThread();
	void send_message(RenderMessage message);
	static void* render_thread_main(void* cookie);
};

struct RenderEngine {
	int width, height;
	Scene* scene;
	Canvas* master_canvas;
	int total_passes;

	std::vector<RenderThread*> workers;
	// This semaphore gets posted to once for each completed pass by a worker thread.
	// Thus, one can wait on this semaphore some number of times to guarantee that all the workers are done working.
	sem_t passes_semaphore;
	// ... specifically, you wait this many times:
	int semaphore_passes_pending;

	RenderEngine(int width, int height, Scene* scene);
	~RenderEngine();
	void perform_passes(int pass_count);
	// This routine makes sure all the workers are done rendering.
	void sync();
	// This routine accumulates the energy from various workers, sums it into master_canvas, and returns the number of passes averaged over.
	int rebuild_master_canvas();
};

#endif

