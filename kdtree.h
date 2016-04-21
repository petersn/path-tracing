// Simple k-d tree for ray casting.

#ifndef _KDTREE_H
#define _KDTREE_H

#include <vector>
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

	void form_as_leaf_from(vector<int>* indices, vector<Triangle>* all_triangles);

public:
	kdTreeNode(int depth, vector<int>* sorted_indices[3], vector<Triangle>* all_triangles);
};

class kdTree {
	kdTreeNode* root;

public:
	kdTree(std::vector<Triangle>& triangles);
};

#endif

