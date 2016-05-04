// Monte-Carlo path tracing integrator.

using namespace std;
#include <sys/time.h>
#include <iostream>
#include <algorithm>
#include <ctime>
#include "integrator.h"
#include "stlreader.h"

Scene::Scene(string path) : main_camera(Vec(-1, 0, 0), Vec(1, 0, 0)) {
	// The convention is that main_camera.cross(scene_up) is camera right.
	scene_up = Vec(0, 0, 1);
	// Field of view is 90 degrees by default.
	camera_image_plane_width = 1.0;
	// Set default depth of field parameters.
	plane_of_focus_distance = 1.0;
	dof_dispersion = 0.0;

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

static inline Real square(Real x) {
	return x * x;
}

Color Integrator::cast_ray(const Ray& ray, int recursions, int branches) {
	Real param;
	Triangle* hit_triangle;
	bool result = scene->tree->ray_test(ray, param, &hit_triangle);
	Color energy(0, 0, 0);
	if (result) {
		Vec hit = ray.origin + param * ray.direction;
		// Lift the point off the surface.
		hit = hit_triangle->project_point_to_given_altitude(hit, 1e-3);
		Vec reflection = ray.direction - 2 * hit_triangle->normal.dot(ray.direction) * hit_triangle->normal;
		if (recursions > 0) {
			for (int branch = 0; branch < branches; branch++) {
				// Compute a Lambertianly scattered ray.
				Vec local_scatter_direction = sample_unit_sphere(engine);
				local_scatter_direction(0) = real_abs(local_scatter_direction(0));
				// Convert the triangle-local direction into global coordinates.
				Vec d1 = hit_triangle->edge01.normalized();
				Vec d2 = hit_triangle->normal.cross(d1);
				Vec scatter_direction = local_scatter_direction(0) * hit_triangle->normal + local_scatter_direction(1) * d1 + local_scatter_direction(2) * d2;
//				Vec scatter_direction = reflection;
				Ray scattered_ray(hit, scatter_direction);
				// Recursively sample the scattered light.
				energy += (1.0 / branches) * 0.8 * cast_ray(scattered_ray, recursions-1, 1);
			}
		}
		// Color by lights.
		for (auto& light : *scene->lights) {
			// Cast a ray to the light.
			Vec to_light = light.position - hit;
			Ray shadow_ray(hit, to_light);
			Real shadow_param;
			bool shadow_result = scene->tree->ray_test(shadow_ray, shadow_param);
			Real distance_to_light = to_light.norm();
			if ((not shadow_result) or shadow_param > distance_to_light) {
				// Light is not obscured -- apply it.
				Color contribution = light.color / (distance_to_light * distance_to_light);
				// Now we modulate the contribution by our surface shaders.
				Real lambertian_coef = hit_triangle->normal.dot(to_light) / distance_to_light;
				reflection.normalize();
				Real phong_coef = real_max(0.0, reflection.dot(to_light) / distance_to_light);
				phong_coef = square(square(square(square(phong_coef))));
				energy += contribution * (lambertian_coef + phong_coef);
			}
		}
	}
	return energy;
}

PassDescriptor::PassDescriptor() : start_x(0), start_y(0), width(-1), height(-1) {
}

PassDescriptor::PassDescriptor(int start_x, int start_y, int width, int height) : start_x(start_x), start_y(start_y), width(width), height(height) {
}

void PassDescriptor::clamp_bounds(int max_width, int max_height) {
	// Do some basic sanity checks.
	if (start_x < 0)
		start_x = 0;
	if (start_y < 0)
		start_y = 0;
	if (start_x > max_width - 1)
		start_x = max_width - 1;
	if (start_y > max_height - 1)
		start_y = max_height - 1;
	// Now compute a stopping value.
	if (width == -1)
		width = max_width;
	if (height == -1)
		height = max_height;
	if (width > max_width - start_x)
		width = max_width - start_x;
	if (height > max_height - start_y)
		height = max_height - start_y;
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

void Integrator::perform_pass(PassDescriptor desc) {
	// Iterate over the image.
	// XXX: Use tiles instead for cache coherency.
	Vec camera_right = scene->main_camera.direction.cross(scene->scene_up);
	// A zero division on this next line indicates that camera_up is parallel to main_camera.
	camera_right.normalize();
	Vec camera_up = camera_right.cross(scene->main_camera.direction);
	camera_up.normalize();
	Real aspect_ratio = canvas->height / (Real) canvas->width;

	struct timeval start, stop, result;

	gettimeofday(&start, NULL);

	// Used for DOF offsets.
	// NB: By using a normal here I effectively have an aperature with a Gaussian response across its surface.
	// This is a really weird assumption to make!
	normal_distribution<> dist(0, 1);
	Real plane_of_focus_distance = scene->plane_of_focus_distance;
	Real dof_dispersion = scene->dof_dispersion;

	// Compute the bounds to iterate over.
	// Here we use the convention that a width/height of -1 means "go all the way to the edge of the canvas".
	desc.clamp_bounds(canvas->width, canvas->height);

//	cout << "Got bounds: " << desc.start_x << "-" << stop_x << " " << desc.start_y << "-" << stop_y << endl;

	#pragma omp parallel for
	for (int y = desc.start_y; y < desc.start_y + desc.height; y++) {
		for (int x = desc.start_x; x < desc.start_x + desc.width; x++) {
//	for (int y = 0; y < canvas->height; y++) {
//		for (int x = 0; x < canvas->width; x++) {
			Real dx = scene->camera_image_plane_width * (x - canvas->width / 2) / (Real) canvas->width;
			Real dy = -scene->camera_image_plane_width * (y - canvas->height / 2) * aspect_ratio / (Real) canvas->height;
			// Compute an offset into the image plane that the camera should face.
			Vec offset = camera_right * dx + camera_up * dy;
			Ray ray(scene->main_camera.origin, scene->main_camera.direction + offset);
			Real dof_x_offset = dist(engine) * dof_dispersion;
			Real dof_y_offset = dist(engine) * dof_dispersion;
			ray.origin += dof_x_offset * camera_right;
			ray.origin += dof_y_offset * camera_up;
			ray.direction -= (dof_x_offset / plane_of_focus_distance) * camera_right;
			ray.direction -= (dof_y_offset / plane_of_focus_distance) * camera_up;
			// Do the big expensive computation.
			Color contribution = cast_ray(ray, 4, 1);
			// Accumulate the energy into our buffer.
			*canvas->pixel_ptr(x, y) += contribution;
			// Mark that another pass is contributing to this pixel.
			*canvas->per_pixel_passes_ptr(x, y) += 1;
		}
	}

	gettimeofday(&stop, NULL);
	timersub(&stop, &start, &result);
	last_pass_seconds = result.tv_sec + result.tv_usec * 1e-6;

	// Track the number of passes we've performed, so we can normalize at the end.
	passes++;
}

// ========== Parallelized rendering engine ========== //

RenderThread::RenderThread(RenderEngine* parent) : parent(parent) {
	// Launch an actual thread.
	sem_init(&messages_semaphore, 0, 0);
	pthread_mutex_init(&messages_lock, NULL);
	pthread_mutex_init(&integrator_lock, NULL);
	// Make an integrator with its own canvas.
	integrator = new Integrator(parent->width, parent->height, parent->scene);
	// Launch our thread!
	pthread_create(&thread, nullptr, RenderThread::render_thread_main, (void*)this);

}

RenderThread::~RenderThread() {
	sem_destroy(&messages_semaphore);
	pthread_mutex_destroy(&messages_lock);
	pthread_mutex_destroy(&integrator_lock);
	delete integrator;
}

void RenderThread::send_message(RenderMessage message) {
	pthread_mutex_lock(&messages_lock);
	messages.push_back(message);
	pthread_mutex_unlock(&messages_lock);
	sem_post(&messages_semaphore);
}

void* RenderThread::render_thread_main(void* cookie) {
	RenderThread* self = (RenderThread*) cookie;
	RenderMessage current_message;
	while (true) {
		// Wait on a new message from the main thread.
		sem_wait(&self->messages_semaphore);
		pthread_mutex_lock(&self->messages_lock);
		// Copy over the message from the main thread.
		current_message = self->messages.front();
		self->messages.pop_front();
		// Release the lock allowing the main thread to write a new message.
		pthread_mutex_unlock(&self->messages_lock);

		// If the message tells us to die, do so.
		if (current_message.do_die)
			break;

		// Mark what tile we're currently processing so that the ProgressDisplay can render little red lines around it.
		self->is_running = true;
		self->currently_processing.start_x = current_message.desc.start_x;
		self->currently_processing.start_y = current_message.desc.start_y;
		self->currently_processing.width   = current_message.desc.width;
		self->currently_processing.height  = current_message.desc.height;

		// Otherwise we execute a single pass.
		// NB: Later if we want workers to do other sorts of things add extra message types here.
		pthread_mutex_lock(&self->integrator_lock);
		self->integrator->perform_pass(current_message.desc);
		pthread_mutex_unlock(&self->integrator_lock);

		self->is_running = false;

		// Increment this counter that is read by Progress{Display,Bar}.
		pthread_mutex_lock(&self->parent->master_lock);
		self->parent->total_passes_completed++;
		pthread_mutex_unlock(&self->parent->master_lock);

		// Inform our parent that a pass has been completed.
		sem_post(&self->parent->passes_semaphore);
	}
	// Be careful! We delete the RenderThread here after we are signaled to die.
	// Don't double free the RenderThread.
	delete self;
	return nullptr;
}

RenderEngine::RenderEngine(int width, int height, Scene* scene) : width(width), height(height), scene(scene) {
	// Initialize our semaphore before launching our threads. (It would also be okay to do it after, though.)
	sem_init(&passes_semaphore, 0, 0);
	pthread_mutex_init(&master_lock, NULL);
	// Spawn our child threads.
	for (int i = 0; i < get_optimal_thread_count(); i++)
		workers.push_back(new RenderThread(this));
	// Allocate a master canvas.
	master_canvas = new Canvas(width, height);
	// Set the default tile width and height to be the full canvas width and height.
	tile_width = width;
	tile_height = height;
	total_passes_issued = 0;
	total_passes_completed = 0;
	semaphore_passes_pending = 0;
}

RenderEngine::~RenderEngine() {
	// Send a do_die message to each worker.
	for (auto worker : workers)
		worker->send_message(RenderMessage({true, PassDescriptor()}));
	// We now have a strange conflict. We need to be sure that the worker threads won't post to passes_semaphore anymore so we can destroy it.
	// However, we can't know that the worker threads have died unless we sync with them.
	// Thus, the next thing we do is sync -- which blocks until all work is done, including all the work we're going to throw away.
	// TODO: Find a workaround for this so that users can intentionally throw away work and kill threads prematurely if they delete a RenderEngine before syncing.
	sync();
	sem_destroy(&passes_semaphore);
	pthread_mutex_destroy(&master_lock);
	// NB: There is no need to delete the RenderThreads here, because they delete themselves when they get the do_die message.
	delete master_canvas;
}

void RenderEngine::issue_pass_desc(PassDescriptor desc) {
	// Get a worker to dispatch to.
	int worker = (total_passes_issued++) % workers.size();
	workers[worker]->send_message(RenderMessage({false, desc}));
	// Keep track of the total number of waits on passes_semaphore we need to be synced with all dispatched work.
	semaphore_passes_pending++;
}

void RenderEngine::perform_full_pass() {
	// Cover the scene in tiles.
	int next_y = 0;
	while (next_y < height) {
		int next_x = 0;
		while (next_x < width) {
			issue_pass_desc(PassDescriptor(next_x, next_y, tile_width, tile_height));
			next_x += tile_width;
		}
		next_y += tile_height;
	}
}

Real global_tile_center_x, global_tile_center_y;
bool tile_compare(const pair<int, int>& a, const pair<int, int>& b) {
	Real a_distance = square(a.first - global_tile_center_x) + square(a.second - global_tile_center_y);
	Real b_distance = square(b.first - global_tile_center_x) + square(b.second - global_tile_center_y);
	return a_distance < b_distance;
}

void RenderEngine::perform_full_passes(int pass_count) {
//	while (pass_count--)
//		perform_full_pass();
	// Cover the scene in tiles.
	vector<pair<int, int>> tile_spots;
	int next_y = 0;
	while (next_y < height) {
		int next_x = 0;
		while (next_x < width) {
			for (int j = 0; j < pass_count; j++)
				tile_spots.push_back(pair<int, int>(next_x, next_y));
			next_x += tile_width;
		}
		next_y += tile_height;
	}
	global_tile_center_x = (width - tile_width) / 2.0;
	global_tile_center_y = (height - tile_height) / 2.0;
	stable_sort(tile_spots.begin(), tile_spots.end(), tile_compare);
	// Push all the pairs.
	for (auto spot : tile_spots)
		issue_pass_desc(PassDescriptor(spot.first, spot.second, tile_width, tile_height));
}

void RenderEngine::sync() {
	// Wait on our semaphore a number of times equal to the number of dispatched jobs.
	while (semaphore_passes_pending) {
		semaphore_passes_pending--;
		sem_wait(&passes_semaphore);
	}
}

int RenderEngine::rebuild_master_canvas() {
	// Clear out our accumulator canvas.
	master_canvas->zero();
	int total_passes = 0;
	for (auto worker : workers) {
		// Grab the lock on the worker's integrator.
		// This guarantees that we don't grab it in between two passes.
		// This might cause us to block for seconds while we wait for a pass to complete!
		// TODO: Implement double buffering so this isn't the case.
		// XXX: For temporary debugging I've disabled these locks.
//		pthread_mutex_lock(&worker->integrator_lock);
		// Accumulate the energy from this worker.
		master_canvas->add_from(worker->integrator->canvas);
		// Count the passes it contributed.
		total_passes += worker->integrator->passes;
//		pthread_mutex_unlock(&worker->integrator_lock);
	}
	return total_passes;
}

