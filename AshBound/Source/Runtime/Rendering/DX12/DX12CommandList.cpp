#include "Runtime/Rendering/DX12/DX12CommandList.h"
#include "Runtime/Rendering/DX12/DX12Buffer.h"
#include "Runtime/Rendering/DX12/DX12GraphicsPipeline.h"
#include "Runtime/Rendering/DX12/DX12RootSignature.h"
#include "Runtime/Rendering/DX12/DX12ConstantBuffer.h"
#include "Runtime/Rendering/DX12/DX12Format.h"

bool DX12CommandList::Initialize(ID3D12Device* device)
{
	if (!device)
	{
		return false;
	}

	if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_allocator))))
	{
		return false;
	}

	if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_allocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList))))
	{
		return false;
	}

	m_commandList->Close();
	return true;
}

void DX12CommandList::Shutdown()
{
	m_commandList.Reset();
	m_allocator.Reset();
}

ID3D12GraphicsCommandList* DX12CommandList::GetCommandList() const
{
	return m_commandList.Get();
}

ID3D12CommandAllocator* DX12CommandList::GetAllocator() const
{
	return m_allocator.Get();
}

void* DX12CommandList::GetNativeList() const
{
	return m_commandList.Get();
}

void DX12CommandList::Reset()
{
	if (m_allocator && m_commandList)
	{
		m_allocator->Reset();
		m_commandList->Reset(m_allocator.Get(), nullptr);
	}
}

void DX12CommandList::Close()
{
	if (m_commandList)
	{
		m_commandList->Close();
	}
}

void DX12CommandList::SetViewport(const RHIViewport& viewport)
{
	if (!m_commandList)
	{
		return;
	}
	D3D12_VIEWPORT dxViewport = {};
	dxViewport.TopLeftX = viewport.x;
	dxViewport.TopLeftY = viewport.y;
	dxViewport.Width = viewport.width;
	dxViewport.Height = viewport.height;
	dxViewport.MinDepth = viewport.minDepth;
	dxViewport.MaxDepth = viewport.maxDepth;
	m_commandList->RSSetViewports(1, &dxViewport);
}

void DX12CommandList::SetScissorRect(const RHIRect& rect)
{
	if (!m_commandList)
	{
		return;
	}
	D3D12_RECT dxRect = {};
	dxRect.left = rect.left;
	dxRect.top = rect.top;
	dxRect.right = rect.right;
	dxRect.bottom = rect.bottom;
	m_commandList->RSSetScissorRects(1, &dxRect);
}

void DX12CommandList::SetPrimitiveTopology(RHIEnum::PrimitiveTopology topology)
{
	if (!m_commandList)
	{
		return;
	}
	D3D_PRIMITIVE_TOPOLOGY dxTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	switch (topology)
	{
	case RHIEnum::PrimitiveTopology::PointList:
		dxTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		break;
	case RHIEnum::PrimitiveTopology::LineList:
		dxTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	case RHIEnum::PrimitiveTopology::LineStrip:
		dxTopology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		break;
	case RHIEnum::PrimitiveTopology::TriangleStrip:
		dxTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		break;
	case RHIEnum::PrimitiveTopology::TriangleList:
	default:
		dxTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	}
	m_commandList->IASetPrimitiveTopology(dxTopology);
}

void DX12CommandList::SetGraphicsRootSignature(IRHIRootSignature* rootSignature)
{
	if (!m_commandList || !rootSignature)
	{
		return;
	}
	auto* dxRootSignature = static_cast<DX12RootSignature*>(rootSignature);
	ID3D12RootSignature* native = dxRootSignature ? dxRootSignature->GetRootSignature() : nullptr;
	if (native)
	{
		m_commandList->SetGraphicsRootSignature(native);
	}
}

void DX12CommandList::SetGraphicsPipeline(IRHIGraphicsPipeline* pipeline)
{
	if (!m_commandList || !pipeline)
	{
		return;
	}
	auto* dxPipeline = static_cast<DX12GraphicsPipeline*>(pipeline);
	ID3D12PipelineState* native = dxPipeline ? dxPipeline->GetPipelineState() : nullptr;
	if (native)
	{
		m_commandList->SetPipelineState(native);
	}
}

void DX12CommandList::SetVertexBuffer(IRHIBuffer* buffer)
{
	if (!m_commandList || !buffer)
	{
		return;
	}
	auto* dxBuffer = static_cast<DX12Buffer*>(buffer);
	ID3D12Resource* resource = dxBuffer ? dxBuffer->GetResource() : nullptr;
	if (!resource)
	{
		return;
	}
	D3D12_VERTEX_BUFFER_VIEW view = {};
	view.BufferLocation = resource->GetGPUVirtualAddress();
	view.SizeInBytes = static_cast<UINT>(buffer->GetDesc().sizeInBytes);
	view.StrideInBytes = static_cast<UINT>(buffer->GetDesc().strideInBytes);
	m_commandList->IASetVertexBuffers(0, 1, &view);
}

void DX12CommandList::SetIndexBuffer(IRHIBuffer* buffer)
{
	if (!m_commandList || !buffer)
	{
		return;
	}
	auto* dxBuffer = static_cast<DX12Buffer*>(buffer);
	ID3D12Resource* resource = dxBuffer ? dxBuffer->GetResource() : nullptr;
	if (!resource)
	{
		return;
	}
	D3D12_INDEX_BUFFER_VIEW view = {};
	view.BufferLocation = resource->GetGPUVirtualAddress();
	view.SizeInBytes = static_cast<UINT>(buffer->GetDesc().sizeInBytes);
	view.Format = DXGI_FORMAT_R32_UINT;
	m_commandList->IASetIndexBuffer(&view);
}

void DX12CommandList::SetGraphicsConstantBuffer(uint32_t slot, IRHIConstantBuffer* buffer)
{
	if (!m_commandList || !buffer)
	{
		return;
	}
	auto* dxBuffer = static_cast<DX12ConstantBuffer*>(buffer);
	ID3D12Resource* resource = dxBuffer ? dxBuffer->GetResource() : nullptr;
	if (!resource)
	{
		return;
	}
	m_commandList->SetGraphicsRootConstantBufferView(slot, resource->GetGPUVirtualAddress());
}

void DX12CommandList::DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex)
{
	if (m_commandList)
	{
		m_commandList->DrawIndexedInstanced(indexCount, 1, startIndex, baseVertex, 0);
	}
}

void DX12CommandList::Draw(uint32_t vertexCount, uint32_t startVertex)
{
	if (m_commandList)
	{
		m_commandList->DrawInstanced(vertexCount, 1, startVertex, 0);
	}
}
