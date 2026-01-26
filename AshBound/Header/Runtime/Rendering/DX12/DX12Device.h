#pragma once

#include "Runtime/Rendering/RHI/RHIDevice.h"
#include "Runtime/Rendering/DX12/DX12Buffer.h"
#include "Runtime/Rendering/DX12/DX12ConstantBuffer.h"
#include "Runtime/Rendering/DX12/DX12GraphicsPipeline.h"
#include "Runtime/Rendering/DX12/DX12ComputePipeline.h"
#include "Runtime/Rendering/DX12/DX12RootSignature.h"
#include "Runtime/Rendering/DX12/DX12ComputeCommandQueue.h"
#include "Runtime/Rendering/DX12/DX12CopyCommandQueue.h"
#include "Runtime/Rendering/DX12/DX12Texture.h"
#include "Runtime/Rendering/DX12/DX12Shader.h"

#include <d3d12.h>
#include <wrl/client.h>

class DX12Device final : public IRHIDevice
{
public:
	bool Initialize(bool enableDebugLayer);
	void Shutdown();

	ID3D12Device* GetDevice() const;
	void* GetNativeDevice() const override;

	std::shared_ptr<IRHIBuffer> CreateBuffer(const RHIBufferDesc& desc, const void* initialData) override;
	std::shared_ptr<IRHIConstantBuffer> CreateConstantBuffer(uint64_t sizeInBytes, const void* initialData) override;
	std::shared_ptr<IRHITexture> CreateTexture2D(const RHITextureDesc& desc) override;
	std::shared_ptr<IRHIShader> CreateShader(const RHIShaderDesc& desc) override;
	std::shared_ptr<IRHIRootSignature> CreateRootSignature(const RHIRootSignatureDesc& desc) override;
	std::shared_ptr<IRHIGraphicsPipeline> CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc) override;
	std::shared_ptr<IRHIComputePipeline> CreateComputePipeline(const RHIComputePipelineDesc& desc) override;
	std::shared_ptr<IRHIComputeCommandQueue> CreateComputeCommandQueue() override;
	std::shared_ptr<IRHICopyCommandQueue> CreateCopyCommandQueue() override;

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
};
