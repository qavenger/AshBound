#pragma once

#include <Windows.h>
#include <dxcapi.h>
#include <cstdint>
#include <string>
#include <vector>

class ShaderCompiler
{
public:
	enum class ShaderDomain
	{
		Vertex,
		Pixel,
		Geometry,
		Hull,
		Domain,
		Compute,
		Mesh,
		Amplification,
		Library
	};

	enum class ShadingModel
	{
		SM6_0,
		SM6_1,
		SM6_2,
		SM6_3,
		SM6_4,
		SM6_5,
		SM6_6,
		SM6_7
	};

	struct CompileOptions
	{
		std::wstring entryPoint;
		ShaderDomain domain = ShaderDomain::Pixel;
		ShadingModel shadingModel = ShadingModel::SM6_0;
		std::vector<DxcDefine> defines;
		std::vector<std::wstring> includeDirectories;
		std::vector<std::wstring> arguments;
		bool writeDxil = false;
		std::wstring outputDirectory;
		std::wstring outputName;
	};

	struct ShaderHotReloadItem
	{
		std::wstring filePath;
		CompileOptions options;
	};

	struct CompileResult
	{
		bool succeeded = false;
		std::vector<uint8_t> bytecode;
		std::wstring errors;
	};

	static CompileResult CompileFromFile(const std::wstring& filePath, const CompileOptions& options);
	static void ClearHotReloadShaders();
	static void AddHotReloadShader(const ShaderHotReloadItem& item);
	static void GetHotReloadShaders(std::vector<ShaderHotReloadItem>& outItems);
	static bool CompileHotReloadShaders(std::vector<CompileResult>& outResults);
};
