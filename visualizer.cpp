// Displays partial results as a RenderEngine progresses.
// Displays partial results as a RenderEngine progresses.

using namespace std;
#include <iostream>
#include "visualizer.h"

ProgressDisplay::ProgressDisplay(RenderEngine* engine) : engine(engine) {
}

bool ProgressDisplay::init() {
	// Initialize SDL.
	// TODO: Evaluate if this is safe to do multiple times.
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		cerr << "Unable to SDL_Init." << endl;
		return false;
	}
	int video_flags = 0;
	video_flags |= SDL_GL_DOUBLEBUFFER;
	video_flags |= SDL_HWPALETTE;
	// TODO: Set screen_width and screen_height intelligently, then allow the user to scroll and zoom around the rendering image.
	screen_width = engine->width;
	screen_height = engine->height;
	screen = SDL_SetVideoMode(screen_width, screen_height, 32, video_flags);
	if (screen == NULL) {
		cerr << "Couldn't set video mode: " << SDL_GetError() << endl;
		SDL_Quit();
		return false;
	}
	return true;
}

void ProgressDisplay::main_loop() {
	while (1) {
		// We begin each loop by getting events.
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			// Close prematurely if the user hits esc or hits the X in the window decoration.
			switch (ev.type) {
				case SDL_QUIT:
					return;
				case SDL_KEYDOWN:
					if (ev.key.keysym.sym == 27)
						return;
					break;
			}
		}

//		cout << "Rebuild." << endl;
		// Aggregate an up-to-date master canvas from the various rendering threads.
		engine->rebuild_master_canvas();

		// We must make these calls, to avoid violating SDL's rules.
		if (SDL_MUSTLOCK(screen))
	        SDL_LockSurface(screen);

		// Copy pixels from the master canvas into SDL's buffer.
		for (int y = 0; y < screen_height; y++) {
			for (int x = 0; x < screen_width; x++) {
				unsigned char* pixel_pointer = ((unsigned char*)screen->pixels) + 4*(x + y * screen_width);
				engine->master_canvas->get_pixel(x, y, (uint8_t*)pixel_pointer);
			}
		}

		if (SDL_MUSTLOCK(screen))
	        SDL_UnlockSurface(screen);

		// This call actually makes our changes visible, by flipping the double buffer.
		SDL_Flip(screen);
	}
	SDL_Quit();
}

