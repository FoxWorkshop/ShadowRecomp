#include "global.h"
#include <kernel/function.h>
#include <input/padstate.h>
#include <user/config.h>

// Player::Behaviors @ 0x4EEE58
enum PlayerBehaviorId : uint32_t
{
	BEHAVIOR_FALL = 0x2,
	BEHAVIOR_JUMP_DASH = 0x6,
	BEHAVIOR_HOMING_ATTACK = 0x8,
	BEHAVIOR_HOMING_JUMP = 0x9,
	BEHAVIOR_SPIN_DASH = 0x12,
	BEHAVIOR_LAUNCH = 0x17,
};

static constexpr uint32_t g_currentStageId = 0x54DDD8;

static uint32_t GetBehaviorId(uint32_t player)
{
	auto base = g_memory.base;
	const uint32_t executer = X86_LOAD_U32(player + 0x22C);
	if (executer == 0)
	{
		return UINT32_MAX;
	}
	const uint32_t behavior = X86_LOAD_U32(executer + 0x30);
	return behavior != 0 ? X86_LOAD_U32(behavior + 0x10) : UINT32_MAX;
}

// Inlined Homing Attack Speed 1000f -> 2000f [Reloaded Phys]
void HomingAttackSpeedMidAsmHook(X86Register& esp)
{
	auto base = g_memory.base;
	esp.u32 -= 4;
	X86_STORE_F32(esp.u32, Config::ReloadedMod ? 2000.0f : 1000.0f);
}

// Circuit Speed Control
// Electric::Apply @ sub_6F9E0
// Spline::Circuit read speed @ 0x4EC5E8
void CircuitSpeedControlMidAsmHook(X86Register& ecx)
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;
	if (!Config::ReloadedMod)
	{
		ctx->xmm[0].f32[0] = X86_LOAD_F32(0x4EC5E8);
		return;
	}

	// ecx = Behavior::ElectricCircuit
	// ecx+4 = player instance
	uint32_t player = X86_LOAD_U32(ecx.u32 + 4);
	uint32_t padIndex = 0;
	for (uint32_t i = 0; i < 4; i++)
	{
		if (g_playerStructAddrs[i] == player)
		{
			padIndex = i;
			break;
		}
	}
	float speed = 800.0f;
	if (PJSPeripheral::IsHeld(padIndex, PJSPeripheral::BTN_A))
	{
		speed = 1600.0f;
	}
	else if (PJSPeripheral::IsHeld(padIndex, PJSPeripheral::BTN_X))
	{
		speed = 400.0f;
	}
	ctx->xmm[0].f32[0] = speed;
}

// GUNMech Walker Revamp
bool WalkerDismountAnywhereMidAsmHook()
{
	return Config::ReloadedMod;
}

void WalkerHoverMoveMidAsmHook(X86Register& esi)
{
	if (!Config::ReloadedMod)
	{
		return;
	}

	auto base = g_memory.base;
	if ((X86_LOAD_U8(esi.u32 + 0x104) & 0x8) == 0)
	{
		return;
	}

	// Get the vehicle stats
	const float friction = X86_LOAD_F32(esi.u32 + 0xE8);
	const float driveMult = X86_LOAD_F32(esi.u32 + 0xE0);
	const float walkFriction = X86_LOAD_F32(0x4721CC);

	if (friction <= 0.0f || driveMult <= 0.0f || walkFriction <= 0.0f)
	{
		return;
	}

	const float speed = driveMult / walkFriction;
	float stickX = X86_LOAD_F32(esi.u32 + 0xF8);
	float stickZ = X86_LOAD_F32(esi.u32 + 0x100);

	X86_STORE_F32(esi.u32 + 0x28, stickX * speed * friction);
	X86_STORE_F32(esi.u32 + 0x30, stickZ * speed * friction);
	X86_STORE_F32(esi.u32 + 0xE4, friction);
	X86_STORE_F32(esi.u32 + 0xE8, friction);

	// Rotation fix
	X86_STORE_F32(esi.u32 + 0xF8, 0.0f);
	X86_STORE_F32(esi.u32 + 0xFC, 0.0f);
	X86_STORE_F32(esi.u32 + 0x100, 0.0f);
}

// Restore EggBreaker0411 Digitize Effect
// true skips mov [edx+0x24A], al
bool EggBreakerDigitizeEffectMidAsmHook()
{
	return Config::ReloadedMod;
}

// Disable Slide on B-press on ground and state extension
bool SlideStateExtensionMidAsmHook(X86Register& eax)
{
	auto base = g_memory.base;
	const uint32_t player = eax.u32;
	if (!Config::ReloadedMod || X86_LOAD_U32(player + 0x238) == 0)
	{
		return (X86_LOAD_U32(player + 0xA0) & 0x10) != 0;
	}

	const uint32_t behavior = GetBehaviorId(player);
	return behavior == BEHAVIOR_JUMP_DASH || behavior == BEHAVIOR_HOMING_ATTACK || behavior == BEHAVIOR_SPIN_DASH;
}

// Slide State Extension - Bad Transition Bugfix
void SlideBadTransitionMidAsmHook(X86Register& ecx)
{
	if (Config::ReloadedMod && (ecx.u32 == 0xF8 || ecx.u32 == 0xF9))
	{
		ecx.u32 = 0xFA;
	}
}

// Slide Steer 2 -> 4
void SlideSteerMidAsmHook()
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;
	ctx->xmm[0].f32[0] *= Config::ReloadedMod ? 4.0f : X86_LOAD_F32(0x429F74);
}

static bool IsSlideHeld(uint32_t sliding)
{
	auto base = g_memory.base;
	const uint32_t player = X86_LOAD_U32(sliding + 0x4);
	const uint32_t input = X86_LOAD_U32(player + 0x218);

	// 0x18 = held actions @ 0x449CF8
	// & 0x60 = X or B
	return input != 0 && (X86_LOAD_U32(input + 0x18) & 0x60) != 0;
}

bool HoldableSlideMidAsmHook(X86Register& ebx)
{
	return Config::ReloadedMod && IsSlideHeld(ebx.u32);
}

bool HoldableSlideReleaseMidAsmHook(X86Register& ebx)
{
	return Config::ReloadedMod && !IsSlideHeld(ebx.u32);
}

// TODO: Might move this as an always on patch, haven't decided, arguably its a bug
// Fix Sliding on Steep Slopes
bool SlideSteepSlopeMidAsmHook()
{
	return Config::ReloadedMod;
}

// LightDash Transition Expansion
bool LightDashTransitionMidAsmHook(X86Register& eax)
{
	auto base = g_memory.base;
	const uint32_t player = eax.u32;
	if ((X86_LOAD_U32(player + 0xA0) & 0x8) != 0)
	{
		return true;
	}

	if (!Config::ReloadedMod)
	{
		return false;
	}

	const uint32_t behavior = GetBehaviorId(player);
	return behavior == BEHAVIOR_JUMP_DASH || behavior == BEHAVIOR_HOMING_JUMP || behavior == BEHAVIOR_LAUNCH;
}

// LightDash Exit State Change to Launched for Westopolis
void LightDashExitStateMidAsmHook(X86Register& esp)
{
	auto base = g_memory.base;
	if (!Config::ReloadedMod || X86_LOAD_U32(g_currentStageId) != 100)
	{
		X86_STORE_U32(esp.u32 + 0x3C, BEHAVIOR_FALL);
		return;
	}

	// InitializeArgsLaunch
	// TODO: Research this more to see if we can disable the radial blur in args instead of in a separate patch
	X86_STORE_U32(esp.u32 + 0x38, 0x448FAC);
	X86_STORE_U32(esp.u32 + 0x3C, BEHAVIOR_LAUNCH);
	X86_STORE_U32(esp.u32 + 0x40, 0x0); // skydive
}

// WallRun Speed 500 -> 550
void WallRunSpeedMidAsmHook(X86Register& esp)
{
	auto base = g_memory.base;
	esp.u32 -= 4;
	X86_STORE_F32(esp.u32, Config::ReloadedMod ? 550.0f : 500.0f);
}

// Triangle Jump Speed 450 -> 1000
void TriangleJumpSpeedMidAsmHook(X86Register& esp)
{
	auto base = g_memory.base;
	esp.u32 -= 4;
	X86_STORE_F32(esp.u32, Config::ReloadedMod ? 1000.0f : 450.0f);
}

// No Tri-Jump Exit
bool NoTriJumpExitMidAsmHook()
{
	if (!Config::ReloadedMod)
	{
		return false;
	}

	auto ctx = GetX86Context();
	ctx->xmm[0].f32[0] = 0.0f;
	return true;
}

// Walk Turn Speed 15 -> 20
void WalkTurnSpeedMidAsmHook(X86Register& esp)
{
	auto base = g_memory.base;
	esp.u32 -= 4;
	X86_STORE_F32(esp.u32, Config::ReloadedMod ? 20.0f : 15.0f);
}

// No Skidding
bool NoSkiddingMidAsmHook()
{
	return Config::ReloadedMod;
}

// Keep Melee Weapons when Mounting Vehicles
bool KeepMeleeOnVehicleMidAsmHook()
{
	return Config::ReloadedMod;
}

// CarType Vehicles Revamp
bool CarGasReleaseMidAsmHook()
{
	if (!Config::ReloadedMod)
	{
		return false;
	}

	auto ctx = GetX86Context();
	ctx->xmm[1].f32[0] = ctx->xmm[0].f32[0];
	return true;
}

bool CarBrakeButtonMidAsmHook(X86Register& eax, X86Register& edx)
{
	auto base = g_memory.base;
	const uint8_t held = X86_LOAD_U8(eax.u32 + edx.u32 * 8);
	return (held & (Config::ReloadedMod ? PJSPeripheral::BTN_Y : PJSPeripheral::BTN_B)) != 0;
}

bool CarRevampMidAsmHook()
{
	return Config::ReloadedMod;
}

// TODO: I may want to rework this later to drift with LB and RB, but tabling that for now
bool CarNoDriftMidAsmHook()
{
	return Config::ReloadedMod;
}

void ReloadedValuePatches()
{
	if (!Config::ReloadedMod)
	{
		return;
	}

	auto base = g_memory.base;

	// 2P Unique Android JumpBall Colors

	// 3 Endings unlocks Last Story

	// Title always skippable

	// Missions Unlocked - Missions can always be switched, regardless if you've met the mission character or not.

	// Mission Focus

	// Life System Modifier

	// Music Update Ending Tracks and Lava Shelter Boss

	// Expert Select

	// Expert Ending SFD Overrides

	// Select Mode Shortcut

	// Ghost Red Rings (STG_CMN REDIRECT)

	// Respawn at last activated checkpoint

	// Jump Ball Turn Speed 6.5 -> 25
	X86_STORE_F32(0x448414, 25.0f);

	// Chaos Blast Damage 12 -> 36
	X86_STORE_F32(0x4EBE4C, 36.0f);

	// Jump Attack Friction 10 -> 4
	X86_STORE_F32(0x4ED69C, 4.0f);

	// Light Dash ValidRing Angle 45 -> 75
	X86_STORE_F32(0x448708, 75.0f);

	// Homing Attack Speed

	// Homing Attack Turn Strength 8 -> 16
	X86_STORE_F32(0x448268, 16.0f);

	// Triangle Jump Recover Time 0.125 -> 0.000001
	X86_STORE_F32(0x448D1C, 0.000001f);

	// Walk Turn Speed 15 -> 20
	X86_STORE_F32(0x448100, 20.0f);

	// Heal Unit Explosion Hitbox Size

	// Instant HealUnit / BombUnit Server Respawn Time

	// Pulley Speed

	// Idle Chatter Tweak

	// Triple Spring Origin Shift

	// == Super AirSaucer ==
	// RotSpeed 250 -> 8000
	X86_STORE_F32(0x471B10, 8000.0f);
	// GroundSpeed 270 -> 540
	X86_STORE_F32(0x471AF4, 540.0f);
	// AirSpeed 220 -> 440
	X86_STORE_F32(0x471AE4, 440.0f);
	// JumpStrength 100 -> 200 [needs PUSH patch - inlined]
	// AirSaucer Replace Second Jump with Stomp
	// == Super AirSaucer ==

	// Global Lock-On Revamp

	// EscapePlane Stat Adjustment

	// EggBalloon Stat Adjustment

	// SolidCollision Triggers are GroundCollision

	// Time Bonus Penalty Always Active

	// Last Way & Devil Doom in Select Menu - CONSIDER EXTENDING / modifying visual with IMGUI?

	// Enemy AppearType WARP Faster Spawn

	// The Last Way Inclusive Scoring

	// ElecCode Enemy Faster Movement Speed

	// The Last Way idle lines

	// Cannon & Lock On Weapons No Knockback

	// Melee Weapon Core Revamp [2P COMPAT]

	// Melee Ammo Adjustments

	// Select Mode cursor no reset

	// Devil Doom Boss Has No Awake Music

	// Light Dash Valid Ring Distance
	// 80 -> 100
	X86_STORE_F32(0x4ECDFC, 100.0f);

	// Slide Run Threshhold
	// 200 -> 75
	X86_STORE_F32(0x4EC7FC, 75.0f);

	// Spindash Startup Delay 0.25 -> 1
	X86_STORE_F32(0x4ED19C, 1.0f);

	// Strafe Walk Speed 30 -> 55
	X86_STORE_F32(0x4EC800, 55.0f);

	// Red Slime Speed

	// Beating Last Story unlocks Expert

	// Expert Ring Loss

	// Devil Doom is a Ranked Mission

	// S Rank System - Overhaul?

	// SuperShadow Always Has Fully Charged Spears while ChaosPowers Active [2P COMPAT]

	// LaserRifle Damage Adjustment

	// Disable Diablon Warp Player Function
}
