// Various utilities.

#include "utils.h"

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

bool AABB::does_ray_intersect(const CastingRay& ray) {
	Vec t0 = (minima - ray.ray.origin).array() * ray.recip_deltas.array();
	Vec t1 = (maxima - ray.ray.origin).array() * ray.recip_deltas.array();
	Real t_start = real_max(t0(0), real_max(t0(1), t0(2)));
	Real t_end   = real_min(t1(0), real_min(t1(1), t1(2)));
	// The box is behind us.
	if (t_end < 0)
		return false;
	// The box is missed.
	if (t_start > t_end)
		return false;
	// Otherwise we're golden.
	return true;
}

Triangle::Triangle() {
}

Triangle::Triangle(Vec p0, Vec p1, Vec p2) {
	points[0] = p0;
	points[1] = p1;
	points[2] = p2;
	normal = (p1 - p0).cross(p2 - p0);
	normal.normalize();
	aabb.set_to_point(p0);
	aabb.update(p1);
	aabb.update(p2);
}

bool Triangle::intersects_axis_aligned_plane(int axis, Real plane_height) {
	return true;
//	assert(false); // TODO: Implement this.
}

