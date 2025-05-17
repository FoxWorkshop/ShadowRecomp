#include <stdafx.h>
#include <array>
#include <SDL.h>
#include "input.h"
#include <kernel/function.h>
#include <string>

input::EInputDevice input::g_inputDevice;
input::EInputDevice input::g_inputDeviceController;
input::EInputDeviceExplicit input::g_inputDeviceExplicit;

#define TRANSLATE_INPUT(S, X) SDL_GameControllerGetButton(controller, S) << FirstBitLow(X)
#define VIBRATION_TIMEOUT_MS 5000

//todo implement vibration
class Controller
{
public:
    SDL_GameController* controller{};
    SDL_Joystick* joystick{};
    SDL_JoystickID id{ -1 };
    XINPUT_GAMEPAD state{};

    int index{};

    Controller() = default;

    explicit Controller(int index) : Controller(SDL_GameControllerOpen(index))
    {
        this->index = index;
    }

    Controller(SDL_GameController* controller) : controller(controller)
    {
        if (!controller)
            return;

        joystick = SDL_GameControllerGetJoystick(controller);
        id = SDL_JoystickInstanceID(joystick);
    }

    SDL_GameControllerType GetControllerType() const
    {
        return SDL_GameControllerGetType(controller);
    }

    input::EInputDevice GetInputDevice() const
    {
        switch (GetControllerType())
        {
        case SDL_CONTROLLER_TYPE_PS3:
        case SDL_CONTROLLER_TYPE_PS4:
        case SDL_CONTROLLER_TYPE_PS5:
            return input::EInputDevice::PlayStation;
        case SDL_CONTROLLER_TYPE_XBOX360:
        case SDL_CONTROLLER_TYPE_XBOXONE:
            return input::EInputDevice::Xbox;
        default:
            return input::EInputDevice::Unknown;
        }
    }

    const char* GetControllerName() const
    {
        auto result = SDL_GameControllerName(controller);

        if (!result)
            return "Unknown Device";

        return result;
    }

    void Close()
    {
        if (!controller)
            return;

        SDL_GameControllerClose(controller);

        controller = nullptr;
        joystick = nullptr;
        id = -1;
    }

    bool CanPoll()
    {
        return controller;
    }

    void PollAxis()
    {
        if (!CanPoll())
            return;

        auto& pad = state;

        pad.sThumbLX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
        pad.sThumbLY = ~SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);

        pad.sThumbRX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX);
        pad.sThumbRY = ~SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY);

        pad.bLeftTrigger = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >> 7;
        pad.bRightTrigger = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >> 7;
    }

    void Poll()
    {
        if (!CanPoll())
            return;

        auto& pad = state;

        pad.wButtons = 0;

        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_DPAD_UP, XINPUT_GAMEPAD_DPAD_UP);
        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_DOWN);
        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_LEFT);
        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, XINPUT_GAMEPAD_DPAD_RIGHT);

        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_START, XINPUT_GAMEPAD_START);
        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_BACK, XINPUT_GAMEPAD_BACK);
        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_TOUCHPAD, XINPUT_GAMEPAD_BACK);

        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_LEFTSTICK, XINPUT_GAMEPAD_LEFT_THUMB);
        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_RIGHTSTICK, XINPUT_GAMEPAD_RIGHT_THUMB);

        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, XINPUT_GAMEPAD_LEFT_SHOULDER);
        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER);

        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_A, XINPUT_GAMEPAD_A);
        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_B, XINPUT_GAMEPAD_B);
        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_X, XINPUT_GAMEPAD_X);
        pad.wButtons |= TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_Y, XINPUT_GAMEPAD_Y);

    }


    void SetLED(const uint8_t r, const uint8_t g, const uint8_t b) const
    {
        SDL_GameControllerSetLED(controller, r, g, b);
    }
};

std::array<Controller, 4> g_controllers;
Controller* g_activeController;

inline Controller* EnsureController(uint32_t dwUserIndex)
{
    if (!g_controllers[dwUserIndex].controller)
        return nullptr;

    return &g_controllers[dwUserIndex];
}

inline size_t FindFreeController()
{
    for (size_t i = 0; i < g_controllers.size(); i++)
    {
        if (!g_controllers[i].controller)
            return i;
    }

    return -1;
}

inline Controller* FindController(int which)
{
    for (auto& controller : g_controllers)
    {
        if (controller.id == which)
            return &controller;
    }

    return nullptr;
}


static void SetControllerInputDevice(Controller* controller)
{
    g_activeController = controller;

    input::g_inputDevice = controller->GetInputDevice();
    input::g_inputDeviceController = input::g_inputDevice;

    auto controllerType = (input::EInputDeviceExplicit)controller->GetControllerType();
    auto controllerName = controller->GetControllerName();


    if (input::g_inputDeviceExplicit != controllerType)
    {
        input::g_inputDeviceExplicit = controllerType;

        if (controllerType == input::EInputDeviceExplicit::Unknown)
        {
            printf("Detected controller: %s (Unknown Controller Type)", controllerName);
        }
        else
        {
            printf("Detected controller: %s", controllerName);
        }
    }
}


int Input_OnSDLEvent(void*, SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_CONTROLLERDEVICEADDED:
    {
        const auto freeIndex = FindFreeController();

        if (freeIndex != -1)
        {
            auto controller = Controller(event->cdevice.which);

            g_controllers[freeIndex] = controller;

        }

        break;
    }

    case SDL_CONTROLLERDEVICEREMOVED:
    {
        auto* controller = FindController(event->cdevice.which);

        if (controller)
            controller->Close();

        break;
    }

    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
    case SDL_CONTROLLERAXISMOTION:
    case SDL_CONTROLLERTOUCHPADDOWN:
    {
        auto* controller = FindController(event->cdevice.which);

        if (!controller)
            break;

        if (event->type == SDL_CONTROLLERAXISMOTION)
        {
            if (abs(event->caxis.value) > 8000)
            {
                SDL_ShowCursor(SDL_DISABLE);
                SetControllerInputDevice(controller);
            }

            controller->PollAxis();
        }
        else
        {
            SDL_ShowCursor(SDL_DISABLE);
            SetControllerInputDevice(controller);

            controller->Poll();
        }

        break;
    }

    case SDL_KEYDOWN:
    case SDL_KEYUP:
        input::g_inputDevice = input::EInputDevice::Keyboard;
        break;

    case SDL_MOUSEMOTION:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    {
        /**if (!GameWindow::IsFullscreen() || GameWindow::s_isFullscreenCursorVisible)
            SDL_ShowCursor(SDL_ENABLE);*/

        input::g_inputDevice = input::EInputDevice::Mouse;

        break;
    }

    case SDL_WINDOWEVENT:
    {
        if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
        {
            // Stop vibrating controllers on focus lost.

        }

        break;
    }


    }

    return 0;
}

void input::Init()
{
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_GAMECUBE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS3, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_WII, "1");
    SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "1");

    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0"); // Uses Button Labels. This hint is disabled for Nintendo Controllers.

    SDL_InitSubSystem(SDL_INIT_EVENTS);
    SDL_AddEventWatch(Input_OnSDLEvent, nullptr);

    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
}

uint32_t input::GetState(uint32_t dwUserIndex, XINPUT_STATE* pState)
{
    static uint32_t packet;

    if (!pState)
        return ERROR_BAD_ARGUMENTS;

    memset(pState, 0, sizeof(*pState));

    pState->dwPacketNumber = packet++;

    if (!g_activeController)
        return ERROR_DEVICE_NOT_CONNECTED;

    SDL_ShowCursor(SDL_DISABLE);
    SetControllerInputDevice(g_activeController);

    //g_activeController->Poll(); //added to do figure out a better way
    //g_activeController->state.wButtons |= XINPUT_GAMEPAD_START;

    pState->Gamepad = g_activeController->state;

    return ERROR_SUCCESS;
}



uint32_t input::SetState(uint32_t dwUserIndex, _XINPUT_VIBRATION* pVibration)
{
    if (!pVibration)
        return ERROR_BAD_ARGUMENTS;

    if (!g_activeController)
        return ERROR_DEVICE_NOT_CONNECTED;

   // g_activeController->SetVibration(*pVibration);

    return ERROR_SUCCESS;
}



int sub_4241C7_r(char* this_, unsigned __int8 a2, char a3)
{
    auto ctx = GetX86Context();
    ctx->esp.u32 += 8;
    return 0;

}

uint8_t sub_424276_r()
{
    return 0;
}

#define _BYTE char
#define _WORD int16_t
#define _DWORD int32_t

struct _XPP_DEVICE_TYPE
{
    uint32_t CurrentConnected;
    uint32_t ChangeConnected;
    uint32_t PreviousConnected;
};

typedef enum class _XBOX_INPUT_DEVICE : int {
    DEVICE_INVALID = -1,
    MS_CONTROLLER_DUKE,
    MS_CONTROLLER_S,
    LIGHTGUN,
    STEERING_WHEEL,
    MEMORY_UNIT,
    IR_DONGLE,
    STEEL_BATTALION_CONTROLLER,
    ARCADE_STICK,
    DEVICE_MAX,
    // Devices with the HW_ prefix (= hardware) indicate a real xbox device. Always add these after DEVICE_MAX
    HW_STEEL_BATTALION_CONTROLLER,
    HW_XBOX_CONTROLLER,
}
XBOX_INPUT_DEVICE;

struct DeviceState {
    std::string port;
    int port_idx;
    XBOX_INPUT_DEVICE type;
    bool bPendingRemoval;
    bool bSignaled;
    char info[56];
    uint32_t padding[2];
};


HANDLE XInputOpen(int* a1, int a2)
{

    if (g_controllers[0].controller)
    {
        SetControllerInputDevice(&g_controllers[0]);
        auto base = g_memory.base;

        X86_STORE_U32(0x559984, (uint32_t)&g_controllers[0]);
    }
    return 0;
}


GUEST_FUNCTION_HOOK_STDCALL(sub_4248A6, input::GetState);
GUEST_FUNCTION_HOOK_STDCALL(sub_424919, input::SetState);
GUEST_FUNCTION_HOOK_STDCALL(sub_2AAC20, XInputOpen);

//GUEST_FUNCTION_HOOK_STDCALL(sub_423D43, XInitDevices);
//GUEST_FUNCTION_HOOK_STDCALL(sub_423B38, sub_423B38_r);
//GUEST_FUNCTION_HOOK_FASTCALL(sub_423E70, IsGamePadInValide);
//GUEST_FUNCTION_HOOK_THISCALL(sub_423E39, IUsbInit_GetMaxDeviceTypeCount);

//GUEST_FUNCTION_HOOK(sub_424276, sub_424276_r);
//GUEST_FUNCTION_HOOK_THISCALL(sub_4241C7, sub_4241C7_r);

