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

	void PollAxis(uint32_t playerIndex)
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

		auto horizontalCamera = ECameraRotationMode::Normal;
		switch (playerIndex)
		{
		case 1:  horizontalCamera = Config::HorizontalCamera2P; break;
		case 2:  horizontalCamera = Config::HorizontalCamera3P; break;
		case 3:  horizontalCamera = Config::HorizontalCamera4P; break;
		default: horizontalCamera = Config::HorizontalCamera1P; break;
		}

		if (horizontalCamera == ECameraRotationMode::Reverse)
			pad.sThumbRX = (int16_t)std::clamp(-(int32_t)pad.sThumbRX, (int32_t)-32768, (int32_t)INT16_MAX);

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

		pad.bAnalogButtons[Buttons_White] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) ? 0xFF : 0;
		pad.bAnalogButtons[Buttons_Black] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) ? 0xFF : 0;
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
uint32_t g_keyboardPort = 1;

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

		if (keystate[Config::Key_RightStickUp])
			state.sThumbRY = STICK_MAX;
		else if (keystate[Config::Key_RightStickDown])
			state.sThumbRY = STICK_MIN;
		else
			state.sThumbRY = 0;

		if (keystate[Config::Key_RightStickLeft])
			state.sThumbRX = STICK_MIN;
		else if (keystate[Config::Key_RightStickRight])
			state.sThumbRX = STICK_MAX;
		else
			state.sThumbRX = 0;

		if (keystate[Config::Key_Start])
			state.wButtons |= XINPUT_GAMEPAD_START;

		if (keystate[Config::Key_DPadUp])
			state.wButtons |= XINPUT_GAMEPAD_DPAD_UP;
		if (keystate[Config::Key_DPadDown])
			state.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
		if (keystate[Config::Key_DPadLeft])
			state.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
		if (keystate[Config::Key_DPadRight])
			state.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;

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
	if (!controller.controller)
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
	case LED_AURA_VILLAIN:
	{
		controller.SetLED(237, 0, 0);
	}
	break;
	}
}

// ds4 / ds5 led color aura fixes
// uses mutex to avoid deadlock
// when touching goal ring, the async / event thread start to be used which can easily deadlock with SDLs stuff
static std::mutex s_deferredMutex;
static bool s_devicesDirty = true;
static bool s_stopVibration = false;
static bool s_hideCursorForController = false;
static SDL_JoystickID s_pendingActiveController = -1;
static std::atomic<bool> s_auraDirty = true;

static void SyncControllers()
{
	for (auto& controller : g_controllers)
	{
		if (!controller.controller || !SDL_GameControllerGetAttached(controller.controller))
			continue;


		if (g_activeController == &controller)
			g_activeController = nullptr;

		controller.Close();
	}

	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{

		if (!SDL_IsGameController(i))
			continue;

		auto controller = Controller(i);

		if (!controller.controller || FindController(SDL_JoystickGetDeviceInstanceID(i)))
			continue;


		const auto freeIndex = FindFreeController();

		if (freeIndex == -1)
			break;

		g_controllers[freeIndex] = Controller(i);

	}

	// a new pad was added which has no aura yet
	s_auraDirty = true;
}

static void ApplyChaosAura()
{
	if (!Config::ChaosPowerLEDSync.Value)
	{
		return;
	}

	static ELedAura s_appliedAura = LED_AURA_NEUTRAL;
	const auto aura = App::s_chaosMode.load(std::memory_order_relaxed);
	const bool forced = s_auraDirty.exchange(false);
	if (!forced && aura == s_appliedAura)
	{
		return;
	}

	s_appliedAura = aura;
	for (auto& controller : g_controllers)
	{
		SetControllerChaosModeLED(controller, aura);
	}
}

void input::SetChaosAura(ELedAura mode)
{
	if ((int)mode < LED_AURA_NEUTRAL || (int)mode > LED_AURA_VILLAIN)
		mode = LED_AURA_NEUTRAL;

	App::s_chaosMode.store(mode, std::memory_order_relaxed);
}

void input::Update()
{
	bool devicesDirty, stopVibration, hideCursor;
	SDL_JoystickID pendingActive;

	{
		std::lock_guard<std::mutex> lock(s_deferredMutex);

		devicesDirty = std::exchange(s_devicesDirty, false);
		stopVibration = std::exchange(s_stopVibration, false);
		hideCursor = std::exchange(s_hideCursorForController, false);
		pendingActive = std::exchange(s_pendingActiveController, (SDL_JoystickID)-1);
	}


	if (devicesDirty)
		SyncControllers();

	if (stopVibration)
	{
		for (auto& controller : g_controllers)
			controller.SetVibration({ 0, 0 });
	}

	if (hideCursor && !GameWindow::s_isFullscreenCursorVisible)
		SDL_ShowCursor(SDL_DISABLE);

	if (pendingActive != -1)
	{
		if (auto* controller = FindController(pendingActive))
			SetControllerInputDevice(controller);
	}

	ApplyChaosAura();
}

int Input_OnSDLEvent(void*, SDL_Event* event)
{
	switch (event->type)
	{
	case SDL_CONTROLLERDEVICEADDED:
	case SDL_CONTROLLERDEVICEREMOVED:
	{
		// opening or closing a pad could deadlock in specific situation so best to play safe
		std::lock_guard lock(s_deferredMutex);
		s_devicesDirty = true;

		break;
	}
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
	case SDL_CONTROLLERAXISMOTION:
	{
		auto* controller = FindController(event->cdevice.which);

		if (!controller)
			break;

		if (g_activeController != controller)
		{
			if (event->type == SDL_CONTROLLERAXISMOTION && abs(event->caxis.value) <= 8000)
				break;

			std::lock_guard lock(s_deferredMutex);
			s_hideCursorForController = true;
			s_pendingActiveController = controller->id;
		}

		break;
	}

	case SDL_KEYDOWN:
		if (event->key.keysym.scancode == SDL_SCANCODE_F6 && event->key.repeat == 0)
		{
			g_keyboardPort = (g_keyboardPort % 4) + 1;
			printf("Keyboard now controlling PORT %d\n", g_keyboardPort);
		}
		if (event->key.keysym.scancode == SDL_SCANCODE_F7 && event->key.repeat == 0)
		{
			auto base = g_memory.base;
			X86_STORE_F32(0x5456C4, 30000.0f);
		}
		input::g_inputDevice = input::EInputDevice::Keyboard;
		break;
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
			std::lock_guard lock(s_deferredMutex);
			s_stopVibration = true;
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
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED, "0");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_WII, "1");
	SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
	SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0"); // Uses Button Labels. This hint is disabled for Nintendo Controllers.

	SDL_InitSubSystem(SDL_INIT_EVENTS);
	SDL_AddEventWatch(Input_OnSDLEvent, nullptr);

	SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);

	input::Update(); // just to ensure controller is available frame 1
}

uint32_t input::GetState(uint32_t dwUserIndex, XINPUT_STATE* pState)
{
	static uint32_t packet;

	if (!pState)
		return ERROR_BAD_ARGUMENTS;

	memset(pState, 0, sizeof(*pState));

	pState->dwPacketNumber = packet++;

	bool hasController = false;
	bool isConnected = dwUserIndex == 1
		|| dwUserIndex == g_keyboardPort
		|| (dwUserIndex >= 1 && dwUserIndex <= 4 && g_controllers[dwUserIndex - 1].controller)
		|| g_activeController != nullptr;

	if (IsInputAllowed())
	{
		if (dwUserIndex >= 1 && dwUserIndex <= 4 && g_controllers[dwUserIndex - 1].controller)
		{
			g_controllers[dwUserIndex - 1].Poll();
			g_controllers[dwUserIndex - 1].PollAxis(dwUserIndex - 1);
			pState->Gamepad = g_controllers[dwUserIndex - 1].state;
			hasController = true;
		}

		if (dwUserIndex == g_keyboardPort)
		{
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

		if (hasController || dwUserIndex == 1)
			return ERROR_SUCCESS;
	}

	return isConnected ? ERROR_SUCCESS : ERROR_DEVICE_NOT_CONNECTED;
}


uint32_t input::SetState(uint32_t dwUserIndex, XboxVibrationPacket* outPacket)
{
	if (!outPacket)
		return ERROR_SUCCESS;

	bool vibrationEnabled = true;
	switch (dwUserIndex)
	{
	case 2:  vibrationEnabled = Config::Vibration2P.Value; break;
	case 3:  vibrationEnabled = Config::Vibration3P.Value; break;
	case 4:  vibrationEnabled = Config::Vibration4P.Value; break;
	default: vibrationEnabled = Config::Vibration1P.Value; break;
	}

	if (!vibrationEnabled)
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
