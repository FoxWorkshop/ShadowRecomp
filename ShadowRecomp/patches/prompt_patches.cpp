#include <kernel/function.h>
#include <locale/locale.h>
#include <ui/message_window.h>
#include <user/config.h>

static int messageID[] = { 6062, 6068, 6075, 6081, 6092 };
const int messageIDLength = 5;
static int g_CreateSaveMessageResult = -1;
static int g_CreateSaveMessageFailedResult = -1;

// skip error message save
X86_FUNC_IMPL(__imp__sub_2A6530);
X86_FUNC(sub_2A6530)
{
	auto mode = X86_LOAD_U32(ctx.ecx.u32);

	if (mode == 6) //this technically should no longer trigger
	{
		X86_STORE_U32(ctx.ecx.u32, 14);
	}

	__imp__sub_2A6530(ctx, base);
}

static bool g_shouldPreventPrompt = false;

// player quit prompt
X86_FUNC_IMPL(__imp__sub_5A7B0);
X86_FUNC(sub_5A7B0)	
{
	g_shouldPreventPrompt = Config::ConfirmationPrompt.Value == false;
	__imp__sub_5A7B0(ctx, base);
	g_shouldPreventPrompt = false;
}

// player restart prompt
X86_FUNC_IMPL(__imp__sub_5A750);
X86_FUNC(sub_5A750)
{
	g_shouldPreventPrompt = Config::ConfirmationPrompt.Value == false;
	__imp__sub_5A750(ctx, base);
	g_shouldPreventPrompt = false;
}

// load or create save file confirmation prompt
X86_FUNC_IMPL(__imp__sub_2A6D00);
X86_FUNC(sub_2A6D00)
{
	g_shouldPreventPrompt = Config::ConfirmationPrompt.Value == false;
	__imp__sub_2A6D00(ctx, base);
	g_shouldPreventPrompt = false;
}

// confirmation prompts logic
X86_FUNC_IMPL(__imp__sub_2A5B60);
X86_FUNC(sub_2A5B60)
{
	if (g_shouldPreventPrompt)
	{
		ctx.eax.u32 = 2;
		ctx.AddEsp(4); // thiscall with 3 param + ret value = (4*4)

		return;
	}

	__imp__sub_2A5B60(ctx, base);
}

static bool ProcessCreateSaveMessage()
{
	std::array<std::string, 2> options = { Localise("Common_Yes"), Localise("Common_No") };
	MessageWindow::Open(Localise("Title_Message_CreateSave"), &g_CreateSaveMessageResult, options, 0, 1, true);
	return true;
}

static bool ProcessFailedCreateSaveMessage()
{
	std::array<std::string, 1> options = { Localise("Common_Ok") };
	MessageWindow::Open(Localise("Title_Message_CreateSaveFailed"), &g_CreateSaveMessageFailedResult, options, 0, 1, true);
	return true;
}

static int g_currentMessageID = 0;
// draw message
X86_FUNC_IMPL(__imp__sub_2A6020);
X86_FUNC(sub_2A6020)
{
	X86Context ctxBackup = ctx;

	__imp__sub_2A6020(ctx, base);

	if (g_currentMessageID) // if we used a custom message, function will return early, we need to fix registers and esp
	{
		ctx = ctxBackup;
		ctx.esp.u32 += 4;
		g_currentMessageID = 0;
	}

}

bool DrawAdvertiseMessageMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	g_currentMessageID = X86_LOAD_U32(esi.u32);

	switch (g_currentMessageID)
	{
	default:
		break;
	case 6070:
		return ProcessFailedCreateSaveMessage();
	case 6068:
	case 6081:
		return ProcessCreateSaveMessage();
	}


	g_currentMessageID = 0;
	return false;
}