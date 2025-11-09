#pragma once

static constexpr int32_t FPS_MIN = 15;
static constexpr int32_t FPS_MAX = 60;

enum class ELanguage : uint32_t
{
    English = 1,
    Japanese,
    German,
    French,
    Spanish,
    Italian
};

class IConfigDef
{
public:
    virtual ~IConfigDef() = default;
    virtual bool IsHidden() = 0;
   // virtual void ReadValue(toml::v3::ex::parse_result& toml) = 0;
    virtual void MakeDefault() = 0;
    virtual std::string_view GetSection() const = 0;
    virtual std::string_view GetName() const = 0;
    virtual std::string GetNameLocalised(ELanguage language) const = 0;
    virtual std::string GetDescription(ELanguage language) const = 0;
    virtual bool IsDefaultValue() const = 0;
    virtual const void* GetValue() const = 0;
    virtual std::string GetValueLocalised(ELanguage language) const = 0;
    virtual std::string GetValueDescription(ELanguage language) const = 0;
    virtual std::string GetDefinition(bool withSection = false) const = 0;
    virtual std::string ToString(bool strWithQuotes = true) const = 0;
    virtual void GetLocaleStrings(std::vector<std::string_view>& localeStrings) const = 0;
    virtual void SnapToNearestAccessibleValue(bool searchUp) = 0;
};

#define CONFIG_LOCALE            std::unordered_map<ELanguage, std::tuple<std::string, std::string>>
#define CONFIG_ENUM_LOCALE(type) std::unordered_map<ELanguage, std::unordered_map<type, std::tuple<std::string, std::string>>>

#define CONFIG_CALLBACK(name)       if (name.Callback) name.Callback(&name)
#define CONFIG_LOCK_CALLBACK(name)  if (name.LockCallback) name.LockCallback(&name)
#define CONFIG_APPLY_CALLBACK(name) if (name.ApplyCallback) name.ApplyCallback(&name)

#define WINDOWPOS_CENTRED        0x2FFF0000

extern std::vector<IConfigDef*> g_configDefinitions;



enum class EVoiceLanguage : uint32_t
{
    English,
    Japanese
};

enum class EControllerIcons : uint32_t
{
    Auto,
    Xbox,
    PlayStation2,
    Gamecube
};

enum class EGraphicsAPI : uint32_t
{
    Auto,
#ifdef SHADOW_RECOMP_D3D12
    D3D12,
#endif
    Vulkan
};

enum class EAspectRatio : uint32_t
{
    Auto,
    Wide,
    Narrow,
    OriginalNarrow
};

enum class EWindowState : uint32_t
{
    Normal,
    Maximised
};

enum class ETripleBuffering : uint32_t
{
    Auto,
    On,
    Off
};


#define CONFIG_DECLARE(type, name)                                              static ConfigDef<type>       name;
#define CONFIG_DECLARE_HIDDEN(type, name)                                       static ConfigDef<type, true> name;

#define CONFIG_DEFINE(section, type, name, defaultValue)                        CONFIG_DECLARE(type, name)
#define CONFIG_DEFINE_HIDDEN(section, type, name, defaultValue)                 CONFIG_DECLARE_HIDDEN(type, name)
#define CONFIG_DEFINE_LOCALISED(section, type, name, defaultValue)              CONFIG_DECLARE(type, name)
#define CONFIG_DEFINE_ENUM(section, type, name, defaultValue)                   CONFIG_DECLARE(type, name)
#define CONFIG_DEFINE_ENUM_LOCALISED(section, type, name, defaultValue)         CONFIG_DECLARE(type, name)

class Config
{
public:

    static inline bool s_isCallbacksCreated;
    static inline bool Fullscreen = false;
    static inline int Monitor = 0;
    static inline EWindowState WindowState = EWindowState::Normal;
    static inline int WindowSize = -1;
    static inline int WindowX = WINDOWPOS_CENTRED;
    static inline int WindowY = WINDOWPOS_CENTRED;
    static inline int WindowWidth = 1280;
    static inline int WindowHeight = 720;
    static inline int32_t FPS = 60;
    static inline bool VSync = true;
    static inline bool DisableLowResolutionFontOnCustomUI = false;
    static std::filesystem::path GetConfigPath();

    static inline EGraphicsAPI GraphicsAPI = EGraphicsAPI::D3D12; //EGraphicsAPI::Auto;
    static inline std::string GraphicsDevice = "";

    static inline ETripleBuffering TripleBuffering = ETripleBuffering::Auto;
    static inline bool ShowFPS = true;//false;
    static inline uint32_t MaxFrameLatency = 2;
    static inline float Brightness = 0.5f;
    static inline bool DisableDWMRoundedCorners = false;
    static inline EAspectRatio AspectRatio = EAspectRatio::Auto;

    static void CreateCallbacks();
    static void Load();
    static void Config::Save();
};
