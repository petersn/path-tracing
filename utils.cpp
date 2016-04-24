// Various utilities.

#include "utils.h"

#define EPSILON 1e-4

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
	edge01 = p1 - p0;
	edge02 = p2 - p0;
	normal = edge01.cross(edge02);
	normal.normalize();
	aabb.set_to_point(p0);
	aabb.update(p1);
	aabb.update(p2);
}

// Performs M\"oller-Trumbore intersection as per Wikipedia.
bool Triangle::ray_test(const Ray& ray, Real& hit_parameter) {
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
	return true;
}

bool Triangle::intersects_axis_aligned_plane(int axis, Real plane_height) {
	assert(false); // TODO: Implement this.
	return true;
}

