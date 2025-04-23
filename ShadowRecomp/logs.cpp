#include <kernel/function.h>


void PrintError(const char* a1)
{
	printf(a1);
	printf("\n\n");
}


void DebugPrint(char* const Buffer, const char* const Format, va_list ArgList)
{
	printf(Buffer);
	printf("\n\n");
	vsprintf(Buffer, Format, ArgList);
}


X86_FUNC_IMPL(__imp__sub_11AD0);
X86_FUNC(sub_11AD0)
{
	printf("PreMainLauncher\n");
	__imp__sub_11AD0(ctx, base);
}


X86_FUNC_IMPL(__imp__sub_2AD4E4);
X86_FUNC(sub_2AD4E4)
{
	printf("XLaunchNewImageA\n");
	__imp__sub_2AD4E4(ctx, base);
}

X86_FUNC_IMPL(__imp__sub_11430);
X86_FUNC(sub_11430)
{
	printf("\nRsRwInitialize\n");
	__imp__sub_11430(ctx, base);
}

X86_FUNC_IMPL(__imp__sub_578B0);
X86_FUNC(sub_578B0)
{
	printf("\nSuccessfully initialized RenderWare\n");
	printf("....Now Loading Game Files...\n");
	__imp__sub_578B0(ctx, base);
}


GUEST_FUNCTION_HOOK(sub_11210, PrintError);
GUEST_FUNCTION_HOOK(sub_37E7BB, DebugPrint);

