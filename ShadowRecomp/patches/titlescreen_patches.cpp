#include <kernel/function.h>
#include <input/input.h>
#include <ui/fader.h>
#include <app.h>
#include <ui/message_window.h>
#include <input/padstate.h>
#include <ui/options_menu.h>
#include <ui/menu_common.h>

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
		if (!g_quitMessageResult) // if user asked to quit, fade out and quit the game
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

// This function controls the timer logic during title screen and menu
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

// Options logic
X86_FUNC_IMPL(__imp__sub_28F720);
X86_FUNC(sub_28F720)
{
	GameMenuCommon::g_currentModeAddr = X86_LOAD_U32(ctx.ecx.u32 + 4);
	auto menuMode = X86_LOAD_U32(GameMenuCommon::g_currentModeAddr);
	auto subMode = X86_LOAD_U32(GameMenuCommon::g_currentModeAddr + 0x4);

	if (menuMode == 0) // if user requested options access, init our custom option menu
	{
		OptionsMenu::Open(false);
		GameMenuCommon::g_OptionsOpen = true;
		__imp__sub_28F720(ctx, base);
	}
	else if (GameMenuCommon::g_OptionsOpen == false) // we are no longer in options mode, let original code logic run
	{
		__imp__sub_28F720(ctx, base);
	}
	else
	{
		ctx.AddEsp(1); // prevent original function to run while our custom option menu run
	}
}

// Options rendering stubbed
X86_FUNC_IMPL(__imp__sub_28F200);
X86_FUNC(sub_28F200)
{
	ctx.AddEsp(1);
}

// get next mode to force it to 0 when we quit options 
X86_FUNC_IMPL(__imp__sub_28E770);
X86_FUNC(sub_28E770)
{
	GameMenuCommon::g_nextModeAddr = ctx.ecx.u32 + 0x20;

	__imp__sub_28E770(ctx, base);
}