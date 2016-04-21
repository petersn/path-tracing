// Simple k-d tree for ray casting.

using namespace std;
#include <assert.h>
#include <algorithm>
#include <iostream>
#include "kdtree.h"

void kdTreeNode::form_as_leaf_from(vector<int>* indices, vector<Triangle>* all_triangles) {
	unsigned int triangle_count = indices->size();
	// This allocation is freed at deletion time.
	leaf_node = true;
	stored_triangles = triangle_count;
	triangles = new Triangle[triangle_count]();
	// Copy the triangles over.
	int i = 0;
	for (auto iter = indices->begin(); iter != indices->end(); iter++, i++)
		triangles[i] = (*all_triangles)[*iter];
	// We're done building!
	low_side = high_side = nullptr;
}

kdTreeNode::kdTreeNode(int depth, vector<int>* sorted_indices[3], vector<Triangle>* all_triangles) : depth(depth) {
	// Make sure the three sorted indices lists are the same length.
	unsigned int triangle_count = sorted_indices[0]->size();
	assert(triangle_count == sorted_indices[1]->size() and triangle_count == sorted_indices[2]->size());
	// We should never get zero triangles to a node!
	//assert(triangle_count > 0);
	if (triangle_count == 0)
		cout << "Warning: Forming leaf node with no triangles.\n";
	// If the there are fewer than three triangles left then store the one or two.
	if (triangle_count < 3) {
		form_as_leaf_from(sorted_indices[0], all_triangles);
		return;
	}
	// Otherwise we perform a split, and have no triangles.
	leaf_node = false;
	stored_triangles = 0;
	triangles = nullptr;
	split_axis = depth % 3;
	// Find a reasonable pivot for this axis, by taking the median.
	int pivot = (*sorted_indices[split_axis])[triangle_count/2 - (triangle_count&1) + 1];
	// Use the very top of the triangle on this axis as the split line so that it ends up on the low side.
	split_height = (*all_triangles)[pivot].aabb.maxima(split_axis);
	// Produce three new sorted lists for each side of the divider.
	vector<int>* low_side_sorted[3];
	vector<int>* high_side_sorted[3];
	// Produce the sorted index sublists for the high and low sides.
	for (int axis = 0; axis < 3; axis++) {
		low_side_sorted[axis] = new vector<int>();
		high_side_sorted[axis] = new vector<int>();
		// Filter through the various indices.
		for (unsigned int i = 0; i < triangle_count; i++) {
			// Pick out the ith triangle index from the given sorted index list.
			int triangle_index = (*sorted_indices[axis])[i];
			Triangle& tri = (*all_triangles)[triangle_index];
			// Figure out of this triangle is crossing the boundary, above, or below.
			bool entirely_below = tri.aabb.maxima(split_axis) <= split_height;
			bool entirely_above = tri.aabb.minima(split_axis) > split_height;
			// The triangle damn well better be on one side of the split or the other.
			assert(entirely_below or entirely_above);
			// Insert the triangle into the appropriate high and low sides.
			if (entirely_below)
				low_side_sorted[axis]->push_back(triangle_index);
			if (entirely_above)
				high_side_sorted[axis]->push_back(triangle_index);
			// If the triangle crosses the boundary then it needs to be split into three.
			if ((not entirely_above) and (not entirely_below)) {
				// XXX: TODO: Do the right thing. For now, pass.
				cout << "Splitting.\n";
			}
		}
	}
	// Make sure the different axis sortings have the same number of nodes.
	assert(low_side_sorted[0]->size() == low_side_sorted[1]->size());
	assert(low_side_sorted[0]->size() == low_side_sorted[2]->size());
	assert(high_side_sorted[0]->size() == high_side_sorted[1]->size());
	assert(high_side_sorted[0]->size() == high_side_sorted[2]->size());
	// Now a special check happens!
	// It's possible to have degenerate situations where we can't actually split successfully along this axis.
	// If this happens then we must form a node that stores a strange number of triangles (possibly unusually many).
	// This should ONLY happen with an empty high side and full low side.
	assert(low_side_sorted[0]->size() > 0);
	if (high_side_sorted[0]->size() == 0) {
		cout << "Forming exceptional leaf with " << low_side_sorted[0]->size() << " triangles.\n";
		form_as_leaf_from(low_side_sorted[0], all_triangles);
	} else {
		cout << "Node at depth " << depth << " with children counts:";
		cout << "  " << low_side_sorted[0]->size();
		cout << "  " << high_side_sorted[0]->size() << endl;
		// Armed with these lists we make our children.
		low_side = new kdTreeNode(depth+1, low_side_sorted, all_triangles);
		high_side = new kdTreeNode(depth+1, high_side_sorted, all_triangles);
	}
	// Finally, free our scratch lists.
	for (int axis = 0; axis < 3; axis++) {
		delete low_side_sorted[axis];
		delete high_side_sorted[axis];
	}
};

// Try not to puke at this global storage used for configuring the comparison function in leu of closures.
vector<Triangle>* global_triangles;
int global_comparison_axis;
// Check whether the xth triangle's first coordinate has a lower coordinate on the nth axis than the triangle with index y, where n is equal to global_comparison_axis.
bool compare_on_nth_axis(const int& x, const int& y) {
	vector<Triangle>& all_triangles = *global_triangles;
	bool answer1 = all_triangles[x].aabb.maxima(global_comparison_axis) < all_triangles[y].aabb.maxima(global_comparison_axis);
	bool answer2 = all_triangles[x].aabb.minima(global_comparison_axis) < all_triangles[y].aabb.minima(global_comparison_axis);
	assert(answer1 == answer2);
	return all_triangles[x].aabb.maxima(global_comparison_axis) < all_triangles[y].aabb.maxima(global_comparison_axis);
}

kdTree::kdTree(vector<Triangle>& all_triangles) {
	// First we build three lists, sorting the indices of the triangles by their first coordinate along each of the three axes.
	vector<int>* sorted_indices[3];
	for (int axis = 0; axis < 3; axis++) {
		// We allocate this list 
		sorted_indices[axis] = new vector<int>();
		for (unsigned int i = 0; i < all_triangles.size(); i++)
			sorted_indices[axis]->push_back(i);
		// In leu of closures, simply assign into globals.
		// XXX: Not re-entrant. :(
		global_triangles = &all_triangles;
		global_comparison_axis = axis;
		sort(sorted_indices[axis]->begin(), sorted_indices[axis]->end(), compare_on_nth_axis);
	}
	// Actually build the tree!
	root = new kdTreeNode(0, sorted_indices, &all_triangles);
}

