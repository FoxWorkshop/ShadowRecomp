#include <audio/audio.h>
#include <audio/embedded_player.h>
#include <user/config.h>


#include <res/music/installer.ogg.h>
#include <res/sounds/sys_menu_ok.ogg.h>
#include <res/sounds/sys_menu_cancel.ogg.h>
#include <res/sounds/sys_menu_cursor.ogg.h>
#include <res/sounds/sys_menu_invalid.ogg.h>


enum class EmbeddedSound
{
    WindowOpen,
    WindowClose,
    Cursor,
    Decide,
    MainDecide,
    CannotDecide,
    CancelDecide,
    Count
};



struct EmbeddedSoundData
{
    Mix_Chunk* chunk{};
};

static std::array<EmbeddedSoundData, size_t(EmbeddedSound::Count)> g_embeddedSoundData = {};
static const std::unordered_map<std::string_view, EmbeddedSound> g_embeddedSoundMap =
{
    { "window_open", EmbeddedSound::WindowOpen },
    { "window_close", EmbeddedSound::WindowClose },
    { "cursor", EmbeddedSound::Cursor },
    { "decide", EmbeddedSound::Decide },
    { "cancel", EmbeddedSound::CancelDecide },
    { "cannot_decide", EmbeddedSound::CannotDecide },
};


static size_t g_channelIndex;

static void PlayEmbeddedSound(EmbeddedSound s)
{
    EmbeddedSoundData& data = g_embeddedSoundData[size_t(s)];
    if (data.chunk == nullptr)
    {
        // The sound hasn't been created yet, create it and pick it.
        const void* soundData = nullptr;
        size_t soundDataSize = 0;
        switch (s)
        {
        case EmbeddedSound::Cursor:
            soundData = g_sys_menu_cursor;
            soundDataSize = sizeof(g_sys_menu_cursor);
            break;
        case EmbeddedSound::Decide:
            soundData = g_sys_menu_ok;
            soundDataSize = sizeof(g_sys_menu_ok);
            break;
        case EmbeddedSound::CannotDecide:
            soundData = g_sys_menu_invalid;
            soundDataSize = sizeof(g_sys_menu_invalid);
            break;
        case EmbeddedSound::CancelDecide:
            soundData = g_sys_menu_cancel;
            soundDataSize = sizeof(g_sys_menu_cancel);
            break;
        default:
            assert(false && "Unknown embedded sound.");
            return;
        }

        data.chunk = Mix_LoadWAV_RW(SDL_RWFromConstMem(soundData, soundDataSize), 1);
    }

    Mix_VolumeChunk(data.chunk, (Config::MasterVolume * Config::EffectsVolume * EmbeddedPlayer::EFFECTS_VOLUME) * MIX_MAX_VOLUME);
    Mix_PlayChannel(g_channelIndex % MIX_CHANNELS, data.chunk, 0);
    ++g_channelIndex;
}

static Mix_Music* g_installerMusic;

void EmbeddedPlayer::Init()
{
    Mix_OpenAudio(XAUDIO_SAMPLES_HZ, AUDIO_F32SYS, 2, 4096);
    g_installerMusic = Mix_LoadMUS_RW(SDL_RWFromConstMem(g_installer_music, sizeof(g_installer_music)), 1);

    s_isActive = true;
}

void EmbeddedPlayer::Play(const char* name)
{
    assert(s_isActive && "Playback shouldn't be requested if the Embedded Player isn't active.");

    auto it = g_embeddedSoundMap.find(name);
    if (it == g_embeddedSoundMap.end())
    {
        return;
    }

    PlayEmbeddedSound(it->second);
}

void EmbeddedPlayer::PlayMusic()
{
    if (!Mix_PlayingMusic())
    {
        Mix_PlayMusic(g_installerMusic, INT_MAX);
        Mix_VolumeMusic(Config::MasterVolume * Config::MusicVolume * MUSIC_VOLUME * MIX_MAX_VOLUME);
    }
}

void EmbeddedPlayer::FadeOutMusic()
{
    if (Mix_PlayingMusic())
        Mix_FadeOutMusic(1000);
}

void EmbeddedPlayer::Shutdown()
{
    for (EmbeddedSoundData& data : g_embeddedSoundData)
    {
        if (data.chunk != nullptr)
            Mix_FreeChunk(data.chunk);
    }

    Mix_HaltMusic();
    Mix_FreeMusic(g_installerMusic);

    Mix_CloseAudio();
    Mix_Quit();

    s_isActive = false;
}