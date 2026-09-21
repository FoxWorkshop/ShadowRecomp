#include <kernel/function.h>
#include <kernel/heap.h>
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


/* --GRIND RAIL SAMPLE PATCH-- */

// The black comet stages all load bank79.pac for sound effect, however on Xbox that bank shipped with 
// the regular grind rail sound in sample slots 0 and 1 instead of the organic grind rail instead.
// Thankfully that sound still exist on a different bank, so we can patch this by loading that bank
// and make the two wrong entries point at the right sound.

// Additionally, the Death Ruins organic rails has missing loop flag and points so the sound never loop.
// Bank71.pac is the only bank that still ships loop points properly for that sound, 
// so we will reuse its values to fix that.

// runtime sample table, filled by sub_25ECF0 and the play sfx functions.
static constexpr uint32_t g_SAMPLE_TABLE = 0x550318;
static constexpr uint32_t SAMPLE_BANK_STRIDE = 4096;
static constexpr uint32_t SAMPLE_ENTRY_STRIDE = 32;
static constexpr uint32_t SAMPLE_HEADER = 0x00; // WAVEFORMATEX, 32 bytes
static constexpr uint32_t SAMPLE_DATA = 0x04;
static constexpr uint32_t SAMPLE_SIZE = 0x08;
static constexpr uint32_t SAMPLE_FLAGS = 0x0C; // bit 0 = loop
static constexpr uint32_t SAMPLE_LOOP_START = 0x10;
static constexpr uint32_t SAMPLE_LOOP_LENGTH = 0x14;
static constexpr uint32_t SAMPLE_VALID = 0x1C; 

static constexpr uint32_t STAGE_BANK_SLOT = 7;
static constexpr uint32_t GRIND_LANDING = 0; // SE_Z0000_GRINDLANDING
static constexpr uint32_t GRIND_LOOP = 1; // SE_Z0001_GRINDING
static constexpr uint32_t GRIND2_LOOP = 3; // SE_Z0003_GRINDING2

static constexpr uint32_t ORGANIC_GRIND_SIZE = 82988; 
static constexpr uint32_t ORGANIC_GRIND_LOOP_START = 4;
static constexpr uint32_t ORGANIC_GRIND_LOOP_LENGTH = 82972;

static constexpr uint32_t g_FILE_LOADER = 0x537768; // sub_25F2F0 sets it up before any bank load
static constexpr char ORGANIC_GRIND_BANK[] = "sound/bank7b.pac"; 


static uint32_t LoadOrganicGrindBankSound()
{
	static uint32_t image = 0;

	if (image)
		return image;

	auto base = g_memory.base;

	const uint32_t loader = X86_LOAD_U32(g_FILE_LOADER);
	if (!loader)
		return 0;

	// build string path dynamically
	auto* path = static_cast<char*>(g_userHeap.Alloc(sizeof(ORGANIC_GRIND_BANK)));
	memcpy(path, ORGANIC_GRIND_BANK, sizeof(ORGANIC_GRIND_BANK));

	image = GuestToHostFunction<uint32_t, CallConvention::Thiscall>(sub_1A330, loader,
		g_memory.MapVirtual(path), 0);

	return image;
}

// bank layout: +0 count, +4 record table, +12 data start. 
// records are 16 bytes, +1 index, +4 offset
static uint32_t FindPacSample(uint32_t image, uint32_t index)
{
	auto base = g_memory.base;

	const uint32_t count = X86_LOAD_U32(image);
	const uint32_t records = image + X86_LOAD_U32(image + 0x4);
	const uint32_t data = image + X86_LOAD_U32(image + 0xC);

	for (uint32_t i = 0; i < count; ++i)
	{
		const uint32_t record = records + i * 16;

		if (X86_LOAD_U8(record + 1) == index)
			return data + X86_LOAD_U32(record + 4); // the 32 byte header, sample data follows it.
	}

	return 0;
}

static uint32_t StageBankEntry(uint32_t index)
{
	return g_SAMPLE_TABLE + STAGE_BANK_SLOT * SAMPLE_BANK_STRIDE + index * SAMPLE_ENTRY_STRIDE;
}

static void SetGrindLoopRegion(uint32_t entry)
{
	auto base = g_memory.base;

	X86_STORE_U32(entry + SAMPLE_FLAGS, X86_LOAD_U32(entry + SAMPLE_FLAGS) | 1);
	X86_STORE_U32(entry + SAMPLE_LOOP_START, ORGANIC_GRIND_LOOP_START);
	X86_STORE_U32(entry + SAMPLE_LOOP_LENGTH, ORGANIC_GRIND_LOOP_LENGTH);
}

static void PointEntryAtSample(uint32_t entry, uint32_t header)
{
	auto base = g_memory.base;

	X86_STORE_U32(entry + SAMPLE_HEADER, header);
	X86_STORE_U32(entry + SAMPLE_VALID, header);
	X86_STORE_U32(entry + SAMPLE_DATA, header + 32);
	X86_STORE_U32(entry + SAMPLE_SIZE, X86_LOAD_U32(header + 28));
}

static void InstallOrganicGrindSound()
{
	auto base = g_memory.base;

	// A mod may have already shipped a fixed bank79.
	if (X86_LOAD_U32(StageBankEntry(GRIND_LOOP) + SAMPLE_SIZE) == ORGANIC_GRIND_SIZE)
		return;

	const uint32_t image = LoadOrganicGrindBankSound();
	if (!image)
		return;

	const uint32_t landing = FindPacSample(image, GRIND_LANDING);
	const uint32_t loop = FindPacSample(image, GRIND_LOOP);

	if (!landing || !loop)
		return;

	PointEntryAtSample(StageBankEntry(GRIND_LANDING), landing);
	PointEntryAtSample(StageBankEntry(GRIND_LOOP), loop);

	// Bank7B has missing loop data for rail grinding
	SetGrindLoopRegion(StageBankEntry(GRIND_LOOP));
}

// wherever the organic grind shows up in a stage bank, check and fix loop points and flag if needed
static void RestoreGrindLoop(uint32_t index)
{
	auto base = g_memory.base;

	const uint32_t entry = StageBankEntry(index);

	if (!X86_LOAD_U32(entry + SAMPLE_VALID))
		return;

	if (X86_LOAD_U32(entry + SAMPLE_SIZE) != ORGANIC_GRIND_SIZE)
		return;

	if (X86_LOAD_U32(entry + SAMPLE_FLAGS) & 1)
		return;

	SetGrindLoopRegion(entry);
}

// InitSoundBank(slot, stageID)
X86_FUNC_IMPL(__imp__sub_25F420);
X86_FUNC(sub_25F420)
{
	const uint32_t slot = X86_LOAD_U32(ctx.esp.u32 + 0x4);
	const int32_t stage = int32_t(X86_LOAD_U32(ctx.esp.u32 + 0x8));

	__imp__sub_25F420(ctx, base);

	if (slot != STAGE_BANK_SLOT)
		return;

	// black comet levels
	if (stage == 601 || stage == 604 || stage == 700)
		InstallOrganicGrindSound();

	RestoreGrindLoop(GRIND_LOOP);
	RestoreGrindLoop(GRIND2_LOOP);
}

/* --END GRIND RAIL SAMPLE PATCH-- */
