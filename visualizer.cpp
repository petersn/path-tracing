// Displays partial results as a RenderEngine progresses.
// Displays partial results as a RenderEngine progresses.

using namespace std;
#include <iostream>
#include <unistd.h>
#include "visualizer.h"

// This is the number of pixels to mark red in the corner of each rendering tile.
#define TILE_CORNER_SIZE 8

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
	video_flags |= SDL_FULLSCREEN;
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
		usleep(100000);
		engine->rebuild_master_canvas();

		// We must make these calls, to avoid violating SDL's rules.
		if (SDL_MUSTLOCK(screen))
	        SDL_LockSurface(screen);

		// Copy pixels from the master canvas into SDL's buffer.
		for (int y = 0; y < screen_height; y++) {
			for (int x = 0; x < screen_width; x++) {
				unsigned char* pixel_pointer = ((unsigned char*)screen->pixels) + 4 * (x + y * screen_width);
				engine->master_canvas->get_pixel(x, y, (uint8_t*)pixel_pointer);
			}
		}

		// Draw the tiles being currently processed.
		for (RenderThread* worker : engine->workers) {
			if (not worker->is_running)
				continue;
			PassDescriptor desc;
			desc.start_x = worker->currently_processing.start_x;
			desc.start_y = worker->currently_processing.start_y;
			desc.width   = worker->currently_processing.width;
			desc.height  = worker->currently_processing.height;
			// We read this value in a thread-unsafe way, so we have to bounds check the values in case one of them is utter garbage and huge.
			desc.clamp_bounds(screen_width, screen_height);
			// To prevent a segfault when we draw the markings for the tile we clamp the size of the markings.
			// TODO: Make this instead use a "set pixel" routine that does bounds checking.
			int tile_corner_size = TILE_CORNER_SIZE;
			if (desc.width < tile_corner_size)
				tile_corner_size = desc.width;
			if (desc.height < tile_corner_size)
				tile_corner_size = desc.height;
			// Draw little red marks marking the box.
#define MARK_RED(xx, yy) (((unsigned char*)screen->pixels) + 4 * ((xx) + (yy) * screen_width))[2] = 255;
			for (int i = 0; i < tile_corner_size; i++) {
				// Top left corner.
				MARK_RED(desc.start_x + i, desc.start_y)
				MARK_RED(desc.start_x, desc.start_y + i)
				// Top right corner.
				MARK_RED(desc.start_x + desc.width - 1 - i, desc.start_y)
				MARK_RED(desc.start_x + desc.width - 1, desc.start_y + i)
				// Bottom left corner.
				MARK_RED(desc.start_x + i, desc.start_y + desc.height - 1)
				MARK_RED(desc.start_x, desc.start_y + desc.height - 1 - i)
				// Bottom right corner.
				MARK_RED(desc.start_x + desc.width - 1 - i, desc.start_y + desc.height - 1)
				MARK_RED(desc.start_x + desc.width - 1, desc.start_y + desc.height - 1 - i)
			}
		}

		if (SDL_MUSTLOCK(screen))
	        SDL_UnlockSurface(screen);

		// This call actually makes our changes visible, by flipping the double buffer.
		SDL_Flip(screen);
	}
	SDL_Quit();
	exit(1);
}

