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

class IRHIComputeCommandQueue : public IRHICommandQueue
{
public:
	~IRHIComputeCommandQueue() override = default;
};

class IRHICopyCommandQueue : public IRHICommandQueue
{
public:
	~IRHICopyCommandQueue() override = default;
};
