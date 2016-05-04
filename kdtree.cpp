// Simple k-d tree for ray casting.

using namespace std;
//#include <math.h>
#include <sys/time.h>
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <map>
#include "kdtree.h"

#define LEAF_THRESHOLD 8
#define MAXIMUM_DEPTH 19 //8
// If a child would have this many or fewer triangles then we just build its node ourselves, rather than paying the overhead of dispatching to a thread.
#define THREADED_DISPATCH_THRESHOLD 16

void kdTreeNode::form_as_leaf_from(vector<int>* indices, vector<Triangle>* all_triangles) {
	unsigned int triangle_count = indices->size();
	is_leaf = true;
	stored_triangle_count = triangle_count;
	// This allocation is freed at deletion time.
	stored_triangles = new Triangle[triangle_count]();
	// Copy the triangles over.
	int i = 0;
	for (auto iter = indices->begin(); iter != indices->end(); iter++, i++) {
		stored_triangles[i] = (*all_triangles)[*iter];
	}
	// We're done building!
	low_side = high_side = nullptr;
}

kdTreeNode::kdTreeNode(kdTree* parent, int depth, vector<int>* sorted_indices_by_min[3], vector<int>* sorted_indices_by_max[3], vector<Triangle>* all_triangles) : depth(depth) {
	// Pull out an arbitrary ordering of all our indices for convenience use later.
	vector<int>& all_our_indices = *sorted_indices_by_min[0];
	// Make a quick way of accessing both _by_min and _by_max via an index to keep the following code DRYer.
	vector<int>** sorted_indices_by[2] = {sorted_indices_by_min, sorted_indices_by_max};
	// Store the total number of triangles that will be managed from here on down in the tree.
	unsigned int triangle_count = sorted_indices_by_min[0]->size();
	total_triangles = triangle_count;
	// Make sure the six sorted indices lists are the same length.
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
	// Update our AABB.
	for (int index : all_our_indices)
		aabb.update((*all_triangles)[index].aabb);
	// We should never get zero triangles to a node!
//	if (triangle_count == 0)
//		cout << "Zero at depth: " << depth << endl;
//	assert(triangle_count > 0);
	// If the there are fewer than three triangles left then store the one or two.
	if (triangle_count <= LEAF_THRESHOLD or depth >= MAXIMUM_DEPTH) {
		form_as_leaf_from(&all_our_indices, all_triangles);
		return;
	}
	// Otherwise we perform a split, and have no triangles.
	is_leaf = false;
	stored_triangle_count = 0;
	stored_triangles = nullptr;
	// Find the best split height and axis.
	// We initialize best_sh_so_far and best_sh_axis only to suppress compiler warnings -- these values should never be used.
	Real best_sh_so_far = 0.0;
	int best_sh_axis = -1;
	Real best_sh_score = FLOAT_INF;
//	#pragma omp parallel for if(depth==0)
	for (int potential_split_axis = 0; potential_split_axis < 3; potential_split_axis++) {
		for (unsigned int tri_test = 0; tri_test < triangle_count; tri_test++) {
			Real sample_sh = (*all_triangles)[all_our_indices[tri_test]].aabb.maxima(potential_split_axis);
			// Apply binary search to count in log(n) time how many triangles will be above and how many will be below.
			int count_on_side[2] = {0, 0};
			for (int minmax = 0; minmax < 2; minmax++) {
				unsigned int low = 0, high = triangle_count;
				while (low + 1 < high) {
					unsigned int mid = (low + high) / 2;
					int triangle_index = (*sorted_indices_by[minmax][potential_split_axis])[mid];
					Triangle& tri = (*all_triangles)[triangle_index];
					Real coord = minmax == 0 ? tri.aabb.minima(potential_split_axis) : tri.aabb.maxima(potential_split_axis);
					// If you check very carefully, the following line is actually equivalent to the obvious implementation, as given by this command:
					/* python -c 'print "eJxTUMAEmWkKGrmZebmJFQq2tgoGmgrVXFhUwZUm5+cXpSjY2CoUJ+YW5KTGF2do4lQPAjn55Qq2\
					   CrmZKdY4laXmFKfiNSMjMz0DnyG1YCOIcrgdse4mZCdRDifg+VouAAh/Svc=".decode("base64").decode("zlib")' */
					(coord <= sample_sh ? low : high) = mid;
				}
				if (minmax == 0)
					count_on_side[minmax] = low + 1;
				else
					count_on_side[minmax] = triangle_count - high;
			}
			// Current heuristic: Minimize the size of the largest subtree.
			Real score = real_max(count_on_side[0], count_on_side[1]);
			// New heuristic: Surface Area Heuristic (SAH)
//			Real low_sa = sample_sh - aabb.minima(potential_split_axis);
//			Real high_sa = aabb.maxima(potential_split_axis) - sample_sh;
//			assert(low_sa >= 0);
//			assert(high_sa >= 0);
//			Real score = low_sa * count_on_side[0] + high_sa * count_on_side[1];
//			score /= (low_sa + high_sa);
//			score += mini_score * 0.1;
//			score = mini_score;
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
				assert(tri.aabb.minima(split_axis) <= tri.aabb.maxima(split_axis));
				assert(overlaps_below or overlaps_above);
//				if (overlaps_below and overlaps_above)
//					continue;
				if (overlaps_below) {
					low_side_sorted_by[minmax][axis]->push_back(triangle_index);
					continue;
				}
				if (overlaps_above)
					high_side_sorted_by[minmax][axis]->push_back(triangle_index);
			}
		}
	}
	// Make sure the different axis sortings have the same number of nodes.
	for (int axis = 0; axis < 3; axis++) {
		for (int minmax = 0; minmax < 2; minmax++) {
			assert(low_side_sorted_by[minmax][0]->size() == low_side_sorted_by[minmax][axis]->size());
			assert(high_side_sorted_by[minmax][0]->size() == high_side_sorted_by[minmax][axis]->size());
		}
	}
	// Now we compute whether or not to become a leaf anyway (not just because we have too few triangles or hit our depth limit), or subdivide.
	// We choose to be a leaf if either our high or low side ends up being the same size as we are -- "non-improvement".
	// Becoming a leaf if this "non-improvement" occurs prevents a pointless recursion up to our depth limit building huge trees.
	// XXX: This ONLY works because the above code currently optimizes for the split axis that minimizes maximum subtree total triangle count!
	// Once I switch to SAH I'll have to change this heuristic, because then we will very frequently want to
	// subdivide with an entire empty subtree, and this heuristic here would prematurely terminate the subdivision.
	unsigned int low_size = low_side_sorted_by_min[0]->size();
	unsigned int high_size = high_side_sorted_by_min[0]->size();
	// Make sure we didn't drop any triangles.
	// This assert should be mutually redundant with the above assert of (overlaps_below or overlaps_above).
//	assert(low_size + high_size >= triangle_count);
	// If we failed to improve, become a leaf.
	if (high_size == triangle_count or low_size == triangle_count) {
//	if (high_size == triangle_count and low_size == triangle_count) {
		form_as_leaf_from(&all_our_indices, all_triangles);
	} else {
		// Otherwise, recursively subdivide.
#ifdef THREADED_KD_BUILD
		if (low_size > THREADED_DISPATCH_THRESHOLD) {
			JobDescriptor job;
			// Acquire the job variable for our thread.
			pthread_mutex_lock(&parent->job_lock);
			job.do_quit = false;
			job.destination = &low_side;
			job.depth = depth+1;
			for (int i = 0; i < 3; i++) {
				job.sorted_indices_by_min[i] = low_side_sorted_by_min[i];
				job.sorted_indices_by_max[i] = low_side_sorted_by_max[i];
			}
			parent->job_list.push_back(job);
			parent->total_job_count++;
			pthread_mutex_unlock(&parent->job_lock);
			// Release the job, so a processing thread can begin processing it.
			sem_post(&parent->job_available);
		} else
#endif
			low_side = new kdTreeNode(parent, depth+1, low_side_sorted_by_min, low_side_sorted_by_max, all_triangles);
#ifdef THREADED_KD_BUILD
		if (high_size > THREADED_DISPATCH_THRESHOLD) {
			JobDescriptor job;
			// Acquire the job variable for our thread.
			pthread_mutex_lock(&parent->job_lock);
			job.do_quit = false;
			job.destination = &high_side;
			job.depth = depth+1;
			for (int i = 0; i < 3; i++) {
				job.sorted_indices_by_min[i] = high_side_sorted_by_min[i];
				job.sorted_indices_by_max[i] = high_side_sorted_by_max[i];
			}
			parent->job_list.push_back(job);
			parent->total_job_count++;
			pthread_mutex_unlock(&parent->job_lock);
			// Release the job, so a processing thread can begin processing it.
			sem_post(&parent->job_available);
		} else
#endif
			high_side = new kdTreeNode(parent, depth+1, high_side_sorted_by_min, high_side_sorted_by_max, all_triangles);
	}
	// Freeing occurs in the worker thread when threaded building is being used, so in that case we don't free here.
#ifndef THREADED_KD_BUILD
	// Finally, free our scratch lists.
	for (int axis = 0; axis < 3; axis++) {
		for (int minmax = 0; minmax < 2; minmax++) {
			delete low_side_sorted_by[minmax][axis];
			delete high_side_sorted_by[minmax][axis];
		}
	}
#endif
}

kdTreeNode::~kdTreeNode() {
	// This is safe because stored_triangles is either nullptr or allocated.
	delete[] stored_triangles;
	delete low_side;
	delete high_side;
}

bool kdTreeNode::ray_test(const Ray& ray, Real& hit_parameter, Triangle** hit_triangle) {
	// Do a quick AABB based early out.
	if (not aabb.does_ray_intersect(ray))
		return false;
	// If we're a leaf we simply try intersecting against all of our triangles.
	if (is_leaf) {
		bool overall_result = false;
		Real best_hit_parameter = FLOAT_INF;
		Triangle* best_hit_triangle = nullptr;
		for (int i = 0; i < stored_triangle_count; i++) {
			Real temp_hit_parameter;
			Triangle* temp_hit_triangle;
			bool result = stored_triangles[i].ray_test(ray, temp_hit_parameter, &temp_hit_triangle);
			if (result and temp_hit_parameter < best_hit_parameter) {
				best_hit_parameter = temp_hit_parameter;
				best_hit_triangle = temp_hit_triangle;
				overall_result = true;
			}
		}
		if (overall_result) {
			hit_parameter = best_hit_parameter;
			if (hit_triangle != nullptr)
				*hit_triangle = best_hit_triangle;
		}
		return overall_result;
	}
	// If not, we perform early out search against our nodes.
	// Determine which side of the splitting plane the origin of the ray starts on.
	Real high_side_minimum = high_side != nullptr ? high_side->aabb.minima(split_axis) : 0.0;
	Real low_side_maximum  = low_side != nullptr ? low_side->aabb.minima(split_axis) : 0.0;
	Real origin = ray.origin(split_axis);
	// Check if we're in both AABBs
//	bool in_middle = origin < low_side_maximum and high_side_minimum < origin and low_side != nullptr and high_side != nullptr;
	bool overlaps_high_side = high_side != nullptr and high_side->aabb.minima(split_axis) < origin;
	bool overlaps_low_side  = low_side  != nullptr and origin <= low_side->aabb.maxima(split_axis);
	bool overlaps_both = overlaps_high_side and overlaps_low_side;
//	bool on_high_side = ray.origin(split_axis) > split_height;
//	Real temp_hit_parameter;
	kdTreeNode* near_side = overlaps_high_side ? high_side : low_side;
	kdTreeNode* far_side  = overlaps_high_side ? low_side : high_side;
//	// First we try intersecting against the near side.
//	if (near_side != nullptr) {
//		near_side->ray_test(ray, hit_parameter, hit_triangle);
//	}
//	// Next we 

	// If we overlap both children, then we must test against both children.
	if (overlaps_both) {
		bool low_result = low_side->ray_test(ray, hit_parameter, hit_triangle);
		Real temp_hit_parameter;
		Triangle* temp_triangle;
		bool high_result = high_side->ray_test(ray, temp_hit_parameter, &temp_triangle);
		// If there's no high side hit use the low side hit.
		if (not high_result)
			return low_result;
		// If there's no low side hit use the high side hit.
		if ((not low_result) or temp_hit_parameter < hit_parameter) {
			hit_parameter = temp_hit_parameter;
			if (hit_triangle != nullptr)
				*hit_triangle = temp_triangle;
		}
		return true;
	}
//	bool near_result = near_side->ray_test(ray, hit_parameter, hit_triangle);
//	if (near_result)
//if (overlaps_low_side) {
//		// Here we know we're on the low side and not on the high side.
//		bool low_result = low_side->ray_test(ray, hit_parameter, hit_triangle);
//		// If we get a hit then we can early out.
//		if 
//	}
	if (near_side != nullptr) {
		if (near_side->ray_test(ray, hit_parameter, hit_triangle)) {
			// If we hit a near side triangle it is possible that it is a shared near/far triangle.
			// If this is the case then it is possible that we hit it on the far side in a way that is occluded
			// by some other far side triangle.
			// NB: If all we want is hit/no-hit then this is an irrelevant check, but it is critical to get
			// the closest hit. Maybe I should add an "occlusion only" flag that skips this path for shadow rays?

			// Compute the ray parameter of the split plane.
			if (far_side == nullptr)
				return true;
			Real effective_far_side_split = near_side == low_side ? far_side->aabb.minima(split_axis) : far_side->aabb.maxima(split_axis);
//			Real split_plane_ray_parameter_times_slope_towards_plane = effective_far_side_split - origin;
			Real hit_parameter_of_far_side_aabb = (effective_far_side_split - origin) / ray.direction(split_axis);
//			bool hit_above_plane = hit_parameter * ray.direction(split_axis) > split_plane_ray_parameter_times_slope_towards_plane;
//			if (overlaps_high_side != hit_above_plane) {
			if (hit_parameter > hit_parameter_of_far_side_aabb) {
				Real temp_hit_parameter;
				Triangle* temp_triangle;
				bool temp_result = far_side->ray_test(ray, temp_hit_parameter, &temp_triangle);
				// We MUST get a far side hit, because the near side triangle we hit must also be a far side triangle.
//				assert(temp_result);
				if (temp_result and temp_hit_parameter < hit_parameter) {
					// It never makes sense to get a worse collision from a far side hit from the far node.
					// This is because the near side triangle we hit on the far side must also be a far side triangle!
//					if (temp_hit_parameter > hit_parameter) {
//						cout << temp_hit_parameter << " " << hit_parameter << "\n";
//					}
//					assert(temp_hit_parameter <= hit_parameter);
//					hit_parameter = real_min(temp_hit_parameter, hit_parameter);
					hit_parameter = temp_hit_parameter;
					if (hit_triangle != nullptr)
						*hit_triangle = temp_triangle;
				}
			}
			// If the above check fails, then the hit is near side, and there's no need to check the far side.
			return true;
		}
	}
	if (far_side != nullptr)
		return far_side->ray_test(ray, hit_parameter, hit_triangle);
	return false;
}

// Try not to puke at this global storage used for configuring the following comparison function in leu of closures.
vector<Triangle>* global_triangles;
int global_comparison_axis;
bool global_use_maxima;

// Check whether the xth triangle's AABB starts (resp. ends) on the nth axis lower than the triangle with index y, where n is equal to global_comparison_axis.
bool compare_on_nth_axis(const int& x, const int& y) {
	vector<Triangle>& all_triangles = *global_triangles;
	bool answer1 = all_triangles[x].aabb.maxima(global_comparison_axis) < all_triangles[y].aabb.maxima(global_comparison_axis);
	bool answer2 = all_triangles[x].aabb.minima(global_comparison_axis) < all_triangles[y].aabb.minima(global_comparison_axis);
	return global_use_maxima ? answer1 : answer2;
}

void kdTreeNode::get_stats(int& deepest_depth, int& biggest_set) {
	if (depth > deepest_depth)
		deepest_depth = depth;
	if (stored_triangle_count > biggest_set)
		biggest_set = stored_triangle_count;
	if (low_side != nullptr)
		low_side->get_stats(deepest_depth, biggest_set);
	if (high_side != nullptr)
		high_side->get_stats(deepest_depth, biggest_set);
}

#ifdef THREADED_KD_BUILD
void BuildingThread::spawn_thread(kdTree* _tree) {
	tree = _tree;
	pthread_create(&thread, nullptr, BuildingThread::computation_thread_main, (void*)this);
}

void* BuildingThread::computation_thread_main(void* cookie) {
	BuildingThread* self = (BuildingThread*) cookie;
	JobDescriptor current_job;
	// Begin waiting on a task.
	while (true) {
		sem_wait(&self->tree->job_available);
		pthread_mutex_lock(&self->tree->job_lock);
		// Copy over the job descriptor.
		current_job = self->tree->job_list.front();
		self->tree->job_list.pop_front();
		// Release the job_writable semaphore, allowing other threads to be issued jobs.
		pthread_mutex_unlock(&self->tree->job_lock);
		// Iff the job tells to quit then we're being signaled that work is done, and we do so.
		if (current_job.do_quit)
			break;
		// Begin processing.
		auto new_node = new kdTreeNode(self->tree, current_job.depth, current_job.sorted_indices_by_min, current_job.sorted_indices_by_max, self->tree->all_triangles);
		*current_job.destination = new_node;
		// Free the lists.
		// Finally, free our scratch lists.
		for (int axis = 0; axis < 3; axis++) {
			delete current_job.sorted_indices_by_min[axis];
			delete current_job.sorted_indices_by_max[axis];
		}
		// Decrement the total job count.
		pthread_mutex_lock(&self->tree->job_lock);
		self->tree->total_job_count--;
		if (self->tree->total_job_count == 0)
			sem_post(&self->tree->jobs_all_done);
		pthread_mutex_unlock(&self->tree->job_lock);
	}
	return nullptr;
}
#endif

// Use this storage to prevent multiple threads from calling kdTree::kdTree() at once.
bool currently_building = false;

kdTree::kdTree(vector<Triangle>* _all_triangles) {
	struct timeval start, stop, result;
	gettimeofday(&start, NULL);

	if (currently_building) {
		cerr << "Error: kdTree::kdTree is currently not re-entrant." << endl;
		assert(false);
	}
	currently_building = true;
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

	// Start up a thread pool to spawn the jobs off to.
#ifdef THREADED_KD_BUILD
	int thread_count = get_optimal_thread_count();
	total_job_count = 0;
	// Initialize our synchronization structures.
	// NB: I had a horrible bug where I had these three lines below the thread spawning, and it caused subtle misbehavior.
	sem_init(&job_available, 0, 0);
	sem_init(&jobs_all_done, 0, 0);
	pthread_mutex_init(&job_lock, NULL);
	auto computation_threads = new BuildingThread[thread_count];
	for (int i = 0; i < thread_count; i++)
		computation_threads[i].spawn_thread(this);
#endif

#ifdef THREADED_KD_BUILD
	// Acquire the job variable for our thread.
	pthread_mutex_lock(&job_lock);
	JobDescriptor job;
	job.do_quit = false;
	job.destination = &root;
	job.depth = 0;
	for (int i = 0; i < 3; i++) {
		job.sorted_indices_by_min[i] = sorted_indices_by_min[i];
		job.sorted_indices_by_max[i] = sorted_indices_by_max[i];
	}
	job_list.push_back(job);
	total_job_count++;
	// Release the job, so a processing thread can begin processing it.
	pthread_mutex_unlock(&job_lock);
	sem_post(&job_available);
#else
	// Actually build the tree!
	root = new kdTreeNode(this, 0, sorted_indices_by_min, sorted_indices_by_max, all_triangles);
#endif

#ifdef THREADED_KD_BUILD
	// Make sure all our threads exit.
	// Our protocol is that job.do_quit = true indicates to a thread that we're done.
	// Thus, we set this flag, and then post to the semaphore once per thread.
	// We must first grab job_writable to avoid overwriting a job being read.
	sem_wait(&jobs_all_done);
	job.do_quit = true;
	// NB: These loops can't be merged without creating a deadlock.
	for (int i = 0; i < thread_count; i++) {
		pthread_mutex_lock(&job_lock);
		job_list.push_back(job);
		pthread_mutex_unlock(&job_lock);
		sem_post(&job_available);
	}
	for (int i = 0; i < thread_count; i++)
		pthread_join(computation_threads[i].thread, nullptr);
	sem_destroy(&job_available);
	sem_destroy(&jobs_all_done);
	pthread_mutex_destroy(&job_lock);
#endif

	// Freeing occurs in the worker thread when threaded building is being used, so in that case we don't free here.
#ifndef THREADED_KD_BUILD
	// Free our scratches.
	for (int axis = 0; axis < 3; axis++) {
		delete sorted_indices_by_min[axis];
		delete sorted_indices_by_max[axis];
	}
#else
	delete[] computation_threads;
#endif

	currently_building = false;

	gettimeofday(&stop, NULL);
	timersub(&stop, &start, &result);
	double elapsed_time = result.tv_sec + result.tv_usec * 1e-6;
	cout << "Elapsed build time: " << elapsed_time << endl;
}

kdTree::~kdTree() {
	delete root;
}

long long rays_cast = 0;

bool kdTree::ray_test(const Ray& ray, Real& hit_parameter, Triangle** hit_triangle) {
	rays_cast++;
	return root->ray_test(ray, hit_parameter, hit_triangle);
}

