#include "Runtime/Rendering/DX12/DX12Device.h"
#include "Runtime/Rendering/DX12/DX12Format.h"

#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstring>
#include <string>
#include <vector>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

namespace
{
	D3D12_RESOURCE_FLAGS GetTextureFlags(const RHITextureDesc& desc)
	{
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
		if (desc.renderTarget)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (desc.depthStencil)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if (desc.unorderedAccess)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		return flags;
	}

	D3D12_CULL_MODE ToCullMode(RHIEnum::CullMode mode)
	{
		switch (mode)
		{
		case RHIEnum::CullMode::Front:
			return D3D12_CULL_MODE_FRONT;
		case RHIEnum::CullMode::Back:
			return D3D12_CULL_MODE_BACK;
		case RHIEnum::CullMode::None:
		default:
			return D3D12_CULL_MODE_NONE;
		}
	}

	D3D12_PRIMITIVE_TOPOLOGY_TYPE ToTopologyType(RHIEnum::PrimitiveTopology topology)
	{
		switch (topology)
		{
		case RHIEnum::PrimitiveTopology::PointList:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		case RHIEnum::PrimitiveTopology::LineList:
		case RHIEnum::PrimitiveTopology::LineStrip:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		case RHIEnum::PrimitiveTopology::TriangleStrip:
		case RHIEnum::PrimitiveTopology::TriangleList:
		default:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		}
	}
}

bool DX12Device::Initialize(bool enableDebugLayer)
{
	if (m_device)
	{
		return true;
	}

	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<IDXGIFactory6> factory;
	UINT factoryFlags = 0;
	if (enableDebugLayer)
	{
		factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory))))
	{
		return false;
	}

	for (UINT adapterIndex = 0;
		factory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
		++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc = {};
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapter.Reset();
			continue;
		}

		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device))))
		{
			break;
		}
		adapter.Reset();
	}

	if (!m_device)
	{
		if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device))))
		{
			return false;
		}
	}

	return true;
}

void DX12Device::Shutdown()
{
	m_device.Reset();
}

ID3D12Device* DX12Device::GetDevice() const
{
	return m_device.Get();
}

void* DX12Device::GetNativeDevice() const
{
	return m_device.Get();
}

std::shared_ptr<IRHIBuffer> DX12Device::CreateBuffer(const RHIBufferDesc& desc, const void* initialData)
{
	if (!m_device || desc.sizeInBytes == 0)
	{
		return {};
	}

	const bool cpuWrite = desc.cpuAccess == RHIEnum::CpuAccess::Write ||
		desc.usage == RHIEnum::BufferUsage::Constant;
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = cpuWrite ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = desc.sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	const D3D12_RESOURCE_STATES initialState = cpuWrite ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;
	if (FAILED(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		initialState, nullptr, IID_PPV_ARGS(&resource))))
	{
		return {};
	}

	if (initialData && cpuWrite)
	{
		void* mapped = nullptr;
		D3D12_RANGE range = { 0, 0 };
		if (SUCCEEDED(resource->Map(0, &range, &mapped)))
		{
			memcpy(mapped, initialData, static_cast<size_t>(desc.sizeInBytes));
			resource->Unmap(0, nullptr);
		}
	}

	return std::make_shared<DX12Buffer>(desc, resource);
}

std::shared_ptr<IRHIConstantBuffer> DX12Device::CreateConstantBuffer(uint64_t sizeInBytes, const void* initialData)
{
	const uint64_t alignedSize = (sizeInBytes + 255u) & ~255u;
	if (!m_device || alignedSize == 0)
	{
		return {};
	}

	RHIConstantBufferDesc desc = {};
	desc.sizeInBytes = alignedSize;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = alignedSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ComPtr<ID3D12Resource> resource;
	if (FAILED(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource))))
	{
		return {};
	}

	void* mapped = nullptr;
	D3D12_RANGE range = { 0, 0 };
	if (FAILED(resource->Map(0, &range, &mapped)))
	{
		return {};
	}

	auto constantBuffer = std::make_shared<DX12ConstantBuffer>(desc, resource, mapped);
	if (initialData)
	{
		constantBuffer->Update(initialData, alignedSize);
	}
	return constantBuffer;
}

std::shared_ptr<IRHITexture> DX12Device::CreateTexture2D(const RHITextureDesc& desc)
{
	if (!m_device || desc.width == 0 || desc.height == 0)
	{
		return {};
	}

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = desc.width;
	texDesc.Height = desc.height;
	texDesc.DepthOrArraySize = desc.arrayLayers;
	texDesc.MipLevels = desc.mipLevels;
	texDesc.Format = DX12Format::ToDxgiFormat(desc.format);
	texDesc.SampleDesc.Count = 1;
	texDesc.Flags = GetTextureFlags(desc);

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
	if (desc.renderTarget)
	{
		initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}
	else if (desc.depthStencil)
	{
		initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	if (FAILED(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
		initialState, nullptr, IID_PPV_ARGS(&resource))))
	{
		return {};
	}

	return std::make_shared<DX12Texture>(desc, resource);
}

std::shared_ptr<IRHIShader> DX12Device::CreateShader(const RHIShaderDesc& desc)
{
	if (desc.bytecode.empty())
	{
		return {};
	}
	return std::make_shared<DX12Shader>(desc);
}

std::shared_ptr<IRHIRootSignature> DX12Device::CreateRootSignature(const RHIRootSignatureDesc& desc)
{
	if (!m_device)
	{
		return {};
	}

	std::vector<D3D12_ROOT_PARAMETER> parameters;
	parameters.reserve(desc.constantBufferCount);
	for (uint32_t i = 0; i < desc.constantBufferCount; ++i)
	{
		D3D12_ROOT_PARAMETER param = {};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		param.Descriptor.ShaderRegister = i;
		param.Descriptor.RegisterSpace = 0;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		parameters.push_back(param);
	}

	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.NumParameters = static_cast<UINT>(parameters.size());
	rootDesc.pParameters = parameters.empty() ? nullptr : parameters.data();
	rootDesc.Flags = desc.allowInputLayout
		? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		: D3D12_ROOT_SIGNATURE_FLAG_NONE;

	ComPtr<ID3DBlob> serialized;
	ComPtr<ID3DBlob> errors;
	if (FAILED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors)))
	{
		return {};
	}

	ComPtr<ID3D12RootSignature> rootSignature;
	if (FAILED(m_device->CreateRootSignature(0, serialized->GetBufferPointer(),
		serialized->GetBufferSize(), IID_PPV_ARGS(&rootSignature))))
	{
		return {};
	}

	return std::make_shared<DX12RootSignature>(desc, rootSignature);
}

std::shared_ptr<IRHIGraphicsPipeline> DX12Device::CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc)
{
	if (!m_device || !desc.rootSignature || !desc.vertexShader || !desc.pixelShader)
	{
		return {};
	}

	std::vector<std::string> semanticNames;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
	semanticNames.reserve(desc.inputLayout.elements.size());
	inputElements.reserve(desc.inputLayout.elements.size());
	for (const auto& element : desc.inputLayout.elements)
	{
		semanticNames.push_back(element.semanticName);
		D3D12_INPUT_ELEMENT_DESC dxElement = {};
		dxElement.SemanticName = semanticNames.back().c_str();
		dxElement.SemanticIndex = element.semanticIndex;
		dxElement.Format = DX12Format::ToDxgiFormat(element.format);
		dxElement.InputSlot = 0;
		dxElement.AlignedByteOffset = element.offset;
		dxElement.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		dxElement.InstanceDataStepRate = 0;
		inputElements.push_back(dxElement);
	}

	D3D12_RASTERIZER_DESC rasterizer = {};
	rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizer.CullMode = ToCullMode(desc.cullMode);
	rasterizer.FrontCounterClockwise = (desc.frontFace == RHIEnum::FrontFace::CounterClockwise) ? TRUE : FALSE;
	rasterizer.DepthClipEnable = TRUE;

	D3D12_BLEND_DESC blend = {};
	blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = static_cast<ID3D12RootSignature*>(desc.rootSignature->GetNativeHandle());
	const auto& vsDesc = desc.vertexShader->GetDesc();
	const auto& psDesc = desc.pixelShader->GetDesc();
	psoDesc.VS = { vsDesc.bytecode.data(), vsDesc.bytecode.size() };
	psoDesc.PS = { psDesc.bytecode.data(), psDesc.bytecode.size() };
	psoDesc.BlendState = blend;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.RasterizerState = rasterizer;
	psoDesc.DepthStencilState.DepthEnable = desc.depthEnabled ? TRUE : FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.InputLayout = { inputElements.data(), static_cast<UINT>(inputElements.size()) };
	psoDesc.PrimitiveTopologyType = ToTopologyType(desc.topology);
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DX12Format::ToDxgiFormat(desc.renderTargetFormat);
	psoDesc.SampleDesc.Count = 1;

	ComPtr<ID3D12PipelineState> pipelineState;
	if (FAILED(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState))))
	{
		return {};
	}

	return std::make_shared<DX12GraphicsPipeline>(desc, pipelineState);
}

std::shared_ptr<IRHIComputePipeline> DX12Device::CreateComputePipeline(const RHIComputePipelineDesc& desc)
{
	if (!m_device || !desc.rootSignature || !desc.computeShader)
	{
		return {};
	}

	const auto& csDesc = desc.computeShader->GetDesc();

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = static_cast<ID3D12RootSignature*>(desc.rootSignature->GetNativeHandle());
	psoDesc.CS = { csDesc.bytecode.data(), csDesc.bytecode.size() };

	ComPtr<ID3D12PipelineState> pipelineState;
	if (FAILED(m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState))))
	{
		return {};
	}

	return std::make_shared<DX12ComputePipeline>(desc, pipelineState);
}

std::shared_ptr<IRHIComputeCommandQueue> DX12Device::CreateComputeCommandQueue()
{
	auto queue = std::make_shared<DX12ComputeCommandQueue>();
	if (!queue->Initialize(m_device.Get()))
	{
		return {};
	}
	return queue;
}

std::shared_ptr<IRHICopyCommandQueue> DX12Device::CreateCopyCommandQueue()
{
	auto queue = std::make_shared<DX12CopyCommandQueue>();
	if (!queue->Initialize(m_device.Get()))
	{
		return {};
	}
	return queue;
}
