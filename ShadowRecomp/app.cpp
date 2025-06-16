#include <kernel/function.h>
#include <chrono>
#include <atomic>
#include <iostream>
#include <thread>

static std::thread::id g_mainThreadId = std::this_thread::get_id();
static auto lastTime = std::chrono::high_resolution_clock::now();
static int frameCount = 0;

X86_FUNC_IMPL(__imp__sub_57630);
X86_FUNC(sub_57630)
{

	SDL_PumpEvents();
	SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

	__imp__sub_57630(ctx, base);

	frameCount++;
	auto currentTime = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime);

	/**if (elapsed.count() >= 1000) { // Every second
		float fps = frameCount / (elapsed.count() / 1000.0f);
		printf("FPS: %.2f\n", fps);
		frameCount = 0;
		lastTime = currentTime;
	}*/

	std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
}