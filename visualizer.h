// Displays partial results as a RenderEngine progresses.

#ifndef _RENDER_VISUALIZER_H
#define _RENDER_VISUALIZER_H

#include <SDL.h>
#include "integrator.h"

class ProgressDisplay {
	int screen_width, screen_height;
	SDL_Surface* screen;
	RenderEngine* engine;

public:
	ProgressDisplay(RenderEngine* engine);
	// Opens a display and returns true on success.
	bool init();
	// Launches into a loop waiting for the render to complete.
	void main_loop();
};

#endif

