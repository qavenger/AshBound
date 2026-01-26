#include "Runtime/Rendering/DX12/DX12ConstantBuffer.h"

#include <cstring>
#include <utility>

DX12ConstantBuffer::DX12ConstantBuffer(const RHIConstantBufferDesc& desc,
	Microsoft::WRL::ComPtr<ID3D12Resource> resource,
	void* mappedData)
	: m_desc(desc)
	, m_resource(std::move(resource))
	, m_mappedData(static_cast<uint8_t*>(mappedData))
{
}

DX12ConstantBuffer::~DX12ConstantBuffer()
{
	if (m_resource && m_mappedData)
	{
		m_resource->Unmap(0, nullptr);
		m_mappedData = nullptr;
	}
}

void* DX12ConstantBuffer::GetNativeResource() const
{
	return m_resource.Get();
}

const RHIConstantBufferDesc& DX12ConstantBuffer::GetDesc() const
{
	return m_desc;
}

void DX12ConstantBuffer::Update(const void* data, uint64_t sizeInBytes)
{
	if (!data || !m_mappedData || sizeInBytes == 0)
	{
		return;
	}
	const uint64_t size = sizeInBytes < m_desc.sizeInBytes ? sizeInBytes : m_desc.sizeInBytes;
	std::memcpy(m_mappedData, data, static_cast<size_t>(size));
}

ID3D12Resource* DX12ConstantBuffer::GetResource() const
{
	return m_resource.Get();
}
