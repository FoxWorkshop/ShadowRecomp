#include "common_vs.hlsli"

struct VS_INPUT
{
    float4 v[16] : TEXCOORD;
};

struct VS_OUTPUT // Declared identical to pixel shader input (see PS_INPUT)
{
    float4 oPos : SV_Position; // Homogeneous clip space position
    float4 oD0 : COLOR0; // Primary color (front-facing)
    float4 oD1 : COLOR1; // Secondary color (front-facing)
    float oFog : FOG; // Fog coordinate
    float oPts : PSIZE; // Point size
    float4 oB0 : TEXCOORD4; // Back-facing primary color
    float4 oB1 : TEXCOORD5; // Back-facing secondary color
    float4 oT0 : TEXCOORD0; // Texture coordinate set 0
    float4 oT1 : TEXCOORD1; // Texture coordinate set 1
    float4 oT2 : TEXCOORD2; // Texture coordinate set 2
    float4 oT3 : TEXCOORD3; // Texture coordinate set 3
};


VS_OUTPUT main(VS_INPUT xIn)
{

    
	// Output variables
   float4 oPos, oD0, oD1, oB0, oB1, oT0, oT1, oT2, oT3;
    oPos = oD0 = oD1 = oB0 = oB1 = oT0 = oT1 = oT2 = oT3 = float4(0, 0, 0, 1); // Pre-initialize w component of outputs to 1

	// Single component outputs
    float4 oFog, oPts; // x is write-only on Xbox. Use float4 as some games use incorrect masks
    oFog = 1; // Default to no fog. 
    oPts = 0;

	// Address (index) register
    int1 a0 = 0;

	// Temporary registers
    float4 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11;
    r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = r10 = r11 = float4(0, 0, 0, 0);
#define r12 oPos // oPos and r12 are two ways of accessing the same register on Xbox

	// Input registers
    float4 v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15;

	// Unpack 16 flags from 4 float4 constant registers
    float vRegisterDefaultFlags[16] = (float[16]) vRegisterDefaultFlagsPacked;

	// Initialize input registers from the vertex buffer data
	// Or use the register's default value (which can be changed by the title)
#define init_v(i) v##i = lerp(xIn.v[i], vRegisterDefaultValues[i], vRegisterDefaultFlags[i]);
	// Note : unroll manually instead of for-loop, because of the ## concatenation
	init_v(0);init_v(1);init_v(2);init_v(3);
	init_v(4);init_v(5);init_v(6);init_v(7);
	init_v(8);init_v(9);init_v(10);init_v(11);
	init_v(12);init_v(13);init_v(14);init_v(15);

	// Temp variable for paired VS instruction
    float4 temp;

	// Xbox shader program starts here
	
  x_arl(a0,x, v2.x);

  x_mov(r11,xyzw, c(-91));
  x_mov(oT0,xyzw, v5);

  x_mul(r8,xyzw, v1.x, c(a0.x-88));

  x_mul(r9,xyzw, v1.x, c(a0.x-87));

  x_mul(r10,xyzw, v1.x, c(a0.x-86));

  x_arl(a0,x, v2.y);

  x_mad(r8,xyzw, v1.y, c(a0.x-88), r8);

  x_mad(r9,xyzw, v1.y, c(a0.x-87), r9);

  x_mad(r10,xyzw, v1.y, c(a0.x-86), r10);

  x_dp4(r3,x, v0, r8);

  x_dp4(r3,y, v0, r9);

  x_dp4(r3,z, v0, r10);

  x_dp3(r5,x, v3.xyzz, r8.xyzz);

  x_dp3(r5,y, v3.xyzz, r9.xyzz);

  x_dp3(r5,z, v3.xyzz, r10.xyzz);

  x_dph(oPos,w, r3, c(-92));

  x_dp3(r5,w, r5.xyzz, r5.xyzz);

  x_dph(oPos,x, r3, c(-95));
  x_rcc(r1,x, r12.w);

  x_dph(oPos,y, r3, c(-94));
  x_rsq(r1,w, r5.w);

  x_dph(oPos,z, r3, c(-93));

  x_mul(r5,xyz, r5.xyzz, r1.w);
  x_mov(oFog,x, r12.w);

  x_mad(oPos,xyz, r12.xyzz, r1.x, c(-96).xyzz);

  x_dp3(r6,x, c(-90).xyzz, r5.xyzz);

  x_max(r6,x, c(-90).w, -r6.x);

  x_mad(r11,xyz, c(-89).xyzz, r6.x, r11.xyzz);


  x_mul(oD0,xyzw, c(95), r11);
	// End Xbox shader program

	// Copy variables to output struct
    VS_OUTPUT xOut;
    


    xOut.oPos = reverseScreenspaceTransform(oPos);
    xOut.oD0 = saturate(oD0);
    xOut.oD1 = saturate(oD1);
    xOut.oFog = oFog.x; // Note : Xbox clamps fog in pixel shader -> *NEEDS TESTING* /was oFog.x 
    xOut.oPts = oPts.x;
    xOut.oB0 = saturate(oB0);
    xOut.oB1 = saturate(oB1);
	// Scale textures (TODO : or should we apply this to the input register values?)
    xOut.oT0 = oT0 / xboxTextureScale[0];
    xOut.oT1 = oT1 / xboxTextureScale[1];
    xOut.oT2 = oT2 / xboxTextureScale[2];
    xOut.oT3 = oT3 / xboxTextureScale[3];
    
    return xOut;
}