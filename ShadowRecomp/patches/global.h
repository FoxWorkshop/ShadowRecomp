#pragma once

typedef float RwReal;
#define RwUInt32 uint32_t
#define RwUInt16 uint16_t
#define RwInt16 int16_t
#define RwInt32 int32_t
#define RwUInt8 uint8_t


struct RwV3d
{
	float x;
	float y;
	float z;
};

enum RwMatrixFlag
{
	rwMATRIXINTERNALIDENTITY = 0x20000,
};

struct RwMatrixTag
{
	RwV3d right;
	RwMatrixFlag flags;
	RwV3d up;
	RwUInt32 pad1;
	RwV3d at;
	RwUInt32 pad2;
	RwV3d pos;
	RwUInt32 pad3;
};


struct GeoMesh
{
	float x, y, z;
	uint8_t r, g, b, a;
	float u, v;
};


struct WeightedMesh
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};


typedef RwMatrixTag RwMatrix;

struct PlayerShadow
{
	int32_t vfTable;
	int32_t pos;
};

extern PlayerShadow* playerPtr[4];
extern RwV3d* playerPos[4];
extern uint32_t g_multiplayerPlayerCount;
extern bool isAwakeAboutToInvoke;



struct RwObject
{
	RwUInt8 type;
	RwUInt8 subType;
	RwUInt8 flags;
	RwUInt8 privateFlags;
	xpointer<void> parent;
};

struct RpGeometry
{
	RwObject object;
	RwUInt32 flags;
	RwUInt16 lockedSinceLastInst;
	RwInt16 refCount;
	RwInt32 numTriangles;
	RwInt32 numVertices;
	RwInt32 numMorphTargets;
	RwInt32 numTexCoordSets;
	uint8_t matList[0xC];
	uint32_t triangles; //ptr
	uint32_t preLitLum; //RwRGBA* ptr
	uint32_t texCoords[8]; // RwTexCoords* ptr
	uint32_t mesh; // RpMeshHeader* ptr
	uint32_t repEntry; // RwResEntry* ptr
	uint32_t morphTarget; // RpMorphTarget* ptr
};
