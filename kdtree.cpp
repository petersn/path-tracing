// Simple k-d tree for ray casting.

using namespace std;
//#include <math.h>
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <map>
#include "kdtree.h"

void kdTreeNode::form_as_leaf_from(vector<int>* indices, vector<Triangle>* all_triangles) {
	unsigned int triangle_count = indices->size();
	leaf_node = true;
	stored_triangles = triangle_count;
	// This allocation is freed at deletion time.
	triangles = new Triangle[triangle_count]();
	// Copy the triangles over.
	int i = 0;
	for (auto iter = indices->begin(); iter != indices->end(); iter++, i++) {
		triangles[i] = (*all_triangles)[*iter];
	}
	// We're done building!
	low_side = high_side = nullptr;
}

kdTreeNode::kdTreeNode(int depth, vector<int>* sorted_indices_by_min[3], vector<int>* sorted_indices_by_max[3], vector<Triangle>* all_triangles) : depth(depth) {
	// Pull out an arbitrary ordering of all our indices for convenience use later.
	vector<int>& all_our_indices = *sorted_indices_by_min[0];
	// Make a quick way of accessing both _by_min and _by_max via an index to keep the following code DRYer.
	vector<int>** sorted_indices_by[2] = {sorted_indices_by_min, sorted_indices_by_max};
	// Make sure the three sorted indices lists are the same length.
	unsigned int triangle_count = sorted_indices_by_min[0]->size();
	// Store the total number of triangles from here on down in the tree.
	managed_triangles = triangle_count;
	cout << endl;
	assert(triangle_count == sorted_indices_by_min[1]->size() and triangle_count == sorted_indices_by_min[2]->size());
	assert(triangle_count == sorted_indices_by_max[0]->size() and triangle_count == sorted_indices_by_max[1]->size() and triangle_count == sorted_indices_by_max[2]->size());
	// Debug: Confirm that our input lists are sorted.
	for (int axis = 0; axis < 3; axis++) {
		for (int minmax = 0; minmax < 2; minmax++) {
			// Be careful with signs here: If triangle_count (an unsigned quantity) is 0 then subtracting one yields a huge loop. Thus the cast.
			for (int i = 0; i < ((int)triangle_count) - 1; i++) {
				int index1 = (*sorted_indices_by[minmax][axis])[i];
				int index2 = (*sorted_indices_by[minmax][axis])[i+1];
				Triangle& tri1 = (*all_triangles)[index1];
				Triangle& tri2 = (*all_triangles)[index2];
				if (minmax == 0)
					assert(tri1.aabb.minima(axis) <= tri2.aabb.minima(axis));
				else
					assert(tri1.aabb.maxima(axis) <= tri2.aabb.maxima(axis));
			}
		}
	}
	// We should never get zero triangles to a node!
	assert(triangle_count > 0);
	if (triangle_count == 0)
		cerr << "Warning: Forming leaf node with no triangles.\n";
	// If the there are fewer than three triangles left then store the one or two.
	if (triangle_count < 3 or depth >= 20) {
		form_as_leaf_from(&all_our_indices, all_triangles);
		return;
	}
	// Otherwise we perform a split, and have no triangles.
	leaf_node = false;
	stored_triangles = 0;
	triangles = nullptr;
	// Find the best split height and axis.
	Real best_sh_so_far;
	int best_sh_axis;
	Real best_sh_score = FLOAT_INF;
	for (int potential_split_axis = 0; potential_split_axis < 3; potential_split_axis++) {
		for (unsigned int tri_test = 0; tri_test < triangle_count; tri_test++) {
			Real sample_sh = (*all_triangles)[all_our_indices[tri_test]].aabb.maxima(potential_split_axis);
			int count_above = 0, count_below = 0;
			for (unsigned int i = 0; i < triangle_count; i++) {
				int triangle_index = all_our_indices[i];
				Triangle& tri = (*all_triangles)[triangle_index];
				// Figure out of this triangle is crossing the boundary, above, or below.
				bool overlaps_below = tri.aabb.minima(potential_split_axis) <= sample_sh;
				bool overlaps_above = tri.aabb.maxima(potential_split_axis) > sample_sh;
				if (overlaps_above)
					count_above++;
				if (overlaps_below)
					count_below++;
			}
			Real score = real_max(count_above, count_below);
			if (score < best_sh_score) {
				best_sh_so_far = sample_sh;
				best_sh_axis = potential_split_axis;
				best_sh_score = score;
			}
		}
	}
	split_axis = best_sh_axis;
	assert(split_axis == 0 or split_axis == 1 or split_axis == 2);
	split_height = best_sh_so_far;
	// Produce three new sorted lists for each side of the divider.
	vector<int>* low_side_sorted_by_min[3];
	vector<int>* low_side_sorted_by_max[3];
	vector<int>** low_side_sorted_by[2] = {low_side_sorted_by_min, low_side_sorted_by_max};
	vector<int>* high_side_sorted_by_min[3];
	vector<int>* high_side_sorted_by_max[3];
	vector<int>** high_side_sorted_by[2] = {high_side_sorted_by_min, high_side_sorted_by_max};
	// Produce the sorted index sublists for the high and low sides.
	for (int axis = 0; axis < 3; axis++) {
		for (int minmax = 0; minmax < 2; minmax++) {
			low_side_sorted_by[minmax][axis] = new vector<int>();
			high_side_sorted_by[minmax][axis] = new vector<int>();
			// Filter through the various indices.
			for (unsigned int i = 0; i < triangle_count; i++) {
				// Pick out the ith triangle index from the given sorted index list.
				int triangle_index = (*sorted_indices_by[minmax][axis])[i];
				Triangle& tri = (*all_triangles)[triangle_index];
				// Figure out if this triangle is crossing the boundary, above, or below.
				bool overlaps_below = tri.aabb.minima(split_axis) <= split_height;
				bool overlaps_above = tri.aabb.maxima(split_axis) > split_height;
				if (overlaps_below)
					low_side_sorted_by[minmax][axis]->push_back(triangle_index);
				if (overlaps_above)
					high_side_sorted_by[minmax][axis]->push_back(triangle_index);
			}
		}
	}
	// Make sure the different axis sortings have the same number of nodes.
	for (int minmax = 0; minmax < 2; minmax++) {
		for (int i = 1; i < 3; i++) {
			assert(low_side_sorted_by[minmax][0]->size() == low_side_sorted_by[minmax][i]->size());
			assert(high_side_sorted_by[minmax][0]->size() == high_side_sorted_by[minmax][i]->size());
		}
	}
	// Now a special check happens!
	// It's possible to have degenerate situations where we can't actually split successfully along this axis.
	// If this happens then we must form a node that stores a strange number of triangles (possibly unusually many).
	// This should ONLY happen with an empty high side and full low side.
	//assert(low_side_sorted[0]->size() > 0); // XXX: I'm temporarily commenting this out because skipping triangles can yield empty nodes!
	unsigned int low_size = low_side_sorted_by_min[0]->size();
	unsigned int high_size = high_side_sorted_by_min[0]->size();
	if (high_size == triangle_count or low_size == triangle_count) {
		form_as_leaf_from(&all_our_indices, all_triangles);
	} else {
		// Armed with these lists we make our children.
		low_side = new kdTreeNode(depth+1, low_side_sorted_by_min, low_side_sorted_by_max, all_triangles);
		high_side = new kdTreeNode(depth+1, high_side_sorted_by_min, high_side_sorted_by_max, all_triangles);
	}
	// Finally, free our scratch lists.
	for (int axis = 0; axis < 3; axis++) {
		for (int minmax = 0; minmax < 2; minmax++) {
			delete low_side_sorted_by[minmax][axis];
			delete high_side_sorted_by[minmax][axis];
		}
	}
}

kdTreeNode::~kdTreeNode() {
	// This is safe because triangles is either nullptr or allocated.
	delete[] triangles;
	delete low_side;
	delete high_side;
}

// Try not to puke at this global storage used for configuring the comparison function in leu of closures.
vector<Triangle>* global_triangles;
int global_comparison_axis;
bool global_use_maxima;
// Check whether the xth triangle's first coordinate has a lower coordinate on the nth axis than the triangle with index y, where n is equal to global_comparison_axis.
bool compare_on_nth_axis(const int& x, const int& y) {
	vector<Triangle>& all_triangles = *global_triangles;
	bool answer1 = all_triangles[x].aabb.maxima(global_comparison_axis) < all_triangles[y].aabb.maxima(global_comparison_axis);
	bool answer2 = all_triangles[x].aabb.minima(global_comparison_axis) < all_triangles[y].aabb.minima(global_comparison_axis);
	if (global_use_maxima)
		return answer1;
	return answer2;
//	return answer1;
//	assert(answer1 == answer2);
//	return all_triangles[x].aabb.maxima(global_comparison_axis) < all_triangles[y].aabb.maxima(global_comparison_axis);
}

void kdTreeNode::get_stats(int& deepest_depth, int& biggest_set) {
	if (depth > deepest_depth)
		deepest_depth = depth;
	if (stored_triangles > biggest_set)
		biggest_set = stored_triangles;
	if (low_side != nullptr)
		low_side->get_stats(deepest_depth, biggest_set);
	if (high_side != nullptr)
		high_side->get_stats(deepest_depth, biggest_set);
}

kdTree::kdTree(vector<Triangle>* _all_triangles) {
	// First we build three lists, sorting the indices of the triangles by their min and max bounds along each of the three axes.
	all_triangles = _all_triangles;
	vector<int>* sorted_indices_by_min[3];
	vector<int>* sorted_indices_by_max[3];
	for (int axis = 0; axis < 3; axis++) {
		sorted_indices_by_min[axis] = new vector<int>();
		sorted_indices_by_max[axis] = new vector<int>();
		for (unsigned int i = 0; i < all_triangles->size(); i++) {
			sorted_indices_by_min[axis]->push_back(i);
			sorted_indices_by_max[axis]->push_back(i);
		}
		// In leu of closures, simply assign into globals.
		// XXX: Not re-entrant. :(
		global_triangles = all_triangles;
		global_comparison_axis = axis;
		global_use_maxima = false;
		sort(sorted_indices_by_min[axis]->begin(), sorted_indices_by_min[axis]->end(), compare_on_nth_axis);
		global_use_maxima = true;
		sort(sorted_indices_by_max[axis]->begin(), sorted_indices_by_max[axis]->end(), compare_on_nth_axis);
	}
	// Actually build the tree!
	root = new kdTreeNode(0, sorted_indices_by_min, sorted_indices_by_max, all_triangles);
	// Free our scratches.
	for (int axis = 0; axis < 3; axis++) {
		delete sorted_indices_by_min[axis];
		delete sorted_indices_by_max[axis];
	}
}

kdTree::~kdTree() {
	delete root;
}

