#include <kernel/function.h>

//stub unnecessary startup functions to force the game to boot


NTSTATUS TryOpenVolume(uint32_t* FileHandle)
{
	printf("TryOpenVolume be gone\n");
	return X_STATUS_SUCCESS;
}

NTSTATUS MapLetterToDirectory(uint32_t* FileHandle)
{
	printf("MapLetterToDirectory be gone\n");
	return X_STATUS_SUCCESS;
}

int XMountUtilityDrive(int a1)
{
	printf("XMountUtilityDrive be gone\n");
	return TRUE;
}


int XAPILIBXGetParentalControlSetting(int a1)
{
	printf("XAPILIBXGetParentalControlSetting be gone\n");
	return X_STATUS_SUCCESS;
}

int sub_2B0164_r(int a1)
{
	return TRUE;
}

int sub_3864B8_r(int a1, int a2)
{
	//RtlInitializeCriticalSection(a1);
	return TRUE;
}

GUEST_FUNCTION_HOOK(sub_2B0CE1, TryOpenVolume);
GUEST_FUNCTION_HOOK_STDCALL(sub_2B0C27, MapLetterToDirectory);
GUEST_FUNCTION_HOOK_STDCALL(sub_2AF4B3, XMountUtilityDrive);
GUEST_FUNCTION_HOOK(sub_2B17C3, XAPILIBXGetParentalControlSetting);
GUEST_FUNCTION_HOOK(sub_2B0164, sub_2B0164_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_3864B8, sub_3864B8_r);

typedef byte _BYTE;
typedef DWORD _DWORD;

struct struct_a1
{
	
	_BYTE gap0[20];
	_BYTE byte14;
	_DWORD dword18;
	_DWORD dword1C;
	_BYTE gap20[16];
	_DWORD dword30;
	_BYTE gap34[40];
	_DWORD* pdword5C;
	_BYTE gap60[288];
	int* pint180;
	_DWORD dword184;
	_BYTE gap188[1016];
	_DWORD dword580;
};


char* AllocateMemoryDogShitEdition(struct_a1* a1, int a2, unsigned int size)
{
	int flag; // ecx
	unsigned int v4; // eax
	unsigned int v5; // edi
	unsigned __int16* v6; // eax
	char v7; // dl
	int v8; // ecx
	int* v9; // edi
	__int16 v10; // cx
	unsigned int v11; // edx
	int v12; // eax
	_BYTE* v13; // edi
	int** v14; // edi
	int* v15; // eax
	int v16; // ecx
	int* v17; // edx
	__int16 v18; // cx
	int** v19; // edx
	struct_a1* v20; // eax


	_DWORD* v40; // eax
	int v41; // eax
	EXCEPTION_RECORD ExceptionRecord; // [esp+Ch] [ebp-190h] BYREF
	unsigned __int16* v44; // [esp+5Ch] [ebp-140h]
	int* v45; // [esp+60h] [ebp-13Ch]
	_DWORD* v46; // [esp+64h] [ebp-138h]
	_DWORD* v47; // [esp+68h] [ebp-134h]
	int v48; // [esp+6Ch] [ebp-130h]
	_BYTE* v49; // [esp+70h] [ebp-12Ch]
	_DWORD* v50; // [esp+74h] [ebp-128h]
	int v51; // [esp+78h] [ebp-124h]
	int v52; // [esp+7Ch] [ebp-120h]
	_DWORD* v53; // [esp+80h] [ebp-11Ch]
	_DWORD* v54; // [esp+84h] [ebp-118h]
	int* v55; // [esp+88h] [ebp-114h]
	int* v56; // [esp+8Ch] [ebp-110h]
	int* v57; // [esp+90h] [ebp-10Ch]
	int v58; // [esp+94h] [ebp-108h]
	int** v59; // [esp+98h] [ebp-104h]
	int v60; // [esp+9Ch] [ebp-100h]
	_DWORD* v61; // [esp+A0h] [ebp-FCh]
	int v62; // [esp+A4h] [ebp-F8h]
	_BYTE* v63; // [esp+A8h] [ebp-F4h]
	int v64; // [esp+ACh] [ebp-F0h]
	int v65; // [esp+B0h] [ebp-ECh]
	unsigned __int16* v66; // [esp+B4h] [ebp-E8h]
	_DWORD* v67; // [esp+B8h] [ebp-E4h]
	int* v68; // [esp+BCh] [ebp-E0h]
	int v69; // [esp+C0h] [ebp-DCh]
	int** v70; // [esp+C4h] [ebp-D8h]
	int* v71; // [esp+C8h] [ebp-D4h]
	_BYTE* v72; // [esp+CCh] [ebp-D0h]
	int v73; // [esp+D0h] [ebp-CCh]
	_DWORD* v74; // [esp+D4h] [ebp-C8h]
	_DWORD* v75; // [esp+D8h] [ebp-C4h]

	_BYTE* v89; // [esp+110h] [ebp-8Ch]
	struct_a1* v90; // [esp+114h] [ebp-88h]
	int v91; // [esp+118h] [ebp-84h]
	int* v92; // [esp+11Ch] [ebp-80h]
	int* v93; // [esp+120h] [ebp-7Ch]
	_BYTE* v94; // [esp+124h] [ebp-78h]
	int* v95; // [esp+128h] [ebp-74h]
	int v96; // [esp+12Ch] [ebp-70h]
	int v97; // [esp+130h] [ebp-6Ch]
	unsigned int v98; // [esp+134h] [ebp-68h]
	_BYTE* v99; // [esp+138h] [ebp-64h]
	_BYTE* v100; // [esp+13Ch] [ebp-60h]
	int v101[3]; // [esp+140h] [ebp-5Ch] BYREF
	int v102; // [esp+14Ch] [ebp-50h]
	_BYTE* v103; // [esp+150h] [ebp-4Ch]
	int v104; // [esp+154h] [ebp-48h]
	struct_a1* v105; // [esp+158h] [ebp-44h]
	unsigned int v112; // [esp+174h] [ebp-28h]
	_DWORD* v113; // [esp+178h] [ebp-24h] BYREF
	unsigned int v114; // [esp+17Ch] [ebp-20h] BYREF
	char v115; // [esp+182h] [ebp-1Ah]
	char v116; // [esp+183h] [ebp-19h]
	int v118; // [esp+1A8h] [ebp+Ch]

	v90 = a1;
	v115 = 0;
	flag = a1->dword18 | a2;
	v118 = flag;
	v4 = size;
	if (!size)
		v4 = 1;
	v114 = (v4 + 31) & 0xFFFFFFF0;
	v5 = v114 >> 4;
	v112 = v114 >> 4;

	if ((flag & 1) == 0)
	{
		//RtlEnterCriticalSection(a1->dword580);
		v115 = 1;
	}

	if (v5 >= 0x80)
	{

	}

	return 0;
}

//GUEST_FUNCTION_HOOK_STDCALL(sub_2AC3EF, AllocateMemoryDogShitEdition);


char* sub_380620_r(int alignMaybe, int size)
{
	auto size_ = size * alignMaybe;
	if (!(size * alignMaybe))
		size_ = 1;

	return 0;
}



//GUEST_FUNCTION_HOOK_STDCALL(sub_380620, sub_380620_r);