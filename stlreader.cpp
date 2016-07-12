// Simple binary STL reader.

using namespace std;
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <map>
#include "stlreader.h"

vector<Triangle>* read_stl(std::string path) {
	FILE* fp = fopen(path.c_str(), "rb");
	if (fp == NULL)
		return nullptr;
	// Read but ignore the header.
	char header[80];
	fread(header, 80, 1, fp);
	// Read the number of triangles.
	uint32_t triangle_count;
	fread(&triangle_count, sizeof(triangle_count), 1, fp);
	// Read in each triangle.
	auto tris = new vector<Triangle>();
	for (uint32_t triangle_index = 0; triangle_index < triangle_count; triangle_index++) {
		float normal[3];
		float vertices[9];
		assert(fread(normal, sizeof(float) * 3, 1, fp) == 1);
		assert(fread(vertices, sizeof(float) * 9, 1, fp) == 1);
		Vec p0(vertices[0], vertices[1], vertices[2]);
		Vec p1(vertices[3], vertices[4], vertices[5]);
		Vec p2(vertices[6], vertices[7], vertices[8]);
		// Currently I ignore the normal and just hope it ends up being correct from the vertex order.
		// TODO: Reorder the vertices to match the normal given, or at least put in an assert.
		tris->push_back(Triangle(p0, p1, p2));
		// Save all the assigned normals.
		tris->back().assigned_normal = Vec(normal[0], normal[1], normal[2]);
		uint16_t attribute_count;
		assert(fread(&attribute_count, sizeof(uint16_t), 1, fp) == 1);
		// We don't handle any extensions.
		assert(attribute_count == 0);
	}
	fclose(fp);
	compute_barycentric_normals(tris);
	return tris;
}

// In order
namespace std {
	template<>
	struct less<Vec> {
		bool operator()(Vec const& a, Vec const& b) const {
//			assert(a.size()==b.size());
			for (int i = 0; i < 3; i++) {
				if (a(i) < b(i))
					return true;
				if (a(i) > b(i))
					return false;
			}
			return false;
		}
	};
}

void compute_barycentric_normals(std::vector<Triangle>* triangles) {
	// First we find all the triangles adjacent to each point.
	map<Vec, vector<Triangle*>> point_to_tris;
	for (auto& triangle : *triangles)
		for (int i = 0; i < 3; i++)
			point_to_tris[triangle.points[i]].push_back(&triangle);
	// Then we average triangle normals around each loop.
	for (auto& triangle : *triangles) {
		Vec vertex_normals[3];
		for (int i = 0; i < 3; i++) {
			Vec& normal = vertex_normals[i];
			normal = Vec(0, 0, 0);
			for (Triangle* other_triangle : point_to_tris[triangle.points[i]])
				normal += other_triangle->normal;
			normal.normalize();
		}
//		triangle.set_normals(triangle.normal, triangle.normal, triangle.normal);
		triangle.set_normals(vertex_normals[0], vertex_normals[1], vertex_normals[2]);
	}
}

