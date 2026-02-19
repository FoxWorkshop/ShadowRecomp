#include "app.h"
#include <gpu/video.h>
#include <kernel/function.h>
#include <ui/game_window.h>
#include <user/config.h>
#include <audio/audio.h>
#include <volk.h>

static std::thread::id g_mainThreadId = std::this_thread::get_id();

void App::Exit()
{

	SDL_Quit();

	//Config::Save();

#ifdef _WIN32
	timeEndPeriod(1);
#endif

	std::_Exit(0);

}
int Window_OnSDLEvent(void*, SDL_Event* event);

//main game loop
X86_FUNC_IMPL(__imp__sub_57630);
X86_FUNC(sub_57630)
{
	Video::WaitOnSwapChain();

	if (Config::FPS >= FPS_MIN && Config::FPS < FPS_MAX)
	{
		double targetDeltaTime = 1.0 / Config::FPS;

		auto frameTime_HHC = X86_LOAD_F32(0x541FA8);
		if (abs(frameTime_HHC - targetDeltaTime) < 0.00001)
			X86_STORE_F32(0x541FA8, targetDeltaTime);
	}

	App::s_deltaTime = X86_LOAD_F32(0x541FA8);
	App::s_time += App::s_deltaTime;

	__imp__sub_57630(ctx, base);
}

std::atomic<bool> shouldQuit = false; 

// We need to call SDL_PumpEvents and SDL_FlushEvents in the Main Thread of our program in a function that run every frames.
// If we don't, the game window will act unresponsive, preventing resizing, closing, etc to work.
// unfortunately Shadow immediately create a new thread in the entry point function and move on
// So we do a mid asm hook in the entry point function and put our loop there instead
void UpdateSDLEventsMidAsmHook()
{
	while (!shouldQuit)
	{
		//fix unresponsive window
		if (std::this_thread::get_id() == g_mainThreadId)
		{
			SDL_PumpEvents();
			SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
			GameWindow::Update();
		}
	}
}