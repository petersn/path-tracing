// Simple k-d tree for ray casting.

#ifndef _KDTREE_H
#define _KDTREE_H

#ifdef THREADED_KD_BUILD
#include <pthread.h>
#include <semaphore.h>
#endif

#include <vector>
#include <list>
#include "utils.h"

extern long long rays_cast;

class kdTree;

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
	kdTreeNode(kdTree* parent, int depth, vector<int>* sorted_indices_by_min[3], vector<int>* sorted_indices_by_max[3], vector<Triangle>* all_triangles);
	~kdTreeNode();
	void get_stats(int& deepest_depth, int& biggest_set);
	bool ray_test(const CastingRay& ray, Real& hit_parameter, const Triangle** hit_triangle=nullptr) const;
};

#ifdef THREADED_KD_BUILD
struct JobDescriptor {
	bool do_quit;
	kdTreeNode** destination;
	int depth;
	vector<int>* sorted_indices_by_min[3];
	vector<int>* sorted_indices_by_max[3];
};

struct BuildingThread {
	pthread_t thread;
	kdTree* tree;

	void spawn_thread(kdTree* _tree);
	static void* computation_thread_main(void* cookie);
};
#endif

class kdTree {
#ifdef THREADED_KD_BUILD
	friend class BuildingThread;
#endif
	std::vector<Triangle>* all_triangles;

public:
	kdTreeNode* root;

#ifdef THREADED_KD_BUILD
	// The following data is used for synchronizing with the building threads.
	sem_t job_available;
	sem_t jobs_all_done;
	pthread_mutex_t job_lock;
	list<JobDescriptor> job_list;
	int total_job_count;
#endif

	kdTree(std::vector<Triangle>* all_triangles);
	~kdTree();
	bool ray_test(const Ray& ray, Real& hit_parameter, const Triangle** hit_triangle=nullptr);
};

#endif

