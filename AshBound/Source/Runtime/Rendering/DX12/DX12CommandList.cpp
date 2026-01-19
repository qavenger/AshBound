#include "Runtime/Rendering/DX12/DX12CommandList.h"

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
