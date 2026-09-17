#include <kernel/function.h>
#include <audio/audio.h>
#include <audio/driver/sdl2_driver.h>
#include <user/config.h>

// It's no secret that this game has a lot of audio issues on the Xbox port
// Sounds not looping properly, Distance for 3D Sound tend to break, volume issue and so on.

// Goal Ring has a 3D Sound issue, the game has no 3D support for stereo sound so it fallbacks to a function that doesn't work at all
// We can patch this on the AudioXBOX class level to make it works regardless of sound type
// This was made with Goal Ring in mind, but shoud in theory work for any similar 3D sound issue in the game.

static constexpr uint32_t SLOT_FLAGS = 0x00; // bit 0x2 = near
static constexpr uint32_t SLOT_BUFFER = 0x08; // Guest key for the IDirectSoundBuffer
static constexpr uint32_t SLOT_VOLUME = 0x10; // centibels volume
static constexpr uint32_t SLOT_POSITION = 0x14; // 25EF70
static constexpr uint32_t SLOT_SOFTWARE_3D = 0x21; // flag when sample is stereo

// arbitrary check (todo could be checked at runtime to ensure accuracy?)
static constexpr float FULL_VOLUME_DISTANCE = 60.0f;
static constexpr float SILENT_DISTANCE = 400.0f;
static constexpr float NEAR_LATCH_DISTANCE = 300.0f;

// recreation of sub_25FD60's volume curve, it's identical ot the original (for volume >=99).
// we clamp the volume to fix the distance sound issue where it originally dissapears too early
static int32_t IsndVolumeToCentibels(float volume)
{
	if (volume > 100.0f)
		volume = 100.0f;

	constexpr float centibelsAt99 = -2301.03f; // 1000 * log10((-99 + 100) * 0.005)
	constexpr float centibelsFloor = -6400.0f;

	float centibels;
	if (volume >= -99.0f)
	{
		centibels = log10f((volume + 100.0f) * 0.005f) * 1000.0f;
	}
	else
	{
		float t = (volume + 99.0f) / (-127.0f + 99.0f);
		if (t > 1.0f)
			t = 1.0f;

		centibels = centibelsAt99 + t * (centibelsFloor - centibelsAt99);
	}

	if (centibels > 0.0f)
		centibels = 0.0f;

	if (centibels < centibelsFloor)
		centibels = centibelsFloor;

	return int32_t(centibels);
}

// thiscall so ecx = slot, stack = listener
X86_FUNC(sub_25FEC0)
{
	const uint32_t slot = ctx.ecx.u32;
	const uint32_t listener = X86_LOAD_U32(ctx.esp.u32 + 0x4);

	ctx.esp.u32 += 0x8; // handle esp return early so we don't have to worry about it later.

	if (slot == 0 || listener == 0)
		return;

	const uint32_t flags = X86_LOAD_U32(slot + SLOT_FLAGS);
	const uint32_t buffer = X86_LOAD_U32(slot + SLOT_BUFFER);

	if (X86_LOAD_U8(slot + SLOT_SOFTWARE_3D) == 0 || (flags & 2) != 0 || buffer == 0)
		return;

	const float dx = X86_LOAD_F32(listener + 0x0) - X86_LOAD_F32(slot + SLOT_POSITION + 0x0);
	const float dy = X86_LOAD_F32(listener + 0x4) - X86_LOAD_F32(slot + SLOT_POSITION + 0x4);
	const float dz = X86_LOAD_F32(listener + 0x8) - X86_LOAD_F32(slot + SLOT_POSITION + 0x8);
	const float distance = sqrtf(dx * dx + dy * dy + dz * dz);

	// clamp volume like the game originally do with IsndCallSE::GetOffsetVolume
	float volume;
	if (distance <= FULL_VOLUME_DISTANCE)
		volume = 0.0f;
	else if (distance >= SILENT_DISTANCE)
		volume = -127.0f;
	else
		volume = -((distance - FULL_VOLUME_DISTANCE) / (SILENT_DISTANCE - FULL_VOLUME_DISTANCE)) * 127.0f;


	if (distance < NEAR_LATCH_DISTANCE)
		X86_STORE_U32(slot + SLOT_FLAGS, flags | 4);
	else if (distance > SILENT_DISTANCE)
		X86_STORE_U32(slot + SLOT_FLAGS, flags & ~4u);

	const int32_t centibels = IsndVolumeToCentibels(volume);

	if (int32_t(X86_LOAD_U32(slot + SLOT_VOLUME)) == centibels)
		return;

	X86_STORE_U32(slot + SLOT_VOLUME, uint32_t(centibels));
	IDirectSoundBuffer_SetVolume(int32_t(buffer), centibels);
}


/* --MUSIC RACE ISSUE PATCH-- */

static constexpr uint32_t RNA_PORT = 0x50; // mwSnd port handle
static constexpr uint32_t PORT_IN_USE = 0x04;
static constexpr uint32_t PORT_BUFFER = 0x08; // used as a guest key for IDirectSoundBuffer
static constexpr uint32_t MAX_PLAY_RETRIES = 16; // about 0.27s at 60Hz

static std::unordered_map<uint32_t, uint32_t> g_rnaPlayRetries;
static std::mutex g_rnaPlayRetriesMutex;

static bool RnaPortIsPlaying(uint32_t rna)
{
	auto base = g_memory.base;

	const uint32_t port = X86_LOAD_U32(rna + RNA_PORT);
	if (!port)
		return false;

	// both are still empty while mwSndOpenPort runs, and the play gives up on them
	if (!X86_LOAD_U32(port + PORT_IN_USE))
		return false;

	const uint32_t key = X86_LOAD_U32(port + PORT_BUFFER);
	if (!key)
		return false;

	// also catches a dead key, or a play that never ran because SetFrequency failed first
	auto* buffer = GetCurrentSoundBuffer(int32_t(key));

	if (!buffer)
		return false;

	uint32_t status = 0;
	return (buffer->GetStatus(&status) & X_DSBSTATUS_PLAYING) != 0;
}

// true means we skip it, so the game asks for the play again next vblank call
bool RnaPlayStateLatchMidAsmHook(X86Register& esi)
{
	const uint32_t rna = esi.u32;

	std::lock_guard<std::mutex> lock(g_rnaPlayRetriesMutex);

	if (RnaPortIsPlaying(rna))
	{
		g_rnaPlayRetries.erase(rna);
		return false;
	}

	uint32_t& retries = g_rnaPlayRetries[rna];
	if (retries >= MAX_PLAY_RETRIES)
	{
		// give up and mark it like the game would, instead of asking all stage long
		g_rnaPlayRetries.erase(rna);
		return false;
	}

	++retries;
	return true;
}

/* --END MUSIC RACE ISSUE PATCH-- */

bool IdleChaosPowersMusicMidAsmHook()
{
	return !Config::IdleChaosPowersMusic;
}
