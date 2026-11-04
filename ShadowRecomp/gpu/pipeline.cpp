#include "pipeline.h"
#include <shader/shader_cache.h>
#include "ubershaders/fixed_function_vertex_shader_variants.h"
#include "ubershaders/fixed_function_pixel_shader_variants.h"
#include "vertexshader.h"

extern std::unique_ptr<RenderPipelineLayout> g_pipelineLayout;
extern std::unique_ptr<RenderShader> FixedFuncVS;
extern std::unique_ptr<RenderShader> PassthroughVS; 
extern std::unique_ptr<RenderShader> FixedFuncPS ;

template <typename T>
static bool IsEnumInRange(T value, T last)
{
	return uint32_t(value) <= uint32_t(last);
}


bool IsPipelineCacheEntryInRange(const PipelineCacheEntry& entry)
{
	return entry.vertexElementCount <= MAX_VERTEX_ELEMENTS &&
		IsEnumInRange(entry.vertexShaderMode, VertexShaderMode::ShaderProgram) &&
		IsEnumInRange(entry.srcBlend, RenderBlend::INV_SRC1_ALPHA) &&
		IsEnumInRange(entry.destBlend, RenderBlend::INV_SRC1_ALPHA) &&
		IsEnumInRange(entry.srcBlendAlpha, RenderBlend::INV_SRC1_ALPHA) &&
		IsEnumInRange(entry.destBlendAlpha, RenderBlend::INV_SRC1_ALPHA) &&
		IsEnumInRange(entry.cullMode, RenderCullMode::BACK) &&
		IsEnumInRange(entry.zFunc, RenderComparisonFunction::ALWAYS) &&
		IsEnumInRange(entry.stencilFunc, RenderComparisonFunction::ALWAYS) &&
		IsEnumInRange(entry.stencilFuncCCW, RenderComparisonFunction::ALWAYS) &&
		IsEnumInRange(entry.stencilFail, RenderStencilOp::DECREMENT_AND_WRAP) &&
		IsEnumInRange(entry.stencilZFail, RenderStencilOp::DECREMENT_AND_WRAP) &&
		IsEnumInRange(entry.stencilPass, RenderStencilOp::DECREMENT_AND_WRAP) &&
		IsEnumInRange(entry.blendOp, RenderBlendOperation::MAX) &&
		IsEnumInRange(entry.blendOpAlpha, RenderBlendOperation::MAX) &&
		IsEnumInRange(entry.primitiveTopology, RenderPrimitiveTopology::TRIANGLE_FAN) &&
		IsEnumInRange(entry.renderTargetFormat, RenderFormat::MAX) &&
		IsEnumInRange(entry.depthStencilFormat, RenderFormat::MAX);
}


bool BuildPipelineCacheEntry(const PipelineState& pipelineState, PipelineCacheEntry& outEntry)
{
	outEntry = {};
	outEntry.vertexShaderMode = pipelineState.vertexShaderType;
	outEntry.isFixedFunctionPS = pipelineState.isFixedFunctionPS;

	if (pipelineState.vertexShaderType == VertexShaderMode::FixedFunction ||
		pipelineState.vertexShaderType == VertexShaderMode::Passthrough)
	{
		outEntry.vertexShaderHash = pipelineState.vertexShaderType == VertexShaderMode::FixedFunction
			? pipelineState.vertexShaderVariant
			: 0;
	}
	else if (pipelineState.vertexShader != nullptr)
	{
		if (pipelineState.vertexShader->shaderCacheEntry != nullptr)
			outEntry.vertexShaderHash = pipelineState.vertexShader->shaderCacheEntry->hash;
		else if (pipelineState.vertexShader->runtimeHash != 0)
			outEntry.vertexShaderHash = pipelineState.vertexShader->runtimeHash;
		else
			return false;
	}
	else
		return false;

	if (pipelineState.isFixedFunctionPS)
	{
		outEntry.pixelShaderHash = pipelineState.pixelShaderVariant;
	}
	else if (pipelineState.pixelShader != nullptr)
	{
		if (pipelineState.pixelShader->shaderCacheEntry != nullptr)
			outEntry.pixelShaderHash = pipelineState.pixelShader->shaderCacheEntry->hash;
		else if (pipelineState.pixelShader->runtimeHash != 0)
			outEntry.pixelShaderHash = pipelineState.pixelShader->runtimeHash;
		else
			return false;
	}
	else if (pipelineState.pixelShader == nullptr)
	{
		outEntry.pixelShaderHash = 0;
	}
	else
		return false;

	if (pipelineState.vertexDeclaration != nullptr)
	{
		outEntry.vertexDeclHash = pipelineState.vertexDeclaration->hash;
		outEntry.vertexElementCount = pipelineState.vertexDeclaration->vertexElementCount;
		if (outEntry.vertexElementCount > MAX_VERTEX_ELEMENTS)
			return false;

		memcpy(outEntry.vertexElements, pipelineState.vertexDeclaration->vertexElements.get(),
			outEntry.vertexElementCount * sizeof(D3DVERTEXELEMENT9));
	}
	else
		return false;

	outEntry.zEnable = pipelineState.zEnable;
	outEntry.zWriteEnable = pipelineState.zWriteEnable;
	outEntry.stencilEnable = pipelineState.stencilEnable;
	outEntry.srcBlend = pipelineState.srcBlend;
	outEntry.destBlend = pipelineState.destBlend;
	outEntry.cullMode = pipelineState.cullMode;
	outEntry.zFunc = pipelineState.zFunc;
	outEntry.stencilFunc = pipelineState.stencilFunc;
	outEntry.stencilFail = pipelineState.stencilFail;
	outEntry.stencilZFail = pipelineState.stencilZFail;
	outEntry.stencilPass = pipelineState.stencilPass;
	outEntry.stencilMask = pipelineState.stencilMask;
	outEntry.stencilWriteMask = pipelineState.stencilWriteMask;
	outEntry.stencilRef = pipelineState.stencilRef;
	outEntry.stencilFuncCCW = pipelineState.stencilFuncCCW;
	outEntry.alphaBlendEnable = pipelineState.alphaBlendEnable;
	outEntry.blendOp = pipelineState.blendOp;
	outEntry.slopeScaledDepthBias = pipelineState.slopeScaledDepthBias;
	outEntry.depthBias = pipelineState.depthBias;
	outEntry.srcBlendAlpha = pipelineState.srcBlendAlpha;
	outEntry.destBlendAlpha = pipelineState.destBlendAlpha;
	outEntry.blendOpAlpha = pipelineState.blendOpAlpha;
	outEntry.colorWriteEnable = pipelineState.colorWriteEnable;
	outEntry.primitiveTopology = pipelineState.primitiveTopology;
	memcpy(outEntry.vertexStrides, pipelineState.vertexStrides, sizeof(outEntry.vertexStrides));
	outEntry.renderTargetFormat = pipelineState.renderTargetFormat;
	outEntry.depthStencilFormat = pipelineState.depthStencilFormat;
	outEntry.sampleCount = pipelineState.sampleCount;
	outEntry.enableAlphaToCoverage = pipelineState.enableAlphaToCoverage;
	outEntry.specConstants = pipelineState.specConstants;

	return true;
}


std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(const PipelineState& pipelineState)
{
	RenderGraphicsPipelineDesc desc;
	desc.pipelineLayout = g_pipelineLayout.get();

	if (pipelineState.vertexShaderType == VertexShaderMode::Passthrough)
	{
		desc.vertexShader = PassthroughVS.get();
	}
	else if (pipelineState.vertexShaderType == VertexShaderMode::FixedFunction)
	{
		desc.vertexShader = FixedFunctionVertexShaderVariants::Resolve(pipelineState.vertexShaderVariant);
		if (desc.vertexShader == nullptr)
			desc.vertexShader = FixedFuncVS.get();
	}
	else
	{
		desc.vertexShader = GetOrLinkShader(pipelineState.vertexShader, pipelineState.specConstants);
	}


	if (pipelineState.isFixedFunctionPS)
	{
		desc.pixelShader = FixedFunctionPixelShaderVariants::Resolve(pipelineState.pixelShaderVariant);
		if (desc.pixelShader == nullptr)
			desc.pixelShader = FixedFuncPS.get();
	}
	else
	{
		desc.pixelShader = GetOrLinkShader(pipelineState.pixelShader, pipelineState.specConstants);
	}
	desc.depthFunction = pipelineState.zFunc;
	desc.depthEnabled = pipelineState.zEnable; //Xbox likes to enable ZEnable without a depth stencil
	desc.depthWriteEnabled = pipelineState.zWriteEnable;
	desc.depthBias = pipelineState.depthBias;
	desc.stencilEnabled = pipelineState.stencilEnable;
	desc.stencilReadMask = pipelineState.stencilMask;
	desc.stencilWriteMask = pipelineState.stencilWriteMask;
	desc.stencilReference = pipelineState.stencilRef;
	desc.stencilFrontFace.compareFunction = pipelineState.stencilFunc;
	desc.stencilFrontFace.failOp = pipelineState.stencilFail;
	desc.stencilFrontFace.depthFailOp = pipelineState.stencilZFail;
	desc.stencilFrontFace.passOp = pipelineState.stencilPass;
	desc.stencilBackFace = desc.stencilFrontFace;
	desc.slopeScaledDepthBias = pipelineState.slopeScaledDepthBias;
	desc.dynamicDepthBiasEnabled = Video::GetRenderDevice()->getCapabilities().dynamicDepthBias;
	desc.depthClipEnabled = true;
	desc.primitiveTopology = pipelineState.primitiveTopology;

	desc.cullMode = pipelineState.cullMode;
	desc.renderTargetFormat[0] = pipelineState.renderTargetFormat;
	desc.renderTargetBlend[0].blendEnabled = pipelineState.alphaBlendEnable;
	desc.renderTargetBlend[0].srcBlend = pipelineState.srcBlend;
	desc.renderTargetBlend[0].dstBlend = pipelineState.destBlend;
	desc.renderTargetBlend[0].blendOp = pipelineState.blendOp;
	desc.renderTargetBlend[0].srcBlendAlpha = pipelineState.srcBlendAlpha;
	desc.renderTargetBlend[0].dstBlendAlpha = pipelineState.destBlendAlpha;
	desc.renderTargetBlend[0].blendOpAlpha = pipelineState.blendOpAlpha;
	desc.renderTargetBlend[0].renderTargetWriteMask = pipelineState.colorWriteEnable;
	desc.renderTargetCount = pipelineState.renderTargetFormat != RenderFormat::UNKNOWN ? 1 : 0;
	desc.depthTargetFormat = pipelineState.depthStencilFormat;
	desc.multisampling.sampleCount = pipelineState.sampleCount;
	desc.alphaToCoverageEnabled = pipelineState.enableAlphaToCoverage;
	desc.inputElements = pipelineState.vertexDeclaration->inputElements.get();
	desc.inputElementsCount = pipelineState.vertexDeclaration->inputElementCount;


	RenderSpecConstant specConstant{};
	specConstant.value = pipelineState.specConstants;

	if (pipelineState.specConstants != 0)
	{
		desc.specConstants = &specConstant;
		desc.specConstantsCount = 1;
	}

	RenderInputSlot inputSlots[16]{};
	uint32_t inputSlotIndices[16]{};
	uint32_t inputSlotCount = 0;

	for (size_t i = 0; i < pipelineState.vertexDeclaration->inputElementCount; i++)
	{
		auto& inputElement = pipelineState.vertexDeclaration->inputElements[i];
		auto& inputSlotIndex = inputSlotIndices[inputElement.slotIndex];

		if (inputSlotIndex == NULL)
			inputSlotIndex = ++inputSlotCount;

		auto& inputSlot = inputSlots[inputSlotIndex - 1];
		inputSlot.index = inputElement.slotIndex;
		inputSlot.stride = pipelineState.vertexStrides[inputElement.slotIndex];
		inputSlot.classification = RenderInputSlotClassification::PER_VERTEX_DATA;
	}

	desc.inputSlots = inputSlots;
	desc.inputSlotsCount = inputSlotCount;

	auto pipeline = Video::GetRenderDevice()->createGraphicsPipeline(desc);

	return pipeline;
}