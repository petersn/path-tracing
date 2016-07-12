// Simple binary STL reader.

#ifndef _RENDER_STLREADER_H
#define _RENDER_STLREADER_H

#include <string>
#include <vector>
#include "utils.h"

std::vector<Triangle>* read_stl(std::string path);

// This routine takes the .assigned_normals of the various triangles and computes appropriate barycentric normals for each triangle.
// XXX: This routine requires that vertices of overlapping triangles be exact floating point equal matches.
void compute_barycentric_normals(std::vector<Triangle>* triangles);

#endif

