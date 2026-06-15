#include "global.h"
#include <kernel/function.h>

// Uncomment the define below to enable
//#define EnableReloaded

// Inlined Homing Attack Speed 1000f -> 2000f [Reloaded Phys]
void HomingAttackSpeedMidAsmHook(X86Register& esp)
{
	auto base = g_memory.base;
	esp.u32 -= 4;
#ifdef EnableReloaded
    X86_STORE_F32(esp.u32, 2000.0f);
#else
    X86_STORE_F32(esp.u32, 1000.0f);  // 0x447A0000, original
#endif
}

void ReloadedValuePatches()
{
#ifdef EnableReloaded
	auto base = g_memory.base;

	// Widescreen Hack (SP CAM/RENDER)
	const uint32_t singlePlayerWidescreen = 0x4E5D6C;
	X86_STORE_F32(singlePlayerWidescreen, 1.33333337306976f);

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

	// Custom Autosave Colors

	// Ghost Red Rings (STG_CMN REDIRECT)

	// Respawn at last activated checkpoint

	// Hold L to JumpDash instead of HomingAttack

	// Disable Slide on B-press on ground and state extension

	// No Tri-Jump Exit

	// No Skidding

	// GUNMech Physics Revamp


	// Circuit Speed Control

	// Jump Ball Turn Speed

	// Chaos Blast Damage
	const uint32_t chaosBlastDamageAddr = 0x4EBE4C;
	X86_STORE_F32(chaosBlastDamageAddr, 36.0f);

	// Jump Attack Friction

	// Light Dash ValidRing Angle

	// Homing Attack Speed

	// Homing Attack Turn Strength

	// Triangle Jump Recover Time

	// Triangle Jump Speed

	// WallRun Speed

	// Walk Turn Speed

	// Heal Unit Explosion Hitbox Size

	// Instant HealUnit / BombUnit Server Respawn Time

	// Coaster/Zipline Fix Bad Collision

	// Pulley Speed

	// Idle Chatter Tweak

	// Triple Spring Origin Shift

	// Keep Melee Weapons when Mounting Vehicles

	// Super AirSaucer
		//	RotSpeed 250 -> 8000
	const uint32_t airSaucerRotSpeedAddr = 0x471B10;
	X86_STORE_F32(airSaucerRotSpeedAddr, 8000.0f);
		//	GroundSpeed 270 -> 540
	const uint32_t airSaucerGroundSpeedAddr = 0x471AF4;
	X86_STORE_F32(airSaucerGroundSpeedAddr, 540.0f);
		//	AirSpeed 220 -> 440
	const uint32_t airSaucerAirSpeedAddr = 0x471AE4;
	X86_STORE_F32(airSaucerAirSpeedAddr, 440.0f);
		//	JumpStrength 100 -> 200 [needs PUSH patch - inlined]

	// AirSaucer Replace Second Jump with Stomp

	// LightDash Transition Expansion [2P COMPAT]

	// Global Lock-On Revamp

	// EscapePlane Stat Adjustment

	// EggBalloon Stat Adjustment

	// SolidCollision Triggers are GroundCollision

	// Time Bonus Penalty Always Active

	// Last Way & Devil Doom in Select Menu - CONSIDER EXTENDING / modifying visual with IMGUI?

	// CarType Vehicles Revamp

	// Enemy AppearType WARP Faster Spawn

	// Restore EggBreaker0411 Digitize Effect

	// The Last Way Inclusive Scoring

	// ElecCode Enemy Faster Movement Speed

	// The Last Way idle lines

	// Cannon & Lock On Weapons No Knockback

	// Melee Weapon Core Revamp [2P COMPAT]

	// Melee Ammo Adjustments

	// Select Mode cursor no reset

	// LightDash Exit State Change to Launched for Westopolis

	// Devil Doom Boss Has No Awake Music

	// Light Dash Valid Ring Distance
	// 80 -> 100
	const uint32_t lightDashValidRingDistanceAddr = 0x4ECDFC; // NOTE: PROBABLY SHARED; TODO Instead should patch ctor read!!
	X86_STORE_F32(lightDashValidRingDistanceAddr, 100.0f);

	// Slide Steer

	// Slide Run Threshhold

	// Spindash Startup Delay

	// Strafe Walk Speed

	// Red Slime Speed

	// Skippable Startup/Boot Logos

	// Beating Last Story unlocks Expert

	// Expert Ring Loss

	// Holdable Slide

	// Devil Doom is a Ranked Mission

	// S Rank System - Overhaul?

	// Fix Sliding on Steep Slopes

	// Disable CarType Vehicle Drifting

	// EggPierrot Stage Fix

	// SuperShadow Always Has Fully Charged Spears while ChaosPowers Active [2P COMPAT]

	// LaserRifle Damage Adjustment

	// Slide State Extension - Bad Transition Bugfix

	// Disable Diablon Warp Player Function
#endif
}
