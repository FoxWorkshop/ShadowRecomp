#include <stdafx.h>
#include <array>
#include <SDL3/SDL.h>
#include "input.h"
#include <kernel/function.h>
#include <string>
#include <app.h>
#include <sdl_events.h>
#include <os/logger.h>

input::EInputDevice input::g_inputDevice;
input::EInputDevice input::g_inputDeviceController;
input::EInputDeviceExplicit input::g_inputDeviceExplicit;

#define VIBRATION_TIMEOUT_MS 5000

// XINPUT_GAMEPAD_TRIGGER_THRESHOLD
#define DIGITAL_BUTTON_THRESHOLD 30

using input::EXboxButton;

static const std::array<input::PadBindings, 4> g_padBindings =
{{
#define PAD_BINDINGS(player) \
	{ \
		&Config::Pad##player##P_A, &Config::Pad##player##P_B, &Config::Pad##player##P_X, &Config::Pad##player##P_Y, \
		&Config::Pad##player##P_LeftTrigger, &Config::Pad##player##P_RightTrigger, \
		&Config::Pad##player##P_Start, &Config::Pad##player##P_Back, \
		&Config::Pad##player##P_DPadUp, &Config::Pad##player##P_DPadDown, \
		&Config::Pad##player##P_DPadLeft, &Config::Pad##player##P_DPadRight, \
		&Config::Pad##player##P_White, &Config::Pad##player##P_Black, \
		&Config::Pad##player##P_LeftThumb, &Config::Pad##player##P_RightThumb \
	}
	PAD_BINDINGS(1),
	PAD_BINDINGS(2),
	PAD_BINDINGS(3),
	PAD_BINDINGS(4)
#undef PAD_BINDINGS
}};

const input::PadBindings& input::GetPadBindings(uint32_t playerIndex)
{
	return g_padBindings[std::min(playerIndex, 3u)];
}

static void ApplyStickResponse(int16_t& x, int16_t& y, float deadzone, float sensitivity)
{
	if (deadzone <= 0.0f && sensitivity == 1.0f)
	{
		return;
	}

	auto fx = x / 32767.0f;
	auto fy = y / 32767.0f;
	auto magnitude = sqrtf(fx * fx + fy * fy);

	if (magnitude <= deadzone || magnitude == 0.0f)
	{
		x = 0;
		y = 0;
		return;
	}

	auto scale = ((magnitude - deadzone) / (1.0f - deadzone)) / magnitude * sensitivity;

	auto peak = std::max(fabsf(fx), fabsf(fy)) * scale;
	if (peak > 1.0f)
	{
		scale /= peak;
	}

	x = (int16_t)(fx * scale * 32767.0f);
	y = (int16_t)(fy * scale * 32767.0f);
}

bool IsInputAllowed()
{
	return GameWindow::s_isFocused || Config::AllowBackgroundInput;
}

bool IsInputDeviceController()
{
	return input::g_inputDevice != input::EInputDevice::Keyboard &&
		input::g_inputDevice != input::EInputDevice::Mouse;
}

class Controller
{
public:
	SDL_Gamepad* controller{};
	SDL_Joystick* joystick{};
	SDL_JoystickID id{};
	_XINPUT_GAMEPAD_OG state{};

	int index{};

	Controller() = default;

	explicit Controller(SDL_JoystickID instanceID) : Controller(SDL_OpenGamepad(instanceID))
	{
	}

	Controller(SDL_Gamepad* controller) : controller(controller)
	{
		if (!controller)
			return;

		joystick = SDL_GetGamepadJoystick(controller);
		id = SDL_GetJoystickID(joystick);
	}

	SDL_GamepadType GetControllerType() const
	{
		return SDL_GetGamepadType(controller);
	}

	input::EInputDevice GetInputDevice() const
	{
		switch (GetControllerType())
		{
		case SDL_GAMEPAD_TYPE_PS3:
		case SDL_GAMEPAD_TYPE_PS4:
		case SDL_GAMEPAD_TYPE_PS5:
			return input::EInputDevice::PlayStation;
		case SDL_GAMEPAD_TYPE_XBOX360:
		case SDL_GAMEPAD_TYPE_XBOXONE:
			return input::EInputDevice::Xbox;
		default:
			return input::EInputDevice::Unknown;
		}
	}

	const char* GetControllerName() const
	{
		auto result = SDL_GetGamepadName(controller);

		if (!result)
			return "Unknown Device";

		return result;
	}

	void Close()
	{
		if (!controller)
			return;

		SDL_CloseGamepad(controller);

		controller = nullptr;
		joystick = nullptr;
		id = 0;
	}

	bool CanPoll()
	{
		return controller;
	}

	uint8_t GetButtonValue(EPadButton button) const
	{
		switch (button)
		{
		case EPadButton::LeftTrigger:
			return (uint8_t)std::clamp(SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 128, 0, 255);
		case EPadButton::RightTrigger:
			return (uint8_t)std::clamp(SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 128, 0, 255);
		case EPadButton::None:
			return 0;
		default:
			return SDL_GetGamepadButton(controller, (SDL_GamepadButton)button) ? 0xFF : 0;
		}
	}

	void PollAxis(uint32_t playerIndex)
	{
		if (!CanPoll())
			return;

		auto& pad = state;

		pad.sThumbLX = SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_LEFTX);
		pad.sThumbLY = (int16_t)std::clamp(-(int32_t)SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_LEFTY),
			(int32_t)-32768, (int32_t)INT16_MAX); //clamp here, fix the int16 overflow with negative angle, this fix "press up" not working


		pad.sThumbRX = SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_RIGHTX);
		pad.sThumbRY = (int16_t)std::clamp(-(int32_t)SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_RIGHTY),
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

		float deadzone;
		float sensitivity;
		switch (playerIndex)
		{
		case 1:  deadzone = Config::StickDeadzone2P; sensitivity = Config::StickSensitivity2P; break;
		case 2:  deadzone = Config::StickDeadzone3P; sensitivity = Config::StickSensitivity3P; break;
		case 3:  deadzone = Config::StickDeadzone4P; sensitivity = Config::StickSensitivity4P; break;
		default: deadzone = Config::StickDeadzone1P; sensitivity = Config::StickSensitivity1P; break;
		}

		ApplyStickResponse(pad.sThumbLX, pad.sThumbLY, deadzone, sensitivity);
		ApplyStickResponse(pad.sThumbRX, pad.sThumbRY, deadzone, sensitivity);
	}

	void Poll(uint32_t playerIndex)
	{
		if (!CanPoll())
			return;

		auto& pad = state;
		auto& bindings = input::GetPadBindings(playerIndex);

		auto getValue = [&](EXboxButton button) { return GetButtonValue(bindings[(size_t)button]->Value); };
		auto isPressed = [&](EXboxButton button) { return getValue(button) > DIGITAL_BUTTON_THRESHOLD; };
		auto getDigitalValue = [&](EXboxButton button) -> uint8_t { return isPressed(button) ? 0xFF : 0; };

		pad.wButtons = 0;

		static constexpr std::pair<EXboxButton, uint16_t> digitalButtons[] =
		{
			{ EXboxButton::DPadUp, XINPUT_GAMEPAD_DPAD_UP },
			{ EXboxButton::DPadDown, XINPUT_GAMEPAD_DPAD_DOWN },
			{ EXboxButton::DPadLeft, XINPUT_GAMEPAD_DPAD_LEFT },
			{ EXboxButton::DPadRight, XINPUT_GAMEPAD_DPAD_RIGHT },
			{ EXboxButton::Start, XINPUT_GAMEPAD_START },
			{ EXboxButton::Back, XINPUT_GAMEPAD_BACK },
			{ EXboxButton::LeftThumb, XINPUT_GAMEPAD_LEFT_THUMB },
			{ EXboxButton::RightThumb, XINPUT_GAMEPAD_RIGHT_THUMB }
		};

		for (auto& [button, mask] : digitalButtons)
		{
			if (isPressed(button))
			{
				pad.wButtons |= mask;
			}
		}

		// touchpad is back if unbound
		auto isTouchpadBound = std::any_of(bindings.begin(), bindings.end(), [](auto* binding) { return binding->Value == EPadButton::Touchpad; });

		if (!isTouchpadBound && GetButtonValue(EPadButton::Touchpad))
		{
			pad.wButtons |= XINPUT_GAMEPAD_BACK;
		}

		pad.bAnalogButtons[Buttons_A] = getDigitalValue(EXboxButton::A);
		pad.bAnalogButtons[Buttons_B] = getDigitalValue(EXboxButton::B);
		pad.bAnalogButtons[Buttons_X] = getDigitalValue(EXboxButton::X);
		pad.bAnalogButtons[Buttons_Y] = getDigitalValue(EXboxButton::Y);
		pad.bAnalogButtons[Buttons_White] = getDigitalValue(EXboxButton::White);
		pad.bAnalogButtons[Buttons_Black] = getDigitalValue(EXboxButton::Black);
		pad.bAnalogButtons[Buttons_LT] = getValue(EXboxButton::LeftTrigger);
		pad.bAnalogButtons[Buttons_RT] = getValue(EXboxButton::RightTrigger);
	}

	void SetVibration(const XboxVibrationPacket& vibration)
	{
		if (!CanPoll())
			return;

		SDL_RumbleGamepad(controller, vibration.wLeftMotorSpeed, vibration.wRightMotorSpeed, VIBRATION_TIMEOUT_MS);
	}

	void SetLED(const uint8_t r, const uint8_t g, const uint8_t b) const
	{
		SDL_SetGamepadLED(controller, r, g, b);
	}
};

std::array<Controller, 4> g_controllers;
Controller* g_activeController;
uint32_t g_keyboardPort = 1;
std::atomic<bool> g_chaosControlHoldCheat = false; // remove before release, dev use only (not the same as the CC code)

class KeyboardController
{
public:
	_XINPUT_GAMEPAD_OG state{};

	static constexpr int16_t STICK_MAX = 32767;
	static constexpr int16_t STICK_MIN = -32768;

	void Poll()
	{
		const bool* keystate = SDL_GetKeyboardState(nullptr);

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

inline Controller* FindController(SDL_JoystickID which)
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
			LOGFN("Detected controller: {} (Unknown Controller Type)", controllerName);
		}
		else
		{
			LOGFN("Detected controller: {}", controllerName);
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
static SDL_JoystickID s_pendingActiveController = 0;
static std::atomic<bool> s_auraDirty = true;

static void SyncControllers()
{
	for (auto& controller : g_controllers)
	{
		if (!controller.controller || SDL_GamepadConnected(controller.controller))
		{
			continue;
		}
		if (g_activeController == &controller)
		{
			g_activeController = nullptr;
		}
		controller.Close();
	}

	auto gamepadCount = 0;
	auto gamepads = SDL_GetGamepads(&gamepadCount);

	for (int i = 0; i < gamepadCount; i++)
	{
		if (FindController(gamepads[i]))
		{
			continue;
		}

		const auto freeIndex = FindFreeController();

		if (freeIndex == -1)
		{
			break;
		}

		auto controller = Controller(gamepads[i]);
		if (controller.controller)
		{
			g_controllers[freeIndex] = controller;
		}
	}

	SDL_free(gamepads);

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
		pendingActive = std::exchange(s_pendingActiveController, (SDL_JoystickID)0);
	}


	if (devicesDirty)
		SyncControllers();

	if (stopVibration)
	{
		for (auto& controller : g_controllers)
			controller.SetVibration({ 0, 0 });
	}

	if (hideCursor && !GameWindow::s_isFullscreenCursorVisible)
		SDL_HideCursor();

	if (pendingActive != 0)
	{
		if (auto* controller = FindController(pendingActive))
			SetControllerInputDevice(controller);
	}

	ApplyChaosAura();
}

bool Input_OnSDLEvent(void*, SDL_Event* event)
{
	switch (event->type)
	{
	case SDL_EVENT_GAMEPAD_ADDED:
	case SDL_EVENT_GAMEPAD_REMOVED:
	{
		// opening or closing a pad could deadlock in specific situation so best to play safe
		std::lock_guard lock(s_deferredMutex);
		s_devicesDirty = true;

		break;
	}
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
	case SDL_EVENT_GAMEPAD_BUTTON_UP:
	case SDL_EVENT_GAMEPAD_AXIS_MOTION:
	{
		auto* controller = FindController(event->gdevice.which);

		if (!controller)
			break;

		if (g_activeController != controller)
		{
			if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION && abs(event->gaxis.value) <= 8000)
				break;

			std::lock_guard lock(s_deferredMutex);
			s_hideCursorForController = true;
			s_pendingActiveController = controller->id;
		}

		break;
	}

	case SDL_EVENT_KEY_DOWN:
		if (event->key.scancode == SDL_SCANCODE_F6 && (event->key.mod & SDL_KMOD_SHIFT) && !event->key.repeat)
		{
			g_keyboardPort = (g_keyboardPort % 4) + 1;
			LOGFN("Keyboard now controlling PORT {}", g_keyboardPort);
		}
		if (event->key.scancode == SDL_SCANCODE_F7 && !event->key.repeat)
		{
			g_chaosControlHoldCheat = !g_chaosControlHoldCheat;
		}
		input::g_inputDevice = input::EInputDevice::Keyboard;
		break;
	case SDL_EVENT_KEY_UP:
		input::g_inputDevice = input::EInputDevice::Keyboard;
		break;

	case SDL_EVENT_MOUSE_MOTION:
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP:
	{
		if (!GameWindow::IsFullscreen() || GameWindow::s_isFullscreenCursorVisible)
			SDL_ShowCursor();

		input::g_inputDevice = input::EInputDevice::Mouse;

		break;
	}

	case SDL_EVENT_WINDOW_FOCUS_LOST:
	{
		// Stop vibrating controllers on focus lost.
		std::lock_guard lock(s_deferredMutex);
		s_stopVibration = true;

		break;
	}
	}

	return false;
}

void input::Init()
{

	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_GAMECUBE, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS3, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED, "0");
	SDL_SetHint(SDL_HINT_JOYSTICK_ENHANCED_REPORTS, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_WII, "1");
	SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");

	SDL_InitSubSystem(SDL_INIT_EVENTS);
	SDL_AddEventWatch(Input_OnSDLEvent, nullptr);
	SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD);

	input::Update(); // just to ensure controller is available frame 1
}

input::EInputDevice input::GetPadDevice(uint32_t playerIndex)
{
	if (playerIndex >= g_controllers.size() || !g_controllers[playerIndex].controller)
	{
		return EInputDevice::Unknown;
	}
	return g_controllers[playerIndex].GetInputDevice();
}

void input::GetRawPadButtons(uint32_t playerIndex, PadButtonValues& values)
{
	values.fill(0);
	auto readPad = [&](const Controller& controller)
	{
		for (size_t i = 0; i < values.size(); i++)
		{
			values[i] = std::max(values[i], controller.GetButtonValue((EPadButton)i));
		}
	};

	if (playerIndex < g_controllers.size() && g_controllers[playerIndex].controller)
	{
		readPad(g_controllers[playerIndex]);
		return;
	}

	for (auto& controller : g_controllers)
	{
		if (controller.controller)
		{
			readPad(controller);
		}
	}
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
		// TODO: Remove before release
		if (dwUserIndex == 1 && g_chaosControlHoldCheat)
		{
			auto base = g_memory.base;
			X86_STORE_U32(0x5456C4, X86_LOAD_U32(0x5456C8));
		}
		// TODO: END Remove before release

		if (dwUserIndex >= 1 && dwUserIndex <= 4 && g_controllers[dwUserIndex - 1].controller)
		{
			g_controllers[dwUserIndex - 1].Poll(dwUserIndex - 1);
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
