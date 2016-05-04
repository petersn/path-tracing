// Interactive model renderer.

using namespace std;
#include <SDL.h>
#include <math.h>
#include <iostream>
#include "integrator.h"

// Global storage. Eww.
int screen_width, screen_height;
SDL_Surface* screen;
bool left_held = false, right_held = false;
int counter = 20, rendered_for = -1;
Scene* scene;
Integrator* integrator;

void main_loop(void) {
	Real gain;
	while (1) {
		// We begin each loop by getting events.
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
				// SDL_QUIT corresponds to someone hitting the X on the window decoration, so we close the program.
				case SDL_QUIT:
					SDL_Quit();
					exit(0);
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (ev.button.button == 1) {
						// Compute the coordinates.
						cout << "Click at: " << ev.motion.x << " " << ev.motion.y << endl;
						// Compute the ray that contributed to this pixel.
						Ray r = integrator->get_ray_for_pixel(ev.motion.x, ev.motion.y);
						cout << r.origin << "\n\n" << r.direction << "\n\n";
					}
					break;
				case SDL_KEYDOWN:
					// This corresponds to a user hitting escape, so we close the program. (keycode 27 = esc)
					if (ev.key.keysym.sym == 27) {
						SDL_Quit();
						exit(0);
					} else if (ev.key.keysym.sym == SDLK_LEFT) {
						left_held = 1;
					} else if (ev.key.keysym.sym == SDLK_RIGHT) {
						right_held = 1;
					} else if (ev.key.keysym.sym == SDLK_SPACE) {
						char path[80];
						sprintf(path, "render_passes=%04i.png", integrator->passes);
//						integrator->canvas->gain = gain;
						integrator->canvas->save(path);
						cout << "Saved to " << path << endl;
					}
					break;
				case SDL_KEYUP:
					if (ev.key.keysym.sym == SDLK_LEFT)
						left_held = 0;
					if (ev.key.keysym.sym == SDLK_RIGHT)
						right_held = 0;
					break;
			}
		}

		// Reposition the camera.
		Real angle = counter * 0.05;
		scene->main_camera.origin = -5 * Vec(cos(angle), sin(angle), 0.0);
		scene->main_camera.direction = -scene->main_camera.origin;
		scene->main_camera.direction.normalize();
		scene->main_camera.origin += Vec(0.0, 0.0, 0.2);
		if (rendered_for != counter) {
			triangle_tests = 0;
			integrator->canvas->zero();
			integrator->passes = 0;
			rendered_for = counter;

		}
		integrator->perform_pass();
		cout << "Pass: " << integrator->passes << " Time: " << integrator->last_pass_seconds << " Rays cast: " << rays_cast << " MT calls: " << triangle_tests << " MTs per pixel: " << triangle_tests / (float) (screen_width * screen_height) << endl;
//		char path[80];
//		sprintf(path, "progressive/frame%04i.png", integrator->passes);
//		gain = 255.0 / integrator->passes;
//		integrator->canvas->gain = gain;
//		integrator->canvas->save(path);
//		cout << "Saved to " << path << endl;

		// Once we're done handling all the key presses, draw some random shit.
		if (left_held)
			counter--;
		if (right_held)
			counter++;

		// We must make these calls, to avoid violating SDL's rules.
		if (SDL_MUSTLOCK(screen))
	        SDL_LockSurface(screen);

		int x, y;
		Color* pixels = integrator->canvas->pixels;
		int height = screen_height;
		int width = screen_width;
		gain = 255.0 / integrator->passes;
		for (y = 0; y < screen_height; y++) {
			for (x = 0; x < screen_width; x++) {
				unsigned char* pixel_pointer = ((unsigned char*)screen->pixels) + 4*(x + y*width);
				// Set the B, G, and R components separately.
				// The fourth byte is reserved for alpha, but not used in RGB video mode for alignment reasons.
//				integrator->canvas->get_pixel(x, y, (uint8_t*)pixel_pointer);
				Color c = pixels[x + y * width];
				for (int i = 0; i < 3; i++)
					pixel_pointer[i] = (unsigned char)real_max(0.0, real_min(255.0, (Real)(c(i) * gain)));

//				pixel_pointer[0] = counter + x*x*y*y*y;
//				pixel_pointer[1] = y-x + counter;
//				pixel_pointer[2] = x*y*counter;
			}
		}

		// We must make these calls, to avoid violating SDL's rules.
		if (SDL_MUSTLOCK(screen))
	        SDL_UnlockSurface(screen);

		// This call actually makes our changes visible, by flipping the double buffer.
		SDL_Flip(screen);
	}
}

int main(int argc, char** argv) {
	// Load up an STL file.
	scene = new Scene(argv[1]);
	// Make a light.
	scene->lights->push_back(Light({Vec(0, 0, 3), 9.0 * Vec(0.8, 0.5, 0.25)}));
	scene->lights->push_back(Light({Vec(-2, 2, 4), 9.0 * Vec(0.25, 0.8, 0.25)}));
	scene->lights->push_back(Light({Vec(-2, -2, 4), 9.0 * Vec(0.25, 0.25, 0.8)}));
	scene->camera_image_plane_width = 0.5 * 1.5;
	// Initialize SDL.
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to SDL_Init.\n");
		exit(1);
	}
	// This call lets us get the screen resolution, to be completely fullscreen.
	const SDL_VideoInfo* info = SDL_GetVideoInfo();
	if (info == NULL) {
		fprintf(stderr, "Unable to get video info: %s\n", SDL_GetError());
		exit(1);
	}
	// Choose the maximum possible screen resolution to be our target resolution.
	// Of course, if you want a smaller window, set these to other values.
//	screen_width  = info->current_w;
//	screen_height = info->current_h;
	screen_width = 1366;
	screen_height = 768;
	int video_flags = 0;
	// These guys aren't super critical, but change video performance/behavior on some systems.
	video_flags |= SDL_GL_DOUBLEBUFFER;
	video_flags |= SDL_HWPALETTE;
	// Comment this one out if you don't want fullscreen!
//	video_flags |= SDL_FULLSCREEN;
	screen = SDL_SetVideoMode(screen_width, screen_height, 32, video_flags);
	if (screen == NULL) {
		fprintf(stderr, "Couldn't set video mode: %s\n", SDL_GetError());
		SDL_Quit();
		exit(1);
	}
	// Allocate an integrator for the scene.
	integrator = new Integrator(screen_width, screen_height, scene);
	// Drop into the main loop.
	main_loop();
	return 0;
}

