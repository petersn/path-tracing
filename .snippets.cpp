
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

