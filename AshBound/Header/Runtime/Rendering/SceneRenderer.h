#pragma once

#include <memory>

#include "Runtime/Rendering/RHI/RHI.h"
#include "Runtime/Rendering/RHI/RHIShader.h"
#include "Runtime/Rendering/RHI/RHIRootSignature.h"
#include "Runtime/Rendering/RHI/RHIGraphicsPipeline.h"
#include "Runtime/Rendering/RHI/RHIConstantBuffer.h"

class Scene;
struct ViewportInfo;
class UserInterfaceSubsystem;

struct SceneRendererResources
{
	std::shared_ptr<IRHIRootSignature> rootSignature;
	std::shared_ptr<IRHIGraphicsPipeline> pipeline;
	std::shared_ptr<IRHIConstantBuffer> viewConstantBuffer;
	std::shared_ptr<IRHIShader> vertexShader;
	std::shared_ptr<IRHIShader> pixelShader;
};

class SceneRenderer
{
public:
	SceneRenderer() = default;
	virtual ~SceneRenderer() = default;

	bool Initialize(const RHIRendererInitInfo& info);
	void Shutdown();
	static bool IsTonemapEnabled();
	static void SetTonemapEnabled(bool enabled);
	static void ToggleTonemapEnabled();
	static uint32_t GetTonemapMode();
	static void SetTonemapMode(uint32_t mode);
	bool RecreateSwapChain(uint32_t width, uint32_t height, RHIEnum::Format backbufferFormat, RHIEnum::ColorSpace colorSpace);
	bool RequestBackbufferBitDepth(uint32_t bitDepth);
	void RequestShaderReload();
	void RequestSwapChainRebuild();
	void ProcessPendingSwapChain(uint32_t width, uint32_t height, RHIEnum::Format backbufferFormat, RHIEnum::ColorSpace colorSpace);
	bool IsSwapChainRebuildPending() const { return m_pendingSwapChainRebuild; }
	void RenderScene(const Scene& scene, const ViewportInfo& viewportInfo);
	void RenderUserInterfaces(UserInterfaceSubsystem* uiSubsystem);

	IRHIRenderer* GetRenderer() const { return m_renderer.get(); }
	IRHIDevice* GetDevice() const { return m_device; }
	IRHISwapChain* GetSwapChain() const { return m_swapChain; }
	IRHICommandQueue* GetCommandQueue() const { return m_commandQueue; }
	IRHICommandList* GetCommandList() const { return m_commandList; }

protected:
	std::unique_ptr<IRHIRenderer> m_renderer;
	IRHIDevice* m_device = nullptr;
	IRHISwapChain* m_swapChain = nullptr;
	IRHICommandQueue* m_commandQueue = nullptr;
	IRHICommandList* m_commandList = nullptr;
	bool m_pendingSwapChainRebuild = false;
	uint32_t m_pendingBackbufferBitDepth = 10;
	RHIEnum::Format m_backbufferFormat = RHIEnum::Format::Unknown;
	std::unique_ptr<SceneRendererResources> m_resources;
	bool m_pendingShaderReload = false;

private:
	static bool s_tonemapEnabled;
	static uint32_t s_tonemapMode;
	bool ReloadShaders();
};
