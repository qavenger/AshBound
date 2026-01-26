#include "Runtime/Rendering/DX12/DX12ComputeCommandQueue.h"
#include "Runtime/Rendering/RHI/RHICommandList.h"

#include <Windows.h>

bool DX12ComputeCommandQueue::Initialize(ID3D12Device* device)
{
	if (!device)
	{
		return false;
	}

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	if (FAILED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_queue))))
	{
		return false;
	}

	if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))))
	{
		return false;
	}

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	return m_fenceEvent != nullptr;
}

void DX12ComputeCommandQueue::Shutdown()
{
	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}
	m_fence.Reset();
	m_queue.Reset();
}

ID3D12CommandQueue* DX12ComputeCommandQueue::GetQueue() const
{
	return m_queue.Get();
}

void* DX12ComputeCommandQueue::GetNativeQueue() const
{
	return m_queue.Get();
}

void DX12ComputeCommandQueue::Execute(IRHICommandList* commandList)
{
	if (!commandList || !m_queue)
	{
		return;
	}

	ID3D12CommandList* lists[] = { reinterpret_cast<ID3D12CommandList*>(commandList->GetNativeList()) };
	m_queue->ExecuteCommandLists(1, lists);
}

void DX12ComputeCommandQueue::Flush()
{
	if (!m_queue || !m_fence)
	{
		return;
	}

	const UINT64 fenceValue = ++m_fenceValue;
	if (SUCCEEDED(m_queue->Signal(m_fence.Get(), fenceValue)))
	{
		if (m_fence->GetCompletedValue() < fenceValue)
		{
			m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}
	}
}
