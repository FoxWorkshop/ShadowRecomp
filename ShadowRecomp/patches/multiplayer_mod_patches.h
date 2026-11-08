#pragma once

struct X86Context;

void MultiplayerPatchesInit();
uint32_t GetMultiplayerPausePort();
void MultiplayerPatchesUpdate(X86Context& ctx, uint8_t* base);
bool RouteMenuAnyPlayerClose();
bool IsExtraPlayerMode();
float GetMultiplayerViewScale();
bool AdjustMultiplayerItemIcon(const X86Context& ctx);
uint32_t PopAwakeGlowClone(uint32_t awake);
