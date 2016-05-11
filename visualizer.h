// Displays partial results as a RenderEngine progresses.

#ifndef _RENDER_VISUALIZER_H
#define _RENDER_VISUALIZER_H

#include <sys/time.h>
#include <SDL.h>
#include "integrator.h"

class ProgressReporter {
public:
	virtual ~ProgressReporter() {};
	virtual bool init() = 0;
	virtual void main_loop() = 0;
};

class ProgressDisplay : public ProgressReporter {
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

class ProgressBar : public ProgressReporter {
	RenderEngine* engine;
	struct timeval start;

public:
	ProgressBar(RenderEngine* engine);
	bool init();
	void main_loop();
};

#endif

