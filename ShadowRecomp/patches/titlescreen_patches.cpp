#include <kernel/function.h>
#include <input/input.h>
#include <ui/fader.h>
#include <app.h>
#include <ui/message_window.h>
#include <input/padstate.h>

static std::atomic<bool> g_faderBegun = false;

bool g_quitMessageOpen = false;
static int g_quitMessageResult = -1;
static float g_timerAllowedToQuit = 0.f;

static bool ProcessQuitMessage()
{
	if (!g_quitMessageOpen)
		return false;

	std::array<std::string, 2> options = { Localise("Common_Yes"), Localise("Common_No") };

	if (MessageWindow::Open(Localise("Title_Message_Quit"), &g_quitMessageResult, options, 0) == MSG_CLOSED)
	{
		g_timerAllowedToQuit = 0;
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

// This function controls the title screen, it checks if it should reset the title screen or move to main menu depending on player action.
X86_FUNC_IMPL(__imp__sub_292CF0);
X86_FUNC(sub_292CF0)
{
	auto menu = ctx.ecx.u32;

	g_timerAllowedToQuit += App::s_deltaTime;

	if (g_timerAllowedToQuit >= 4.f)
	{
		if (PJSPeripheral::IsJustPressed(0, PJSPeripheral::BTN_B) && !g_faderBegun)
		{
			g_quitMessageOpen = true;
		}
	}


	if (!ProcessQuitMessage() && g_quitMessageOpen == false)
		__imp__sub_292CF0(ctx, base);
	else
		ctx.esp.u32 += 4;
}

// This function controls the timer logic during title screen (maybe menu too?) 
// to prevent the game from trying to reset the title screen while we have the message open we block
// the function until player close the message box
X86_FUNC_IMPL(__imp__sub_17ED0);
X86_FUNC(sub_17ED0)
{

	if (g_quitMessageOpen || !g_quitMessageResult)
	{
		ctx.eax.u8_lo = 0;
		ctx.AddEsp(1);
	}
	else
	{
		__imp__sub_17ED0(ctx, base);
	}


}