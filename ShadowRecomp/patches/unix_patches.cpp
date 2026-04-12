#include <kernel/function.h>

// The game mixes lowercase and uppercase across files and code references. 
// This works fine on Windows / Xbox as casing is ignored. Unix-like systems however care...

enum ASCIITables : uint8_t
{
	c = 0x63, 
	K = 0x4B, 
};

const uint32_t enemyBkChaosDataAddr = 0x458234;
const uint32_t BkChaosDataAddr = 0x4529A8;
const uint32_t csd2PCommandAddr = 0x477318;
const uint32_t Csdfiles2pCommAddr = 0x477334;
const uint32_t CsdfilesCommanAddr = 0x477350;
const uint32_t Csdfiles2pRingAddr = 0x4773D0;
const uint32_t CsdfilesInfowiAddr = 0x4773B0;
const uint32_t CsdfilesBossgaAddr = 0x478000;
const uint32_t CsdfilesDarkgAddr = 0x4774BC;
const uint32_t CsdfilesHerogaAddr = 0x4774D0;
const uint32_t CsdfilesHissatAddr = 0x477554; //dark
const uint32_t CsdfilesHissat2Addr = 0x4775A0; //hero
const uint32_t CsdfilesHpbarAddr = 0x47805C; 
const uint32_t CsdfilesTwindoAddr = 0x477670;
const uint32_t CsdfilesPauseP1Addr = 0x477620;
const uint32_t CsdfilesPauseP2Addr = 0x47763C;
const uint32_t CsdfilesPauseP0Addr = 0x477654;



void PatchUnixStrings()
{
	auto base = g_memory.base;

	X86_STORE_U8(enemyBkChaosDataAddr + 7, K);
	X86_STORE_U8(BkChaosDataAddr + 1, K);
	X86_STORE_U8(csd2PCommandAddr, c);
	X86_STORE_U8(Csdfiles2pCommAddr, c);
	X86_STORE_U8(CsdfilesCommanAddr, c);
	X86_STORE_U8(Csdfiles2pRingAddr, c);
	X86_STORE_U8(CsdfilesInfowiAddr, c);
	X86_STORE_U8(CsdfilesBossgaAddr, c);
	X86_STORE_U8(CsdfilesDarkgAddr, c);
	X86_STORE_U8(CsdfilesHerogaAddr, c);
	X86_STORE_U8(CsdfilesHissatAddr, c);
	X86_STORE_U8(CsdfilesHissat2Addr, c);
	X86_STORE_U8(CsdfilesHpbarAddr, c);
	X86_STORE_U8(CsdfilesTwindoAddr, c);
	X86_STORE_U8(CsdfilesPauseP0Addr, c);
	X86_STORE_U8(CsdfilesPauseP1Addr, c);
	X86_STORE_U8(CsdfilesPauseP2Addr, c);

}