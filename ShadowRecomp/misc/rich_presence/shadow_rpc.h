#pragma once

// Discord Rich Presence for Shadow The Hedgehog.
//
// This is a straight 1:1 C++ port of Sewer's original C# Reloaded-II mod for Dolphin.
// https://github.com/Sewer56/ShadowTheHedgehog.RPC.ReloadedII
//
// Differences from the original:
//   - The "offensive captions" setting is not ported.
//   - Uses Discord's `discord-rpc` library instead of the `DiscordRichPresence` NuGet package.

namespace RichPresence::ShadowRpc
{
    // Starts updating the presence in the background, if enabled in the config.
    void Start();

    // Stops updating the presence and clears it from Discord.
    void Stop();

    // Starts or stops the presence after `Config::DiscordRichPresence` changes.
    void ConfigValueChangedCallback();
}
