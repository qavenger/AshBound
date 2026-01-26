#include "Runtime/Rendering/SceneRenderer.h"

#include "Editor/ShaderCompiler.h"
#include "Runtime/Core/ColorManagement/ColorManagement.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Core/Viewport.h"
#include "Runtime/Core/UI/UserInterfaceSubsystem.h"
#include "Runtime/Engine/Camera.h"
#include "Runtime/Engine/Scene.h"
#include "Runtime/Engine/StaticMesh.h"
#include "Runtime/Engine/StaticMeshComponent.h"
#include "Runtime/Engine/SceneObject.h"
#include "Runtime/Rendering/ViewParameters.h"
#include "Runtime/Rendering/RHI/RHICommandList.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <utility>
#include <vector>

namespace
{
	DEFINE_LOG_CATEGORY(LogSceneRenderer, Info)

	RHIEnum::Format ResolveBackbufferFormat(uint32_t backbufferBitDepth)
	{
		if (backbufferBitDepth >= 16)
		{
			return RHIEnum::Format::RGBA16_FLOAT;
		}
		if (backbufferBitDepth >= 10)
		{
			return RHIEnum::Format::R10G10B10A2_UNORM;
		}
		return RHIEnum::Format::R10G10B10A2_UNORM;
	}

	std::filesystem::path GetExecutableDirectory()
	{
#if defined(_WIN32)
		std::array<wchar_t, 512> buffer = {};
		const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
		if (length > 0 && length < buffer.size())
		{
			return std::filesystem::path(buffer.data()).parent_path();
		}
#endif
		return std::filesystem::current_path();
	}

	std::filesystem::path FindShaderRootPath()
	{
		std::filesystem::path base = GetExecutableDirectory();
		for (int i = 0; i < 6; ++i)
		{
			const std::filesystem::path directShaders = base / "Shaders";
			const std::filesystem::path directHeader = base / "Header";
			if (std::filesystem::exists(directShaders) && std::filesystem::exists(directHeader))
			{
				return base;
			}

			const std::filesystem::path nestedShaders = base / "AshBound" / "Shaders";
			const std::filesystem::path nestedHeader = base / "AshBound" / "Header";
			if (std::filesystem::exists(nestedShaders) && std::filesystem::exists(nestedHeader))
			{
				return base / "AshBound";
			}

			if (!base.has_parent_path())
			{
				break;
			}
			base = base.parent_path();
		}
		return std::filesystem::current_path();
	}

	std::wstring GetShaderOutputDirectory(const std::filesystem::path& rootPath)
	{
		const wchar_t* configName =
#if defined(_DEBUG)
			L"Debug";
#else
			L"Release";
#endif
		std::filesystem::path base = rootPath.empty() ? std::filesystem::current_path() : rootPath;
		base /= "Intermediate";
		base /= "Shaders";
		base /= configName;
		std::filesystem::create_directories(base);
		return base.wstring();
	}

	RHIShaderStage ToRhiShaderStage(ShaderCompiler::ShaderDomain domain)
	{
		switch (domain)
		{
		case ShaderCompiler::ShaderDomain::Vertex:
			return RHIShaderStage::Vertex;
		case ShaderCompiler::ShaderDomain::Pixel:
			return RHIShaderStage::Pixel;
		case ShaderCompiler::ShaderDomain::Geometry:
			return RHIShaderStage::Geometry;
		case ShaderCompiler::ShaderDomain::Hull:
			return RHIShaderStage::Hull;
		case ShaderCompiler::ShaderDomain::Domain:
			return RHIShaderStage::Domain;
		case ShaderCompiler::ShaderDomain::Compute:
			return RHIShaderStage::Compute;
		case ShaderCompiler::ShaderDomain::Mesh:
			return RHIShaderStage::Mesh;
		case ShaderCompiler::ShaderDomain::Amplification:
			return RHIShaderStage::Amplification;
		case ShaderCompiler::ShaderDomain::Library:
			return RHIShaderStage::Library;
		default:
			return RHIShaderStage::Pixel;
		}
	}

	std::wstring BuildShaderOutputName(const std::wstring& filePath, const std::wstring& entryPoint)
	{
		std::filesystem::path shaderPath(filePath);
		std::wstring outputName = shaderPath.stem().wstring();
		outputName.push_back(L'_');
		outputName.append(entryPoint);
		return outputName;
	}

	bool CreateRootSignatureAndPipeline(SceneRendererResources& resources, IRHIDevice* device, RHIEnum::Format backbufferFormat)
	{
		if (!device || !resources.vertexShader || !resources.pixelShader)
		{
			return false;
		}

		RHIRootSignatureDesc rootDesc = {};
		rootDesc.constantBufferCount = 1;
		rootDesc.allowInputLayout = true;
		resources.rootSignature = device->CreateRootSignature(rootDesc);
		if (!resources.rootSignature)
		{
			return false;
		}

		RHIGraphicsPipelineDesc pipelineDesc = {};
		pipelineDesc.rootSignature = resources.rootSignature;
		pipelineDesc.vertexShader = resources.vertexShader;
		pipelineDesc.pixelShader = resources.pixelShader;
		pipelineDesc.renderTargetFormat = backbufferFormat;
		pipelineDesc.topology = RHIEnum::PrimitiveTopology::TriangleList;
		pipelineDesc.cullMode = RHIEnum::CullMode::Back;
		pipelineDesc.frontFace = RHIEnum::FrontFace::CounterClockwise;
		pipelineDesc.depthEnabled = false;
		pipelineDesc.inputLayout.strideInBytes = sizeof(StaticMeshVertex);
		pipelineDesc.inputLayout.elements = {
			{ "POSITION", 0, RHIEnum::Format::R32G32B32_FLOAT, 0 },
			{ "NORMAL", 0, RHIEnum::Format::R32G32B32_FLOAT, 12 },
			{ "TEXCOORD", 0, RHIEnum::Format::R32G32_FLOAT, 24 }
		};

		resources.pipeline = device->CreateGraphicsPipeline(pipelineDesc);
		return static_cast<bool>(resources.pipeline);
	}

	bool CreateViewConstantBuffer(SceneRendererResources& resources, IRHIDevice* device)
	{
		if (!device)
		{
			return false;
		}
		resources.viewConstantBuffer = device->CreateConstantBuffer(sizeof(ViewParameters), nullptr);
		return static_cast<bool>(resources.viewConstantBuffer);
	}
}

bool SceneRenderer::s_tonemapEnabled = true;
uint32_t SceneRenderer::s_tonemapMode = 0u;

bool SceneRenderer::IsTonemapEnabled()
{
	return s_tonemapEnabled;
}

void SceneRenderer::SetTonemapEnabled(bool enabled)
{
	s_tonemapEnabled = enabled;
}

void SceneRenderer::ToggleTonemapEnabled()
{
	s_tonemapEnabled = !s_tonemapEnabled;
}

uint32_t SceneRenderer::GetTonemapMode()
{
	return s_tonemapMode;
}

void SceneRenderer::SetTonemapMode(uint32_t mode)
{
	s_tonemapMode = mode;
}

bool SceneRenderer::Initialize(const RHIRendererInitInfo& info)
{
	if (m_renderer)
	{
		return true;
	}

	m_renderer = CreateRendererForPlatform();
	if (!m_renderer)
	{
		return false;
	}

	if (!m_renderer->Initialize(info))
	{
		m_renderer.reset();
		return false;
	}

	m_device = m_renderer->GetDevice();
	m_swapChain = m_renderer->GetSwapChain();
	m_commandQueue = m_renderer->GetCommandQueue();
	m_commandList = m_renderer->GetCommandList();
	m_backbufferFormat = info.backbufferFormat;

	m_resources = std::make_unique<SceneRendererResources>();
	const std::filesystem::path shaderRoot = FindShaderRootPath();
	const std::wstring outputDir = GetShaderOutputDirectory(shaderRoot);
	ShaderCompiler::ClearHotReloadShaders();
	ShaderCompiler::ShaderHotReloadItem vertexItem;
	vertexItem.filePath = (shaderRoot / "Shaders" / "FullscreenVertexShader.hlsl").wstring();
	vertexItem.options.entryPoint = L"VSMain";
	vertexItem.options.domain = ShaderCompiler::ShaderDomain::Vertex;
	vertexItem.options.shadingModel = ShaderCompiler::ShadingModel::SM6_0;
	vertexItem.options.includeDirectories.push_back((shaderRoot / "Header").wstring());
	vertexItem.options.writeDxil = true;
	vertexItem.options.outputDirectory = outputDir;
	vertexItem.options.outputName = BuildShaderOutputName(vertexItem.filePath, vertexItem.options.entryPoint);
	ShaderCompiler::AddHotReloadShader(vertexItem);

	ShaderCompiler::ShaderHotReloadItem pixelItem;
	pixelItem.filePath = (shaderRoot / "Shaders" / "FullscreenPixelShader.hlsl").wstring();
	pixelItem.options.entryPoint = L"PSMain";
	pixelItem.options.domain = ShaderCompiler::ShaderDomain::Pixel;
	pixelItem.options.shadingModel = ShaderCompiler::ShadingModel::SM6_0;
	pixelItem.options.includeDirectories.push_back((shaderRoot / "Header").wstring());
	pixelItem.options.writeDxil = true;
	pixelItem.options.outputDirectory = outputDir;
	pixelItem.options.outputName = BuildShaderOutputName(pixelItem.filePath, pixelItem.options.entryPoint);
	ShaderCompiler::AddHotReloadShader(pixelItem);

	if (!std::filesystem::exists(vertexItem.filePath))
	{
		LOG(LogSceneRenderer, Warning, L"Shader file missing: %s", vertexItem.filePath.c_str());
	}
	if (!std::filesystem::exists(pixelItem.filePath))
	{
		LOG(LogSceneRenderer, Warning, L"Shader file missing: %s", pixelItem.filePath.c_str());
	}

	if (!ReloadShaders())
	{
		return false;
	}

	if (!CreateRootSignatureAndPipeline(*m_resources, m_device, m_backbufferFormat))
	{
		return false;
	}
	if (!CreateViewConstantBuffer(*m_resources, m_device))
	{
		return false;
	}
	return true;
}

bool SceneRenderer::RecreateSwapChain(uint32_t width, uint32_t height, RHIEnum::Format backbufferFormat, RHIEnum::ColorSpace colorSpace)
{
	const bool result = m_renderer ? m_renderer->RecreateSwapChain(width, height, backbufferFormat, colorSpace) : false;
	if (result)
	{
		m_backbufferFormat = backbufferFormat;
		if (m_resources)
		{
			m_resources->pipeline.reset();
			CreateRootSignatureAndPipeline(*m_resources, m_device, m_backbufferFormat);
		}
	}
	return result;
}

bool SceneRenderer::RequestBackbufferBitDepth(uint32_t bitDepth)
{
	const uint32_t current = ColorManagement::GetBackbufferBitDepth();
	if (current == bitDepth)
	{
		return true;
	}
	if (!ColorManagement::SetBackbufferBitDepth(bitDepth))
	{
		return false;
	}

	m_pendingBackbufferBitDepth = ColorManagement::GetBackbufferBitDepth();
	m_pendingSwapChainRebuild = true;
	return true;
}

void SceneRenderer::ProcessPendingSwapChain(uint32_t width, uint32_t height, RHIEnum::Format backbufferFormat, RHIEnum::ColorSpace colorSpace)
{
	if (!m_pendingSwapChainRebuild)
	{
		return;
	}

	if (RecreateSwapChain(width, height, backbufferFormat, colorSpace))
	{
		m_pendingSwapChainRebuild = false;
	}
}

void SceneRenderer::Shutdown()
{
	if (m_renderer)
	{
		m_renderer->Shutdown();
		m_renderer.reset();
	}

	m_device = nullptr;
	m_swapChain = nullptr;
	m_commandQueue = nullptr;
	m_commandList = nullptr;
	m_resources.reset();
}

void SceneRenderer::RequestShaderReload()
{
	m_pendingShaderReload = true;
}

void SceneRenderer::RequestSwapChainRebuild()
{
	m_pendingSwapChainRebuild = true;
}

bool SceneRenderer::ReloadShaders()
{
	if (!m_device || !m_resources)
	{
		return false;
	}

	std::vector<ShaderCompiler::ShaderHotReloadItem> items;
	ShaderCompiler::GetHotReloadShaders(items);
	if (items.empty())
	{
		LOG(LogSceneRenderer, Warning, L"No shaders registered for hot reload.");
		return false;
	}

	std::vector<ShaderCompiler::CompileResult> results;
	const bool compiled = ShaderCompiler::CompileHotReloadShaders(results);
	if (!compiled || results.size() != items.size())
	{
		for (size_t i = 0; i < results.size(); ++i)
		{
			if (!results[i].succeeded && !results[i].errors.empty())
			{
				LOG(LogSceneRenderer, Warning, L"Shader compile failed: %s", results[i].errors.c_str());
			}
		}
		return false;
	}

	SceneRendererResources temp = *m_resources;
	for (size_t i = 0; i < items.size(); ++i)
	{
		RHIShaderDesc desc;
		desc.stage = ToRhiShaderStage(items[i].options.domain);
		desc.bytecode = std::move(results[i].bytecode);
		std::shared_ptr<IRHIShader> shader = m_device->CreateShader(desc);
		if (!shader)
		{
			LOG(LogSceneRenderer, Warning, L"Failed to create shader for %s.", items[i].filePath.c_str());
			return false;
		}
		if (desc.stage == RHIShaderStage::Vertex)
		{
			temp.vertexShader = shader;
		}
		else if (desc.stage == RHIShaderStage::Pixel)
		{
			temp.pixelShader = shader;
		}
	}

	if (!temp.vertexShader || !temp.pixelShader)
	{
		LOG(LogSceneRenderer, Warning, L"Missing vertex/pixel shader after reload.");
		return false;
	}

	if (!CreateRootSignatureAndPipeline(temp, m_device, m_backbufferFormat))
	{
		LOG(LogSceneRenderer, Warning, L"Failed to rebuild pipeline after shader reload.");
		return false;
	}

	m_resources->rootSignature = temp.rootSignature;
	m_resources->pipeline = temp.pipeline;
	m_resources->vertexShader = temp.vertexShader;
	m_resources->pixelShader = temp.pixelShader;
	return true;
}

void SceneRenderer::RenderScene(const Scene& scene, const ViewportInfo& viewportInfo)
{
	if (!m_resources || !m_resources->rootSignature || !m_resources->pipeline || !m_commandList || !m_device)
	{
		return;
	}

	if (m_pendingShaderReload)
	{
		m_pendingShaderReload = false;
		if (!ReloadShaders())
		{
			LOG(LogSceneRenderer, Warning, L"Shader reload failed; keeping existing shaders.");
		}
	}

	Camera* camera = nullptr;
	for (RenderComponent* renderComponent : scene.GetRenderComponents())
	{
		auto* meshComponent = dynamic_cast<StaticMeshComponent*>(renderComponent);
		if (!meshComponent)
		{
			continue;
		}

		SceneObject* owner = meshComponent->GetOwner();
		if (!owner)
		{
			continue;
		}

		if (!camera)
		{
			for (const auto& object : scene.GetObjects())
			{
				if (!object)
				{
					continue;
				}
				camera = dynamic_cast<Camera*>(object.get());
				if (camera)
				{
					break;
				}
			}
		}

		if (!camera)
		{
			return;
		}

		StaticMesh* mesh = meshComponent->GetStaticMesh();
		if (!mesh)
		{
			continue;
		}

		camera->SetAspectRatio(viewportInfo.aspectRatio);

		if (!mesh->GetVertexBuffer())
		{
			mesh->UploadToGPU(m_device);
		}
		const std::shared_ptr<IRHIBuffer>& vertexBuffer = mesh->GetVertexBuffer();
		const std::shared_ptr<IRHIBuffer>& indexBuffer = mesh->GetIndexBuffer();
		if (!vertexBuffer)
		{
			return;
		}

		ViewParameters viewParams = {};
		viewParams.View = camera->GetViewMatrix();
		viewParams.Projection = camera->GetProjectionMatrix();
		viewParams.ViewProjection = camera->GetViewProjectionMatrix();
		viewParams.InverseView = camera->GetInverseViewMatrix();
		viewParams.InverseProjection = camera->GetInverseProjectionMatrix();
		viewParams.InverseViewProjection = camera->GetInverseViewProjectionMatrix();

		const auto& camPos = camera->GetTransform().GetPosition();
		viewParams.CameraPositionX = camPos.x;
		viewParams.CameraPositionY = camPos.y;
		viewParams.CameraPositionZ = camPos.z;
		viewParams.CameraPositionW = 1.0f;

		viewParams.CameraFovDegrees = camera->GetFovDegrees();
		viewParams.CameraOrthoSize = camera->GetOrthoSize();
		viewParams.CameraAspectRatio = camera->GetAspectRatio();
		viewParams.CameraIsOrthographic = camera->IsOrthographic() ? 1.0f : 0.0f;

		viewParams.CameraNearPlane = camera->GetNearPlane();
		viewParams.CameraFarPlane = camera->GetFarPlane();
		viewParams.CameraPadding0 = 0.0f;
		viewParams.CameraPadding1 = 0.0f;

		viewParams.ViewportWidth = viewportInfo.width;
		viewParams.ViewportHeight = viewportInfo.height;
		viewParams.ViewportInvWidth = viewportInfo.invWidth;
		viewParams.ViewportInvHeight = viewportInfo.invHeight;

		viewParams.ViewportAspectRatio = viewportInfo.aspectRatio;
		viewParams.ViewportInvAspectRatio = viewportInfo.invAspectRatio;
		viewParams.ViewportCenterX = viewportInfo.centerX;
		viewParams.ViewportCenterY = viewportInfo.centerY;

		viewParams.ViewportWorkingColorSpaceId = static_cast<uint32_t>(viewportInfo.workingColorSpaceId);
		viewParams.ViewportOutputGamutId = static_cast<uint32_t>(viewportInfo.outputGamutId);
		viewParams.ViewportEotfId = static_cast<uint32_t>(viewportInfo.eotfId);
		viewParams.TonemapEnabled = SceneRenderer::IsTonemapEnabled() ? 1u : 0u;
		viewParams.TonemapMode = SceneRenderer::GetTonemapMode();
		viewParams.TonemapPadding0 = 0u;
		viewParams.TonemapPadding1 = 0u;
		viewParams.TonemapPadding2 = 0u;

		viewParams.ViewportMaxLuminance = viewportInfo.maxLuminance;
		viewParams.ViewportMinLuminanceLog10 = viewportInfo.minLuminanceLog10;
		viewParams.ViewportGamma = viewportInfo.gamma;
		viewParams.ViewportInvGamma = viewportInfo.invGamma;

		const Math::Matrix3 workingToOutput = ColorManagement::GetWorkingToOutputMatrix(
			static_cast<OutputGamut>(viewportInfo.outputGamutId));
		viewParams.WorkingToOutputRow0X = workingToOutput.m[0][0];
		viewParams.WorkingToOutputRow0Y = workingToOutput.m[0][1];
		viewParams.WorkingToOutputRow0Z = workingToOutput.m[0][2];
		viewParams.WorkingToOutputRow0W = 0.0f;

		viewParams.WorkingToOutputRow1X = workingToOutput.m[1][0];
		viewParams.WorkingToOutputRow1Y = workingToOutput.m[1][1];
		viewParams.WorkingToOutputRow1Z = workingToOutput.m[1][2];
		viewParams.WorkingToOutputRow1W = 0.0f;

		viewParams.WorkingToOutputRow2X = workingToOutput.m[2][0];
		viewParams.WorkingToOutputRow2Y = workingToOutput.m[2][1];
		viewParams.WorkingToOutputRow2Z = workingToOutput.m[2][2];
		viewParams.WorkingToOutputRow2W = 0.0f;

		const float maxNits = (std::max)(viewportInfo.maxLuminance, 1.0f);
		const float minNits = (viewportInfo.minLuminanceLog10 > 0.0f)
			? pow(10.0f, viewportInfo.minLuminanceLog10)
			: 0.0f;
		const float midGreyNits = (viewportInfo.sdrWhiteLevelNits > 0.0f)
			? viewportInfo.sdrWhiteLevelNits * 0.18f
			: 18.0f;
		const float normalizedBlack = (minNits > 0.0f) ? (minNits / maxNits) : 0.0f;
		const float normalizedMidGrey = (midGreyNits > 0.0f) ? (midGreyNits / maxNits) : 0.18f;

		viewParams.Tonemap.Exposure = 1.0f;
		viewParams.Tonemap.Contrast = 1.1f;
		viewParams.Tonemap.ToeStrength = 0.5f;
		viewParams.Tonemap.ToeLength = 0.4f;
		viewParams.Tonemap.ShoulderStrength = 0.6f;
		viewParams.Tonemap.ShoulderLength = 1.0f;
		viewParams.Tonemap.ShoulderAngle = 0.4f;
		viewParams.Tonemap.Gamma = 1.0f;
		viewParams.Tonemap.Saturation = 1.0f;
		viewParams.Tonemap.Lift = 0.0f;
		viewParams.Tonemap.Gain = 1.0f;
		viewParams.Tonemap.WhitePoint = 1.0f;
		viewParams.Tonemap.BlackPoint = normalizedBlack;
		viewParams.Tonemap.MaxLuminance = maxNits;
		viewParams.Tonemap.MidGrey = normalizedMidGrey;
		viewParams.Tonemap.Padding0 = 0.0f;

		static int lastLoggedOutputGamutId = -1;
		static int lastLoggedEotfId = -1;
		static float lastLoggedMaxLuminance = -1.0f;
		if (lastLoggedOutputGamutId != viewportInfo.outputGamutId ||
			lastLoggedEotfId != viewportInfo.eotfId ||
			lastLoggedMaxLuminance != viewportInfo.maxLuminance)
		{
			LOG(LogSceneRenderer, Info,
				L"ViewParams: OutputGamut=%d EOTF=%d MaxLum=%.1f",
				viewportInfo.outputGamutId,
				viewportInfo.eotfId,
				viewportInfo.maxLuminance);
			lastLoggedOutputGamutId = viewportInfo.outputGamutId;
			lastLoggedEotfId = viewportInfo.eotfId;
			lastLoggedMaxLuminance = viewportInfo.maxLuminance;
		}

		if (m_resources->viewConstantBuffer)
		{
			m_resources->viewConstantBuffer->Update(&viewParams, sizeof(ViewParameters));
		}

		RHIViewport viewport = {};
		viewport.width = viewportInfo.width;
		viewport.height = viewportInfo.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		RHIRect scissor = {};
		scissor.right = static_cast<int32_t>(viewportInfo.width);
		scissor.bottom = static_cast<int32_t>(viewportInfo.height);

		m_commandList->SetGraphicsRootSignature(m_resources->rootSignature.get());
		m_commandList->SetGraphicsPipeline(m_resources->pipeline.get());
		m_commandList->SetViewport(viewport);
		m_commandList->SetScissorRect(scissor);
		m_commandList->SetPrimitiveTopology(RHIEnum::PrimitiveTopology::TriangleList);
		m_commandList->SetVertexBuffer(vertexBuffer.get());

		if (indexBuffer)
		{
			m_commandList->SetIndexBuffer(indexBuffer.get());
		}

		if (m_resources->viewConstantBuffer)
		{
			m_commandList->SetGraphicsConstantBuffer(0, m_resources->viewConstantBuffer.get());
		}

		if (indexBuffer)
		{
			const uint32_t indexCount = static_cast<uint32_t>(mesh->GetIndices().size());
			m_commandList->DrawIndexed(indexCount, 0, 0);
		}
		else
		{
			const uint32_t vertexCount = static_cast<uint32_t>(mesh->GetVertices().size());
			m_commandList->Draw(vertexCount, 0);
		}
	}
}

void SceneRenderer::RenderUserInterfaces(UserInterfaceSubsystem* uiSubsystem)
{
	if (!m_commandList || !uiSubsystem)
	{
		return;
	}
	uiSubsystem->RenderAll(m_commandList);
}
