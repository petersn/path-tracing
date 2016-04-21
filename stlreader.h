// Simple binary STL reader.

#ifndef _RENDER_STLREADER_H
#define _RENDER_STLREADER_H

#include <string>
#include <vector>
#include "utils.h"

std::vector<Triangle>* read_stl(std::string path);

#endif

