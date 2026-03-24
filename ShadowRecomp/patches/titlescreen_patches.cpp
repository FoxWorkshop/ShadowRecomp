#include <kernel/function.h>
#include <input/input.h>
#include <ui/fader.h>
#include <app.h>

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

X86_FUNC_IMPL(__imp__sub_294190);
X86_FUNC(sub_294190)
{
    
	auto twk = X86_LOAD_U32(ctx.ecx.u32 + 4);
    auto mode = X86_LOAD_U32(twk);
	if (mode == 2)
	{
        auto submode = X86_LOAD_U32(twk + 8);

        g_timerAllowedToQuit += App::s_deltaTime;
        if (g_timerAllowedToQuit >= 4.f)
        {
            if (submode && isPressedAnalogButton(0, Buttons_B) && !g_faderBegun)
            {
                g_quitMessageOpen = true;
            }
        }
	}
    else
    {
        g_timerAllowedToQuit = 0;
    }

    if (!ProcessQuitMessage())
	    __imp__sub_294190(ctx, base);
}