// Various utilities.

#include <math.h>
#include <sys/time.h>
#include "utils.h"

#define EPSILON 1e-8

using namespace std;
#include <iostream>
#include <thread>

long long triangle_tests;

Ray::Ray() {
	origin = Vec(0, 0, 0);
	direction = Vec(0, 0, 1);
}

Ray::Ray(Vec _origin, Vec _direction) : origin(_origin), direction(_direction) {
	// We must make sure this vector is normalized at all times!
	direction.normalize();
}

Real Ray::distance_along_ray(Vec p) const {
	return direction.dot(p - origin);
}

CastingRay::CastingRay(const Ray& _ray) {
	ray = _ray;
	for (int i = 0; i < 3; i++)
		recip_deltas(i) = 1.0 / ray.direction(i);
}

AABB::AABB() {
	// These are invalid bounds!
	// We set these simply so that updating an initialized AABB effectively sets it to the first point.
	minima = Vec(FLOAT_INF, FLOAT_INF, FLOAT_INF);
	maxima = -minima;
}

void AABB::set_to_point(Vec p) {
	minima = maxima = p;
}

void AABB::update(Vec p) {
	minima = vec_min(minima, p);
	maxima = vec_max(maxima, p);
}

void AABB::update(const AABB& other) {
	minima = vec_min(minima, other.minima);
	maxima = vec_max(maxima, other.maxima);
}

#define PRINT_VEC(v) \
	cout << v(0) << ", " << v(1) << ", " << v(2) << "\n\n";

bool AABB::does_ray_intersect(const CastingRay& ray) {
	Vec t0 = (minima - ray.ray.origin).array() * ray.recip_deltas.array();
	Vec t1 = (maxima - ray.ray.origin).array() * ray.recip_deltas.array();
	Vec entrance_times = vec_min(t0, t1);
	Vec exit_times = vec_max(t0, t1);
	Real t_start = real_max(entrance_times(0), real_max(entrance_times(1), entrance_times(2)));
	Real t_end   = real_min(exit_times(0), real_min(exit_times(1), exit_times(2)));
/*
	cout << "=== AABB check ===" << endl;
	PRINT_VEC(ray.ray.origin)
	PRINT_VEC(ray.ray.direction)
	PRINT_VEC(minima)
	PRINT_VEC(maxima)
	PRINT_VEC(t0)
	PRINT_VEC(t1)
*/
	// The box is behind us.
	if (t_end < 0)
		return false;
	// The box is missed.
	if (t_start > t_end)
		return false;
	// Otherwise we're golden.
	return true;
}

void AABB::surface_areas_on_sides_of_split_axis(int axis, Real height, Real& sa_low, Real& sa_high) {
	// Compute the perimeter and area of the face normal to `axis`.
	Real perimeter = 0.0;
	Real area = 1.0;
	for (int i = 0; i < 3; i++) {
		if (i != axis)
			continue;
		Real length = maxima(axis) - minima(axis);
		perimeter += 2 * length;
		area *= length;
	}
	// Compute the surface area on the low side of the split.
	Real low_thickness = height - minima(axis);
	Real high_thickness = maxima(axis) - height;
	sa_low = 2 * area + perimeter * low_thickness;
	sa_high = 2 * area + perimeter * high_thickness;
}

Triangle::Triangle() {
}

Triangle::Triangle(Vec p0, Vec p1, Vec p2) {
	points[0] = p0;
	points[1] = p1;
	points[2] = p2;
	edge01 = p1 - p0;
	edge02 = p2 - p0;
	normal = edge01.cross(edge02);
	normal.normalize();
	// In theory these three plane parameters should be equal, but let's average them.
	plane_parameter = (normal.dot(p0) + normal.dot(p1) + normal.dot(p2)) / 3.0;
	aabb.set_to_point(p0);
	aabb.update(p1);
	aabb.update(p2);
}

// Performs M\"oller-Trumbore intersection as per Wikipedia.
bool Triangle::ray_test(const Ray& ray, Real& hit_parameter, Triangle** hit_triangle) {
	triangle_tests++;
	Vec P = ray.direction.cross(edge02);
	Real det = edge01.dot(P);
	if (det > -EPSILON and det < EPSILON)
		return false;
	Real inv_det = 1.0 / det;
	Vec T = ray.origin - points[0];
	Real u = T.dot(P) * inv_det;
	if (u < 0 or u > 1)
		return false;
	Vec Q = T.cross(edge01);
	Real v = ray.direction.dot(Q) * inv_det;
	if (v < 0 or u + v > 1)
		return false;
	Real t = edge02.dot(Q) * inv_det;
	if (t <= EPSILON)
		return false;
	// In this case t is the parameter on the ray of the hit.
	hit_parameter = t;
	if (hit_triangle != nullptr)
		*hit_triangle = this;
	return true;
}

Vec Triangle::project_point_to_given_altitude(Vec point, Real desired_altitude) {
	Real parameter = normal.dot(point);
	Real change = (plane_parameter - parameter) + desired_altitude;
	return point + change * normal;
}

bool Triangle::intersects_axis_aligned_plane(int axis, Real plane_height) {
	assert(false); // TODO: Implement this.
	return true;
}

Vec sample_unit_sphere(mt19937& engine) {
	// I experimentally determined that instantiating this object costs about as much as actually drawing from the distribution once.
	// Thus, I could get maybe a 33% performance improvement by hoisting this out of the loop by moving this distribution into the Integrator object.
	normal_distribution<> dist(0, 1);
	// Technically this next line leaves the evaluation order unspecified, so a seeded render could render differently on different compiled binaries for different platforms. For now I don't care.
	Vec samples(dist(engine), dist(engine), dist(engine));
	samples.normalize();
	return samples;
}

int get_optimal_thread_count() {
	static bool have_gotten_answer = false;
	static int answer;
	if (not have_gotten_answer) {
		answer = thread::hardware_concurrency();
		// If the underlying system isn't sure default to 8.
		// Some day around 2040 I'm going to laugh at myself for setting this to merely 8...
		if (answer <= 0)
			answer = 8;
		have_gotten_answer = true;
		cout << "Using " << answer << " threads." << endl;
	}
	return answer;
}

struct timeval perf_counter_start;

void start_performance_counter() {
	gettimeofday(&perf_counter_start, NULL);
}

void print_performance_counter() {
	struct timeval stop, result;
	gettimeofday(&stop, NULL);
	timersub(&stop, &perf_counter_start, &result);
	double seconds = result.tv_sec + result.tv_usec * 1e-6;
	cout << seconds;
}

string format_seconds_as_hms(double seconds, int width) {
	string s;
	// Flip negatives to positives, then add in the minus sign at the end.
	bool is_negative = seconds < 0;
	if (is_negative)
		seconds *= -1;
	// For now we don't render fractional seconds.
	int total_seconds = (int) ceil(seconds);
	int second_count = total_seconds;
	int minute_count = second_count / 60;
	int hour_count = minute_count / 60;
	second_count %= 60;
	minute_count %= 60;
	// First we add in seconds.
	s += to_string(second_count);
	// Pad if we had just one digit of seconds.
	if (s.size() == 1)
		s = "0" + s;
	s = to_string(minute_count) + ":" + s;
	// Check if hours are needed.
	if (total_seconds >= 3600) {
		// Again, pad if we have one digit of minutes.
		if (s.size() == 4)
			s = "0" + s;
		s = to_string(hour_count) + ":" + s;
	}
	// Add in the minus sign if required.
	if (is_negative)
		s = "-" + s;
	// Pad with spaces out to the desired width.
	while (s.size() < width)
		s = " " + s;
	return s;
}

