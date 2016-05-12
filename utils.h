// Various utilities.

#ifndef _RENDER_UTILS_H
#define _RENDER_UTILS_H

#include <string>
#include <random>
#include <Eigen/Dense>

#define FLOAT_INF (1e100)

extern long long triangle_tests;

#ifdef DOUBLE_PRECISION
typedef double Real;
typedef Eigen::Vector3d Vec;
#define real_abs fabs
#else
typedef float Real;
typedef Eigen::Vector3f Vec;
#define real_abs fabsf
#endif

typedef Eigen::Matrix<Real, 3, 3> Mat;
typedef Vec Color;

inline static Real real_min(Real x, Real y) {
	return x < y ? x : y;
}

inline static Real real_max(Real x, Real y) {
	return x < y ? y : x;
}

inline static Vec vec_min(Vec x, Vec y) {
	return Vec(real_min(x(0), y(0)), real_min(x(1), y(1)), real_min(x(2), y(2)));
}

inline static Vec vec_max(Vec x, Vec y) {
	return Vec(real_max(x(0), y(0)), real_max(x(1), y(1)), real_max(x(2), y(2)));
}

struct Ray {
	Vec origin;
	Vec direction;

	Ray();
	Ray(Vec origin, Vec direction);
	Real distance_along_ray(Vec p) const;
};

struct CastingRay {
	Vec recip_deltas;
	Ray ray;

	CastingRay(const Ray& ray);
};

struct AABB {
	Vec minima, maxima;

	// The default initializer sets minima and maxima to +inf and -inf (an invalid AABB) so that updating from there sets it.
	// Beware of this.
	AABB();
	void set_to_point(Vec p);
	void update(Vec p);
	void update(const AABB& other);
	bool does_ray_intersect(const CastingRay& ray) const;
	void surface_areas_on_sides_of_split_axis(int axis, Real height, Real& sa_low, Real& sa_high) const;
	int longest_axis() const;
};

struct Triangle {
	Vec points[3];
	Vec edge01, edge02;
	Vec normal;
	AABB aabb;
	Real plane_parameter;

	Triangle();
	Triangle(Vec p0, Vec p1, Vec p2);
	bool ray_test(const Ray& ray, Real& hit_parameter, const Triangle** hit_triangle) const;
	Vec project_point_to_given_altitude(Vec point, Real desired_altitude) const;
	bool intersects_axis_aligned_plane(int axis, Real plane_height) const;
};

Vec sample_unit_sphere(std::mt19937& engine);

void override_thread_count(int thread_count);
int get_optimal_thread_count();
void start_performance_counter();
void print_performance_counter();
std::string format_seconds_as_hms(double seconds, int width);

#endif

