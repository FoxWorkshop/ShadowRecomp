// TODO: Move any mod loader cheat code related patches here
// TODO: e.g. Chaos Power refills, vac patch, event skip, etc
// TODO: Depending on how we handle code mods, this may be redundant/obsolete

#include <kernel/function.h>
#include "global.h"
#include <input/padstate.h>
#include <user/config.h>

// TODO: Move to code or scrap
// Fix the missing ScoreCommand if vacuum picks up fire (branch is skipped since owner check fails)
// Fire VacuumCommand handler @ 0x1AA820
// Fire attack callback @ 0x19B550
// X86_FUNC_IMPL(__imp__sub_5C4D0); // ScoreCommand
// void FireVacuumCallbackMidAsmHook(X86Register& ebp, X86Register& edi)
// {
//     auto ctx = GetX86Context();
//     auto base = g_memory.base;
//
//     // ebp = GadgetCharInfo (object being sucked)
//     // ebp+0x50 = GadgetID, 0x1A = Fire
//     // ebp+0x64 = CharCommandUnit, handler instance is 0x8 inside CharCommandUnit
//     // edi = Vacuum weapon instance (0xC = player who is holding it)
//     if (ebp.u32 == 0 || X86_LOAD_U32(ebp.u32 + 0x50) != 0x1A)
//     {
//         // skip if not fire
//         return;
//     }
//
//     auto cmdUnit = X86_LOAD_U32(ebp.u32 + 0x64);
//     if (cmdUnit == 0) // command null check (matching OG)
//     {
//         return;
//     }
//
//     auto node = X86_LOAD_U32(cmdUnit + 8);
//     if (node == 0 || X86_LOAD_U32(node) != 0x1AA820) { // check for Fire/PoisonGas handler
//         return;
//     }
//
//     // 0x466F6C = Fire & PoisonGas vtable (shared, but PoisonGas has no score attribute)
//     // fire+0x50 = state
//     auto fire = X86_LOAD_U32(node + 4);
//     if (X86_LOAD_U32(fire) != 0x466F6C || X86_LOAD_U32(fire + 0x50) != 1)
//     {
//         return;
//     }
//
//     // safety check if the vac command drops
//     if (X86_LOAD_U32(edi.u32 + 0xC) == 0)
//     {
//         return;
//     }
//
//     ctx->esp.u32 -= 4;
//     X86_STORE_U32(ctx->esp.u32, 0x1B); // score callback entry
//     ctx->esp.u32 -= 4;
//     X86_STORE_U32(ctx->esp.u32, edi.u32 + 0xC); // player instance
//     ctx->esp.u32 -= 4;
//     __imp__sub_5C4D0(*ctx, base);
//     ctx->esp.u32 += 8;
// }

// Expert Select (port of Reloaded v1.2 version)
// TODO: Switch to Zetti's version before release
// TODO: Move to reloaded or cheat code only (probably a code)
// This one has the bug where it can apply to Story Mode / Last Story too
void ExpertSelectMidAsmHook(X86Register& edx)
{
    if (PJSPeripheral::IsHeld(0, PJSPeripheral::BTN_X))
    {
        auto base = g_memory.base;
        X86_STORE_U8(edx.u32 + 0x59, 1);
    }
}


// CreateBehavior (all player actions)
//
// Table of all PlayerBehavior @ 0x4EEE58
// 0x6 = JumpDash
// 0x8 = HomingAttack
// TODO: We will extend this further to enable state transitions like SpinDash->Slide etc in the future
// TODO: Probably move this to ReloadedPatches later, but for now keep it in
X86_FUNC_IMPL(__imp__sub_977D0);
X86_FUNC(sub_977D0)
{
    // esp+8 = BehaviorId
    // ecx+4 = PlayerStruct targetted by Behavior
    if (X86_LOAD_U32(ctx.esp.u32 + 8) == 8) // HomingAttack
    {
        uint32_t player = X86_LOAD_U32(ctx.ecx.u32 + 4);
        for (uint32_t i = 0; i < 4; i++) // Checking player structs 1-4 TODO: Maybe grab the extended multiplayer value later?
        {
            if (g_playerStructAddrs[i] == player)
            {
                // TODO: Account for this if we do Action remapping; Input remapping should be fine to auto handle this
                if (PJSPeripheral::IsHeld(i, PJSPeripheral::BTN_LEFT_TRIG) ||
                    PJSPeripheral::IsHeld(i, PJSPeripheral::BTN_WHITE))
                {
                    X86_STORE_U32(ctx.esp.u32 + 8, 6); // Change target Behavior to JumpDash
                }
                break;
            }
        }
    }
    __imp__sub_977D0(ctx, base);
}

// Chaos Gauge Fill Cheat
// RB + R fills the hero gauge, RB + L fills the dark one.
static constexpr uint32_t HeroGaugeOffset = 0x70C;
static constexpr uint32_t DarkGaugeOffset = 0x718;

static bool s_heroComboHeld[4] = {};
static bool s_darkComboHeld[4] = {};

static void FillChaosGauge(uint32_t gauge, uint32_t player)
{
    auto base = g_memory.base;
    uint32_t gameState = X86_LOAD_U32(0x54963C);
    if (!gameState)
        return;

    uint32_t max = X86_LOAD_U32(gameState + gauge + 4);

    if ((int32_t)max <= 0)
        return;

    X86_STORE_U32(gameState + gauge, max);
}

static void HandleChaosGaugeCheat(X86Context& ctx, uint8_t* base)
{
    const bool isCCCheat = Config::FillChaosControlGaugeOnInput;
    const bool isCBCheat = Config::FillChaosBlastGaugeOnInput;

    if (isCCCheat || isCBCheat)
    {
        uint32_t player = X86_LOAD_U32(ctx.ecx.u32 + 0x28);
        uint32_t pad = 0;
        for (uint32_t i = 0; i < 4; i++)
        {
            if (g_playerStructAddrs[i] == player)
            {
                pad = i;
                break;
            }
        }

        using namespace PJSPeripheral;
        bool hero = IsHeld(pad, BTN_BLACK) && IsHeld(pad, BTN_RIGHT_TRIG);
        bool dark = IsHeld(pad, BTN_BLACK) && IsHeld(pad, BTN_LEFT_TRIG);

        if (player && hero && !s_heroComboHeld[pad] && isCCCheat)
            FillChaosGauge(HeroGaugeOffset, player);
        else if (player && dark && !s_darkComboHeld[pad] && isCBCheat)
            FillChaosGauge(DarkGaugeOffset, player);

        s_heroComboHeld[pad] = hero;
        s_darkComboHeld[pad] = dark;
    }
}

// ShadowExecuter::Exec
X86_FUNC_IMPL(__imp__sub_98670);
X86_FUNC(sub_98670)
{
    HandleChaosGaugeCheat(ctx, base);
    __imp__sub_98670(ctx, base);
}

// Infinite Ammo Cheat
// If the game is removing an ammo, swap it to 0 so shot cost nothing
X86_FUNC_IMPL(__imp__sub_9FC00);
X86_FUNC(sub_9FC00)
{
    if (Config::InfiniteAmmo && (int32_t)X86_LOAD_U32(ctx.esp.u32 + 4) < 0)
    {
        X86_STORE_U32(ctx.esp.u32 + 4, 0);
    }
    __imp__sub_9FC00(ctx, base);
}

// Vacuum Any Enemy Cheat
// Both Vacuum weapons look for the collisions flag 0x80 to detect if they can work on the current enemy
// The game don't use it on some enemy like the BkNinja and Big Foot, so we add it manually 
void VacuumAnyEnemyColliMidAsmHook(X86Register& ecx)
{
    if (Config::VacuumAnyEnemy)
        ecx.u32 |= 0x80;
}

// Both BkWingLarge and BkChaos can resist the vacuum command, force it to always work.
bool VacuumAnyEnemySkipResistMidAsmHook()
{
    return Config::VacuumAnyEnemy;
}

// BkNinja puts a head aim callback on one bone with the enemy as data, which makes the game reads freed memory if you suck it, this eventually corrupts and crash the game.
// We can fix that by clearing all callback nodes.
static void ClearNodeCallbacks(uint8_t* base, uint32_t node, uint32_t depth)
{
    while (node != 0 && depth < 64)
    {
        X86_STORE_U32(node + 0x1C, 0); // callback
        X86_STORE_U32(node + 0x20, 0); // callback data
        ClearNodeCallbacks(base, X86_LOAD_U32(node + 0x24), depth + 1); // child
        node = X86_LOAD_U32(node + 0x28); // sibling
    }
}

// CEnemyVacuumObject ctor
X86_FUNC_IMPL(__imp__sub_CEED0);
X86_FUNC(sub_CEED0)
{
    uint32_t vacuumObject = ctx.ecx.u32;
    __imp__sub_CEED0(ctx, base);

    if (!Config::VacuumAnyEnemy)
        return;

    uint32_t model = X86_LOAD_U32(vacuumObject + 0x34);
    if (model == 0 || X86_LOAD_U32(model) == 0)
        return;

    uint32_t hierarchy = X86_LOAD_U32(X86_LOAD_U32(model) + 8);
    if (hierarchy != 0)
        ClearNodeCallbacks(base, X86_LOAD_U32(hierarchy + 0xC), 0);
}

