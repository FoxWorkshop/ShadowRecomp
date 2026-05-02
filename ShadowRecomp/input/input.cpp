#include <stdafx.h>
#include <array>
#include <SDL.h>
#include "input.h"
#include <kernel/function.h>
#include <string>
#include <app.h>
#include <sdl_events.h>

input::EInputDevice input::g_inputDevice;
input::EInputDevice input::g_inputDeviceController;
input::EInputDeviceExplicit input::g_inputDeviceExplicit;

#define TRANSLATE_INPUT(S, X) SDL_GameControllerGetButton(controller, S) << FirstBitLow(X)
#define VIBRATION_TIMEOUT_MS 5000

bool IsInputAllowed()
{
	return GameWindow::s_isFocused || Config::AllowBackgroundInput;
}

bool IsInputDeviceController()
{
	return input::g_inputDevice != input::EInputDevice::Keyboard &&
		input::g_inputDevice != input::EInputDevice::Mouse;
}


//todo implement vibration
class Controller
{
public:
	SDL_GameController* controller{};
	SDL_Joystick* joystick{};
	SDL_JoystickID id{ -1 };
	_XINPUT_GAMEPAD_OG state{};

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
		pad.sThumbLY = (int16_t)std::clamp(-(int32_t)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY),
			(int32_t)-32768, (int32_t)INT16_MAX); //clamp here, fix the int16 overflow with negative angle, this fix "press up" not working


		pad.sThumbRX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX);
		pad.sThumbRY = (int16_t)std::clamp(-(int32_t)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY),
			(int32_t)-32768, (int32_t)INT16_MAX);

		int16_t leftTrig = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
		int16_t rightTrig = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
		pad.bAnalogButtons[Buttons_LT] = (uint8_t)std::clamp(leftTrig / 128, 0, 255);
		pad.bAnalogButtons[Buttons_RT] = (uint8_t)std::clamp(rightTrig / 128, 0, 255);
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

		pad.bAnalogButtons[Buttons_White] = TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, XINPUT_GAMEPAD_LEFT_SHOULDER);
		pad.bAnalogButtons[Buttons_Black] = TRANSLATE_INPUT(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER);
		pad.bAnalogButtons[Buttons_A] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A) ? 0xFF : 0;
		pad.bAnalogButtons[Buttons_B] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B) ? 0xFF : 0;
		pad.bAnalogButtons[Buttons_X] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X) ? 0xFF : 0;
		pad.bAnalogButtons[Buttons_Y] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y) ? 0xFF : 0;

	}

	void SetVibration(const XboxVibrationPacket& vibration)
	{
		if (!CanPoll())
			return;

		SDL_GameControllerRumble(controller, vibration.wLeftMotorSpeed, vibration.wRightMotorSpeed, VIBRATION_TIMEOUT_MS);
	}

	void SetLED(const uint8_t r, const uint8_t g, const uint8_t b) const
	{
		SDL_GameControllerSetLED(controller, r, g, b);
	}
};

std::array<Controller, 4> g_controllers;
Controller* g_activeController;

class KeyboardController
{
public:
	_XINPUT_GAMEPAD_OG state{};

	static constexpr int16_t STICK_MAX = 32767;
	static constexpr int16_t STICK_MIN = -32768;

	void Poll()
	{
		const uint8_t* keystate = SDL_GetKeyboardState(nullptr);

		state.wButtons = 0;

		if (keystate[Config::Key_LeftStickUp])
			state.sThumbLY = STICK_MAX;
		else if (keystate[Config::Key_LeftStickDown])
			state.sThumbLY = STICK_MIN;
		else
			state.sThumbLY = 0;

		if (keystate[Config::Key_LeftStickLeft])
			state.sThumbLX = STICK_MIN;
		else if (keystate[Config::Key_LeftStickRight])
			state.sThumbLX = STICK_MAX;
		else
			state.sThumbLX = 0;

		if (keystate[SDL_SCANCODE_I])
			state.sThumbRY = STICK_MAX;
		else if (keystate[SDL_SCANCODE_K])
			state.sThumbRY = STICK_MIN;
		else
			state.sThumbRY = 0;

		if (keystate[SDL_SCANCODE_J])
			state.sThumbRX = STICK_MIN;
		else if (keystate[SDL_SCANCODE_L])
			state.sThumbRX = STICK_MAX;
		else
			state.sThumbRX = 0;

		if (keystate[Config::Key_Start])
			state.wButtons |= XINPUT_GAMEPAD_START;

		if (keystate[Config::Key_Y])
			state.bAnalogButtons[Buttons_Y] = 0xFF;
		else
			state.bAnalogButtons[Buttons_Y] = 0;

		if (keystate[Config::Key_B])
			state.bAnalogButtons[Buttons_B] = 0xFF;
		else
			state.bAnalogButtons[Buttons_B] = 0;

		if (keystate[Config::Key_A])
			state.bAnalogButtons[Buttons_A] = 0xFF;
		else
			state.bAnalogButtons[Buttons_A] = 0;

		if (keystate[Config::Key_X])
			state.bAnalogButtons[Buttons_X] = 0xFF;
		else
			state.bAnalogButtons[Buttons_X] = 0;

		if (keystate[Config::Key_LeftBumper])
			state.bAnalogButtons[Buttons_White] = 0xFF;
		else
			state.bAnalogButtons[Buttons_White] = 0;

		if (keystate[Config::Key_RightBumper])
			state.bAnalogButtons[Buttons_Black] = 0xFF;
		else
			state.bAnalogButtons[Buttons_Black] = 0;

		if (keystate[Config::Key_LeftTrigger])
			state.bAnalogButtons[Buttons_LT] = 0xFF;
		else
			state.bAnalogButtons[Buttons_LT] = 0;

		if (keystate[Config::Key_RightTrigger])
			state.bAnalogButtons[Buttons_RT] = 0xFF;
		else
			state.bAnalogButtons[Buttons_RT] = 0;
	}
};

KeyboardController g_keyboardController;

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

int controlLogSpam = 0;
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

		if (controlLogSpam < 20) // Limit log for plug controllers since tool like DS4 keep switching between 360 and ps4 type.
		{
			if (controllerType == input::EInputDeviceExplicit::Unknown)
			{
				printf("Detected controller: %s (Unknown Controller Type)\n", controllerName);
			}
			else
			{
				printf("Detected controller: %s\n", controllerName);
			}

			controlLogSpam++;
		}
	}
}


static void SetControllerChaosModeLED(Controller& controller, ELedAura mode)
{
	return;

	switch (mode)
	{
	case LED_AURA_NEUTRAL:
	default:
	{
		controller.SetLED(91, 91, 91);
	}
	break;
	case LED_AURA_HERO:
	{
		controller.SetLED(0, 94, 255);
	}
	break;
	case LED_AURA_VILAIN:
	{
		controller.SetLED(237, 0, 0);
	}
	break;
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
			SetControllerChaosModeLED(controller, App::s_chaosMode);

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
		if (!GameWindow::IsFullscreen() || GameWindow::s_isFullscreenCursorVisible)
			SDL_ShowCursor(SDL_ENABLE);

		input::g_inputDevice = input::EInputDevice::Mouse;

		break;
	}

	case SDL_WINDOWEVENT:
	{
		if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
		{
			// Stop vibrating controllers on focus lost.
			for (auto& controller : g_controllers)
				controller.SetVibration({ 0, 0 });

		}

		break;
	}


	case SDL_USER_CHAOSMODE:
	{
		for (auto& controller : g_controllers)
			SetControllerChaosModeLED(controller, (ELedAura)event->user.code);

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
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED, "0");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_WII, "1");
	SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
	SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0"); // Uses Button Labels. This hint is disabled for Nintendo Controllers.

	SDL_InitSubSystem(SDL_INIT_EVENTS);
	SDL_AddEventWatch(Input_OnSDLEvent, nullptr);

	SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);

}

uint32_t input::GetState(uint32_t dwUserIndex, XINPUT_STATE* pState)
{
	static uint32_t packet;

	if (!pState)
		return ERROR_BAD_ARGUMENTS;

	memset(pState, 0, sizeof(*pState));

	pState->dwPacketNumber = packet++;

	if (dwUserIndex == 1)
	{
		if (g_controllers[0].controller)
		{
			g_controllers[0].Poll();
			g_controllers[0].PollAxis();
			pState->Gamepad = g_controllers[0].state;
		}
		else
		{
			memset(&pState->Gamepad, 0, sizeof(pState->Gamepad));
		}

		g_keyboardController.Poll();
		auto& kbPad = g_keyboardController.state;
		auto& pad = pState->Gamepad;

		if (kbPad.sThumbLX != 0) pad.sThumbLX = kbPad.sThumbLX;
		if (kbPad.sThumbLY != 0) pad.sThumbLY = kbPad.sThumbLY;
		if (kbPad.sThumbRX != 0) pad.sThumbRX = kbPad.sThumbRX;
		if (kbPad.sThumbRY != 0) pad.sThumbRY = kbPad.sThumbRY;

		pad.wButtons |= kbPad.wButtons;

		for (int i = 0; i < 8; i++)
		{
			if (kbPad.bAnalogButtons[i] != 0)
				pad.bAnalogButtons[i] = kbPad.bAnalogButtons[i];
		}

		return ERROR_SUCCESS;
	}

	if (!g_activeController)
		return ERROR_DEVICE_NOT_CONNECTED;

	if (g_controllers[dwUserIndex - 1].controller)
	{
		g_controllers[dwUserIndex - 1].Poll();
		g_controllers[dwUserIndex - 1].PollAxis();
		pState->Gamepad = g_controllers[dwUserIndex - 1].state;
		return ERROR_SUCCESS;
	}

	return ERROR_DEVICE_NOT_CONNECTED;
}


uint32_t input::SetState(uint32_t dwUserIndex, XboxVibrationPacket* outPacket)
{
	if (!outPacket || !Config::Vibration.Value)
		return ERROR_SUCCESS;

	if (!g_activeController)
		return ERROR_DEVICE_NOT_CONNECTED;

	auto* packet = reinterpret_cast<XboxVibrationPacket*>(outPacket);
	if (g_controllers[dwUserIndex - 1].controller)
	{
		g_controllers[dwUserIndex - 1].SetVibration(*outPacket);
	}

	return ERROR_SUCCESS;
}


HANDLE XInputOpen(int* a1, int a2)
{
	auto base = g_memory.base;

	for (int i = 0; i < 4; i++)
	{
		if (g_controllers[i].controller)
		{
			SetControllerInputDevice(&g_controllers[i]);
		}

		g_controllers[i].index = i + 1;

		X86_STORE_U32(0x559984 + (4 * i), i + 1);
	}

	return 0;
}


GUEST_FUNCTION_HOOK_STDCALL(sub_4248A6, input::GetState);
GUEST_FUNCTION_HOOK_STDCALL(sub_424919, input::SetState);
GUEST_FUNCTION_HOOK_STDCALL(sub_2AAC20, XInputOpen);
