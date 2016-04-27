// Simple k-d tree for ray casting.

#ifndef _KDTREE_H
#define _KDTREE_H

#include <vector>
#include <map>
#include "utils.h"

extern long long rays_cast;

class kdTreeNode {
public:
	// Depth is 0 for the root of the tree, and increments going down the tree.
	int depth;
	// The split axis is 0, 1, or 2, and sets which axis the splitting plane is normal to.
	int split_axis;
	Real split_height;
	kdTreeNode* low_side;
	kdTreeNode* high_side;
	AABB aabb;
	// Leaf nodes have is_leaf true, stored_triangle_count positive, and stored_triangles non-null.
	// stored_triangles points to an array of triangles stored in the leaf.
	bool is_leaf;
	int stored_triangle_count;
	Triangle* stored_triangles;
	// In contrast, total_triangles counts all the triangles in the tree from this node down.
	int total_triangles;

	void form_as_leaf_from(vector<int>* indices, vector<Triangle>* all_triangles);

public:
	kdTreeNode(int depth, vector<int>* sorted_indices_by_min[3], vector<int>* sorted_indices_by_max[3], vector<Triangle>* all_triangles);
	~kdTreeNode();
	void get_stats(int& deepest_depth, int& biggest_set);
	bool ray_test(const Ray& ray, Real& hit_parameter, Triangle** hit_triangle=nullptr);
};

class kdTree {
	std::vector<Triangle>* all_triangles;

public:
	kdTreeNode* root;

	kdTree(std::vector<Triangle>* all_triangles);
	~kdTree();
	bool ray_test(const Ray& ray, Real& hit_parameter, Triangle** hit_triangle=nullptr);
};

#endif

