#include <kernel/function.h>
#include <user/config.h>
#include <gpu/video.h>
#include <app.h>
#include <patches/global.h>
#include <patches/patch_utils.h>
#include <gpu/frame_pacing/frame_pacer.h>
#include <patches/guest_variable.h>

static const GuestVariable<float> g_deltaTime{ 0x541FA8 };
static const GuestVariable<float> g_unroundedDeltaTime{ 0x541FAC }; // delta time pre-round (sub_56FC0)
static const GuestVariable<uint32_t> g_refRate{ 0x54DD80 };
static const GuestVariable<int32_t> g_frameRateDivisor{ 0x54DD40 };
static const GuestVariable<int32_t> g_maxFrames{ 0x54DD64 }; // Most frames' worth of time one frame can count for.

static bool g_isEventPlaying = false; // Note for future self, we CANNOT use this for audio detection

static bool IsCutsceneFramerateUnlocked()
{
	return Config::CutsceneFramerate == ECutsceneFramerate::Unlocked;
}

thread_local bool g_isStartingEventAudio = false;
void SetFramerateMidAsmHook()
{
	if (g_isEventPlaying && !IsCutsceneFramerateUnlocked())
	{
		return;
	}

	uint32_t refRate = (uint32_t)GetTargetFPS();
	g_refRate = refRate;
	if (g_isEventPlaying == false)
	{
		// == Sewer note ==
		//
		// The original game has a mechanic where if the framerate is
		// `fpsMax / 3`; then the game slows down to accomodate; and treats
		// it as if `fpsMax / 3` time has passed.
		//
		// That `fpsMax / 3` is 50ms on consoles.
		//
		// This is presumably to ensure that physics don't break at low
		// framerates, as e.g. getting a half second pause/stutter could
		// lead to clipping through a wall.
		//
		// When FPS drops below `fpsMax / 3`, the game compensates by slowing
		// down the time accordingly in order to 'keep up'.
		//
		// But on PC this doesn't make much sense; as we can do several 100s of
		// frames a second. If someone were to however, remove their FPS cap,
		// by setting it to 9999 FPS for instance, the game would be laggy;
		// because it can't hit `fpsMax / 3`.
		//
		// So we recalculate this integer divisor value to the nearest approximation
		// that preserves '20FPS' (50ms) as the slowdown point.

		constexpr uint32_t maxLagMs = 50;
		double framesPerMs = refRate / 1000.0;
		double framesIn50Ms = framesPerMs * maxLagMs;
		uint32_t maxFrames = (uint32_t)std::ceil(framesIn50Ms);

		// Always allow at least one frame.
		maxFrames = std::max(maxFrames, 1u);
		g_maxFrames = maxFrames;
	}
	else
	{
		// matching TPjsEventActionManager audio sync of 1
		g_maxFrames = std::max(refRate, 1u);
	}
}

// TPjsEventActionManager::Exec
X86_FUNC_IMPL(__imp__sub_228740);
X86_FUNC(sub_228740)
{
	g_isEventPlaying = true;
	g_isStartingEventAudio = true; // This is different from g_isEventPlaying because of in-stage event invoke and chained event audio (last story)
	__imp__sub_228740(ctx, base);
	g_isStartingEventAudio = false;
}

// TPjsEventActionManager Destructor
X86_FUNC_IMPL(__imp__sub_2293E0);
X86_FUNC(sub_2293E0)
{
	g_isEventPlaying = false;
	__imp__sub_2293E0(ctx, base);

}

// Get the requested in-game framerate
// TPjsEventActionManager sets divisor to 2 for events
// 0 represents no internal framerate request (safe to do any limit)
float GetInternalFrameRate()
{
	uint32_t refRate = g_refRate;
	int32_t divisor = g_frameRateDivisor;
	if (refRate == 0 || divisor <= 0)
	{
		return 0.0f;
	}

	float fps = (float)refRate / (float)divisor;
	return (fps >= 1.0f && fps <= (float)FPS_MAX) ? fps : 0.0f;
}

// Which frame a time falls in.
static int64_t TimeToFrame(FramePacer::Clock::duration time, int64_t refRate)
{
	int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();

	// Whole seconds first, so the multiply can't overflow.
	int64_t seconds = ns / 1000000000;
	int64_t leftoverNs = ns % 1000000000;

	int64_t framesInSeconds = seconds * refRate;
	int64_t framesInLeftover = leftoverNs * refRate / 1000000000;
	return framesInSeconds + framesInLeftover;
}

// When a frame starts, rounded up to the next nanosecond.
static FramePacer::Clock::duration FrameToTime(int64_t frame, int64_t refRate)
{
	// Whole seconds first, so the multiply can't overflow.
	int64_t seconds = frame / refRate;
	int64_t leftoverFrames = frame % refRate;

	int64_t secondsNs = seconds * 1000000000;
	int64_t leftoverNs = (leftoverFrames * 1000000000 + refRate - 1) / refRate; // round up
	return std::chrono::ceil<FramePacer::Clock::duration>(std::chrono::nanoseconds(secondsNs + leftoverNs));
}

// == Settings, flipped by other parts of the game ==

// e.g. 1 on loading screens and in cutscenes, 0 otherwise.
// NowLoading and TPjsEventActionManager turn it on.
static const GuestVariable<uint8_t> g_yieldEachFrame{ 0x54C770 };

// e.g. 1 on loading screens and during FMVs, 0 otherwise.
// Skips the wait. FMVs wait on the vblank instead; see g_movieManager.
static const GuestVariable<uint8_t> g_frameLimiterOff{ 0x54DD44 };

// e.g. always 0 as far as I can tell. No writers at runtime?? unused??
// Above 0, every frame lasts exactly this many seconds, whatever the clock says.
static const GuestVariable<float> g_fixedDeltaTime{ 0x541FB0 };

// e.g. 1 at boot. And then cleared on first frame.
// Maybe some sort of init flag. Weird nonetheless.
static const GuestVariable<uint8_t> g_frameClockReset{ 0x4EACDD };

// e.g. 1 while paused. TPauseManager sets it.
static const GuestVariable<uint8_t> g_pauseFrameCounters{ 0x5415A0 };

// e.g. a TMovieManager pointer while an FMV plays, 0 otherwise.
static const GuestVariable<uint32_t> g_movieManager{ 0x541808 };

// == How much time passed this frame. (See: sub_258340, it writes these bozos) ==
//
// e.g. Our last frame took (33.3ms), 2x the normal @ 60FPS
//   - g_refRate = 60 (FPS):
//   - g_frameStep = 2 (we passed 2 frames of time)
//   - g_scaledDeltaTime = 0.0333 (2/60 seconds , @ g_timeScale = 1.0)

// How many frames passed. Usually 1.
static const GuestVariable<int32_t> g_frameStep{ 0x54DD3C };

// e.g. 1.0 normally, 0 to freeze time.
static const GuestVariable<float> g_timeScale{ 0x54DD4C };
static const GuestVariable<float> g_timeScale2{ 0x54DD54 };

// Delta time in seconds, times the time scale above.
static const GuestVariable<float> g_scaledDeltaTime{ 0x54DD48 };
static const GuestVariable<float> g_scaledDeltaTime2{ 0x54DD50 };

// == Running totals. sub_258680 adds to these every frame ==

// e.g. 100 after 100 frames.
static const GuestVariable<int32_t> g_frameCount{ 0x54DDB8 };

// e.g. 60 after 100 frames with 40 paused. Stop while g_pauseFrameCounters is set.
static const GuestVariable<int32_t> g_unpausedFrameCount{ 0x54DDBC };
static const GuestVariable<int32_t> g_unpausedFrameCount2{ 0x54DDC0 };

// e.g. 1.6667 after 100 frames at 60 FPS. g_scaledDeltaTime2, added up.
static const GuestVariable<float> g_playTime{ 0x54DDC4 };
static const GuestVariable<float> g_playTime2{ 0x54DDC8 };

// The game's frame limiter, rewritten to use Sewer's frame pacer.
// (Hey! That's my ~~chaos emerald~~ framepacer! Eggman!)
//
// The original rounds each frame down to whole microseconds.
// That introduces some error:
//   - 60 FPS:  16666us a frame, 60.0024 FPS
//   - 240 FPS: 4166us a frame,  240.038 FPS
//   - 960 FPS: 1041us a frame,  960.615 FPS
//
//
// We can do a bit better!
//
// Instead, we work out when each frame *should* start
// from the time we started.
//
// e.g. at 60 FPS, counting from when the game started:
//   - Frame 1 starts at 16.666...ms
//   - Frame 2 starts at 33.333...ms
//   - Frame 3 starts at 50ms exactly
//   - Frame 60 starts at 1 second exactly
X86_FUNC(sub_56FC0)
{
	// kinda ripped from Ghidra with cleanup.
	using Clock = FramePacer::Clock;
	static const Clock::time_point start = Clock::now();
	static Clock::time_point lastTime = start;

	int64_t refRate = std::max<int64_t>(int32_t(g_refRate), 1); // we divide by it, nyo div by 0 >:3
	int32_t divisor = g_frameRateDivisor;
	int32_t maxFrames = g_maxFrames;

	// Handle the flags.
	if (g_frameClockReset)
	{
		g_frameClockReset = 0;
		lastTime = Clock::now();
	}

	if (g_yieldEachFrame)
	{
		ctx.SubEsp(1);
		sub_2ADDC9(ctx, base); // NtYieldExecution
	}

	if (g_movieManager)
	{
		ctx.SubEsp(1);
		sub_342960(ctx, base); // ends up in CRI_BlockUntilVerticalBlank
	}

	// Wait until `divisor` frames have passed.
	int64_t lastFrame = TimeToFrame(lastTime - start, refRate);
	if (!g_frameLimiterOff && divisor > 0)
	{
		static FramePacer pacer; // do the waitin'
		auto deadline = start + FrameToTime(lastFrame + divisor, refRate);
		pacer.SleepUntil(deadline, FramePacer::WaitMode::SleepThenSpin);
	}

	// Update the time.
	auto now = Clock::now();
	int64_t frames = TimeToFrame(now - start, refRate) - lastFrame;
	float deltaTime = float(double(frames) / refRate);
	float unroundedDeltaTime = float(std::chrono::duration<double>(now - lastTime).count());
	lastTime = now;

	float fixedDeltaTime = g_fixedDeltaTime;
	if (fixedDeltaTime > 0.0f)
		deltaTime = unroundedDeltaTime = fixedDeltaTime;

	// A frame counts for between 1 and maxFrames frames.
	float minDeltaTime = float(1.0 / refRate);
	float maxDeltaTime = float(double(maxFrames) / refRate);
	if (deltaTime > maxDeltaTime)
		deltaTime = maxDeltaTime;
	else if (deltaTime < minDeltaTime)
		deltaTime = minDeltaTime;

	g_deltaTime = deltaTime;
	g_unroundedDeltaTime = unroundedDeltaTime;

	// Pass the frame count and delta time on, to whatever the heck that does.
	X86_PUSH(ctx, base, std::bit_cast<uint32_t>(deltaTime));
	X86_PUSH(ctx, base, uint32_t(frames));
	ctx.ecx.u32 = g_frameStep.address;
	ctx.SubEsp(1);
	sub_258340(ctx, base);

	X86_PUSH(ctx, base, uint32_t(g_pauseFrameCounters));
	ctx.ecx.u32 = g_frameCount.address;
	ctx.SubEsp(1);
	sub_258680(ctx, base);

	ctx.esp.u32 += 4;
}

std::unordered_map<uint32_t, float> g_sfxTimers;
// SFX duration are hardcoded without using any kind of delta time
// We need to patch them so they don't cut too early
void SFXTimingMidAsmHook(X86Register &esi)
{
	auto base = g_memory.base;
	uint32_t timerAddr = esi.u32 + 0xc;
	uint16_t timer = X86_LOAD_U16(timerAddr) + 1; // restore og value that the game removed using hardcoded 60 fps.

	g_sfxTimers[esi.u32] += g_deltaTime * 60.f;
	int steps = (int)(g_sfxTimers[esi.u32] + 0.5f); // use extra 0.5 for rounding, avoid having 0.999960 that prevent step to pass.

	// Some sounds that loop uses a task that sets the timer back to 2 every frame.
	// This creates a side effect: any slow frame or wrong rounding can make the sound stop and play 
	// immediately again which acts like the sound played twice in a row.
	// Ensure to never go faster than 1 per frame to prevent that.
	// at 60 FPS or lower this is always 1 step
	steps = std::min(steps, 1);
	if (steps > 0)
	{
		g_sfxTimers[esi.u32] = std::min(g_sfxTimers[esi.u32] - steps, 1.0f);
		int result = (int)timer - steps;
		timer = (uint16_t)std::max(result, 0);
	}
	X86_STORE_U16(timerAddr, timer);
}

void FreeSFXMapMidAsmHook(X86Register& esi)
{
	g_sfxTimers.erase(esi.u32);
}

void FpsConfigValueChangedCallback(IConfigDef* config)
{
	if (config == &Config::FPS)
	{
		float rounded = std::roundf(Config::FPS.Value * 100.0f) / 100.0f;

		Config::FPS.Value = rounded;

		if (Config::FPS.Value < 0.01f)
			Config::FPS.Value = 0.0f;

		if (Config::FPS.Value > FPS_MAX)
			Config::FPS.Value = FPS_MAX;
	}
}

// Set while the jump board patch is replaying a launch collision pass with 60 FPS geometry.
// See the jump board patch further down
static int g_launchCollisionPass = 0;

// Collision patch globally, but sticking in here to check for the high FPS launch fix
// Fix ray to ground 'stick'
// Mainly for higher framerates, since even small slopes broke
// Unlike the original 61FPS+ patch, I scope to non-grounded (no more camera, snap, or slide Y push issue!)
// There are a few spots that originally break on 60 too (Space Gadget CC Orb area, Death Ruins spawn)
bool CollisionGroundStickMidAsmHook(X86Register& esi, X86Register& esp)
{
	auto base = g_memory.base;

	// Don't interfere with the Launch fix
	if (g_launchCollisionPass > 0)
	{
		return false;
	}

	const uint32_t player = X86_LOAD_U32(esi.u32);
	if (player == NULL)
	{
		return false;
	}

	// Behavior is already Grounded
	if ((X86_LOAD_U32(player + 0xa8) & 0x4) != 0)
	{
		return false;
	}

	// ray did not hit
	if (X86_LOAD_U8(esp.u32 + 0x3f) == 0)
	{
		return false;
	}

	// skip to second sweep
	return true;
}

// Common platforms that are used in many different stages
// the mode linear type movement don't use delta time so the platforms move way too fast above 60 fps
// we can patch this to cap the speed properly
void FootingMovableLinearMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const uint32_t this_ = esi.u32;
	const float speed = X86_LOAD_F32(this_ + 0x54); // degree
	const float phase = X86_LOAD_F32(this_ + 0x7C); // also degree
	const float frames = g_deltaTime * 60.0f;
	X86_STORE_F32(this_ + 0x7C, phase + speed * (frames - 1.0f));
}

// reel (pulley) goes with fixed speed instead of delta time

static const uint32_t g_upDownReelStep = 0x468A18; // 0.18f originally

static void UpDownReelPreBias(uint32_t this_, float sign)
{
	auto base = g_memory.base;
	const float frames = g_deltaTime * 60.f;
	const float bias = sign * X86_LOAD_F32(g_upDownReelStep) * (frames - 1.0f);
	X86_STORE_F32(this_ + 0x58, X86_LOAD_F32(this_ + 0x58) + bias);
}

void UpDownReelRaiseMidAsmHook(X86Register& edi)
{
	UpDownReelPreBias(edi.u32, 1.0f);
}

void UpDownReelLowerMidAsmHook(X86Register& edi)
{
	UpDownReelPreBias(edi.u32, -1.0f);
}

static const uint32_t g_upDownReelAngleRate = 0x54CD64;

void UpDownReelAngleMidAsmHook(X86Register& edi)
{
	auto base = g_memory.base;
	const float frames = g_deltaTime * 60.0f;
	const float angle = X86_LOAD_F32(edi.u32 + 0x7C);
	const float extra = X86_LOAD_F32(g_upDownReelAngleRate) * (frames - 1.0f);

	if (extra >= std::fabs(angle))
	{
		X86_STORE_F32(edi.u32 + 0x7C, 0.0f);
	}
	else
	{
		X86_STORE_F32(edi.u32 + 0x7C, angle - (angle >= 0.0f ? extra : -extra));
	}
}

void UpDownReelHangTurnMidAsmHook(X86Register& eax)
{
	const float frames = g_deltaTime * 60.0f;

	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const float rate = std::bit_cast<float>(eax.u32) * frames;
	eax.u32 = std::bit_cast<uint32_t>(rate);
}

/**	--SetStretchGrass Patch START-- */
// aka death ruin object that launch you 
// for some reason only the mode 5 of this object use delta time
// anything else doesn't so we have to patch the math

static const uint32_t g_stretchGrassChargeStep = 0x46A558; // 0.005f
static const uint32_t g_stretchGrassRecoverStep = 0x46A560; //0.42f
static const uint32_t g_stretchGrassDecayFactor = 0x46A608; // -0.93f

// for mode 2 of the object
// the charge builds with frame count and then launch gets stronger the higher the framerate goes
void StretchGrassChargeMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const uint32_t this_ = esi.u32;
	const float frames = g_deltaTime * 60.0f;
	const float step = X86_LOAD_F32(g_stretchGrassChargeStep);
	const float acc = X86_LOAD_F32(this_ + 0x4C) + step * frames;

	X86_STORE_F32(this_ + 0x4C, acc - step);
	X86_STORE_F32(this_ + 0x48, X86_LOAD_F32(this_ + 0x48) + acc * (frames - 1.0f));
}

// for mode 3 of the object
void StretchGrassRecoverMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const uint32_t self = esi.u32;
	const float frames = g_deltaTime * 60.0f;
	const float step = X86_LOAD_F32(g_stretchGrassRecoverStep);
	X86_STORE_F32(self + 0x48, X86_LOAD_F32(self + 0x48) - step * (frames - 1.0f));
}

// for mode 4 of the object
std::unordered_map<uint32_t, float> g_stretchGrassDecay;

void StretchGrassDecayMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const uint32_t this_ = esi.u32;
	const float factor = X86_LOAD_F32(g_stretchGrassDecayFactor);

	float &carry = g_stretchGrassDecay[this_];
	carry += g_deltaTime * 60.0f;

	// just to be safe
	int steps = std::min((int)carry, 128);
	carry -= steps;

	float value = X86_LOAD_F32(this_ + 0x48);

	if (steps == 0)
	{
		value /= factor; // the game's own multiply puts it straight back
	}
	else
	{
		for (int i = 1; i < steps; i++)
		{
			value *= factor;
		}
	}

	X86_STORE_F32(this_ + 0x48, value);


	if (std::fabs(value * factor) < 0.001f)
		g_stretchGrassDecay.erase(this_);
}


/**	--SetStretchGrass Patch END-- */

// rocket goes way too fast above 60 fps
// some part of the velocity logic uses raw value which increase above 60
// we can patch it with proper delta time
X86_FUNC_IMPL(__imp__sub_1AD1A0);
X86_FUNC(sub_1AD1A0)
{
	const uint32_t rocketPtr = ctx.ecx.u32; // thiscall
	const float frames = g_deltaTime * 60.0f;

	const float velX = X86_LOAD_F32(rocketPtr + 0x4C);
	const float velY = X86_LOAD_F32(rocketPtr + 0x50);
	const float velZ = X86_LOAD_F32(rocketPtr + 0x54);

	X86_STORE_F32(rocketPtr + 0x4C, velX * frames);
	X86_STORE_F32(rocketPtr + 0x50, velY * frames);
	X86_STORE_F32(rocketPtr + 0x54, velZ * frames);

	__imp__sub_1AD1A0(ctx, base);

	X86_STORE_F32(rocketPtr + 0x4C, velX);
	X86_STORE_F32(rocketPtr + 0x50, velY);
	X86_STORE_F32(rocketPtr + 0x54, velZ);
}

// Light in digital circuit that yeet Shadow
// the speed when getting launch is way too high above 60
// we can patch the scale value stored in the vector just before the launch to use delta time.
void ElecSearchLightPullMidAsmHook(X86Register &esp)
{
	auto base = g_memory.base;
	const uint32_t scaleAddr = esp.u32;
	float step = X86_LOAD_F32(scaleAddr) * (g_deltaTime * 60.0f);
	X86_STORE_F32(scaleAddr, std::min(step, 45.0f));
}

/** --ScatteringRing Patch START-- */

// Rings that drop on the floor when you get hit
// they go too fast above 60 fps so we adjust the physics to use delta time.

static const uint32_t g_scatteringRingDamp = 0x46806C; // 0.98437297f
static const uint32_t g_scatteringRingDrag = 0x46B380; // 0.1f
static const uint32_t g_scatteringRingGravity = 0x468030; // 0.12138f

void ScatteringRingPhysicsMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const uint32_t this_ = esi.u32;
	const float frames = g_deltaTime * 60.0f;

	if (!(frames >= 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
		return;

	const float damp = X86_LOAD_F32(g_scatteringRingDamp);
	const float drag = 1.0f - X86_LOAD_F32(g_scatteringRingDrag);
	const float gravity = X86_LOAD_F32(g_scatteringRingGravity);

	const float velX = X86_LOAD_F32(this_ + 0x54);
	const float velY = X86_LOAD_F32(this_ + 0x58);
	const float velZ = X86_LOAD_F32(this_ + 0x5C);

	const float dampN = std::pow(damp, frames);
	const float dragN = std::pow(drag, frames);
	const float terminal = -gravity / (1.0f - drag);
	const float endVelX = velX * dampN;
	const float endVelY = terminal + (velY - terminal) * dragN;
	const float endVelZ = velZ * dampN;

	// distance stuff
	const float sumDamp = damp * (1.0f - dampN) / (1.0f - damp);
	const float sumDrag = drag * (1.0f - dragN) / (1.0f - drag);
	const float travelX = velX * sumDamp;
	const float travelY = terminal * frames + (velY - terminal) * sumDrag;
	const float travelZ = velZ * sumDamp;

	X86_STORE_F32(this_ + 0x54, endVelX / damp);
	X86_STORE_F32(this_ + 0x58, (endVelY + gravity) / drag);
	X86_STORE_F32(this_ + 0x5C, endVelZ / damp);

	X86_STORE_F32(this_ + 0x48, X86_LOAD_F32(this_ + 0x48) + (travelX - endVelX));
	X86_STORE_F32(this_ + 0x4C, X86_LOAD_F32(this_ + 0x4C) + (travelY - endVelY));
	X86_STORE_F32(this_ + 0x50, X86_LOAD_F32(this_ + 0x50) + (travelZ - endVelZ));
}

// Simple visual fix so the rings rotate at normal speed at high fps.
static const uint32_t g_scatteringRingSpinPi = 0x47E0A4; // pi
static const uint32_t g_scatteringRingSpinDeg = 0x42A880; // 1/180
static const uint32_t g_scatteringRingSpinRate = 0x468054; // 1.5f

void ScatteringRingSpinMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const float frames = g_deltaTime * 60.f;
	const float spinPi = X86_LOAD_F32(g_scatteringRingSpinPi);
	const float spinDeg = X86_LOAD_F32(g_scatteringRingSpinDeg);
	const float spinRate = X86_LOAD_F32(g_scatteringRingSpinRate);
	const float stepRes = spinPi * spinDeg * spinRate;

	X86_STORE_F32(esi.u32 + 0x64, X86_LOAD_F32(esi.u32 + 0x64) + stepRes * (frames - 1.0f));
}

/** --ScatteringRing Patch END-- */

// Regular rings spin too fast just like their drop variation
static const uint32_t g_regularRingSpinStep = 0x467D00; // 0.01f
static const uint32_t g_regularRingSpinAmplitude = 0x467D04; // 0.05f

void RegularRingSpinMidAsmHook(X86Register& eax, X86Register& ebp)
{
	auto base = g_memory.base;
	const float frames = g_deltaTime * 60.0f;

	if (!(frames >= 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
		return;

	// eax is the current ring array while ebp is the current offset
	const uint32_t entry = eax.u32 + ebp.u32;
	const float step = X86_LOAD_F32(g_regularRingSpinStep);
	const float amplitude = X86_LOAD_F32(g_regularRingSpinAmplitude);
	const float phase = X86_LOAD_F32(entry + 0x24);
	const float extra = frames - 1.0f;

	const float sway = amplitude * (std::sin(extra * step * 0.5f) / std::sin(step * 0.5f)) * std::sin(phase + (extra + 1.0f) * step * 0.5f);
	X86_STORE_F32(entry + 0x24, phase + step * extra);
	X86_STORE_F32(entry + 0x1C, X86_LOAD_F32(entry + 0x1C) + sway);
}

/**	--Weight Patch START-- */

// The big black block that drops on the player.
// State 5 and 6 don't use delta time for some reason so we patch this as usual

static const uint32_t g_weightRiseSpeed = 0x46B438; // 1.0f
static const uint32_t g_weightFallSpeed = 0x468C74; // 2.5f

// eax is the transform the block is positioned through
void WeightRiseMidAsmHook(X86Register& eax)
{
	auto base = g_memory.base;
	const float frames = g_deltaTime * 60.0f;

	if (!(frames >= 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
		return;

	const float step = X86_LOAD_F32(g_weightRiseSpeed);
	X86_STORE_F32(eax.u32 + 0x8, X86_LOAD_F32(eax.u32 + 0x8) + step * (frames - 1.0f));
}

void WeightFallMidAsmHook(X86Register& eax)
{
	auto base = g_memory.base;
	const float frames = g_deltaTime * 60.0f;

	if (!(frames >= 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
		return;

	const float step = X86_LOAD_F32(g_weightFallSpeed);
	X86_STORE_F32(eax.u32 + 0x8, X86_LOAD_F32(eax.u32 + 0x8) - step * (frames - 1.0f));
}

/**	--Weight Patch END-- */

// 'ContainerBehavior' patch for fall speed
// Behavior is used by Containers with type Normal, Hard, Unbreak, Crate, Weapon, Secret (aka Shadow Box)
// ContainerBehavior
// 0x20 = Y
// 0x34 = Fall speed
void ContainerBehaviorFallMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const float frames = g_deltaTime * 60.0f;

	if (!(frames >= 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
		return;

	const uint32_t containerBehavior = esi.u32; // ContainerBehavior
	const float fallSpeed = X86_LOAD_F32(containerBehavior + 0x34); // applies per frame, usually static per container type

	X86_STORE_F32(containerBehavior + 0x20, X86_LOAD_F32(containerBehavior + 0x20) + fallSpeed * (frames - 1.0f));
}


/**	--Damage blink patch START-- */

// anything that the game would not treat as a valid remaining time should use the original logic
static bool isNotValidTimer(float timer)
{
	return (!(timer > 0.0f) || !std::isfinite(timer) || timer > 60.0f);
}

static bool DamageBlinkPhaseHidden(uint32_t player, uint32_t motion)
{
	auto base = g_memory.base;
	const float timeLeft = player != 0 ? X86_LOAD_F32(player + 0x27C) : 0.0f;

	if (isNotValidTimer(timeLeft))
		return (X86_LOAD_U32(motion + 0x3EC) & 1) != 0;

	return (static_cast<int>(timeLeft * 60.0f + 0.25f) & 1) != 0;
}

// ShadowMotion::Exec, the counter above still run and can be used here to patch the timing being wrong at > 60 fps
// False = run original code
bool ShadowMotionBlinkPhaseMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	return DamageBlinkPhaseHidden(X86_LOAD_U32(esi.u32 + 0x58), esi.u32);
}

// Partners have similar blink logic than Shadow
// False = run original code
bool NpcMotionBlinkPhaseMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	return DamageBlinkPhaseHidden(X86_LOAD_U32(esi.u32 + 0x458), esi.u32);
}

/* --Damage blink patch END-- */

/**	--EggFleet hit blink patch START-- */

// Sky Troops battleships and jewel (power devices thing) blink for a short time when getting hit, the timer is correctly delta timed,
// but the check to stop the blink isn't correct (& 2 moment) so it doesn't work.
// We can remake the sequence from the timer instead using 2 slices of 1/60s when it's on, then 2 when it's off as intended.
static bool HitTimerBlinkPhaseOff(uint32_t timer)
{
	auto base = g_memory.base;
	const float timeLeft = timer != 0 ? X86_LOAD_F32(timer) : 0.0f;

	if (isNotValidTimer(timeLeft))
		return (X86_LOAD_U32(0x54DDC0) & 2) == 0;

	return (static_cast<int>(timeLeft * 60.0f + 0.25f) & 2) == 0;
}

static bool EggFleetBlinkPhaseOff(uint32_t fleet)
{
	return HitTimerBlinkPhaseOff(fleet != 0 ? fleet + 0x44 : 0);
}

// EggFleet exec (sub_1C50A0)
// True = regular (0x1C5051)
// false = hit blink (0x1C504A)
bool EggFleetBlinkPhaseMidAsmHook(X86Register& esi)
{
	return EggFleetBlinkPhaseOff(esi.u32);
}

// True = regular (0x1C429F)
// false = hit blink (0x1C428A)
bool EggFleetPartBlinkPhaseMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	return EggFleetBlinkPhaseOff(X86_LOAD_U32(esi.u32 + 4));
}

// PowerDeviceNaked exec (sub_1C78E0)
// True = regular (0x1C7B0D)
// false = hit blink (0x1C7B06)
bool PowerDeviceBlinkPhaseMidAsmHook(X86Register& esi)
{
	return HitTimerBlinkPhaseOff(esi.u32 + 0x60);
}

/* --EggFleet hit blink patch END-- */

/**	--Enemy Damage Blink Patch START-- */

// How long enemies and bosses blink is correctly delta timed, however what toggle the flick is frame locked so it looks randomly wrong at high fps (similar to egg fleet).
// Fix: We use a custom timer which uses delta time and process it in 1/60s steps (slices).
struct EnemyBlinkPatch
{
	uint32_t lastFrame = 0;
	float slices = 0.0f;

	uint32_t Update(uint32_t frame)
	{
		if (frame != lastFrame)
		{
			lastFrame = frame;

			// 60 fps or less steps one slice per frame
			const float frames = g_deltaTime * 60.0f;
			slices += (frames > 0.0f && frames < 0.99f) ? frames : 1.0f;

			while (slices >= 4.0f) // lowkey arbitrary, but should be good enough me think
				slices -= 4.0f;
		}

		return static_cast<uint32_t>(slices + 0.25f);
	}
};


static bool AllFramesBlinkOdd()
{
	static EnemyBlinkPatch newPatchedTimer;
	return (newPatchedTimer.Update(g_frameCount) & 1) != 0;
}

static bool UnpausedBlinkOdd()
{
	static EnemyBlinkPatch newPatchedTimer;
	return (newPatchedTimer.Update(g_unpausedFrameCount2) & 1) != 0;
}

// Boss hit flash (MaterialBlinkTask::Exec)
// True = no tint (0x58721)
// false = tint (0x58706)
bool MaterialBlinkPhaseMidAsmHook()
{
	return !AllFramesBlinkOdd();
}

// Enemy hit blink function seems to hide the model on odd frames
// True = render (0xC6442)
// false = hide (0xC642E)
bool EnemyHitBlinkPhaseMidAsmHook()
{
	return !UnpausedBlinkOdd();
}

// Enemy death blink
// True = hide (0xCC775)
// false = render (0xCC76E)
bool EnemyDeathBlinkPhaseMidAsmHook()
{
	return !UnpausedBlinkOdd();
}

// BkLarva death blink
// True = render (0x137B6D)
// false = hide (0x137B60)
bool ObjectDeathBlinkPhaseMidAsmHook()
{
	return !UnpausedBlinkOdd();
}

// ThrowBombTask does flash the bomb between a full and dim red on odd frames
// True = full color (0x63192)
// false = dim (0x6316A)
bool ThrowBombBlinkPhaseMidAsmHook()
{
	return !UnpausedBlinkOdd();
}

// Shared model draw classes (0x256AEF, 0x2581BD, 0x26A846) blink on odd frames as well
bool ModelBlinkPhaseMidAsmHook()
{
	return !AllFramesBlinkOdd();
}

void SoldierFlashParityMidAsmHook(X86Register& ecx)
{
	ecx.u32 = AllFramesBlinkOdd() ? 1 : 0;
}

void SoldierFlashParityEdxMidAsmHook(X86Register& edx)
{
	edx.u32 = AllFramesBlinkOdd() ? 1 : 0;
}

/* --Enemy Damage Blink Patch END-- */

// Helper to match exponential smoothing for vehicles
static float ScaleFirstOrderLag(float before, float after, float retention, float frames)
{
	const float span = 1.0f - retention;
	if (!(span > 0.0f))
	{
		return after;
	}
	return before - (before - after) * ((1.0f - std::pow(retention, frames)) / span);
}


/**	--Vehicle shared fixed step START-- */
// AirSaucer and Walker/WalkerCannon (aka GUN Jump Mechs)
// Collide & slide (sub_24E780) is tuned for 60 fps steps
// Solve and mover are fixed at 60FPS and interpolate at higher FPS

static constexpr float g_vehicleStepDt = 1.0f / 60.0f;
static constexpr int g_vehicleMaxSteps = 4;

// collider+0x1C
// bit 0 = grounded
// bit 1 = kill surface
// bit 2 = landed
static constexpr uint32_t g_vehicleEdgeFlags = 0x6;

struct VehicleStep
{
	uint32_t collider = 0;
	double lastTime = 0.0;
	double phase = 1.0;
	float from[3] = {};
	float to[3] = {};
	float shownPos[3] = {};
	bool groundHidden = false;
	uint32_t realFlags = 0;
};

static std::unordered_map<uint32_t, VehicleStep> g_vehicleSteps;
static uint32_t g_steppedVehicle = 0;

static bool NeedsFpsRescale(float frames)
{
	return frames > 0.0f && std::fabs(frames - 1.0f) >= 0.0001f;
}

static void CallWithDt(X86Context& ctx, uint8_t* base, X86Func* func, uint32_t vehicle, float dt)
{
	X86Context call = ctx;
	call.ecx.u32 = vehicle;
	X86_PUSH(call, base, std::bit_cast<uint32_t>(dt));
	call.esp.u32 -= 4;
	func(call, base);
}

// replaces the solver call
static bool TryStepVehicle(X86Context& ctx, uint8_t* base, uint32_t colliderOffset, X86Func* solver, X86Func* mover, X86Func* afterMove, bool jumped)
{
	uint32_t vehicle = ctx.ecx.u32;
	uint32_t collider = X86_LOAD_U32(vehicle + colliderOffset);
	float dt = X86_LOAD_F32(ctx.esp.u32 + 4);
	float frames = dt * 60.0f;

	g_steppedVehicle = 0;
	if (collider == 0 || !NeedsFpsRescale(frames))
	{
		return false;
	}

	auto& step = g_vehicleSteps[vehicle];
	float pos[3] = { X86_LOAD_F32(vehicle + 0x4), X86_LOAD_F32(vehicle + 0x8), X86_LOAD_F32(vehicle + 0xC) };
	float accel[3] = { X86_LOAD_F32(vehicle + 0x28), X86_LOAD_F32(vehicle + 0x2C), X86_LOAD_F32(vehicle + 0x30) };

	// collider can change on mount and touching a rail
	if (step.collider != collider || App::s_time - step.lastTime > App::s_deltaTime * 1.5)
	{
		if (step.collider == collider && step.groundHidden)
		{
			X86_STORE_U32(collider + 0x1C, step.realFlags);
		}
		step = {};
		step.collider = collider;
		for (int i = 0; i < 3; i++)
		{
			step.from[i] = step.to[i] = step.shownPos[i] = pos[i];
		}
	}
	step.lastTime = App::s_time;

	// external forces (platforms, bumping etc)
	for (int i = 0; i < 3; i++)
	{
		float moved = pos[i] - step.shownPos[i];
		step.from[i] += moved;
		step.to[i] += moved;
	}

	int steps = 0;
	uint32_t edges = 0;
	step.phase += frames;
	while (step.phase > 1.0001 && steps < g_vehicleMaxSteps)
	{
		if (step.groundHidden)
		{
			X86_STORE_U32(collider + 0x1C, step.realFlags);
			step.groundHidden = false;
		}

		// match exit state
		for (int i = 0; i < 3; i++)
		{
			X86_STORE_F32(vehicle + 0x4 + i * 4, step.to[i]);
			X86_STORE_F32(vehicle + 0x64 + i * 4, step.to[i]);
			X86_STORE_F32(vehicle + 0x28 + i * 4, accel[i]);
		}

		CallWithDt(ctx, base, solver, vehicle, g_vehicleStepDt);
		CallWithDt(ctx, base, mover, vehicle, g_vehicleStepDt);
		if (afterMove != nullptr)
		{
			CallWithDt(ctx, base, afterMove, vehicle, g_vehicleStepDt);
		}

		for (int i = 0; i < 3; i++)
		{
			step.from[i] = step.to[i];
			step.to[i] = X86_LOAD_F32(vehicle + 0x4 + i * 4);
		}
		edges |= X86_LOAD_U32(collider + 0x1C) & g_vehicleEdgeFlags;
		step.phase -= 1.0;
		steps++;
	}
	step.phase = std::min(step.phase, 1.0);

	// landing and destroy flags
	uint32_t flags = (X86_LOAD_U32(collider + 0x1C) & ~g_vehicleEdgeFlags) | edges;

	// hide grounded to prevent jump skip
	if (steps == 0 && jumped && !step.groundHidden)
	{
		step.realFlags = flags;
		step.groundHidden = true;
		flags &= ~1u;
	}
	X86_STORE_U32(collider + 0x1C, flags);

	for (int i = 0; i < 3; i++)
	{
		step.shownPos[i] = step.from[i] + (step.to[i] - step.from[i]) * (float)step.phase;
		X86_STORE_F32(vehicle + 0x4 + i * 4, step.shownPos[i]);
	}

	g_steppedVehicle = vehicle;
	ctx.esp.u32 += 8;
	return true;
}

static bool SkipIfStepped(X86Context& ctx)
{
	if (ctx.ecx.u32 != g_steppedVehicle)
	{
		return false;
	}
	ctx.esp.u32 += 8;
	return true;
}

// only the turn cap is per frame
static void RescaleVehicleTurn(uint8_t* base, uint32_t yawAddr, float yawBefore, uint32_t dirAddr, float turn, float cap, float frames)
{
	if (X86_LOAD_F32(yawAddr) == yawBefore)
	{
		return;
	}

	// match the original turn code
	float delta = WrapAngle(std::atan2(-X86_LOAD_F32(dirAddr), -X86_LOAD_F32(dirAddr + 8)) - yawBefore);
	float t = std::min(turn / std::fabs(delta), 1.0f - std::pow(1.0f - cap, frames));
	X86_STORE_F32(yawAddr, WrapAngle(yawBefore + delta * t));
}
/**	--Vehicle shared fixed step END-- */

/**	--Vehicle::AirSaucer Patch START-- */
// Air Saucer
// Solver @ sub_24B6F0
// Mover @ sub_24B5F0
// Idle damp @ sub_24C190
// Grip & lean anchor @ sub_24BEC0
// Heading turn @ sub_24C600
// Tilt slerps @ sub_24B880 / sub_24BA90
static const uint32_t g_saucerGripMax = 0x471B4C;       // 0.96875f
static const uint32_t g_saucerGripPerSpeed = 0x42A85C;  // 0.01f
static const uint32_t g_saucerSteerDeadzone = 0x471AA8; // 0.0001f
static const uint32_t g_saucerGripStep = 0x46B380;      // 0.1f
static const uint32_t g_saucerTurnSpeed = 0x471B10;     // 250
static const uint32_t g_saucerLeanRate = 0x448E48;      // 15.0f
static const uint32_t g_saucerAlignRate = 0x46BF18;     // 7.5f
static const uint32_t g_saucerAnchorHeight = 0x471B3C;  // 5.0f
static const uint32_t g_saucerAnchorSlack = 0x471B54;   // 2.0f

// Mover
X86_FUNC_IMPL(__imp__sub_24B5F0);
X86_FUNC(sub_24B5F0)
{
	if (!SkipIfStepped(ctx))
	{
		__imp__sub_24B5F0(ctx, base);
	}
}

// Idle damp
X86_FUNC_IMPL(__imp__sub_24C190);
X86_FUNC(sub_24C190)
{
	if (!SkipIfStepped(ctx))
	{
		__imp__sub_24C190(ctx, base);
	}
}

// Solver
X86_FUNC_IMPL(__imp__sub_24B6F0);
X86_FUNC(sub_24B6F0)
{
	uint32_t saucer = ctx.ecx.u32;
	uint32_t state = X86_LOAD_U32(saucer + 0x17C);
	bool grounded = (X86_LOAD_U32(X86_LOAD_U32(saucer + 0x9C) + 0x1C) & 1) != 0;

	// states
	// 0 = no one on saucer (both enemy and player)
	// 3 and 4 = jump related
	X86Func* idleDamp = state == 0 ? __imp__sub_24C190 : nullptr;
	if (!TryStepVehicle(ctx, base, 0x9C, __imp__sub_24B6F0, __imp__sub_24B5F0, idleDamp, grounded && (state == 3 || state == 4)))
	{
		__imp__sub_24B6F0(ctx, base);
	}
}

// Grip & lean anchor
// grip and anchor lag are per frame
X86_FUNC_IMPL(__imp__sub_24BEC0);
X86_FUNC(sub_24BEC0)
{
	uint32_t saucer = ctx.ecx.u32;
	float dt = X86_LOAD_F32(ctx.esp.u32 + 4);
	float frames = dt * 60.0f;
	if (!NeedsFpsRescale(frames))
	{
		__imp__sub_24BEC0(ctx, base);
		return;
	}

	float vel[3] = { X86_LOAD_F32(saucer + 0x1C), X86_LOAD_F32(saucer + 0x20), X86_LOAD_F32(saucer + 0x24) };
	float stick[3] = { X86_LOAD_F32(saucer + 0x10C), X86_LOAD_F32(saucer + 0x110), X86_LOAD_F32(saucer + 0x114) };
	float gripMax = X86_LOAD_F32(g_saucerGripMax);
	float speed = std::sqrt(vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2]);
	float target = (1.0f - speed * X86_LOAD_F32(g_saucerGripPerSpeed)) * gripMax;
	float grip = X86_LOAD_F32(saucer + 0x154);

	// grip only drops while steering
	if (stick[0] * stick[0] + stick[1] * stick[1] + stick[2] * stick[2] >= X86_LOAD_F32(g_saucerSteerDeadzone))
	{
		target = std::min(target, grip);
	}
	grip = target + (grip - target) * std::pow(1.0f - X86_LOAD_F32(g_saucerGripStep), frames);
	grip = std::min(std::max(grip, 0.0f), gripMax);
	X86_STORE_F32(saucer + 0x154, grip);

	// copy sub_23E930
	uint32_t colliMove = X86_LOAD_U32(saucer + 0x7C);
	uint32_t floorVel = colliMove != 0 ? X86_LOAD_U32(colliMove + 0x28) + 0x98 : 0x5601C4;
	float chase = 1.0f - std::pow(grip, frames);
	float slack = X86_LOAD_F32(g_saucerAnchorSlack);

	for (int i = 0; i < 3; i++)
	{
		float goal = X86_LOAD_F32(saucer + 0x4 + i * 4) + (i == 1 ? X86_LOAD_F32(g_saucerAnchorHeight) : 0.0f) + vel[i] / 128.0f;
		float anchor = X86_LOAD_F32(saucer + 0x148 + i * 4) + X86_LOAD_F32(floorVel + i * 4) * dt;
		anchor += (goal - anchor) * chase;

		// clamp height gap
		if (i == 1)
		{
			float gap = goal - anchor;
			anchor += gap - std::min(std::max(gap, -slack), slack);
		}
		X86_STORE_F32(saucer + 0x148 + i * 4, anchor);
	}
	ctx.esp.u32 += 8;
}

// Heading turn
X86_FUNC_IMPL(__imp__sub_24C600);
X86_FUNC(sub_24C600)
{
	uint32_t saucer = ctx.ecx.u32;
	float dt = X86_LOAD_F32(ctx.esp.u32 + 4);
	uint32_t dirAddr = X86_LOAD_U32(ctx.esp.u32 + 8);
	float yawBefore = X86_LOAD_F32(saucer + 0x11C);

	__imp__sub_24C600(ctx, base);

	float frames = dt * 60.0f;
	if (NeedsFpsRescale(frames))
	{
		float turn = (1.0f - X86_LOAD_F32(saucer + 0x154)) * dt * X86_LOAD_F32(g_saucerTurnSpeed) * 0.017453294f;
		RescaleVehicleTurn(base, saucer + 0x11C, yawBefore, dirAddr, turn, X86_LOAD_F32(g_saucerGripStep), frames);
	}
}

// helper to swap dt for the 60 fps step
static void Set60FPSSlerpDt(X86Context& ctx, uint8_t* base, uint32_t rateAddr)
{
	uint32_t dtAddr = ctx.esp.u32 + 4;
	float frames = X86_LOAD_F32(dtAddr) * 60.0f;
	float rate = X86_LOAD_F32(rateAddr);
	if (NeedsFpsRescale(frames) && rate > 0.0f)
	{
		float step = std::min(rate / 60.0f, 1.0f);
		X86_STORE_F32(dtAddr, (1.0f - std::pow(1.0f - step, frames)) / rate);
	}
}

// Lean toward the anchor
X86_FUNC_IMPL(__imp__sub_24B880);
X86_FUNC(sub_24B880)
{
	Set60FPSSlerpDt(ctx, base, g_saucerLeanRate);
	__imp__sub_24B880(ctx, base);
}

// Align to the floor
X86_FUNC_IMPL(__imp__sub_24BA90);
X86_FUNC(sub_24BA90)
{
	Set60FPSSlerpDt(ctx, base, g_saucerAlignRate);
	__imp__sub_24BA90(ctx, base);
}
/**	--Vehicle::AirSaucer Patch END-- */

/**	--Vehicle::Walker Patch START-- */
// Walker / WalkerCannon (GUN Jump Mechs), same class
// Solver @ sub_2549F0
// Mover @ sub_2546B0
// Heading turn @ sub_255780
static const uint32_t g_walkerTurnSpeed = 0x4721F8; // 125
static const uint32_t g_walkerTurnCap = 0x46C3B8; // 0.5f

// Mover
X86_FUNC_IMPL(__imp__sub_2546B0);
X86_FUNC(sub_2546B0)
{
	if (!SkipIfStepped(ctx))
	{
		__imp__sub_2546B0(ctx, base);
	}
}

// Solver
X86_FUNC_IMPL(__imp__sub_2549F0);
X86_FUNC(sub_2549F0)
{
	// jump already clears grounded (sub_24E4D0)
	if (!TryStepVehicle(ctx, base, 0xA8, __imp__sub_2549F0, __imp__sub_2546B0, nullptr, false))
	{
		__imp__sub_2549F0(ctx, base);
	}
}

// Heading turn
// also steers the ground drive
X86_FUNC_IMPL(__imp__sub_255780);
X86_FUNC(sub_255780)
{
	uint32_t walker = ctx.ecx.u32;
	float dt = X86_LOAD_F32(ctx.esp.u32 + 4);
	float yawBefore = X86_LOAD_F32(walker + 0x14);

	__imp__sub_255780(ctx, base);

	float frames = dt * 60.0f;
	if (NeedsFpsRescale(frames))
	{
		float turn = dt * X86_LOAD_F32(g_walkerTurnSpeed) * 0.017453294f;
		RescaleVehicleTurn(base, walker + 0x14, yawBefore, walker + 0xF8, turn, X86_LOAD_F32(g_walkerTurnCap), frames);
	}
}
/**	--Vehicle::Walker Patch END-- */

/**	--Vehicle::TVehicleCar Patch START-- */
// TVehicleCar / CarType vehicles.
// Affects 'Norael' (yes that is not a typo lol) aka Normal Car [Jeep], Open Car [Sports Car], Armed Car [Armored GUN Car], Bike [GUN Motorcycle]
// This obj has a lot of weird "almost using deltatime" going on, so this patch is a bit of a mess.
// Issues are mostly exponential smoothing related
// It already uses gravity and steps properly, but there's a bunch of per-frame adjustments happening.

// Input low-pass @ sub_243BB0
// Axle velocity blend @ sub_246120 (tail)
// In-air velocity projection @ sub_245FB0 (tail)
// Sweep slide gate / resting pad @ sub_244CD0, fixed in sub_245FB0
// coast dead-stop that falls out of the gate @ sub_245FB0

// The byte flags record which held/released K the game used, so the lag can be carried.
struct VehicleInputChannel
{
	uint32_t value;
	uint32_t flag;
	uint32_t heldK;
	uint32_t releasedK;
};

static const VehicleInputChannel g_vehicleInputChannels[] =
{
	{ 0x58, 0x00, 0x51CC1C, 0x51CC1C }, // steering
	{ 0x64, 0x5E, 0x51CC20, 0x51CC24 }, // accelerate
	{ 0x68, 0x5F, 0x51CC30, 0x51CC34 }, // trigger
	{ 0x6C, 0x60, 0x51CC28, 0x51CC2C }, // brake / reverse
};

X86_FUNC_IMPL(__imp__sub_243BB0);
X86_FUNC(sub_243BB0)
{
	const uint32_t self = ctx.ecx.u32;
	float before[std::size(g_vehicleInputChannels)];

	for (size_t i = 0; i < std::size(g_vehicleInputChannels); i++)
	{
		before[i] = X86_LOAD_F32(self + g_vehicleInputChannels[i].value);
	}

	__imp__sub_243BB0(ctx, base);

	const float frames = g_deltaTime * 60.0f;
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	for (size_t i = 0; i < std::size(g_vehicleInputChannels); i++)
	{
		const VehicleInputChannel& channel = g_vehicleInputChannels[i];
		const bool held = channel.flag == 0 || X86_LOAD_U8(self + channel.flag) != 0;
		const float k = X86_LOAD_F32(held ? channel.heldK : channel.releasedK);
		if (!(k > 0.0f))
		{
			continue;
		}

		const uint32_t addr = self + channel.value;
		X86_STORE_F32(addr, ScaleFirstOrderLag(before[i], X86_LOAD_F32(addr), k / (k + 1.0f), frames));
	}
}

// One wheel step
// config[14] (-9) is a flat ground-adhesion bias. Only applies while airborne
// Per-wheel sweep request
static float g_vehicleReq[2][3];

// sub_246120
// Fix not being able to move at higher fps by removing the per frame blend, then re-applying as if running at 60fps
// Also fix in-air movement
static void VehicleRelaxAxleVelocity(uint8_t* base, uint32_t axle, uint32_t prevPos, float invDt,
	float retention, float adhesion, float frames, const float* req)
{
	if (adhesion != 0.0f)
	{
		const float dt = 1.0f / invDt;
		constexpr float dt60 = 1.0f / 60.0f;
		const float fixH = 3.75f * (dt60 - dt) * 0.5f;
		const float fixY = 0.6666667f * (dt60 - dt) * 0.5f;
		float v0[3];
		float m[3];
		float m60[3];

		for (uint32_t i = 0; i < 3; i++)
		{
			// remove the per frame blend (prevPos)
			m[i] = (X86_LOAD_F32(axle + i * 4) - X86_LOAD_F32(prevPos + i * 4)) * invDt;
			const float blended = X86_LOAD_F32(axle + 12 + i * 4);
			v0[i] = blended + blended - m[i];

			// Trapezoid fix vertical: v - a/k, a/k = -190.5 [note: this is kind of a crap workaround, maybe should use base gravity]
			const float fix = i == 1 ? (v0[1] + 190.5f) * fixY : v0[i] * fixH;
			m60[i] = m[i] + fix;
		}

		const float axis[3] = { req[0], req[1] + adhesion * dt, req[2] };
		const float len = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);

		if (len > 0.000001f)
		{
			const float dot = (v0[0] * axis[0] + v0[1] * axis[1] + v0[2] * axis[2]) / len;

			for (uint32_t i = 0; i < 3; i++)
			{
				const float v60 = (axis[i] / len * dot + m60[i]) * 0.5f;
				X86_STORE_F32(axle + 12 + i * 4, v0[i] + frames * (v60 - v0[i]));
			}
		}
		return;
	}

	for (uint32_t i = 0; i < 12; i += 4)
	{
		const float measured = (X86_LOAD_F32(axle + i) - X86_LOAD_F32(prevPos + i)) * invDt;
		const float blended = X86_LOAD_F32(axle + 12 + i);
		const float previous = blended + blended - measured;
		X86_STORE_F32(axle + 12 + i, measured + (previous - measured) * retention);
	}
}

X86_FUNC_IMPL(__imp__sub_246120);
X86_FUNC(sub_246120)
{
	const uint32_t self = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);

	__imp__sub_246120(ctx, base);

	const float frames = dt * 60.0f;
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const float retention = std::pow(0.5f, frames);
	const float invDt = 1.0f / dt;

	const uint32_t config = 0x51CCD8 + 0xAC * X86_LOAD_U32(self + 604);
	const float adhesion = X86_LOAD_F32(config + 0x38);

	const float frontAir = (X86_LOAD_U8(self + 220) & 1) == 0 ? adhesion : 0.0f;
	const float rearAir = (X86_LOAD_U8(self + 272) & 1) == 0 ? adhesion : 0.0f;

	VehicleRelaxAxleVelocity(base, self + 184, self + 468, invDt, retention, frontAir, frames, g_vehicleReq[0]);
	VehicleRelaxAxleVelocity(base, self + 236, self + 520, invDt, retention, rearAir, frames, g_vehicleReq[1]);
}

// Helper to scope our angle fix just to TVehicle
static bool VehicleShouldScaleAngleLag(uint32_t returnAddress)
{
	switch (returnAddress)
	{
	case 0x24665A: // CarType chassis roll damper @ 0x246655
	case 0x2469A7: // CarType body pitch (0xAC) @ 0x2469A2
	case 0x2469D0: // CarType body roll (0xB4) @ 0x2469CB
	case 0x246AA0: // CarType steer lean (0x4C) @ 0x246A9B
		return true;
	default:
		return false;
	}
}

// Shared angle smoother (TVehicle and other unknown objects use it)
X86_FUNC_IMPL(__imp__sub_1D860);
X86_FUNC(sub_1D860)
{
	if (VehicleShouldScaleAngleLag(X86_LOAD_U32(ctx.esp.u32)))
	{
		const uint32_t rateAddr = ctx.esp.u32 + 0xC;
		const float a = X86_LOAD_F32(rateAddr);
		const float b = X86_LOAD_F32(ctx.esp.u32 + 0x10);
		const float frames = g_deltaTime * 60.0f;

		// Handle the pow that scales with higher framerate and prevent divide by 0 or endless low speed
		if (frames > 0.0f && a > 0.0f && b > 0.0f && std::fabs(frames - 1.0f) >= 0.0001f)
		{
			const float retention = std::pow(a / (a + b), frames);
			const float span = 1.0f - retention;
			X86_STORE_F32(rateAddr, span > 0.0001f ? b * (retention / span) : b * 10000.0f);
		}
	}
	__imp__sub_1D860(ctx, base);
}

// Per-frame sweep fix
// Swap below globals in sub_245FB0 only. If we ignore it constant missing, if we overcorrect it makes slopes crawl
// 0x42A18C (0.001) 'minimum' pad/slider
// 0x51CCD4 (0.02) 'pad' for ground
static const uint32_t g_carPadAddr = 0x51CCD4;
static const float g_carPadAt60 = 0.02f;
X86_FUNC_IMPL(__imp__sub_245FB0);
X86_FUNC(sub_245FB0)
{
	const uint32_t self = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);
	const uint32_t axle = X86_LOAD_U32(ctx.esp.u32 + 8);

	constexpr uint32_t slideGateAddr = 0x42A18C;
	constexpr float slideGateAt60 = 0.001f;

	float frames = dt * 60.0f;
	if (!(frames > 0.0f))
	{
		frames = 1.0f;
	}
	frames = std::min(frames, 1.0f);
	frames = std::max(frames, 0.05f);

	// Remove the blend and let VehicleRelaxAxleVelocity handle it
	const float rawFrames = dt * 60.0f;
	const bool correctProjection = rawFrames > 0.0f && std::fabs(rawFrames - 1.0f) >= 0.0001f;

	float velPre[3];
	for (uint32_t i = 0; i < 3; i++)
	{
		velPre[i] = X86_LOAD_F32(axle + 12 + i * 4);
	}
	{
		// Coasting fix (ignored while accelerating)
		const uint32_t prevPos = X86_LOAD_U32(ctx.esp.u32 + 0xC);
		float* const req = g_vehicleReq[axle == self + 236 ? 1 : 0];
		if (correctProjection &&
			(X86_LOAD_U8(axle + 0x24) & 1) != 0 &&
			std::fabs(X86_LOAD_F32(self + 100)) < 0.02f &&
			std::fabs(X86_LOAD_F32(self + 108)) < 0.02f)
		{
			const float hSq = (X86_LOAD_F32(axle) - X86_LOAD_F32(prevPos)) *
				(X86_LOAD_F32(axle) - X86_LOAD_F32(prevPos)) +
				(X86_LOAD_F32(axle + 8) - X86_LOAD_F32(prevPos + 8)) *
				(X86_LOAD_F32(axle + 8) - X86_LOAD_F32(prevPos + 8));
			const float freeze = std::sqrt(slideGateAt60);
			if (hSq < freeze * freeze)
			{
				X86_STORE_F32(axle, X86_LOAD_F32(prevPos));
				X86_STORE_F32(axle + 8, X86_LOAD_F32(prevPos + 8));
			}
		}
		for (uint32_t i = 0; i < 3; i++)
		{
			req[i] = X86_LOAD_F32(axle + i * 4) - X86_LOAD_F32(prevPos + i * 4);
		}
	}
	const float restoreGate = X86_LOAD_F32(slideGateAddr);
	const float restorePad = X86_LOAD_F32(g_carPadAddr);

	const float gate = slideGateAt60 * frames * frames * frames * frames;
	const float pad = g_carPadAt60 * frames;

	X86_STORE_F32(slideGateAddr, gate);
	X86_STORE_F32(g_carPadAddr, pad);

	__imp__sub_245FB0(ctx, base);

	X86_STORE_F32(slideGateAddr, restoreGate);
	X86_STORE_F32(g_carPadAddr, restorePad);

	if (correctProjection && (X86_LOAD_U8(axle + 0x24) & 1) == 0)
	{
		// Restore the original data for wheels in-air
		for (uint32_t i = 0; i < 3; i++)
		{
			X86_STORE_F32(axle + 12 + i * 4, velPre[i]);
		}
	}
}

/**	--Vehicle::TVehicleCar Patch END-- */

/**	--Library Cursor patch START-- */

 // There is an issue with the cursor of the library menu at higher fps.
 // When player held the stick, it scrolls down way too fast
 // One press make it goes 4-5 items further instead of 1 like it should.
 // This is due to a counter that use raw frames to progress.
 // We can patch this by forcing the counter to stick at 60 hz using delta time
 // and let a repeat pass only on the frame where a counter actually moved.


constexpr uint32_t g_menuGroupMasks = 0x47B698; // 8 directions group masks

// 60 Hz leftovers per pad and group, as well as the groups that ticked this frame.
// Technically only player 1 can control menu, but maybe one day we will have a way to do more so...
static float g_menuHoldCarry[4][8];
static uint8_t g_menuHoldTicked[4];

// The pad task, runs once a frame. Redo its counter bumps in 60 Hz steps.
X86_FUNC_IMPL(__imp__sub_2A7700);
X86_FUNC(sub_2A7700)
{
	const uint32_t pad = ctx.ecx.u32;
	uint16_t beforeState[4][8];

	if (pad == 0)
	{
		__imp__sub_2A7700(ctx, base);
		return;
	}

	for (uint32_t slot = 0; slot < 4; slot++)
	{
		for (uint32_t group = 0; group < 8; group++)
			beforeState[slot][group] = X86_LOAD_U16(pad + 16 * slot + 2 * group);
	}

	__imp__sub_2A7700(ctx, base);

	float frames = g_deltaTime * 60.0f;
	if (frames <= 0.0f)
		frames = 1.0f;

	for (uint32_t slot = 0; slot < 4; slot++)
	{
		uint8_t ticked = 0;

		for (uint32_t group = 0; group < 8; group++)
		{
			const uint32_t addr = pad + 16 * slot + 2 * group;
			const uint16_t held = beforeState[slot][group];
			const uint16_t pressed = X86_LOAD_U16(addr);
			float &carry = g_menuHoldCarry[slot][group];

			if (pressed == 0)
			{
				carry = 0.0f; // give up if the game already reset the counter
				continue;
			}

			if (pressed != (uint16_t)(held + 1))
				continue; // pad missing this frame, the game left the counter alone

			if (held == 0)
			{
				// first frame of the hold, keep the game's own 1
				carry = 0.0f;
				ticked |= 1 << group;
				continue;
			}

			carry += frames;

			// keeps 60 fps on exactly one step per frame
			const int steps = (int)(carry + 0.0001f);
			carry -= steps;

			if (steps > 0)
				ticked |= 1 << group;

			int value = held + steps;

			if (value > 0x7000)
				value = 0x7000; // never wrap

			X86_STORE_U16(addr, (uint16_t)value);
		}

		g_menuHoldTicked[slot] = ticked;
	}
}

// small recreation of sub_59F70 for convenience
static bool MenuKeyPressed(uint8_t* base, uint32_t pad, uint32_t slot, uint32_t mask)
{
	const uint32_t pressed = X86_LOAD_U32(pad + 40 * slot + 88);

	if ((mask & pressed) == mask)
		return true;

	if ((mask & 0x3C0) != 0 && (mask & 0x3C0 & pressed) == (mask & 0x3C0))
		return true;

	if ((mask & 0x3C0000) != 0 && (mask & 0x3C0000 & pressed) == (mask & 0x3C0000))
		return true;

	return false;
}


static bool MenuHoldTicked(uint8_t* base, uint32_t slot, uint32_t mask)
{
	const uint8_t ticked = g_menuHoldTicked[slot];

	for (uint32_t group = 0; group < 8; group++)
	{
		// did one of the groups fired a tick on the 60 Hz clock
		if ((ticked & (1 << group)) != 0 && (mask & X86_LOAD_U32(g_menuGroupMasks + group * 4)) != 0)
			return true;
	}

	return false;
}

// Function that seems to handle the scrolling in the library based on input held
// We can let the og code run for the math, we simply need to prevent it to continue scrolling if we reach extra frames.

X86_FUNC_IMPL(__imp__sub_2A7560);
X86_FUNC(sub_2A7560)
{
	const uint32_t pad = ctx.ecx.u32;
	uint32_t slot = X86_LOAD_U32(ctx.esp.u32 + 4);
	const uint32_t mask = X86_LOAD_U32(ctx.esp.u32 + 8);

	__imp__sub_2A7560(ctx, base);

	if (ctx.eax.u8_lo == 0 || pad == 0)
		return;

	// 6 and 7 means "the pad this menu owns"
	if (slot == 6)
		slot = X86_LOAD_U32(pad + 404);
	else if (slot == 7)
		slot = X86_LOAD_U32(pad + 408);

	if (slot < 4)
	{
		if (MenuKeyPressed(base, pad, slot, mask) || MenuHoldTicked(base, slot, mask))
			return;
	}
	else
	{
		for (uint32_t p = 0; p < 4; p++)
		{
			if (MenuKeyPressed(base, pad, p, mask) || MenuHoldTicked(base, p, mask))
				return;
		}
	}

	ctx.eax.u8_lo = 0; // this is a repeat from a duplicated frame, give up.
}

// The Library route list also add its own cooldown on top of all of this:
// after a single cursor move, it counts 2 frames before it starts to listen again for more.
// Those two frames increment without delta time, which will also affect cursor speed in the end.

X86_FUNC_IMPL(__imp__sub_291B70);
X86_FUNC(sub_291B70)
{
	const uint32_t self = ctx.ecx.u32;
	const uint32_t before = self ? X86_LOAD_U32(self + 284) : 0;

	__imp__sub_291B70(ctx, base);

	if (self == 0)
		return;

	const uint32_t after = X86_LOAD_U32(self + 284);

	if (after <= before)
		return; // no cursor move this frame

	const float frames = g_deltaTime * 60.0f;
	if (frames <= 0.0f || frames >= 1.0f)
		return;

	uint32_t scaled = (uint32_t)(after / frames + 0.5f);
	if (scaled > 64)
		scaled = 64;

	X86_STORE_U32(self + 284, scaled);
}

/**	--Library Cursor patch END-- */

// CharaColliMove
// catapult, footing (platform) and any other moving surface uses this
// fixes Catapult (aka Prison Island launchers)
static bool g_footingCarry = false;
X86_FUNC_IMPL(__imp__sub_80BA0);
X86_FUNC(sub_80BA0)
{
	g_footingCarry = true;
	__imp__sub_80BA0(ctx, base);
	g_footingCarry = false;
}

/** --Jump Board Patch START-- */

// Object that launch Shadow far away (far farrrrrr... away!)
// Player collision is a swept move, it takes a line from where Shadow was to where he wants to be and
// checks it consistently. The problem is, above 60 FPS, that line is too short and stays inside the ramp, 
// so collision consistently pushes Shadow creating some kind of "bump" effect.
// This can kill player under specific situation (like in space gadget). 
// We can do a hacky workaround by adding a grace method where collision will be ignored for the first 1/60s of launch, then we can call 
// the original function once to get the proper destination (like if the game was running at 60 fps).
// Would be nice to have a better way to fix this, but this is the only idea I had for now.

static constexpr float g_launchGracePeriod = 1.0f / 60.0f;

struct LaunchGrace
{
	uint32_t player;
	float elapsed;
	float sweepOrigin[3]; // (player+0xD0, which is basically old position)
	bool originValid;
	bool replayed;
	int skipped;
	bool selfTimed; // (For DarkSpin exit)
};

static LaunchGrace g_launchGrace[4] = {};

static LaunchGrace* FindLaunchGrace(uint32_t player)
{
	if (player == NULL)
		return nullptr;

	for (auto& slot : g_launchGrace)
	{
		if (slot.player == player)
			return &slot;
	}
	return nullptr;
}

// This is one of the function used to detect collision 
// We can trick the game that no collision were found for the first 1/60 of a second so we can progress
// Then we run the original code once to store the intended destination
// Hacky, but this is good enough for now. (Used for both Jump Board, Dark Spin, and Catapult / technically any CharaColliMove invoker).

X86_FUNC_IMPL(__imp__sub_84E50);
X86_FUNC(sub_84E50)
{
	const uint32_t playerData = X86_LOAD_U32(ctx.esp.u32 + 4);
	if (playerData != NULL)
	{
		const uint32_t player = X86_LOAD_U32(playerData);
		LaunchGrace* grace = FindLaunchGrace(player);
		if (grace != nullptr && grace->selfTimed)
		{
			const float deltatime = X86_LOAD_F32(X86_LOAD_U32(player + 0x60));
			grace->elapsed += deltatime > 0.0f ? deltatime : g_launchGracePeriod;

			// Nothing was held back here (60 FPS or lower), so there is nothing else to do.
			if (grace->elapsed >= g_launchGracePeriod - 1.0e-6f && grace->skipped == 0)
			{
				*grace = {};
				grace = nullptr;
			}
		}

		if (grace != nullptr)
		{
			// First pass of this launch will tell us where 60 FPS would start its sweep
			if (!grace->originValid)
			{
				grace->sweepOrigin[0] = X86_LOAD_F32(player + 0xD0);
				grace->sweepOrigin[1] = X86_LOAD_F32(player + 0xD4);
				grace->sweepOrigin[2] = X86_LOAD_F32(player + 0xD8);
				grace->originValid = true;
			}

			if (grace->elapsed < g_launchGracePeriod - 1.0e-6f)
			{
				// 0xD0-0xD8 holds CharaColliMove force
				if (g_footingCarry)
				{
					grace->sweepOrigin[0] += X86_LOAD_F32(player + 0x4) - X86_LOAD_F32(player + 0xD0);
					grace->sweepOrigin[1] += X86_LOAD_F32(player + 0x8) - X86_LOAD_F32(player + 0xD4);
					grace->sweepOrigin[2] += X86_LOAD_F32(player + 0xC) - X86_LOAD_F32(player + 0xD8);
				}
				grace->skipped++;
				ctx.eax.u32 = 0;
				ctx.AddEsp(1);
				return;
			}

			// Grace just finished and we held passes back, so replay the one pass 60 FPS
			// would have give us originally. 
			if (grace->skipped > 0 && !grace->replayed)
			{
				grace->replayed = true;

				const float saved[3] =
				{
					X86_LOAD_F32(player + 0xD0),
					X86_LOAD_F32(player + 0xD4),
					X86_LOAD_F32(player + 0xD8)
				};

				X86_STORE_F32(player + 0xD0, grace->sweepOrigin[0]);
				X86_STORE_F32(player + 0xD4, grace->sweepOrigin[1]);
				X86_STORE_F32(player + 0xD8, grace->sweepOrigin[2]);

				g_launchCollisionPass++;
				__imp__sub_84E50(ctx, base);
				g_launchCollisionPass--;

				// restore value (game should overwrite this right after, but just to be safe...)
				X86_STORE_F32(player + 0xD0, saved[0]);
				X86_STORE_F32(player + 0xD4, saved[1]);
				X86_STORE_F32(player + 0xD8, saved[2]);
				return;
			}
		}
	}

	__imp__sub_84E50(ctx, base);
}

// Player::Behavior::Launch::Start
X86_FUNC_IMPL(__imp__sub_76DE0);
X86_FUNC(sub_76DE0)
{
	const uint32_t player = X86_LOAD_U32(ctx.ecx.u32 + 4);

	LaunchGrace* grace = FindLaunchGrace(player);
	for (auto& slot : g_launchGrace)
	{
		if (grace == nullptr && slot.player == 0)
		{
			grace = &slot;
		}
	}

	if (grace == nullptr)
		grace = &g_launchGrace[0]; // either player is not found or all players are using it, force player 1 slot as a failsafe.

	grace->player = player;
	grace->elapsed = 0.0f;
	grace->originValid = false;
	grace->replayed = false;
	grace->skipped = 0;
	grace->selfTimed = false;

	__imp__sub_76DE0(ctx, base);
}

// Player::Behavior::Launch::Exec
// Delta time is added before the original runs, so at 60 FPS the grace period is already spent
// by the time collision is reached and the skip above can never fire.
X86_FUNC_IMPL(__imp__sub_76B20);
X86_FUNC(sub_76B20)
{
	LaunchGrace* grace = FindLaunchGrace(X86_LOAD_U32(ctx.ecx.u32 + 4));

	if (grace != nullptr)
	{
		const float deltatime = X86_LOAD_F32(ctx.esp.u32 + 4);

		if (deltatime > 0.0f)
			grace->elapsed += deltatime;
		else
			grace->elapsed = g_launchGracePeriod;
	}

	__imp__sub_76B20(ctx, base);
}

// Player::Behavior::Launch::End
X86_FUNC_IMPL(__imp__sub_76A70);
X86_FUNC(sub_76A70)
{
	LaunchGrace* grace = FindLaunchGrace(X86_LOAD_U32(ctx.ecx.u32 + 4));

	if (grace != nullptr)
	{
		grace->player = 0;
		grace->elapsed = 0.0f;
		grace->originValid = false;
		grace->replayed = false;
		grace->skipped = 0;
	}

	__imp__sub_76A70(ctx, base);
}

/**	--Jump Board Patch END-- */

// GUNLiftLocal / GUNLiftExpress aim speed
// ecx = vehicle
// 0x160 = vertical angle
// 0x164 = horizontal angle
// esi = command (aim / move command)
// 0x14 = stickX
// 0x1C = stickY
bool GunLiftAimMidAsmHook(X86Register& ecx, X86Register& esi)
{
	auto base = g_memory.base;
	const float frames = g_deltaTime * 60.0f;

	// 60 FPS
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return false;
	}

	// 61+ FPS
	const uint32_t vehicle = ecx.u32;
	const uint32_t command = esi.u32;
	const float deg2rad = X86_LOAD_F32(0x47E0A4) * X86_LOAD_F32(0x42A880); // pi * 1/180
	const float delta = deg2rad * X86_LOAD_F32(0x46D830) * frames; // 1.5 GUNLift sensitivity value
	const float pitch = X86_LOAD_F32(vehicle + 0x160) + X86_LOAD_F32(command + 0x1C) * delta;
	X86_STORE_F32(vehicle + 0x164, X86_LOAD_F32(vehicle + 0x164) - X86_LOAD_F32(command + 0x14) * delta);

	const float minPitch = deg2rad * X86_LOAD_F32(0x46D828); // GUNLift min angle -10
	const float maxPitch = deg2rad * X86_LOAD_F32(0x46D82C); // GUNLift max angle 60
	X86_STORE_F32(vehicle + 0x160, std::min(std::max(pitch, minPitch), maxPitch));

	return true;
}

/** --SetWatchCamera Patch START-- */

// Exec @ 0x21C4F0 respects deltatime
//
// Subfuncs that do not (per frame instead)

// Idle behavior @ 0x21B670
void SetWatchCameraSightChaseMidAsmHook(X86Register& esi, X86Register& esp)
{
	auto base = g_memory.base;
	const float frames = g_deltaTime * 60.0f;

	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const uint32_t watchCamera = esi.u32;
	const uint32_t deltaAddr = esp.u32 + 0x14;

	const float ramp = std::min(X86_LOAD_F32(watchCamera + 0xC4) + 0.01f * frames, 1.0f);
	X86_STORE_F32(watchCamera + 0xC4, std::min(ramp - 0.01f, 1.0f));

	if (ramp > 0.0001f)
	{
		const float scale = (1.0f - std::pow(1.0f - ramp, frames)) / ramp;
		for (uint32_t i = 0; i < 3; i++)
		{
			X86_STORE_F32(deltaAddr + i * 4, X86_LOAD_F32(deltaAddr + i * 4) * scale);
		}
	}
}

// Position update @ 0x21BCF0
void SetWatchCameraSightMoveMidAsmHook(X86Register& edi)
{
	auto base = g_memory.base;
	const float frames = g_deltaTime * 60.0f;

	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const uint32_t watchCamera = edi.u32;
	for (uint32_t i = 0; i < 3; i++)
	{
		X86_STORE_F32(watchCamera + 0xE0 + i * 4, X86_LOAD_F32(watchCamera + 0xE0 + i * 4) + X86_LOAD_F32(watchCamera + 0xC8 + i * 4) * (frames - 1.0f));
	}
}

/** --SetWatchCamera Patch END-- */

// Vine::FireBall::Exec @ 0x1E3540
// AKA "Killer Plant"
void VineFireBallTravelMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const float frames = g_deltaTime * 60.0f;

	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const uint32_t vine = esi.u32;

	if (X86_LOAD_U32(vine + 0x60) != 1) // state check for fireball
	{
		return;
	}

	const uint32_t fireballPosition = X86_LOAD_U32(vine + 0x48) + 4;
	for (uint32_t i = 0; i < 3; i++)
	{
		X86_STORE_F32(fireballPosition + i * 4, X86_LOAD_F32(fireballPosition + i * 4) + X86_LOAD_F32(vine + 0x38 + i * 4) * (frames - 1.0f));
	}
}

/** --ScatteringWeapon weapon hang fix START-- */

// Shadow Boxes create ScatteringWeapon
// Higher FPS = higher chance for ray to hit while still mid-air. Funny enough early versions of Shadow have this
// issue where the weapons collide with each other. Katana02 tends to get stuck the most at 240FPS
// Threshold value is @ 0x2493B0
// Restore for other funcs, as this may be a shared constant
static const uint32_t g_weaponDropSettleThreshold = 0x46B380; // 0.1 (original threshold)
void WeaponDropSettleThresholdMidAsmHook()
{
	auto base = g_memory.base;
	const float frames = g_deltaTime * 60.0f;

	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}
	X86_STORE_F32(g_weaponDropSettleThreshold, 0.1f * frames);
}

void WeaponDropSettleThresholdRestoreMidAsmHook()
{
	auto base = g_memory.base;
	X86_STORE_F32(g_weaponDropSettleThreshold, 0.1f);
}

/** --ScatteringWeapon landing Patch END-- */

/** --Belt::Effect (HomingAttack/TriJump/JumpDash/Slide/Spin trails) Patch START-- */

std::unordered_map<uint32_t, float> g_beltTrailCarry; // map per BeltTask to support multiplayer
// Belt::Exec
bool BeltTrailPushMidAsmHook(X86Register& esi)
{
	const float frames = g_deltaTime * 60.0f;
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return true;
	}

	float &carry = g_beltTrailCarry[esi.u32];
	carry += frames;
	if (carry >= 1.0f)
	{
		carry -= (float)(int)carry; // floor to int first
		return true;
	}
	return false;
}

// Eff::Belt update @ 0x260670
// Fade call @ 0x260716
// Per frame fade of '8'
struct BeltFadeTiming
{
	std::chrono::steady_clock::time_point last;
	float carry;
};
std::unordered_map<uint32_t, BeltFadeTiming> g_beltFadeTimings;
void BeltTrailFadeMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const uint32_t beltEff = esi.u32;
	const auto now = std::chrono::steady_clock::now();

	if (g_beltFadeTimings.size() > 64)
	{
		g_beltFadeTimings.clear(); // stale entries of dead effects, safe to reset
	}

	// if not tracked, skip our delta
	auto [it, first] = g_beltFadeTimings.try_emplace(beltEff, now, 0.0f);
	if (first)
	{
		return;
	}

	const double elapsed = std::chrono::duration<double>(now - it->second.last).count();
	it->second.last = now;
	const float total = std::min((float)(elapsed * 60.0), 4.0f) * 8.0f + it->second.carry;
	const int step = (int)total;
	it->second.carry = total - step;

	const int value = std::min((int)X86_LOAD_U8(beltEff + 8) + (8 - step), 255);
	X86_STORE_U8(beltEff + 8, (uint8_t)std::max(value, 0));
	if (X86_LOAD_U8(beltEff + 8) == 0)
	{
		g_beltFadeTimings.erase(beltEff);
	}
}

/** --Belt::Effect Patch END-- */

/** --Eff::LocusMultiVtx Patch START-- */

std::unordered_map<uint32_t, float> g_locusCarry; // map per Locus effect to support multiplayer
static bool EffectLocusMultiVtxStep(uint32_t locus)
{
	const float frames = g_deltaTime * 60.0f;
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return true;
	}

	if (g_locusCarry.size() > 64)
	{
		g_locusCarry.clear(); // stale entries of dead effects, safe to reset
	}

	float& carry = g_locusCarry[locus];
	carry += frames;
	if (carry >= 1.0f)
	{
		carry -= (float)(int)carry;
		return true;
	}
	return false;
}

// Locus push @ sub_260B40
// esi = locus
// locus+0x20 sample count
bool MeleeEffectLocusMultiVtxTrailPushMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	// first sample always push, not perfect depending on current MTN the player is doing,
	// but makes the effect start much more consistently
	return X86_LOAD_U32(esi.u32 + 0x20) == 0 || EffectLocusMultiVtxStep(esi.u32);
}

// Locus fade @ sub_260B40
bool MeleeEffectLocusMultiVtxTrailFadeMidAsmHook(X86Register& esi)
{
	return EffectLocusMultiVtxStep(esi.u32);
}
/** --Eff::LocusMultiVtx Patch END-- */

/** --Destructible/Break object pieces Patch START-- */

// Gadgets destroy and create several TObjEffCrash3DChildObj and TObjEffCrash3DRChildObj
// Patch to do 60FPS step, similar to ScatterWeapon fix
static std::unordered_map<uint32_t, float> g_debrisStepCarry;
static void DebrisStepGate(X86Context& ctx, uint8_t* base, X86Func* original)
{
	const float frames = g_deltaTime * 60.0f;

	// 60FPS
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		original(ctx, base);
		return;
	}

	// 61+
	if (g_debrisStepCarry.size() > 512) // clear if 512+ tasks
	{
		g_debrisStepCarry.clear();
	}

	const uint32_t debris = ctx.ecx.u32;
	float &carry = g_debrisStepCarry[debris];
	carry += frames;

	if (carry < 1.0f)
	{
		ctx.AddEsp(2); // skip the call
		return;
	}

	carry -= (float)(int)carry;

	original(ctx, base);

	// clean if destroyed
	if ((X86_LOAD_U8(debris + 0xC) & 1) != 0)
	{
		g_debrisStepCarry.erase(debris);
	}
}

// TObjEffCrash3DChildObj::Exec
X86_FUNC_IMPL(__imp__sub_178850);
X86_FUNC(sub_178850)
{
	DebrisStepGate(ctx, base, __imp__sub_178850);
}

// TObjEffCrash3DRChildObj::Exec
X86_FUNC_IMPL(__imp__sub_178AD0);
X86_FUNC(sub_178AD0)
{
	DebrisStepGate(ctx, base, __imp__sub_178AD0);
}

/** --Destructible/Break object pieces Patch END-- */

/**	--Global FX Alpha Fade Patch START-- */
// Every FX object (including implicit ones in GDT) are affected by this. This fixes a bunch of color bugs as a side effect.
// Fixes the Cosmic Fall 'invisible/missing' BackDrop cycle, Glypic Canyon Platform 'red' issue, and probably a bunch more
// Ended up not needing to patch it, but the Effect::PJS::EffectTask::Exec root is sub_C1080 (handles deltatime fine as is)

// alpha += (int)(fadeRate * dt) (step)
// alpha > 0x3FC00000 = fade done
// alpha < 0x3FFFFF = fade killed off

// Fade rate is 1/60 so 61FPS truncates to fade kill
// Cosmic Fall backdrop never fades in because of this at 61
// Glyphic Canyon Platform and other objs do the same thing (red FX and other weird side effects in the three channel caller)

// FX Fade Node (ecx in both funcs)
// 0x10 started flag
// 0x14 dirty flag
// 0x18 alpha target ptr
// 0x1C timer
// 0x20 duration
// 0x24 fade rate

// Per address carry leftovers to compute proper alpha for 61FPS+
static std::unordered_map<uint32_t, double> g_fxFadeCarry;
static void FxFadeCarryTrim()
{
	if (g_fxFadeCarry.size() > 4096)
	{
		g_fxFadeCarry.clear();
	}
}

// Flag parent state for redraw (completion, 1:1 with original)
static void FxFadeMarkDirty(uint8_t* base, uint32_t node)
{
	uint32_t type = X86_LOAD_U32(node + 4);
	if (type != 0 && X86_LOAD_U8(type + 2) != 0)
	{
		uint32_t owner = X86_LOAD_U32(node + 0x14);
		if (owner != 0)
		{
			X86_STORE_U32(owner, X86_LOAD_U32(owner) | 1);
		}
	}
}

// Helper function to clamp the leftover frames that were skipped on the last step of a command for effects that run at 61 FPS or higher.
// Mostly used for alpha fade issues
static float FxCmdLeftoverFrames(float prevTimer, float duration, float dt)
{
	// 60 FPS or less
	if (dt >= 1.0f)
	{
		return 0.0f;
	}

	return std::clamp((duration - 0.0001f) - prevTimer, 0.0f, dt);
}

// 61 FPS+ alpha step for the single channel node (prevent over carry)
static void FxAlphaStepSingle(uint8_t* base, uint32_t node, uint32_t target, float frames)
{
	const int32_t rate = (int32_t)X86_LOAD_U32(node + 0x24);

	if (rate == 0)
	{
		return;
	}

	FxFadeCarryTrim();
	double exact = (double)rate * (double)frames + g_fxFadeCarry[node];
	int32_t step = (int32_t)exact;
	g_fxFadeCarry[node] = exact - (double)step;

	if (step != 0)
	{
		X86_STORE_U32(target, (uint32_t)((int32_t)X86_LOAD_U32(target) + step));
	}

	const int32_t alpha = (int32_t)X86_LOAD_U32(target);

	if (alpha > (int32_t)0x3FC00000)
	{
		X86_STORE_U32(target, 0x3FC00000);
		X86_STORE_U32(node + 0x24, 0);
		g_fxFadeCarry.erase(node);
	}
	else if (rate < 0 && alpha < (int32_t)0x3FFFFF)
	{
		X86_STORE_U32(node + 0x18, 0);
		X86_STORE_U32(node + 0x24, 0);
		g_fxFadeCarry.erase(node);
	}
}


static void FxAlphaStepChannel(uint8_t* base, uint32_t chan, uint32_t rateAddr, float frames)
{
	const int32_t rate = (int32_t)X86_LOAD_U32(rateAddr);
	if (rate == 0)
	{
		return;
	}

	FxFadeCarryTrim();
	double exact = (double)rate * (double)frames + g_fxFadeCarry[rateAddr];
	int32_t step = (int32_t)exact;
	g_fxFadeCarry[rateAddr] = exact - (double)step;

	if (step != 0)
	{
		X86_STORE_U32(chan, (uint32_t)((int32_t)X86_LOAD_U32(chan) + step));
	}

	const int32_t alpha = (int32_t)X86_LOAD_U32(chan);
	if (alpha > (int32_t)0x3FC00000)
	{
		X86_STORE_U32(chan, 0x3FC00000);
		X86_STORE_U32(rateAddr, 0);
		g_fxFadeCarry.erase(rateAddr);
	}
	else if (rate < 0 && alpha < (int32_t)0x3FFFFF)
	{
		X86_STORE_U32(chan, 0);
		X86_STORE_U32(rateAddr, 0);
		g_fxFadeCarry.erase(rateAddr);
	}
}

// Single channel impl
// Cosmic Fall BackDrop FX DFFs
static void FxAlphaSingleExec(X86Context& ctx, uint8_t* base)
{
	const uint32_t node = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);

	const uint32_t target = X86_LOAD_U32(node + 0x18);
	if (target == 0)
	{
		g_fxFadeCarry.erase(node);
		FxFadeMarkDirty(base, node);
		ctx.eax.u32 = 0;
	}
	else if (X86_LOAD_U8(node + 0x10) == 0)
	{
		X86_STORE_U8(node + 0x10, 1);
		ctx.eax.u32 = 1;
	}
	else
	{
		const float prevTimer = X86_LOAD_F32(node + 0x1C);
		const float timer = prevTimer + dt;
		X86_STORE_F32(node + 0x1C, timer);

		if (timer > X86_LOAD_F32(node + 0x20)) // completed
		{
			const float leftover = FxCmdLeftoverFrames(prevTimer, X86_LOAD_F32(node + 0x20), dt);
			if (leftover > 0.0f)
			{
				FxAlphaStepSingle(base, node, target, leftover);
			}

			g_fxFadeCarry.erase(node);
			FxFadeMarkDirty(base, node);
			ctx.eax.u32 = 0;
		}
		else
		{
			const int32_t rate = (int32_t)X86_LOAD_U32(node + 0x24);
			if (rate != 0)
			{
				if (dt >= 1.0f)
				{
					// 60 FPS or less
					const int32_t step = (int32_t)((float)rate * dt);
					X86_STORE_U32(target, (uint32_t)((int32_t)X86_LOAD_U32(target) + step));

					const int32_t alpha = (int32_t)X86_LOAD_U32(target);
					if (alpha > (int32_t)0x3FC00000)
					{
						X86_STORE_U32(target, 0x3FC00000);
						X86_STORE_U32(node + 0x24, 0);
					}
					else if (alpha < (int32_t)0x3FFFFF)
					{
						X86_STORE_U32(node + 0x18, 0);
						X86_STORE_U32(node + 0x24, 0);
					}
				}
				else
				{
					// 61 FPS+
					FxAlphaStepSingle(base, node, target, dt);
				}
			}
			ctx.eax.u32 = 1;
		}
	}
	ctx.esp.u32 += 0x8;
}


// Some destructibles (known as gadgets internally), like Sky Troops pillars play a break effect.
// At some very specific FPS (like 165 or 270), once the break animation is complete, some kind of white effect shows up for about half a second
// This is due to the alpha fade not completing properly, so the effect is still alive and visible.
// Fix: Keep the leftover alpha step at high fps and apply it manually on the last frame of the fade, acting basically like a clamp.
static std::unordered_map<uint32_t, float> g_fxAlphaStepCarry;
X86_FUNC_IMPL(__imp__sub_316BC0);
X86_FUNC(sub_316BC0)
{
	const uint32_t node = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);

	// 60 FPS or less
	if (dt >= 1.0f)
	{
		FxAlphaSingleExec(ctx, base);
		return;
	}

	if (g_fxAlphaStepCarry.size() > 4096)
	{
		g_fxAlphaStepCarry.clear();
	}

	float &fxalphaStep = g_fxAlphaStepCarry[node];
	fxalphaStep += dt;
	if (fxalphaStep < 1.0f)
	{
		// Not a complete frame yet, keep the node alive for now
		ctx.eax.u32 = 1;
		ctx.esp.u32 += 0x8;
		return;
	}

	fxalphaStep -= 1.0f;
	X86_STORE_F32(ctx.esp.u32 + 4, 1.0f);
	FxAlphaSingleExec(ctx, base);

	if (ctx.eax.u32 == 0)
	{
		g_fxAlphaStepCarry.erase(node);
	}
}

// Three channel impl (zeroes on completion instead of clearing)
// Glyphic Canyon Platform, RingShot bullet color
X86_FUNC_IMPL(__imp__sub_3167D0);
X86_FUNC(sub_3167D0)
{
	const uint32_t node = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);

	const uint32_t target = X86_LOAD_U32(node + 0x18);
	if (target == 0)
	{
		FxFadeMarkDirty(base, node);
		ctx.eax.u32 = 0;
	}
	else if (X86_LOAD_U8(node + 0x10) == 0)
	{
		X86_STORE_U8(node + 0x10, 1);
		ctx.eax.u32 = 1;
	}
	else
	{
		const float prevTimer = X86_LOAD_F32(node + 0x1C);
		const float timer = prevTimer + dt;
		X86_STORE_F32(node + 0x1C, timer);

		if (timer > X86_LOAD_F32(node + 0x20))
		{
			const float leftover = FxCmdLeftoverFrames(prevTimer, X86_LOAD_F32(node + 0x20), dt);
			for (uint32_t i = 0; i < 3; i++)
			{
				const uint32_t rateAddr = node + 0x24 + i * 4;
				if (leftover > 0.0f)
				{
					FxAlphaStepChannel(base, target + i * 4, rateAddr, leftover);
				}
				g_fxFadeCarry.erase(rateAddr);
			}

			FxFadeMarkDirty(base, node);
			ctx.eax.u32 = 0;
		}
		else
		{
			for (uint32_t i = 0; i < 3; i++)
			{
				const uint32_t rateAddr = node + 0x24 + i * 4;
				const int32_t rate = (int32_t)X86_LOAD_U32(rateAddr);
				if (rate == 0)
				{
					continue;
				}

				const uint32_t chan = target + i * 4;
				if (dt >= 1.0f)
				{
					// 60 FPS or less
					const int32_t step = (int32_t)((float)rate * dt);
					X86_STORE_U32(chan, (uint32_t)((int32_t)X86_LOAD_U32(chan) + step));

					const int32_t alpha = (int32_t)X86_LOAD_U32(chan);
					if (alpha < (int32_t)0x3FC00001)
					{
						if (alpha < (int32_t)0x3FFFFF)
						{
							X86_STORE_U32(chan, 0);
							X86_STORE_U32(rateAddr, 0);
						}
					}
					else
					{
						X86_STORE_U32(chan, 0x3FC00000);
						X86_STORE_U32(rateAddr, 0);
					}
				}
				else
				{
					// 61 FPS+
					FxAlphaStepChannel(base, chan, rateAddr, dt);
				}
			}
			ctx.eax.u32 = 1;
		}
	}
	ctx.esp.u32 += 0x8;
}

// EffCmdSingleExec (effect also take in account gravity, friction, amplitude etc...)
X86_FUNC_IMPL(__imp__sub_315220);
X86_FUNC(sub_315220)
{
	const uint32_t node = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);
	const uint32_t target = X86_LOAD_U32(node + 0x18);
	const bool started = X86_LOAD_U8(node + 0x10) != 0;
	const float prevTimer = X86_LOAD_F32(node + 0x1C);

	__imp__sub_315220(ctx, base);

	if (target != 0 && started && ctx.eax.u8_lo == 0)
	{
		const float leftover = FxCmdLeftoverFrames(prevTimer, X86_LOAD_F32(node + 0x20), dt);
		if (leftover > 0.0f)
		{
			X86_STORE_F32(target, X86_LOAD_F32(node + 0x24) * leftover + X86_LOAD_F32(target));
		}
	}
}

// EffCmdVector3Exec (3D pos, rot, vel, scale etc.)
X86_FUNC_IMPL(__imp__sub_316240);
X86_FUNC(sub_316240)
{
	const uint32_t node = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);
	const uint32_t target = X86_LOAD_U32(node + 0x18);
	const bool started = X86_LOAD_U8(node + 0x10) != 0;
	const float prevTimer = X86_LOAD_F32(node + 0x1C);

	__imp__sub_316240(ctx, base);

	if (target != 0 && started && ctx.eax.u8_lo == 0)
	{
		const float leftover = FxCmdLeftoverFrames(prevTimer, X86_LOAD_F32(node + 0x20), dt);
		if (leftover > 0.0f)
		{
			for (uint32_t i = 0; i < 0xC; i += 4)
			{
				X86_STORE_F32(target + i, X86_LOAD_F32(node + 0x24 + i) * leftover + X86_LOAD_F32(target + i));
			}
		}
	}
}
/**	--Global FX Alpha Fade Patch END-- */

/**	--UV Anim Baked Flipbook Patch START-- */

// Some UV anims are "baked flipbooks" with a key every 1/60s. (e.g GunBeetle's electric shield through EFGUNTHUNDER.UVA)
// RenderWare blends linearly between keys, so at any fps but 60, it lands between two keys and show half a row.
// This creates some kind of weird visual effect, as if it was skipping frames.
// Fix: hold the key instead of blending when two keys are 1/60s apart, same as what basically happens at 60fps.

// UV anim keyframe (memo: +0x04 time, +0x08 to +0x1C six floats)
static bool UVAnimHoldBakedKey(uint8_t* base, uint32_t out, uint32_t key0, uint32_t key1, float time)
{
	const float time0 = X86_LOAD_F32(key0 + 0x4);
	const float span = X86_LOAD_F32(key1 + 0x4) - time0;

	if (std::fabs(span - (1.0f / 60.0f)) > 0.0001f)
		return false;

	// 60 fps lands a tiny bit before the next key sometimes so treat it as the next key to be safe.
	const uint32_t key = ((time - time0) / span >= 0.999f) ? key1 : key0;

	for (uint32_t i = 0x8; i <= 0x1C; i += 4)
	{
		X86_STORE_U32(out + i, X86_LOAD_U32(key + i));
	}

	return true;
}

// UV anim param scheme (interpolate callback)
X86_FUNC_IMPL(__imp__sub_2C9B50);
X86_FUNC(sub_2C9B50)
{
	const uint32_t out = X86_LOAD_U32(ctx.esp.u32 + 0x4);
	const uint32_t key0 = X86_LOAD_U32(ctx.esp.u32 + 0x8);
	const uint32_t key1 = X86_LOAD_U32(ctx.esp.u32 + 0xC);
	const float time = X86_LOAD_F32(ctx.esp.u32 + 0x10);

	if (UVAnimHoldBakedKey(base, out, key0, key1, time))
	{
		ctx.esp.u32 += 0x4;
		return;
	}

	__imp__sub_2C9B50(ctx, base);
}

// UV anim linear scheme (interpolate callback)
X86_FUNC_IMPL(__imp__sub_2CA020);
X86_FUNC(sub_2CA020)
{
	const uint32_t out = X86_LOAD_U32(ctx.esp.u32 + 0x4);
	const uint32_t key0 = X86_LOAD_U32(ctx.esp.u32 + 0x8);
	const uint32_t key1 = X86_LOAD_U32(ctx.esp.u32 + 0xC);
	const float time = X86_LOAD_F32(ctx.esp.u32 + 0x10);

	if (UVAnimHoldBakedKey(base, out, key0, key1, time))
	{
		ctx.esp.u32 += 0x4;
		return;
	}

	__imp__sub_2CA020(ctx, base);
}

/**	--UV Anim Baked Flipbook Patch END-- */

/** --EnemyBase Rotation Patch START-- */
// EnemyBase Rotation To Target / Shared_RotateToPlayerCalc
// Enemy Yaw = (enemy+0x30)->0x14
// Target Yaw = (enemy+0x88)
X86_FUNC_IMPL(__imp__sub_C23B0);
X86_FUNC(sub_C23B0)
{
	const uint32_t enemy = ctx.ecx.u32;
	const uint32_t enemyModel = X86_LOAD_U32(enemy + 0x30);

	if (enemyModel == 0)
	{
		__imp__sub_C23B0(ctx, base);
		return;
	}

	const float yawBefore = X86_LOAD_F32(enemyModel + 0x14);
	const float target = X86_LOAD_F32(enemy + 0x88);
	__imp__sub_C23B0(ctx, base);

	const float frames = g_deltaTime * 60.0f;
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const float step = WrapAngle(X86_LOAD_F32(enemyModel + 0x14) - yawBefore);
	if (step == 0.0f)
	{
		return;
	}

	const float remaining = WrapAngle(target - yawBefore);
	float scaled = step * frames;
	if (std::fabs(scaled) > std::fabs(remaining))
	{
		scaled = remaining;
	}

	const float yawAfter = WrapAngle(yawBefore + scaled);
	X86_STORE_F32(enemyModel + 0x14, yawAfter);
	// Face target check (same constant)
	ctx.eax.u8_lo = std::fabs(WrapAngle(target - yawAfter)) < 0.017453294f ? 1 : 0;
}
/** --EnemyBase Rotation Patch END-- */

/** --BirdBase Vehicle Rotation Fix Patch START-- */

// BirdBase Vehicle aka the rideable BlackVolt / BlackHawk
// Rotation update @ 0x251F00 (called by the bird Exec @ 0x253740)
// Only the stick release rotation was not using delta time
void BirdVehicleTurnScaleMidAsmHook(X86Register& esp)
{
	auto base = g_memory.base;
	const float frames = g_deltaTime * 60.0f;
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const uint32_t rotationRate = esp.u32 + 0x10;
	X86_STORE_F32(rotationRate, std::min(X86_LOAD_F32(rotationRate) * frames, 1.0f));
}

/** --BirdBase Vehicle Patch END-- */


/** --DarkSpin exit Patch START-- */

// That red slime thing that Shadow absorb with a spin dash which carry him along a path.
// Above 60 FPS, the jump at the end can make Shadow gets stuck on it depending on the stage (Death Ruins being an example).
// Same fix than the jump board, but a bit simpler: we register a grace when it's about to bump Shadow, and the
// original collision code does the rest.

// Player::Behavior::DarkSpin::Exec
X86_FUNC_IMPL(__imp__sub_6D700);
X86_FUNC(sub_6D700)
{
	// State 2 is the last one, it always switches to Jump
	if (X86_LOAD_U32(ctx.ecx.u32 + 0x5C) == 2)
	{
		const uint32_t player = X86_LOAD_U32(ctx.ecx.u32 + 4);
		LaunchGrace* grace = FindLaunchGrace(player);

		for (auto& slot : g_launchGrace)
		{
			if (grace == nullptr && slot.player == 0)
				grace = &slot;
		}

		if (grace != nullptr && player != NULL)
		{
			*grace = {};
			grace->player = player;
			grace->selfTimed = true;
		}
	}

	__imp__sub_6D700(ctx, base);
}

/** --DarkSpin exit Patch END-- */

// TornadoTowing
// The end check fails at high fps (it uses dt in the check itself, high fps = small dt)
X86_FUNC_IMPL(__imp__sub_1CD350);
X86_FUNC(sub_1CD350)
{
	const uint32_t tornadoTowing = ctx.ecx.u32;
	const uint32_t dtAddr = ctx.esp.u32 + 0xC;
	const float dt = X86_LOAD_F32(dtAddr);
	const float speed = X86_LOAD_F32(X86_LOAD_U32(tornadoTowing + 0x28) + 0x54);
	if (dt > 0.0f && speed > 0.0f && dt < 1.0f / 60.0f)
	{
		float delta[3];
		float len = 0.0f;
		for (uint32_t i = 0; i < 3; i++)
		{
			delta[i] = X86_LOAD_F32(tornadoTowing + 0x44 + i * 4) - X86_LOAD_F32(tornadoTowing + 0x38 + i * 4);
			len += delta[i] * delta[i];
		}
		len = std::sqrt(len);

		// misses the target barely since dt is so small at high fps
		// write to dt with the approx 60 equivalent check
		if (len > speed * dt && len <= speed / 60.0f)
		{
			X86_STORE_F32(dtAddr, len / speed * 1.001f);
		}
	}
	__imp__sub_1CD350(ctx, base);
}

// Hero / Dark gauge fill draw callback @ sub_280B60
// Every frame gap target is halved each frame
void ChaosGaugeFillMidAsmHook(X86Register& eax, X86Register& ecx)
{
	const float dt = g_deltaTime;
	const auto target = (int32_t)eax.u32;
	const auto shown = (int32_t)ecx.u32;

	// Always render 1 from target, never above (OG behavior)
	if (target - shown <= 1 || dt <= 0.0f || dt >= 1.0f / 60.0f)
	{
		return;
	}

	// match 60FPS behavior for high FPS
	const float step = (float)(target - shown) * (1.0f - std::pow(0.5f, dt * 60.0f));
	const int32_t next = std::min(shown + (int32_t)std::ceil(step), target - 1);
	eax.u32 = ecx.u32 = (uint32_t)next;
}

// Egg Dealer roulette UI bounce fix
X86_FUNC_IMPL(__imp__sub_115A50);
X86_FUNC(sub_115A50)
{
	const uint32_t reel = ctx.ecx.u32;
	__imp__sub_115A50(ctx, base);
	const float dt = g_deltaTime;
	if (X86_LOAD_U32(reel) != 3 || dt <= 0.0f || dt >= 1.0f / 60.0f)
	{
		return;
	}

	// OG wobble timer but scale to 60 FPS
	const float elapsed = 0.2f - X86_LOAD_F32(reel + 0x14);
	const float timer = 0.2f - std::max(1.0f, std::floor(elapsed * 60.0f + 0.001f)) / 60.0f;

	// 1:1 bounce but with scaled timer
	const float width = X86_LOAD_F32(0x4F9988);
	const float bounce = (0.5f / X86_LOAD_F32(0x4F9984)) * timer * 5.0f * std::sin(X86_LOAD_F32(0x54BED8) * timer);
	float angle = (float)(int32_t)X86_LOAD_U32(reel + 0x10) * width + bounce;
	if (angle > X86_LOAD_F32(0x4F998C))
	{
		angle -= X86_LOAD_F32(0x4F998C);
	}
	X86_STORE_F32(reel + 0x38, angle);
}

// CSD color key lerp fix
// At 60 FPS, the CSD timer is always a whole frame, so two colour keys create a smooth transition.
// At high FPS however, we can reach between two frames and get some kind of weird blend that the game never originally show. 
// (Example: Pause story route relies on this and the whole screen flash brighter for a couple of frames.)
// Patch: Force the first key when needed to match 60 FPS behaviour.
X86_FUNC_IMPL(__imp__sub_3899C0);
X86_FUNC(sub_3899C0)
{
	const uint32_t key0 = X86_LOAD_U32(ctx.esp.u32 + 8);
	const uint32_t key1 = X86_LOAD_U32(ctx.esp.u32 + 12);
	const int32_t frame0 = (int32_t)X86_LOAD_U32(key0);
	const int32_t frame1 = (int32_t)X86_LOAD_U32(key1);

	if (frame1 - frame0 == 1 && X86_LOAD_F32(ctx.esp.u32 + 16) < (float)frame1)
	{
		X86_STORE_F32(ctx.esp.u32 + 16, (float)frame0);
	}

	__imp__sub_3899C0(ctx, base);
}

/** --Melee weapon attack Patch START-- */

// Melee swing trail and hit sweep follow the hand bone, which blends rotation between keys
// Off 60 FPS frames land between keys, the pose goes off path and the blade tip spikes up/down
// We can cheat and use the 'single frame' animation mode that events use, and apply while melee weapon is swinging

// To track which shadow is swinging, also necessary otherwise on enemy using melee weapon the patch would affect players
static uint32_t g_holderPlayer = 0; // which player
static uint32_t g_swingingModel = 0; // which skeleton
static std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> g_lastSwings;

// Player weapon update
// task + 0x58
X86_FUNC_IMPL(__imp__sub_9FA10);
X86_FUNC(sub_9FA10)
{
	g_holderPlayer = X86_LOAD_U32(ctx.ecx.u32 + 0x28);
	__imp__sub_9FA10(ctx, base);
	g_holderPlayer = 0;
}

// Melee hit sweep
X86_FUNC_IMPL(__imp__sub_2704F0);
X86_FUNC(sub_2704F0)
{
	g_lastSwings[g_holderPlayer] = std::chrono::steady_clock::now();
	__imp__sub_2704F0(ctx, base);
}

// Player skeleton update pass
X86_FUNC_IMPL(__imp__sub_995C0);
X86_FUNC(sub_995C0)
{
	const uint32_t data = X86_LOAD_U32(ctx.ecx.u32 + 0x58);
	const auto swing = g_lastSwings.find(data);
	const bool swinging = swing != g_lastSwings.end() && std::chrono::steady_clock::now() - swing->second < std::chrono::milliseconds(200);

	g_swingingModel = swinging ? (uint32_t)X86_LOAD_U32(ctx.ecx.u32 + 0x3E8) : 0;
	__imp__sub_995C0(ctx, base);
	g_swingingModel = 0;
}

// Skeleton animation sampler
X86_FUNC_IMPL(__imp__sub_374A60);
X86_FUNC(sub_374A60)
{
	const uint32_t model = X86_LOAD_U32(ctx.esp.u32 + 0x4);
	const uint32_t flags = X86_LOAD_U32(model + 0x14);

	// apply the event MTN single frame mode
	if (model == g_swingingModel)
	{
		X86_STORE_U32(model + 0x14, flags | 0x8);
		__imp__sub_374A60(ctx, base);
		X86_STORE_U32(model + 0x14, flags);
	}
	else
	{
		__imp__sub_374A60(ctx, base);
	}
}
/** --Melee weapon attack Patch END-- */

/** --Advertise Message Timer Patch START-- */
// Advertise has a 1.5s hardcoded to step at 1/60 instead of basing off dt.
static float GetMessageTimerExcess(uint8_t* base)
{
	const float dt = g_deltaTime;
	const float minStep = X86_LOAD_F32(0x42BE10); // message timer's hardcoded 1/60 value
	return dt > 0.0f && dt < minStep ? minStep - dt : 0.0f;
}

// Message timer @ sub_2A6020
// esi = message
void AdvertiseMessageElapsedTimerMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	X86_STORE_F32(esi.u32 + 0x24, X86_LOAD_F32(esi.u32 + 0x24) - GetMessageTimerExcess(base));
}

void AdvertiseMessageHoldTimerMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	X86_STORE_F32(esi.u32 + 0x20, X86_LOAD_F32(esi.u32 + 0x20) + GetMessageTimerExcess(base));
}
/** --Advertise Message Timer Patch END-- */

/** --Event FPS Patch START-- */

static thread_local bool g_isEventCameraCutting = false;
static thread_local bool g_isSamplingEventMotion = false;
static float g_eventStreamWaitCarry = 0.0f;
static float g_eventBattleFadeHoldTime = 0.0f;

// TPjsEventActionManager constructor
// Hooked after the divisor is set (override for unlocked)
void EventCreatedMidAsmHook()
{
	g_isEventCameraCutting = false;
	g_eventStreamWaitCarry = 0.0f;
	g_eventBattleFadeHoldTime = 0.0f;

	if (IsCutsceneFramerateUnlocked())
	{
		g_frameRateDivisor = 1;
	}
}

static const uint32_t g_eventClockFrame = 0x54D498; // clip / event current progress
static const uint32_t g_eventFramesPerSecond = 0x448150;
static constexpr float g_eventMaxDeltaTime = 1.0f;

// real frametime without rounding so event audio doesn't desync
static float GetEventDeltaTime()
{
	const float deltaTime = g_unroundedDeltaTime;
	return std::min(deltaTime, g_eventMaxDeltaTime);
}

void EventClockStepMidAsmHook(X86Register& esp)
{
	if (IsCutsceneFramerateUnlocked())
	{
		auto base = g_memory.base;
		X86_STORE_F32(esp.u32 + 0xC, GetEventDeltaTime());
	}
}

static constexpr float g_eventCameraCutMinJump = 1.0f;
static constexpr float g_eventCameraCutSlopeRatio = 4.0f;


// camera motion (.SNB)
// +0xC track count
// +0x10 tracks
//
// camera track
// +0x0 channel type
// +0x4 interp
// +0x14 start
// +0x18 end
// +0x1C key count
// +0x20 key size
// +0x24 keys (frame, value)
//
// 2 and 4 interpolation modes are used by events
// 0x20 is used by boss cameras / in-gameplay snbs
static bool IsCameraMotionCut(uint32_t cameraMotion, float frame)
{
	auto base = g_memory.base;
	const uint32_t trackCount = X86_LOAD_U32(cameraMotion + 0xC);
	const uint32_t tracks = X86_LOAD_U32(cameraMotion + 0x10);

	for (uint32_t i = 0; i < trackCount; i++)
	{
		const uint32_t track = tracks + i * 0x28;
		const uint32_t channelType = X86_LOAD_U32(track) & 0xFFFFFF00;
		const uint32_t interpolation = X86_LOAD_U32(track + 0x4) & 0xFFFF;
		const bool isPosition = channelType == 0x100 || channelType == 0x200 || channelType == 0x400;
		const bool isTarget = channelType == 0x40000 || channelType == 0x80000 || channelType == 0x100000;

		if ((!isPosition && !isTarget) || (interpolation != 2 && interpolation != 4 && interpolation != 0x20))
		{
			continue;
		}

		if (frame < X86_LOAD_F32(track + 0x14) || frame > X86_LOAD_F32(track + 0x18))
		{
			continue;
		}

		const uint32_t keyCount = X86_LOAD_U32(track + 0x1C);
		const uint32_t keySize = X86_LOAD_U32(track + 0x20);
		const uint32_t keys = X86_LOAD_U32(track + 0x24);
		auto keyFrame = [&](uint32_t index) { return X86_LOAD_F32(keys + index * keySize); };
		auto keyValue = [&](uint32_t index) { return X86_LOAD_F32(keys + index * keySize + 0x4); };

		if (keyCount < 2 || frame < keyFrame(0))
		{
			continue;
		}

		uint32_t keyIndex = 0;
		uint32_t upperIndex = keyCount;
		while (upperIndex - keyIndex > 1)
		{
			const uint32_t middleIndex = (keyIndex + upperIndex) / 2;
			if (keyFrame(middleIndex) <= frame)
			{
				keyIndex = middleIndex;
			}
			else
			{
				upperIndex = middleIndex;
			}
		}

		if (keyIndex + 1 >= keyCount || keyFrame(keyIndex + 1) - keyFrame(keyIndex) > 1.0f)
		{
			continue;
		}

		float neighbourSlope = 0.0f;
		if (keyIndex > 0)
		{
			neighbourSlope = std::fabs(keyValue(keyIndex) - keyValue(keyIndex - 1)) / (keyFrame(keyIndex) - keyFrame(keyIndex - 1));
		}

		if (keyIndex + 2 < keyCount)
		{
			neighbourSlope = std::max(neighbourSlope, std::fabs(keyValue(keyIndex + 2) - keyValue(keyIndex + 1)) / (keyFrame(keyIndex + 2) - keyFrame(keyIndex + 1)));
		}

		const float jump = std::fabs(keyValue(keyIndex + 1) - keyValue(keyIndex));
		if (jump > g_eventCameraCutMinJump && jump > neighbourSlope * g_eventCameraCutSlopeRatio)
		{
			return true;
		}
	}
	return false;
}

// Event camera frame
// use prior frame on a camera switch to avoid the awkward snap at 61FPS+
void EventCameraFrameMidAsmHook(X86Register& esi, X86Register& esp)
{
	if (!IsCutsceneFramerateUnlocked())
	{
		return;
	}
	auto base = g_memory.base;
	const float frame = X86_LOAD_F32(g_eventClockFrame);
	g_isEventCameraCutting = IsCameraMotionCut(X86_LOAD_U32(esi.u32), frame);
	X86_STORE_F32(esp.u32 + 0x4, g_isEventCameraCutting ? std::trunc(frame) : frame);
}

// sub_22A540 (Single Frame Mode setter for events)
// If true we skip the set, if false we let it enable the mode
bool EventMotionSingleFrameMidAsmHook()
{
	return IsCutsceneFramerateUnlocked();
}

// Event actor frame
void EventActorFrameMidAsmHook(X86Register& ebp)
{
	if (g_isEventCameraCutting)
	{
		auto base = g_memory.base;
		X86_STORE_F32(ebp.u32 + 0xC, std::trunc(X86_LOAD_F32(ebp.u32 + 0xC)));
	}
}

static constexpr float g_eventMotionPopMinJump = 0.05f;
static constexpr float g_eventMotionPopSlopeRatio = 4.0f;

// Event actor update (calls sub_36E270)
X86_FUNC_IMPL(__imp__sub_22A240);
X86_FUNC(sub_22A240)
{
	g_isSamplingEventMotion = IsCutsceneFramerateUnlocked();
	__imp__sub_22A240(ctx, base);
	g_isSamplingEventMotion = false;
}

// motion key (sub_37A6B0)
// +0x0 frame
// +0x4 in slope
// +0x8 out slope
// +0xC value
// 1 frame apart with an extreme jump = a teleport
static bool IsEventMotionPop(uint8_t* base, uint32_t prevKey, uint32_t nextKey)
{
	if (X86_LOAD_F32(nextKey) - X86_LOAD_F32(prevKey) > 1.0f)
	{
		return false;
	}

	float maxSlope = 0.0f;
	for (uint32_t slopeAddr : { prevKey + 0x4, prevKey + 0x8, nextKey + 0x4, nextKey + 0x8 })
	{
		maxSlope = std::max(maxSlope, std::fabs(X86_LOAD_F32(slopeAddr)));
	}

	const float jump = std::fabs(X86_LOAD_F32(nextKey + 0xC) - X86_LOAD_F32(prevKey + 0xC));
	return jump > g_eventMotionPopMinJump && jump > maxSlope * g_eventMotionPopSlopeRatio;
}

// Motion key interpolation
// Single frames are blended equally
X86_FUNC_IMPL(__imp__sub_379D20);
X86_FUNC(sub_379D20)
{
	if (!g_isSamplingEventMotion)
	{
		__imp__sub_379D20(ctx, base);
		return;
	}

	const uint32_t prevKey = X86_LOAD_U32(ctx.esp.u32 + 0x4);
	const uint32_t nextKey = X86_LOAD_U32(ctx.esp.u32 + 0x8);
	const float sampleFrame = X86_LOAD_F32(ctx.esp.u32 + 0xC);
	const float singleFrame = std::floor(sampleFrame);

	if (singleFrame < X86_LOAD_F32(prevKey) || singleFrame + 1.0f > X86_LOAD_F32(nextKey))
	{
		__imp__sub_379D20(ctx, base);
		return;
	}

	X86_STORE_F32(ctx.esp.u32 + 0xC, singleFrame);
	__imp__sub_379D20(ctx, base);

	if (sampleFrame == singleFrame || IsEventMotionPop(base, prevKey, nextKey))
	{
		return;
	}

	const double valueAtSingleFrame = ctx.fpu.pop();
	ctx.esp.u32 -= 0x4;
	X86_STORE_F32(ctx.esp.u32 + 0xC, singleFrame + 1.0f);
	__imp__sub_379D20(ctx, base);

	const double value = ctx.fpu.st0();
	ctx.fpu.st0() = valueAtSingleFrame + (value - valueAtSingleFrame) * (double)(sampleFrame - singleFrame);
}

// Event stream wait
// sub_228740 pauses to align with the audio track
void EventStreamWaitMidAsmHook(X86Register& ecx)
{
	if (!IsCutsceneFramerateUnlocked())
	{
		return;
	}

	auto base = g_memory.base;
	g_eventStreamWaitCarry += GetEventDeltaTime() * X86_LOAD_F32(g_eventFramesPerSecond);
	if (g_eventStreamWaitCarry < 1.0f)
	{
		ecx.u32 -= 1;
		return;
	}
	g_eventStreamWaitCarry = std::min(g_eventStreamWaitCarry - 1.0f, 1.0f);
}

// TPjsEventActionFade::Exec
// type 2 is for 2P battle only
// At higher framerates the 2P fade ends too early
X86_FUNC_IMPL(__imp__sub_2280D0);
X86_FUNC(sub_2280D0)
{
	if (IsCutsceneFramerateUnlocked() && X86_LOAD_U32(ctx.ecx.u32 + 0x28) == 2) // checking for black fade mode (2P)
	{
		g_eventBattleFadeHoldTime += X86_LOAD_F32(ctx.esp.u32 + 0x4);
		if (g_eventBattleFadeHoldTime < 1.0f / X86_LOAD_F32(g_eventFramesPerSecond))
		{
			ctx.esp.u32 += 8;
			return;
		}
		g_eventBattleFadeHoldTime = 0.0f;
	}
	__imp__sub_2280D0(ctx, base);
}
/** --Event FPS Patch END-- */

// sub_237DA0 used by jingle/music/fade timing
// Per frame step
// 0x456784 = goal tick rate
static float g_goalActionTickCarry = 0.0f;
void GoalActionTicksMidAsmHook(X86Register& ticks)
{
	auto base = g_memory.base;
	g_goalActionTickCarry += g_unroundedDeltaTime * X86_LOAD_F32(0x456784);
	const float flooredTicks = std::floor(g_goalActionTickCarry);
	g_goalActionTickCarry -= flooredTicks;
	ticks.u32 = (uint32_t)flooredTicks;
}

// Boss / special camera 61FPS+ fix
X86_FUNC_IMPL(__imp__sub_23950);
X86_FUNC(sub_23950)
{
	const uint32_t cameraMotion = X86_LOAD_U32(ctx.esp.u32 + 0x8);
	const float frame = X86_LOAD_F32(ctx.esp.u32 + 0xC);
	for (float offset : { -1.0f, 0.0f, 1.0f })
	{
		if (IsCameraMotionCut(cameraMotion, frame + offset))
		{
			X86_STORE_F32(ctx.esp.u32 + 0xC, std::trunc(frame));
			break;
		}
	}
	__imp__sub_23950(ctx, base);
}

/** --Weapon fire rate Patch START-- */
// This is a pretty minor fix. Fix auto weapon fire rate to align with 60FPS behavior
// For 2PGunAndroid for example:
// Wait is 100ms + one frame
// at 60fps its 100ms + 16ms
// at 240fps its 100ms + 4ms
static void WeaponFireRate60FPS(uint8_t* base, uint32_t control, float prevCooldown, float dt)
{
	const float step = 1.0f / 60.0f;
	const float cooldown = X86_LOAD_F32(control + 0x10);
	if (dt >= step || cooldown <= 0.0f || cooldown <= prevCooldown)
	{
		return;
	}

	int frames = 0;
	float left = cooldown;
	while (left > 0.0f)
	{
		left -= step;
		frames++;
	}
	X86_STORE_F32(control + 0x10, frames * step - dt * 0.5f);
}

// ControlCannon
X86_FUNC_IMPL(__imp__sub_A0CD0);
X86_FUNC(sub_A0CD0)
{
	const uint32_t control = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 0x4);
	const float cooldown = X86_LOAD_F32(control + 0x10);
	__imp__sub_A0CD0(ctx, base);
	WeaponFireRate60FPS(base, control, cooldown, dt);
}

// ControlGun
X86_FUNC_IMPL(__imp__sub_A1740);
X86_FUNC(sub_A1740)
{
	const uint32_t control = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 0x4);
	const float cooldown = X86_LOAD_F32(control + 0x10);
	__imp__sub_A1740(ctx, base);
	WeaponFireRate60FPS(base, control, cooldown, dt);
}
/** --Weapon fire rate Patch END-- */
