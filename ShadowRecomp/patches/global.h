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

extern PlayerShadow* playerPtr; // NOTE: This is last Shadow allocated, not necessarily P1!
extern RwV3d* playerPos; // NOTE: This is last Shadow allocated, not necessarily P1!
