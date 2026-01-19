#pragma once

class IRHICommandList;

class IRHICommandQueue
{
public:
	virtual ~IRHICommandQueue() = default;
	virtual void* GetNativeQueue() const = 0;
	virtual void Execute(IRHICommandList* commandList) = 0;
	virtual void Flush() = 0;
};
