// Various utilities.

#ifndef _RENDER_UTILS_H
#define _RENDER_UTILS_H

#include <Eigen/Dense>

typedef float Real;
typedef Eigen::Vector3f Vec;
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

	void set_to_point(Vec p);
	void update(Vec p);
	void update(const AABB& other);
	bool does_ray_intersect(const CastingRay& ray);
};

struct Triangle {
	Vec points[3];
	Vec normal;
	AABB aabb;

	Triangle();
	Triangle(Vec p0, Vec p1, Vec p2);
	bool intersects_axis_aligned_plane(int axis, Real plane_height);
};

#endif

