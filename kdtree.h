// Simple k-d tree for ray casting.

#ifndef _KDTREE_H
#define _KDTREE_H

#include <vector>
#include <map>
//#include <list>
#include "utils.h"

class kdTreeNode {
	int depth;
	int split_axis;
	Real split_height;
//	AABB aabb;
	kdTreeNode* low_side;
	kdTreeNode* high_side;
	bool leaf_node;
	int stored_triangles;
	Triangle* triangles;
	int managed_triangles;

	void form_as_leaf_from(vector<int>* indices, vector<Triangle>* all_triangles);
	bool split_triangle(int split_axis, Real split_height, int triangle_index, Triangle tri, vector<int>* low_side_sorted, vector<int>* high_side_sorted, std::map<int, std::pair<std::vector<int>, std::vector<int>>>& split_table, std::vector<Triangle>* all_triangles);

public:
	kdTreeNode(int depth, vector<int>* sorted_indices[3], vector<Triangle>* all_triangles);
	~kdTreeNode();
	void get_stats(int& deepest_depth, int& biggest_set);
};

class kdTree {
public: // XXX: For temporary debugging.
	kdTreeNode* root;
	std::vector<Triangle>* all_triangles;

public:
	kdTree(std::vector<Triangle>* all_triangles);
	~kdTree();
};

#endif

