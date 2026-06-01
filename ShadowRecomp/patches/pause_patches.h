#pragma once

void PausePatchesInit();
void DrawPauseText();

void RequestQuitOptionForPause();
void TogglePauseCSD(bool toggle);
void TogglePauseBossCSD(bool toggle);
const uint8_t GetAddedPauseItemsCount();