#include "app.h"
#include <gpu/video.h>
#include <kernel/function.h>
#include <ui/game_window.h>
#include <os/process.h>
#include <user/config.h>
#include <audio/audio.h>
#include <volk.h>
#include <user/paths.h>
#include <user/registry.h>
#include <audio/driver/sdl2_driver.h>

static std::thread::id g_mainThreadId = std::this_thread::get_id();

void App::Restart(std::vector<std::string> restartArgs)
{
	os::process::StartProcess(os::process::GetExecutablePath(), restartArgs, os::process::GetWorkingDirectory());
	Exit();
}

void App::Exit()
{
	AudioShutDown();
	Config::Save();

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
	//std::this_thread::sleep_for(std::chrono::milliseconds(16));

}

//init
X86_FUNC_IMPL(__imp__sub_11AD0);
X86_FUNC(sub_11AD0)
{
	App::s_isInit = true;
	App::s_language = Config::Language;

	Registry::Save();

	__imp__sub_11AD0(ctx, base);
}

//Set Voice Language
X86_FUNC_IMPL(__imp__sub_55BE0);
X86_FUNC(sub_55BE0)
{
	__imp__sub_55BE0(ctx, base);

	X86_STORE_U32(0x54DD8C, (uint32_t)Config::VoiceLanguage.Value);
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
		// fix unresponsive window
		if (std::this_thread::get_id() == g_mainThreadId)
		{
			MainThreadDispatcher::Get().Flush(); // This runs the callbacks needed from user's config
			SDL_PumpEvents();
			SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
			GameWindow::Update();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(16)); // This is a hack; Without it two+ controllers connected makes framerate tank even on menus
	}
}