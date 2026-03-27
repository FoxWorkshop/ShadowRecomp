#include <kernel/function.h>
#include <input/input.h>
#include <ui/fader.h>
#include <app.h>
#include <ui/message_window.h>

static std::atomic<bool> g_faderBegun = false;

bool g_quitMessageOpen = false;
static int g_quitMessageResult = -1;
static float g_timerAllowedToQuit = 0.f;

static bool ProcessQuitMessage()
{
	if (!g_quitMessageOpen)
		return false;

	std::array<std::string, 2> options = { Localise("Common_Yes"), Localise("Common_No") };


	g_quitMessageResult = 0; //to do implement message
	//if (MessageWindow::Open(Localise("Title_Message_Quit"), &g_quitMessageResult, options, 1) == MSG_CLOSED)
	{

		if (!g_quitMessageResult)
		{
			Fader::FadeOut(1, []() { App::Exit(); });
			g_faderBegun = true;
		}

		g_quitMessageOpen = false;
		g_quitMessageResult = -1;
	}

	return true;
}

X86_FUNC_IMPL(__imp__sub_292CF0);
X86_FUNC(sub_292CF0)
{

	g_timerAllowedToQuit += App::s_deltaTime;

	if (g_timerAllowedToQuit >= 4.f)
	{
		if (isPressedAnalogButton(0, Buttons_B) && !g_faderBegun)
		{
			g_quitMessageOpen = true;
		}
	}


	if (!ProcessQuitMessage())
		__imp__sub_292CF0(ctx, base);
	else
		ctx.esp.u32 += 4;
}