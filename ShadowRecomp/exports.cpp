#include <audio/embedded_player.h>
#include <kernel/function.h>
#include <ui/game_window.h>

void Game_PlaySound(const char* pName)
{
    if (EmbeddedPlayer::s_isActive)
    {
        EmbeddedPlayer::Play(pName);
    }
    else
    {
        // to do, use the game's own sound system 0x292DAC
    }

}