#pragma once

#include "Runtime/Rendering/RHI/RHICommandList.h"
#include "Runtime/Rendering/RHI/RHIRootSignature.h"
#include "Runtime/Rendering/RHI/RHIGraphicsPipeline.h"
#include "Runtime/Rendering/RHI/RHIBuffer.h"
#include "Runtime/Rendering/RHI/RHIConstantBuffer.h"

#include <d3d12.h>
#include <wrl/client.h>

class DX12CommandList final : public IRHICommandList
{
public:
	bool Initialize(ID3D12Device* device);
	void Shutdown();

	ID3D12GraphicsCommandList* GetCommandList() const;
	ID3D12CommandAllocator* GetAllocator() const;
	void* GetNativeList() const override;
	void Reset() override;
	void Close() override;
	void SetViewport(const RHIViewport& viewport) override;
	void SetScissorRect(const RHIRect& rect) override;
	void SetPrimitiveTopology(RHIEnum::PrimitiveTopology topology) override;
	void SetGraphicsRootSignature(IRHIRootSignature* rootSignature) override;
	void SetGraphicsPipeline(IRHIGraphicsPipeline* pipeline) override;
	void SetVertexBuffer(IRHIBuffer* buffer) override;
	void SetIndexBuffer(IRHIBuffer* buffer) override;
	void SetGraphicsConstantBuffer(uint32_t slot, IRHIConstantBuffer* buffer) override;
	void DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) override;
	void Draw(uint32_t vertexCount, uint32_t startVertex) override;

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_allocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
};
