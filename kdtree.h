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
	kdTreeNode* low_side;
	kdTreeNode* high_side;
	bool leaf_node;
	int stored_triangles;
	Triangle* triangles;
	int managed_triangles;

	void form_as_leaf_from(vector<int>* indices, vector<Triangle>* all_triangles);

public:
	kdTreeNode(int depth, vector<int>* sorted_indices_by_min[3], vector<int>* sorted_indices_by_max[3], vector<Triangle>* all_triangles);
	~kdTreeNode();
	void get_stats(int& deepest_depth, int& biggest_set);
};

class kdTree {
	std::vector<Triangle>* all_triangles;

public:
	kdTreeNode* root;

	kdTree(std::vector<Triangle>* all_triangles);
	~kdTree();
};

#endif

