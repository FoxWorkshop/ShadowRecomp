#pragma once

struct RwV3d
{
	float x;
	float y;
	float z;
};

struct PlayerShadow
{
	int32_t vfTable;
	int32_t pos;
};

extern PlayerShadow* playerPtr[4];
extern RwV3d* playerPos[4];
extern uint32_t g_multiplayerPlayerCount;
extern bool isAwakeAboutToInvoke;