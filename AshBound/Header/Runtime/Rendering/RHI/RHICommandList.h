#pragma once

class IRHICommandList
{
public:
	virtual ~IRHICommandList() = default;
	virtual void* GetNativeList() const = 0;
	virtual void Reset() = 0;
	virtual void Close() = 0;
};
