#pragma once

#include "variant_000.hlsl.spirv.h"
#include "variant_001.hlsl.spirv.h"
#include "variant_002.hlsl.spirv.h"
#include "variant_003.hlsl.spirv.h"
#include "variant_004.hlsl.spirv.h"
#include "variant_005.hlsl.spirv.h"
#include "variant_006.hlsl.spirv.h"
#include "variant_007.hlsl.spirv.h"
#include "variant_008.hlsl.spirv.h"
#include "variant_009.hlsl.spirv.h"
#include "variant_010.hlsl.spirv.h"
#include "variant_011.hlsl.spirv.h"
#include "variant_012.hlsl.spirv.h"
#include "variant_013.hlsl.spirv.h"
#include "variant_014.hlsl.spirv.h"
#include "variant_015.hlsl.spirv.h"
#include "variant_016.hlsl.spirv.h"
#include "variant_017.hlsl.spirv.h"
#include "variant_018.hlsl.spirv.h"
#include "variant_019.hlsl.spirv.h"
#include "variant_020.hlsl.spirv.h"
#include "variant_021.hlsl.spirv.h"
#include "variant_022.hlsl.spirv.h"
#include "variant_023.hlsl.spirv.h"
#include "variant_024.hlsl.spirv.h"
#include "variant_025.hlsl.spirv.h"
#include "variant_026.hlsl.spirv.h"
#include "variant_027.hlsl.spirv.h"
#include "variant_028.hlsl.spirv.h"
#include "variant_029.hlsl.spirv.h"
#include "variant_030.hlsl.spirv.h"
#include "variant_031.hlsl.spirv.h"
#include "variant_032.hlsl.spirv.h"
#include "variant_033.hlsl.spirv.h"
#include "variant_034.hlsl.spirv.h"
#include "variant_035.hlsl.spirv.h"
#include "variant_036.hlsl.spirv.h"
#include "variant_037.hlsl.spirv.h"
#include "variant_038.hlsl.spirv.h"
#include "variant_039.hlsl.spirv.h"
#include "variant_040.hlsl.spirv.h"
#include "variant_041.hlsl.spirv.h"
#include "variant_042.hlsl.spirv.h"
#include "variant_043.hlsl.spirv.h"
#include "variant_044.hlsl.spirv.h"
#include "variant_045.hlsl.spirv.h"
#include "variant_046.hlsl.spirv.h"
#include "variant_047.hlsl.spirv.h"
#include "variant_048.hlsl.spirv.h"
#include "variant_049.hlsl.spirv.h"
#include "variant_050.hlsl.spirv.h"
#include "variant_051.hlsl.spirv.h"
#include "variant_052.hlsl.spirv.h"
#include "variant_053.hlsl.spirv.h"
#include "variant_054.hlsl.spirv.h"
#include "variant_055.hlsl.spirv.h"
#include "variant_056.hlsl.spirv.h"
#include "variant_057.hlsl.spirv.h"
#include "variant_058.hlsl.spirv.h"

#ifdef SHADOW_RECOMP_D3D12
#include "variant_000.hlsl.dxil.h"
#include "variant_001.hlsl.dxil.h"
#include "variant_002.hlsl.dxil.h"
#include "variant_003.hlsl.dxil.h"
#include "variant_004.hlsl.dxil.h"
#include "variant_005.hlsl.dxil.h"
#include "variant_006.hlsl.dxil.h"
#include "variant_007.hlsl.dxil.h"
#include "variant_008.hlsl.dxil.h"
#include "variant_009.hlsl.dxil.h"
#include "variant_010.hlsl.dxil.h"
#include "variant_011.hlsl.dxil.h"
#include "variant_012.hlsl.dxil.h"
#include "variant_013.hlsl.dxil.h"
#include "variant_014.hlsl.dxil.h"
#include "variant_015.hlsl.dxil.h"
#include "variant_016.hlsl.dxil.h"
#include "variant_017.hlsl.dxil.h"
#include "variant_018.hlsl.dxil.h"
#include "variant_019.hlsl.dxil.h"
#include "variant_020.hlsl.dxil.h"
#include "variant_021.hlsl.dxil.h"
#include "variant_022.hlsl.dxil.h"
#include "variant_023.hlsl.dxil.h"
#include "variant_024.hlsl.dxil.h"
#include "variant_025.hlsl.dxil.h"
#include "variant_026.hlsl.dxil.h"
#include "variant_027.hlsl.dxil.h"
#include "variant_028.hlsl.dxil.h"
#include "variant_029.hlsl.dxil.h"
#include "variant_030.hlsl.dxil.h"
#include "variant_031.hlsl.dxil.h"
#include "variant_032.hlsl.dxil.h"
#include "variant_033.hlsl.dxil.h"
#include "variant_034.hlsl.dxil.h"
#include "variant_035.hlsl.dxil.h"
#include "variant_036.hlsl.dxil.h"
#include "variant_037.hlsl.dxil.h"
#include "variant_038.hlsl.dxil.h"
#include "variant_039.hlsl.dxil.h"
#include "variant_040.hlsl.dxil.h"
#include "variant_041.hlsl.dxil.h"
#include "variant_042.hlsl.dxil.h"
#include "variant_043.hlsl.dxil.h"
#include "variant_044.hlsl.dxil.h"
#include "variant_045.hlsl.dxil.h"
#include "variant_046.hlsl.dxil.h"
#include "variant_047.hlsl.dxil.h"
#include "variant_048.hlsl.dxil.h"
#include "variant_049.hlsl.dxil.h"
#include "variant_050.hlsl.dxil.h"
#include "variant_051.hlsl.dxil.h"
#include "variant_052.hlsl.dxil.h"
#include "variant_053.hlsl.dxil.h"
#include "variant_054.hlsl.dxil.h"
#include "variant_055.hlsl.dxil.h"
#include "variant_056.hlsl.dxil.h"
#include "variant_057.hlsl.dxil.h"
#include "variant_058.hlsl.dxil.h"
#endif

namespace FixedFunctionVertexShaderVariants::Generated
{
struct Program
{
    uint64_t key;
    const uint8_t* spirv;
    size_t spirvSize;
#ifdef SHADOW_RECOMP_D3D12
    const uint8_t* dxil;
    size_t dxilSize;
#endif
};

static const Program Programs[] =
{
    {
        0x06D4C3C4C383B150ULL,
        g_variant_027_spirv, sizeof(g_variant_027_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_027_dxil, sizeof(g_variant_027_dxil),
#endif
    },
    {
        0x09860D0D70439218ULL,
        g_variant_056_spirv, sizeof(g_variant_056_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_056_dxil, sizeof(g_variant_056_dxil),
#endif
    },
    {
        0x0AB6EE9AE538EC75ULL,
        g_variant_008_spirv, sizeof(g_variant_008_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_008_dxil, sizeof(g_variant_008_dxil),
#endif
    },
    {
        0x1F49D54881EEE59DULL,
        g_variant_022_spirv, sizeof(g_variant_022_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_022_dxil, sizeof(g_variant_022_dxil),
#endif
    },
    {
        0x1FAB707A1EA2BCD9ULL,
        g_variant_016_spirv, sizeof(g_variant_016_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_016_dxil, sizeof(g_variant_016_dxil),
#endif
    },
    {
        0x331CE56F16602D01ULL,
        g_variant_004_spirv, sizeof(g_variant_004_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_004_dxil, sizeof(g_variant_004_dxil),
#endif
    },
    {
        0x33AF6A2016B00BFCULL,
        g_variant_044_spirv, sizeof(g_variant_044_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_044_dxil, sizeof(g_variant_044_dxil),
#endif
    },
    {
        0x36DD7448F8D8B481ULL,
        g_variant_024_spirv, sizeof(g_variant_024_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_024_dxil, sizeof(g_variant_024_dxil),
#endif
    },
    {
        0x3C62211EE30A3A3EULL,
        g_variant_036_spirv, sizeof(g_variant_036_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_036_dxil, sizeof(g_variant_036_dxil),
#endif
    },
    {
        0x3EFE089C9AA220BCULL,
        g_variant_003_spirv, sizeof(g_variant_003_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_003_dxil, sizeof(g_variant_003_dxil),
#endif
    },
    {
        0x4281AB6AE25E866BULL,
        g_variant_002_spirv, sizeof(g_variant_002_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_002_dxil, sizeof(g_variant_002_dxil),
#endif
    },
    {
        0x43B4671FF78C63CFULL,
        g_variant_034_spirv, sizeof(g_variant_034_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_034_dxil, sizeof(g_variant_034_dxil),
#endif
    },
    {
        0x46E436034A25AF1BULL,
        g_variant_054_spirv, sizeof(g_variant_054_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_054_dxil, sizeof(g_variant_054_dxil),
#endif
    },
    {
        0x4788761627F6C3DAULL,
        g_variant_049_spirv, sizeof(g_variant_049_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_049_dxil, sizeof(g_variant_049_dxil),
#endif
    },
    {
        0x4B4A90C302F4CF4EULL,
        g_variant_045_spirv, sizeof(g_variant_045_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_045_dxil, sizeof(g_variant_045_dxil),
#endif
    },
    {
        0x4E757F36431FA35BULL,
        g_variant_028_spirv, sizeof(g_variant_028_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_028_dxil, sizeof(g_variant_028_dxil),
#endif
    },
    {
        0x5128A604A0D03776ULL,
        g_variant_007_spirv, sizeof(g_variant_007_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_007_dxil, sizeof(g_variant_007_dxil),
#endif
    },
    {
        0x5197112CA90FC6F5ULL,
        g_variant_020_spirv, sizeof(g_variant_020_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_020_dxil, sizeof(g_variant_020_dxil),
#endif
    },
    {
        0x553281A7BAA05831ULL,
        g_variant_023_spirv, sizeof(g_variant_023_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_023_dxil, sizeof(g_variant_023_dxil),
#endif
    },
    {
        0x558FF6B96783FAECULL,
        g_variant_013_spirv, sizeof(g_variant_013_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_013_dxil, sizeof(g_variant_013_dxil),
#endif
    },
    {
        0x62A99559A8DFBEC4ULL,
        g_variant_011_spirv, sizeof(g_variant_011_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_011_dxil, sizeof(g_variant_011_dxil),
#endif
    },
    {
        0x661B3300ECE5BC0FULL,
        g_variant_039_spirv, sizeof(g_variant_039_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_039_dxil, sizeof(g_variant_039_dxil),
#endif
    },
    {
        0x6671991BAFB532F0ULL,
        g_variant_040_spirv, sizeof(g_variant_040_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_040_dxil, sizeof(g_variant_040_dxil),
#endif
    },
    {
        0x6C32EF02933C0DA1ULL,
        g_variant_041_spirv, sizeof(g_variant_041_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_041_dxil, sizeof(g_variant_041_dxil),
#endif
    },
    {
        0x704A7D532DD9EE6DULL,
        g_variant_012_spirv, sizeof(g_variant_012_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_012_dxil, sizeof(g_variant_012_dxil),
#endif
    },
    {
        0x7139DD570931AEBDULL,
        g_variant_050_spirv, sizeof(g_variant_050_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_050_dxil, sizeof(g_variant_050_dxil),
#endif
    },
    {
        0x73042DDD2F5E1DBFULL,
        g_variant_006_spirv, sizeof(g_variant_006_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_006_dxil, sizeof(g_variant_006_dxil),
#endif
    },
    {
        0x77AF33D724E8EF11ULL,
        g_variant_046_spirv, sizeof(g_variant_046_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_046_dxil, sizeof(g_variant_046_dxil),
#endif
    },
    {
        0x7CEF70426DA4D831ULL,
        g_variant_009_spirv, sizeof(g_variant_009_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_009_dxil, sizeof(g_variant_009_dxil),
#endif
    },
    {
        0x7E3F904D58611C6EULL,
        g_variant_058_spirv, sizeof(g_variant_058_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_058_dxil, sizeof(g_variant_058_dxil),
#endif
    },
    {
        0x85F7D76731CF8DFCULL,
        g_variant_033_spirv, sizeof(g_variant_033_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_033_dxil, sizeof(g_variant_033_dxil),
#endif
    },
    {
        0x866BC133295A026AULL,
        g_variant_038_spirv, sizeof(g_variant_038_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_038_dxil, sizeof(g_variant_038_dxil),
#endif
    },
    {
        0x91A0032DC74797A9ULL,
        g_variant_018_spirv, sizeof(g_variant_018_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_018_dxil, sizeof(g_variant_018_dxil),
#endif
    },
    {
        0x928F593181ADD0DCULL,
        g_variant_048_spirv, sizeof(g_variant_048_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_048_dxil, sizeof(g_variant_048_dxil),
#endif
    },
    {
        0x96C40A9A2B60354FULL,
        g_variant_010_spirv, sizeof(g_variant_010_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_010_dxil, sizeof(g_variant_010_dxil),
#endif
    },
    {
        0x99A54764634C1849ULL,
        g_variant_019_spirv, sizeof(g_variant_019_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_019_dxil, sizeof(g_variant_019_dxil),
#endif
    },
    {
        0x9D9AF9026A8FA48FULL,
        g_variant_035_spirv, sizeof(g_variant_035_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_035_dxil, sizeof(g_variant_035_dxil),
#endif
    },
    {
        0xA3A2DA7E9C2A8956ULL,
        g_variant_037_spirv, sizeof(g_variant_037_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_037_dxil, sizeof(g_variant_037_dxil),
#endif
    },
    {
        0xA4567362963F6A8AULL,
        g_variant_021_spirv, sizeof(g_variant_021_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_021_dxil, sizeof(g_variant_021_dxil),
#endif
    },
    {
        0xAB9D92183F87F53AULL,
        g_variant_042_spirv, sizeof(g_variant_042_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_042_dxil, sizeof(g_variant_042_dxil),
#endif
    },
    {
        0xAE823BBA197A8946ULL,
        g_variant_014_spirv, sizeof(g_variant_014_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_014_dxil, sizeof(g_variant_014_dxil),
#endif
    },
    {
        0xB18EF1DA3D3788CEULL,
        g_variant_032_spirv, sizeof(g_variant_032_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_032_dxil, sizeof(g_variant_032_dxil),
#endif
    },
    {
        0xB337E6B2A6A8395EULL,
        g_variant_000_spirv, sizeof(g_variant_000_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_000_dxil, sizeof(g_variant_000_dxil),
#endif
    },
    {
        0xB54391F70A8436F0ULL,
        g_variant_052_spirv, sizeof(g_variant_052_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_052_dxil, sizeof(g_variant_052_dxil),
#endif
    },
    {
        0xB7628A764E758AB9ULL,
        g_variant_017_spirv, sizeof(g_variant_017_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_017_dxil, sizeof(g_variant_017_dxil),
#endif
    },
    {
        0xC36648B5B5B14CB6ULL,
        g_variant_051_spirv, sizeof(g_variant_051_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_051_dxil, sizeof(g_variant_051_dxil),
#endif
    },
    {
        0xC546B2F40EEE522EULL,
        g_variant_001_spirv, sizeof(g_variant_001_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_001_dxil, sizeof(g_variant_001_dxil),
#endif
    },
    {
        0xCD715566004B10E4ULL,
        g_variant_015_spirv, sizeof(g_variant_015_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_015_dxil, sizeof(g_variant_015_dxil),
#endif
    },
    {
        0xD3E8EE793F261CEBULL,
        g_variant_025_spirv, sizeof(g_variant_025_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_025_dxil, sizeof(g_variant_025_dxil),
#endif
    },
    {
        0xD84DADB96A241F20ULL,
        g_variant_057_spirv, sizeof(g_variant_057_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_057_dxil, sizeof(g_variant_057_dxil),
#endif
    },
    {
        0xD85AF30FA765DE07ULL,
        g_variant_030_spirv, sizeof(g_variant_030_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_030_dxil, sizeof(g_variant_030_dxil),
#endif
    },
    {
        0xD91364B74868C0D9ULL,
        g_variant_031_spirv, sizeof(g_variant_031_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_031_dxil, sizeof(g_variant_031_dxil),
#endif
    },
    {
        0xDBED1F91889652CAULL,
        g_variant_053_spirv, sizeof(g_variant_053_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_053_dxil, sizeof(g_variant_053_dxil),
#endif
    },
    {
        0xDD39F8BCAC1B8559ULL,
        g_variant_047_spirv, sizeof(g_variant_047_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_047_dxil, sizeof(g_variant_047_dxil),
#endif
    },
    {
        0xE6E0FCC9288A6A56ULL,
        g_variant_043_spirv, sizeof(g_variant_043_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_043_dxil, sizeof(g_variant_043_dxil),
#endif
    },
    {
        0xFB20DBF244E81690ULL,
        g_variant_029_spirv, sizeof(g_variant_029_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_029_dxil, sizeof(g_variant_029_dxil),
#endif
    },
    {
        0xFD8A18E713203C4DULL,
        g_variant_055_spirv, sizeof(g_variant_055_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_055_dxil, sizeof(g_variant_055_dxil),
#endif
    },
    {
        0xFDA9D2E7AC60BC33ULL,
        g_variant_005_spirv, sizeof(g_variant_005_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_005_dxil, sizeof(g_variant_005_dxil),
#endif
    },
    {
        0xFEB49CDDAD792BF7ULL,
        g_variant_026_spirv, sizeof(g_variant_026_spirv),
#ifdef SHADOW_RECOMP_D3D12
        g_variant_026_dxil, sizeof(g_variant_026_dxil),
#endif
    },
};

struct State
{
    uint64_t hash;
    FixedFunctionVertexShaderControlState state;
    uint64_t programKey;
};

static const State States[] =
{
    { 0x0301C3D0F8F04F73ULL, {1,1,1,0,0,1,0,0,0,0,0,0,{3,0,0,0,0,0,0,0},{0,1,0,0},{0,0,0,0},{0,0,2,3},{0,1,0,0},{2,0,0,0},6528,0,0,0}, 0xB54391F70A8436F0ULL },
    { 0x075EED1BD204BB05ULL, {1,1,0,0,0,0,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,2,0,0},6544,0,0,0}, 0x36DD7448F8D8B481ULL },
    { 0x0AF1A8FA56080AC3ULL, {1,1,1,0,0,1,0,0,0,0,1,2,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{0,0,0,0},8064,0,0,0}, 0xDBED1F91889652CAULL },
    { 0x0B6BA0F4DF0C9B22ULL, {1,1,1,0,0,1,0,0,0,0,1,2,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0x46E436034A25AF1BULL },
    { 0x0B72FCDEA49D7EC2ULL, {1,1,0,0,0,0,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{1,1,2,3},{0,0,0,0},{0,2,0,0},6544,0,0,0}, 0x1F49D54881EEE59DULL },
    { 0x0FB8CF5C5C302637ULL, {1,1,0,0,0,0,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,2,0,0},6544,0,0,0}, 0x91A0032DC74797A9ULL },
    { 0x16C4A34E2D0397ABULL, {1,1,1,0,0,0,0,0,0,1,0,0,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0x661B3300ECE5BC0FULL },
    { 0x1DB51CA9AC381399ULL, {1,1,0,0,0,1,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{1,0,0,0},{0,0,0,0},7576,0,0,0}, 0x4E757F36431FA35BULL },
    { 0x2D9CF6978AB997D8ULL, {1,1,1,0,0,0,0,0,0,0,0,0,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0xB18EF1DA3D3788CEULL },
    { 0x2DBEF68399527C14ULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{1,1,2,3},{0,0,0,0},{0,2,0,0},6544,0,0,0}, 0x0AB6EE9AE538EC75ULL },
    { 0x2F17915EB03843BCULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7576,0,0,0}, 0x331CE56F16602D01ULL },
    { 0x31D95A6CABCE06D8ULL, {1,1,1,0,0,0,0,0,0,0,0,0,{3,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0x43B4671FF78C63CFULL },
    { 0x3582D509033140A0ULL, {1,1,0,0,0,0,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{0,1,0,0},{0,0,0,0},{0,0,2,3},{0,1,0,0},{2,0,0,0},6544,0,0,0}, 0xA4567362963F6A8AULL },
    { 0x3A46676E589D611CULL, {1,1,1,0,0,0,0,0,0,1,1,2,{3,0,0,0,0,0,0,0},{0,1,0,0},{0,0,0,0},{0,0,2,3},{0,1,0,0},{2,0,0,0},6528,0,0,0}, 0x4B4A90C302F4CF4EULL },
    { 0x4E082A0F841FC2E7ULL, {1,1,0,0,0,1,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{1,0,0,0},{0,0,0,0},7576,0,0,0}, 0xD85AF30FA765DE07ULL },
    { 0x503FBD989DC2C88CULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{0,1,0,0},{0,0,0,0},{0,0,2,3},{0,1,0,0},{2,0,0,0},6544,0,0,0}, 0x5128A604A0D03776ULL },
    { 0x54E768E56A0C9B0AULL, {1,1,0,0,0,0,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{0,0,0,0},8080,0,0,0}, 0xCD715566004B10E4ULL },
    { 0x553D7372AD33B634ULL, {1,1,1,0,0,0,0,0,0,0,1,2,{3,0,0,0,0,0,0,0},{0,1,0,0},{0,0,0,0},{0,0,2,3},{0,1,0,0},{2,0,0,0},6528,0,0,0}, 0xA3A2DA7E9C2A8956ULL },
    { 0x57260F3DD257D133ULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7568,0,0,0}, 0x3EFE089C9AA220BCULL },
    { 0x57E07112454C4EB1ULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{1,0,0,0},{0,0,0,0},7568,0,0,0}, 0x704A7D532DD9EE6DULL },
    { 0x59A88E85686B181EULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,2,0,0},6544,0,0,0}, 0xFDA9D2E7AC60BC33ULL },
    { 0x5AC970C490294465ULL, {1,1,0,0,0,0,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,3,3,0},4496,0,0,0}, 0x5197112CA90FC6F5ULL },
    { 0x60D5182AC7D765BBULL, {1,1,1,0,0,0,0,0,0,1,0,0,{3,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0xAB9D92183F87F53AULL },
    { 0x6332ADE20DA9B7A2ULL, {1,1,0,0,0,0,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{1,0,0,0},{0,0,0,0},7576,0,0,0}, 0xFEB49CDDAD792BF7ULL },
    { 0x637C2673A1190A2DULL, {1,1,1,0,0,1,0,0,0,1,0,0,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0x09860D0D70439218ULL },
    { 0x664EDD7316130D8AULL, {1,1,1,0,0,0,0,0,0,0,1,2,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0x3C62211EE30A3A3EULL },
    { 0x66D2A6C2888508AAULL, {1,1,0,0,0,1,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{0,1,0,0},{0,0,0,0},{0,0,2,3},{0,1,0,0},{2,0,0,0},6544,0,0,0}, 0xFB20DBF244E81690ULL },
    { 0x67FFAF171063CB8AULL, {1,1,0,0,0,0,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{0,0,1,0},{0,0,0,0},{0,1,0,3},{0,0,1,0},{2,2,0,0},4496,0,0,0}, 0x99A54764634C1849ULL },
    { 0x74E173D6DF4D402FULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{0,0,0,0},8080,0,0,0}, 0xB337E6B2A6A8395EULL },
    { 0x76BF1D2BF103DDDEULL, {1,1,0,0,0,0,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{0,0,0,0},8088,0,0,0}, 0x1FAB707A1EA2BCD9ULL },
    { 0x7742C32D86871A2BULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7568,0,0,0}, 0x7CEF70426DA4D831ULL },
    { 0x79D1D9933C6CE761ULL, {1,1,1,0,0,0,0,0,0,0,1,2,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{0,0,0,0},8064,0,0,0}, 0x9D9AF9026A8FA48FULL },
    { 0x7B583F1A5994B7BBULL, {1,1,1,0,0,1,0,0,0,0,0,0,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0xC36648B5B5B14CB6ULL },
    { 0x7F5A84CC8CCA3D17ULL, {1,1,1,0,0,1,0,0,0,0,1,2,{3,0,0,0,0,0,0,0},{0,1,0,0},{0,0,0,0},{0,0,2,3},{0,1,0,0},{2,0,0,0},6528,0,0,0}, 0xFD8A18E713203C4DULL },
    { 0x83E0B6D8D2423EF9ULL, {1,1,1,0,0,0,0,0,0,0,0,0,{3,0,0,0,0,0,0,0},{0,1,0,0},{0,0,0,0},{0,0,2,3},{0,1,0,0},{2,0,0,0},6528,0,0,0}, 0x85F7D76731CF8DFCULL },
    { 0x84493A73F7712180ULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{1,1,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,2,0,0},6544,0,0,0}, 0x62A99559A8DFBEC4ULL },
    { 0x8DC3C5AAD8864E12ULL, {1,1,1,0,0,0,0,0,0,1,1,2,{3,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0x77AF33D724E8EF11ULL },
    { 0x92964F3F36209E46ULL, {1,1,1,0,0,1,0,0,0,1,1,2,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0xD84DADB96A241F20ULL },
    { 0x9652708FDD8B4077ULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,2,3},{0,0,0,0},{2,0,0,0},6544,0,0,0}, 0x4281AB6AE25E866BULL },
    { 0xA6F406CE5AC2B36FULL, {1,1,1,0,0,1,0,0,0,0,0,0,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{0,0,0,0},8064,0,0,0}, 0x7139DD570931AEBDULL },
    { 0xA944E3DDE21DFFC6ULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{0,0,0,0},8088,0,0,0}, 0xC546B2F40EEE522EULL },
    { 0xA9A06B6D12224F9FULL, {1,1,1,0,0,0,0,0,0,1,1,2,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0xE6E0FCC9288A6A56ULL },
    { 0xACB0C1490683153EULL, {1,1,1,0,0,0,0,0,0,1,0,0,{3,0,0,0,0,0,0,0},{0,1,0,0},{0,0,0,0},{0,0,2,3},{0,1,0,0},{2,0,0,0},6528,0,0,0}, 0x6C32EF02933C0DA1ULL },
    { 0xB0BAAC1B64389FCFULL, {1,1,1,0,0,0,0,0,0,1,1,2,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,2,0,0},6528,0,0,0}, 0x33AF6A2016B00BFCULL },
    { 0xB0D917EAE1954750ULL, {1,1,1,0,0,0,0,1,0,1,0,0,{3,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0x928F593181ADD0DCULL },
    { 0xB2E0B070C18B8DC1ULL, {1,1,1,0,0,0,0,0,0,0,0,0,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{0,0,0,0},8064,0,0,0}, 0xD91364B74868C0D9ULL },
    { 0xB94161233A0559C7ULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{1,0,0,0},{0,0,0,0},7576,0,0,0}, 0x558FF6B96783FAECULL },
    { 0xBEDDA2F95D574D14ULL, {1,1,1,0,0,0,0,1,0,1,1,2,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0x4788761627F6C3DAULL },
    { 0xC377DF8942B46584ULL, {1,1,0,0,0,0,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{1,0,0,0},{0,0,0,0},7568,0,0,0}, 0xD3E8EE793F261CEBULL },
    { 0xC4F02A1677095F36ULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,2,0,0},6544,0,0,0}, 0x96C40A9A2B60354FULL },
    { 0xCCD3DE93F2C69731ULL, {1,1,0,0,0,0,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7568,0,0,0}, 0x553281A7BAA05831ULL },
    { 0xCECB8C0193C980B2ULL, {1,1,1,0,0,1,0,0,0,1,1,2,{3,0,0,0,0,0,0,0},{0,1,0,0},{0,0,0,0},{0,0,2,3},{0,1,0,0},{2,0,0,0},6528,0,0,0}, 0x7E3F904D58611C6EULL },
    { 0xD4856C49AB2E62BBULL, {1,1,1,0,0,0,0,1,0,1,0,0,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0xDD39F8BCAC1B8559ULL },
    { 0xD90D32FB2D37DF31ULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,3,3,0},4496,0,0,0}, 0x73042DDD2F5E1DBFULL },
    { 0xE4D34534E96D5234ULL, {1,1,0,0,0,0,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7568,0,0,0}, 0xB7628A764E758AB9ULL },
    { 0xEBF0D418A6E0B24BULL, {1,1,0,0,0,0,0,0,0,0,1,2,{0,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{1,1,2,3},{0,0,0,0},{0,2,0,0},6544,0,0,0}, 0x06D4C3C4C383B150ULL },
    { 0xEE8870E6BC5BF299ULL, {1,1,1,0,0,0,0,0,0,0,1,2,{3,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,0,0,0},7552,0,0,0}, 0x866BC133295A026AULL },
    { 0xFBB986633DE798D2ULL, {1,1,0,0,0,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{1,0,0,0},{0,0,0,0},{1,1,2,3},{0,0,0,0},{0,2,0,0},6544,0,0,0}, 0xAE823BBA197A8946ULL },
    { 0xFD44EB8048E520E0ULL, {1,1,1,0,0,0,0,0,0,1,0,0,{3,0,0,0,0,0,0,0},{0,0,0,0},{0,0,0,0},{0,1,2,3},{0,0,0,0},{2,2,0,0},6528,0,0,0}, 0x6671991BAFB532F0ULL },
};
static_assert(std::size(Programs) == 59);
static_assert(std::size(States) == 59);
}
