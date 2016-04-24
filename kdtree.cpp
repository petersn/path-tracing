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
	// This allocation is freed at deletion time.
	leaf_node = true;
	stored_triangles = triangle_count;
	triangles = new Triangle[triangle_count]();
	// Copy the triangles over.
	int i = 0;
//	vector<int> lss, hss;
//	map<int, pair<vector<int>, vector<int>>> split_table;
	for (auto iter = indices->begin(); iter != indices->end(); iter++, i++) {
//		Triangle& tri = (*all_triangles)[*iter];
//		assert(*iter < all_triangles->size());
//		split_triangle(2, 0.0, *iter, tri, &lss, &hss, split_table, all_triangles);
//		cout << "Doing a split." << endl;
		triangles[i] = (*all_triangles)[*iter];
	}
	// We're done building!
	low_side = high_side = nullptr;
}

bool kdTreeNode::split_triangle(int split_axis, Real split_height, int triangle_index, Triangle tri, vector<int>* low_side_sorted, vector<int>* high_side_sorted, map<int, pair<vector<int>, vector<int>>>& split_table, vector<Triangle>* all_triangles) {
	assert(false); // XXX: I should never subdivide in this attempt.
	// Check if the triangle is in the split table.
	if (split_table.count(triangle_index) != 0) {
		// If so, just write out the cached values.
		pair<vector<int>, vector<int>>& p = split_table[triangle_index];
		vector<int>& add_to_low_side = p.first;
		vector<int>& add_to_high_side = p.second;
		// Add the already-split triangle indices to the low and high sides respectively.
		for_each(add_to_low_side.begin(), add_to_low_side.end(), [low_side_sorted](int& index){ low_side_sorted->push_back(index); });
		for_each(add_to_high_side.begin(), add_to_high_side.end(), [high_side_sorted](int& index){ high_side_sorted->push_back(index); });
		// Return false to indicate that we added no extra geometry to al_triangles.
		return false;
	}
	// Now we must figure out how to split this triangle.
	bool on_low_side[3];
	for (int i = 0; i < 3; i++)
		on_low_side[i] = tri.points[i](split_axis) <= split_height;
	// It should be the case that exactly one or two of the points are on the low side.
	// If not, then clearly the splitting plane doesn't actually intersect this triangle.
	int count_on_low_side = on_low_side[0] + on_low_side[1] + on_low_side[2];
	// XXX: Temporarily, for debugging, skip guys that aren't hit.
	if (not (count_on_low_side == 1 or count_on_low_side == 2))
		return false;
	assert(count_on_low_side == 1 or count_on_low_side == 2); // split_triangle called on non-intersected triangle!
	// Find the point that is all by itself on its side of the splitting plane,
	// and the two "popular points" that are together on the other side.
	Vec lonely;
	Vec popular[2];
	int next_popular = 0;
	bool lonely_on_low_side = count_on_low_side == 1;
	// TODO: I really need to make it the case that lonely, popular[0], popular[1] maintains the order of points[0], points[1], points[2] to preserve normals.
	for (int i = 0; i < 3; i++) {
		if (on_low_side[i] == lonely_on_low_side)
			lonely = tri.points[i];
		else
			popular[next_popular++] = tri.points[i];
	}
	assert(next_popular == 2); // Some dumb bug in the above loop logic.
	// Find the intersection of the line segments lonely <-> popular[0] and lonely <-> popular[1] with the splitting plane.
	Vec hits[2];
	for (int i = 0; i < 2; i++) {
		Vec direction = popular[i] - lonely;
		Real approach_rate = direction(split_axis);
		Real height_to_hit = split_height - lonely(split_axis);
		hits[i] = lonely + direction * (height_to_hit / approach_rate);
		// Do a quick sanity check of this hit.
		Real hit_error = hits[i](split_axis) - split_height;
		assert(fabsf(hit_error) < 1e-5); // Split sanity check failure. Maybe the geometry is very close to degenerate, or huge?
	}
	//cout << "Did a split!" << endl;
	// Grab the current length of all_triangles, because that's the next index we'll get upon inserting.
	unsigned int next_index = all_triangles->size();
	// Add new triangles.
	// TODO: XXX: Think carefully about the normals and triangle ordering in this case!
	// I think I got it right for these three triangles, assuming the above code gives lonely, popular[0], popular[1] in the right order.
	// Build the one triangle on the lonely side of the split.
	Triangle t1, t2, t3;
	all_triangles->push_back(t1 = Triangle(lonely, hits[0], hits[1]));
	// Build the two triangles on the popular side of the split.
	all_triangles->push_back(t2 = Triangle(hits[0], popular[0], hits[1]));
	all_triangles->push_back(t3 = Triangle(hits[1], popular[0], popular[1]));
	// Insert appropriate indices into the high and low side lists, and make a split_table entry noting that this was done.
	pair<vector<int>, vector<int>>& table_entry = split_table[triangle_index];
	if (lonely_on_low_side) {
		// I'm going to check that 
		//assert(t1.
		// XXX: TODO: Why isn't this causing sorting issues?
		// I'm not being particularly careful that I'm inserting these triangles in an order that maintains the invariant
		// that high_side_sorted is actually sorted by AABB maxima along split_axis...
		// I probably need to add a check to see if I should insert next_index+1 then next_index+2 or the other way around.
		// What's most bizarre to me is that my lack of carefulness isn't causing an assertion failure over at the first few
		// lines of kdTreeNode::kdTreeNode(), when I check this sortedness invariant.
		low_side_sorted->push_back(next_index);
		high_side_sorted->push_back(next_index+1);
		high_side_sorted->push_back(next_index+2);
		// The convention is that table_entry.first is the folks pushed into low_side_sorted, and table_entry.second is into high_side_sorted.
		table_entry.first.push_back(next_index);
		table_entry.second.push_back(next_index+1);
		table_entry.second.push_back(next_index+2);
	} else {
		// Everything here is the same as above, only the lonely node is on the high side.
		high_side_sorted->push_back(next_index);
		low_side_sorted->push_back(next_index+1);
		low_side_sorted->push_back(next_index+2);
		table_entry.second.push_back(next_index);
		table_entry.first.push_back(next_index+1);
		table_entry.first.push_back(next_index+2);
	}
	// We must access this triangle via all_triangles, rather than a pointer or a reference because vectors can reallocate on us.
	(*all_triangles)[triangle_index].from_split = true;
	// Indicate that we did make new geometry.
	return true;
}

kdTreeNode::kdTreeNode(int depth, vector<int>* sorted_indices[3], vector<Triangle>* all_triangles) : depth(depth) {
	reason_to_form = nullptr;
	// Make sure the three sorted indices lists are the same length.
	unsigned int triangle_count = sorted_indices[0]->size();
	managed_triangles = triangle_count;
	assert(triangle_count == sorted_indices[1]->size() and triangle_count == sorted_indices[2]->size());
	// Debug: Confirm that our input lists are sorted.
	for (int axis = 0; axis < 3; axis++) {
		// Be careful with signs here: If triangle_count (an unsigned quantity) is 0 then subtracting one yields a huge loop. Thus the cast.
		for (int i = 0; i < ((int)triangle_count) - 1; i++) {
			int index1 = (*sorted_indices[axis])[i];
			int index2 = (*sorted_indices[axis])[i+1];
			Triangle& tri1 = (*all_triangles)[index1];
			Triangle& tri2 = (*all_triangles)[index2];
			//assert(tri1.aabb.maxima(axis) <= tri2.aabb.maxima(axis)); // XXX: Until I fix sorting in triangle splitting, I must ignore this.
		}
	}
	// We should never get zero triangles to a node!
	//assert(triangle_count > 0);
	if (triangle_count == 0)
		cout << "Warning: Forming leaf node with no triangles.\n";
	// If the there are fewer than three triangles left then store the one or two.
	if (triangle_count < 3 or depth >= 21) {
		reason_to_form = "Just a few triangles.";
		if (depth >= 21)
			reason_to_form = "Too deep.";
		form_as_leaf_from(sorted_indices[0], all_triangles);
		return;
	}
	// Otherwise we perform a split, and have no triangles.
	leaf_node = false;
	stored_triangles = 0;
	triangles = nullptr;
	// Figure out the split axis that is most discriminating.
	Real best_score_so_far = FLOAT_INF;
	int best_axis_so_far = -1;
	for (int potential_split_axis = 0; potential_split_axis < 3; potential_split_axis++) {
		int potential_pivot = (*sorted_indices[potential_split_axis])[triangle_count/2];
		Real _bottom_of_box = (*all_triangles)[(*sorted_indices[potential_split_axis])[0]].aabb.minima(potential_split_axis);
		Real _top_of_box    = (*all_triangles)[(*sorted_indices[potential_split_axis])[triangle_count - 1]].aabb.maxima(potential_split_axis);
//		cout << "Depth: " << depth << " gap: " << gap << endl;
		Real potential_split_height = (_top_of_box + _bottom_of_box) / 2.0;

		// Use the very top of the triangle on this axis as the split line so that it ends up on the low side.
//		Real potential_split_height = (*all_triangles)[potential_pivot].aabb.maxima(potential_split_axis);
		Real axis_score = 0.0;
		int count_above = 0, count_below = 0, count_split = 0, count_doubled = 0;
		for (unsigned int i = 0; i < triangle_count; i++) {
			int triangle_index = (*sorted_indices[0])[i];
			Triangle& tri = (*all_triangles)[triangle_index];
			// Figure out of this triangle is crossing the boundary, above, or below.
//			bool entirely_below = tri.aabb.maxima(potential_split_axis) <= potential_split_height;
//			bool entirely_above = tri.aabb.minima(potential_split_axis) >= potential_split_height;
			bool overlaps_below = tri.aabb.minima(potential_split_axis) <= split_height;
			bool overlaps_above = tri.aabb.maxima(potential_split_axis) > split_height;
			if (overlaps_above)
				count_above++;
			if (overlaps_below)
				count_below++;
			if (overlaps_above and overlaps_below)
				count_doubled++;
//			if ((not entirely_above) and (not entirely_below))
//				count_split++;
		}
		// Here's our primary heuristic -- difference between above and below.
		axis_score = count_above > count_below ? count_above - count_below : count_below - count_above;
		axis_score += count_doubled * 3.0;
		//axis_score += count_split * 3.0;
		if (axis_score < best_score_so_far) {
			best_score_so_far = axis_score;
			best_axis_so_far = potential_split_axis;
		}
	}
	split_axis = depth % 3;
	// Find the best split height.
	Real best_sh_so_far;
	int best_sh_axis;
	Real best_sh_score = FLOAT_INF;
//	if (triangle_count == 1130)
//		cerr << "Doing eval at depth " << depth << endl;
	for (int potential_split_axis = 0; potential_split_axis < 3; potential_split_axis++) {
		for (int tri_test = 0; tri_test < triangle_count; tri_test++) {
			Real sample_sh = (*all_triangles)[(*sorted_indices[potential_split_axis])[tri_test]].aabb.maxima(potential_split_axis);
			int count_above = 0, count_below = 0, count_doubled = 0;
			for (unsigned int i = 0; i < triangle_count; i++) {
				int triangle_index = (*sorted_indices[0])[i];
				Triangle& tri = (*all_triangles)[triangle_index];
				// Figure out of this triangle is crossing the boundary, above, or below.
				bool overlaps_below = tri.aabb.minima(potential_split_axis) <= sample_sh;
				bool overlaps_above = tri.aabb.maxima(potential_split_axis) > sample_sh;
				if (overlaps_above)
					count_above++;
				if (overlaps_below)
					count_below++;
				if (overlaps_above and overlaps_below)
					count_doubled++;
			}
			Real score = real_max(count_above, count_below);
//			if (triangle_count == 1130)
//				cerr << "Potential score: " << score << endl;
			if (score < best_sh_score) {
				best_sh_so_far = sample_sh;
				best_sh_axis = potential_split_axis;
				best_sh_score = score;
			}
		}
	}
	split_axis = best_sh_axis;
//	split_axis = best_axis_so_far;
	assert(split_axis == 0 or split_axis == 1 or split_axis == 2);
//	cout << "At depth of: " << depth << " using axis " << split_axis << " with " << triangle_count << " triangles." << endl;
	/*
	if (depth > 100) {
		for (unsigned int i = 0; i < triangle_count; i++) {
			Triangle& tri = (*all_triangles)[i];
			cout << "Projection: " << tri.aabb.minima(split_axis) << " -- " << tri.aabb.maxima(split_axis) << endl;
		}	
		exit(1);
	}
	*/
	// Find a reasonable pivot for this axis, by taking the median.
//	int pivot_spot = triangle_count % 2 == 0 ? (triangle_count - 1) / 2 : triangle_count / 2;
//	int pivot_spot = triangle_count / 2;
//	int pivot_spot = 0;
//	int pivot = (*sorted_indices[split_axis])[pivot_spot];
	// Use the very top of the triangle on this axis as the split line so that it ends up on the low side.
//	split_height = (*all_triangles)[pivot].aabb.maxima(split_axis);
	Real bottom_of_box = (*all_triangles)[(*sorted_indices[split_axis])[0]].aabb.minima(split_axis);
	Real top_of_box    = (*all_triangles)[(*sorted_indices[split_axis])[triangle_count - 1]].aabb.maxima(split_axis);
	Real gap = top_of_box - bottom_of_box;
//	cout << "Depth: " << depth << " gap: " << gap << endl;
	//split_height = (top_of_box + bottom_of_box) / 2.0;
	split_height = best_sh_so_far;
//	cout << "Values: " << bottom_of_box << " " << top_of_box << endl;
	// Produce three new sorted lists for each side of the divider.
	vector<int>* low_side_sorted[3];
	vector<int>* high_side_sorted[3];
	// Produce the sorted index sublists for the high and low sides.
	// While doing this we will split triangles.
	// All the triangles will get split on the first pass when axis = 0,
	// but we don't want to resplit any triangles, so we save a map mapping a triangle index to a vector containing the indices of the new triangles we split it into.
	// Note: The new geometry is added to the end of all_triangles!
	map<int, pair<vector<int>, vector<int>>> split_table;
//	auto split_table = new map<int, pair<vector<int>, vector<int>>>();
	for (int axis = 0; axis < 3; axis++) {
		low_side_sorted[axis] = new vector<int>();
		high_side_sorted[axis] = new vector<int>();
		// Filter through the various indices.
		for (unsigned int i = 0; i < triangle_count; i++) {
			// Pick out the ith triangle index from the given sorted index list.
			int triangle_index = (*sorted_indices[axis])[i];
			Triangle& tri = (*all_triangles)[triangle_index];
			// Figure out if this triangle is crossing the boundary, above, or below.
//			bool entirely_below = tri.aabb.maxima(split_axis) <= split_height;
//			bool entirely_above = tri.aabb.minima(split_axis) >= split_height;
			bool overlaps_below = tri.aabb.minima(split_axis) <= split_height;
			bool overlaps_above = tri.aabb.maxima(split_axis) > split_height;
			// The triangle damn well better be on one side of the split or the other or both, but not entirely on both sides.
/*
			if (entirely_above and entirely_below) {
				cout << "Split axis and height: " << split_axis << " " << split_height << "\n";
				cout << "Coordinates:\n";
				cout << tri.points[0] << "\n\n" << tri.points[1] << "\n\n" << tri.points[2] << endl;
				cout << "AABB:\n";
				cout << tri.aabb.minima << "\n\n" << tri.aabb.maxima << endl;
				AABB aabb;
				aabb.set_to_point(tri.points[0]);
				aabb.update(tri.points[1]);
				aabb.update(tri.points[2]);
				cout << "Fresh AABB:\n";
				cout << aabb.minima << "\n\n" << aabb.maxima << endl;
			}
*/
//			assert(not (entirely_below and entirely_above));
			// If we're entirely above and entirely below, just drop the geometry.
//			// XXX: Temporary debugging lines, the next two are.
//			if (entirely_below and entirely_above)
//				continue;
			// Insert the triangle into the appropriate high and low sides.
/*
			if (entirely_below)
				low_side_sorted[axis]->push_back(triangle_index);
			if (entirely_above)
				high_side_sorted[axis]->push_back(triangle_index);
*/
			if (overlaps_below)
				low_side_sorted[axis]->push_back(triangle_index);
			if (overlaps_above)
				high_side_sorted[axis]->push_back(triangle_index);
			// If the triangle crosses the boundary then it needs to be split into three.
/*
			if ((not entirely_above) and (not entirely_below)) {
				// XXX: TODO: Do the right thing. For now, pass.
//				cout << "Splitting.\n";
				bool made_more_geometry = split_triangle(split_axis, split_height, triangle_index, tri, low_side_sorted[axis], high_side_sorted[axis], split_table, all_triangles);
				// All splits should be finished making more geometry by the first pass, so assert as such.
				assert(axis == 0 or not made_more_geometry);
			}
*/
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
	//assert(low_side_sorted[0]->size() > 0); // XXX: I'm temporarily commenting this out because skipping triangles can yield empty nodes!
	if (high_side_sorted[0]->size() == triangle_count or low_side_sorted[0]->size() == triangle_count) {
		reason_to_form = "No improvement possible";
		form_as_leaf_from(sorted_indices[0], all_triangles);
	} else if (high_side_sorted[0]->size() == 0 or (low_side_sorted[0]->size() == triangle_count and low_side_sorted[0]->size() == high_side_sorted[0]->size())) {
		reason_to_form = "Both sides have every triangle.";
		if (high_side_sorted[0]->size() == 0)
			reason_to_form = "High side empty.";
		//cout << "Forming exceptional leaf with " << low_side_sorted[0]->size() << " triangles.\n";
		//cout << "Split height: " << split_height << endl;
		for (unsigned int i = 0; i < triangle_count; i++) {
			//Triangle& tri = (*all_triangles)[i];
			//cout << "Projection: " << tri.aabb.minima(split_axis) << " -- " << tri.aabb.maxima(split_axis) << endl;
		}
		form_as_leaf_from(low_side_sorted[0], all_triangles);
	} else if (low_side_sorted[0]->size() == 0) {
		//cout << "Forming exceptional leaf with " << low_side_sorted[0]->size() << " triangles.\n";
		//cout << "Split height: " << split_height << endl;
		for (unsigned int i = 0; i < triangle_count; i++) {
			//Triangle& tri = (*all_triangles)[i];
			//cout << "Projection: " << tri.aabb.minima(split_axis) << " -- " << tri.aabb.maxima(split_axis) << endl;
		}
		reason_to_form = "Low side empty.";
		form_as_leaf_from(high_side_sorted[0], all_triangles);
	} else {
		//cout << "Node at depth " << depth << " with children counts:";
		//cout << "  " << low_side_sorted[0]->size();
		//cout << "  " << high_side_sorted[0]->size() << endl;
		// Armed with these lists we make our children.
		low_side = new kdTreeNode(depth+1, low_side_sorted, all_triangles);
		high_side = new kdTreeNode(depth+1, high_side_sorted, all_triangles);
	}
	// Finally, free our scratch lists.
	for (int axis = 0; axis < 3; axis++) {
		delete low_side_sorted[axis];
		delete high_side_sorted[axis];
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
// Check whether the xth triangle's first coordinate has a lower coordinate on the nth axis than the triangle with index y, where n is equal to global_comparison_axis.
bool compare_on_nth_axis(const int& x, const int& y) {
	vector<Triangle>& all_triangles = *global_triangles;
	bool answer1 = all_triangles[x].aabb.maxima(global_comparison_axis) < all_triangles[y].aabb.maxima(global_comparison_axis);
//	bool answer2 = all_triangles[x].aabb.minima(global_comparison_axis) < all_triangles[y].aabb.minima(global_comparison_axis);
	return answer1;
//	assert(answer1 == answer2);
//	return all_triangles[x].aabb.maxima(global_comparison_axis) < all_triangles[y].aabb.maxima(global_comparison_axis);
}

void kdTreeNode::get_stats(int& deepest_depth, int& biggest_set) {
	char* _rtf = reason_to_form;
	if (_rtf == nullptr)
		_rtf = "";
	cout << "(" << depth << ", " << managed_triangles << ", " << stored_triangles << ", \"" << _rtf << "\", [";
	if (depth > deepest_depth)
		deepest_depth = depth;
	if (stored_triangles > biggest_set)
		biggest_set = stored_triangles;
//	if (stored_triangles != 0)
//		cout << stored_triangles;
	if (low_side != nullptr)
		low_side->get_stats(deepest_depth, biggest_set);
	else cout << "None";
	cout << ", ";
	if (high_side != nullptr)
		high_side->get_stats(deepest_depth, biggest_set);
	else cout << "None";
	cout << "])";
}

kdTree::kdTree(vector<Triangle>* _all_triangles) {
//	srand(unsigned(time(NULL)));
//	auto engine = default_random_engine{};
	// First we build three lists, sorting the indices of the triangles by their first coordinate along each of the three axes.
	all_triangles = _all_triangles;
	vector<int>* sorted_indices[3];
	for (int axis = 0; axis < 3; axis++) {
		// We allocate this list 
		sorted_indices[axis] = new vector<int>();
		for (unsigned int i = 0; i < all_triangles->size(); i++)
			sorted_indices[axis]->push_back(i);
		// In leu of closures, simply assign into globals.
		// XXX: Not re-entrant. :(
		global_triangles = all_triangles;
		global_comparison_axis = axis;
		sort(sorted_indices[axis]->begin(), sorted_indices[axis]->end(), compare_on_nth_axis);
//		random_shuffle(sorted_indices[axis]->begin(), sorted_indices[axis]->end());
	}
	// Actually build the tree!
	root = new kdTreeNode(0, sorted_indices, all_triangles);
	// Free our scratches.
	for (int axis = 0; axis < 3; axis++)
		delete sorted_indices[axis];
}

kdTree::~kdTree() {
	delete root;
}

