#include <kernel/function.h>


static std::thread::id g_mainThreadId = std::this_thread::get_id();


X86_FUNC_IMPL(__imp__sub_57630);
X86_FUNC(sub_57630)
{

	SDL_PumpEvents();
	SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

	__imp__sub_57630(ctx, base);
}