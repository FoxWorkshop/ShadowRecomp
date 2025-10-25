
#define XINPUT_GAMEPAD_DPAD_UP        0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN      0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT      0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT     0x0008
#define XINPUT_GAMEPAD_START          0x0010
#define XINPUT_GAMEPAD_BACK           0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB     0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB    0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER  0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER 0x0200
#define XINPUT_GAMEPAD_A              0x1000
#define XINPUT_GAMEPAD_B              0x2000
#define XINPUT_GAMEPAD_X              0x4000
#define XINPUT_GAMEPAD_Y              0x8000

enum bAnalogBtnE
{
    Buttons_A,
    Buttons_B,
    Buttons_X,
    Buttons_Y,
    Buttons_Black,
    Buttons_White,
    Buttons_LT,
    Buttons_RT
};


typedef struct _XINPUT_GAMEPAD_OG
{
    uint16_t wButtons;         // DPad + Start/Back + Thumb buttons
    uint8_t  bAnalogButtons[8]; // A,B,X,Y,Black,White,LTrig,RTrig
    int16_t  sThumbLX;
    int16_t  sThumbLY;
    int16_t  sThumbRX;
    int16_t  sThumbRY;
} XINPUT_GAMEPAD_OG;

typedef struct _XINPUT_VIBRATION
{
    int16_t                                wLeftMotorSpeed;
    int16_t                                wRightMotorSpeed;
} XINPUT_VIBRATION, * PXINPUT_VIBRATION;

struct XINPUT_STATE
{
    unsigned int dwPacketNumber;
    _XINPUT_GAMEPAD_OG Gamepad;
};



namespace input
{
    enum class EInputDevice
    {
        Unknown,
        Keyboard,
        Mouse,
        Xbox,
        PlayStation
    };

    enum class EInputDeviceExplicit
    {
        Unknown,
        Xbox360,
        XboxOne,
        DualShock3,
        DualShock4,
        SwitchPro,
        Virtual,
        DualSense,
        Luna,
        Stadia,
        NvShield,
        SwitchJCLeft,
        SwitchJCRight,
        SwitchJCPair
    };

    extern EInputDevice g_inputDevice;
    extern EInputDevice g_inputDeviceController;
    extern EInputDeviceExplicit g_inputDeviceExplicit;

    void Init();

    uint32_t GetState(uint32_t dwUserIndex, XINPUT_STATE* pState);
    uint32_t SetState(uint32_t dwUserIndex, XINPUT_VIBRATION* pVibration);
}

