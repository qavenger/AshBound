#include "Editor/ShaderCompiler.h"

#include <Windows.h>
#include <wrl/client.h>

#include <cstring>

#pragma comment(lib, "dxcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace
{
	bool GetDxcInterfaces(ComPtr<IDxcUtils>& outUtils, ComPtr<IDxcCompiler3>& outCompiler)
	{
		thread_local ComPtr<IDxcUtils> tlsUtils;
		thread_local ComPtr<IDxcCompiler3> tlsCompiler;

		if (!tlsUtils || !tlsCompiler)
		{
			if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&tlsUtils))) ||
				FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&tlsCompiler))))
			{
				tlsUtils.Reset();
				tlsCompiler.Reset();
				return false;
			}
		}

		outUtils = tlsUtils;
		outCompiler = tlsCompiler;
		return true;
	}

	const wchar_t* ToDomainPrefix(ShaderCompiler::ShaderDomain domain)
	{
		switch (domain)
		{
		case ShaderCompiler::ShaderDomain::Vertex:
			return L"vs";
		case ShaderCompiler::ShaderDomain::Pixel:
			return L"ps";
		case ShaderCompiler::ShaderDomain::Geometry:
			return L"gs";
		case ShaderCompiler::ShaderDomain::Hull:
			return L"hs";
		case ShaderCompiler::ShaderDomain::Domain:
			return L"ds";
		case ShaderCompiler::ShaderDomain::Compute:
			return L"cs";
		case ShaderCompiler::ShaderDomain::Mesh:
			return L"ms";
		case ShaderCompiler::ShaderDomain::Amplification:
			return L"as";
		case ShaderCompiler::ShaderDomain::Library:
			return L"lib";
		default:
			return L"ps";
		}
	}

	const wchar_t* ToShadingModelSuffix(ShaderCompiler::ShadingModel model)
	{
		switch (model)
		{
		case ShaderCompiler::ShadingModel::SM6_0:
			return L"6_0";
		case ShaderCompiler::ShadingModel::SM6_1:
			return L"6_1";
		case ShaderCompiler::ShadingModel::SM6_2:
			return L"6_2";
		case ShaderCompiler::ShadingModel::SM6_3:
			return L"6_3";
		case ShaderCompiler::ShadingModel::SM6_4:
			return L"6_4";
		case ShaderCompiler::ShadingModel::SM6_5:
			return L"6_5";
		case ShaderCompiler::ShadingModel::SM6_6:
			return L"6_6";
		case ShaderCompiler::ShadingModel::SM6_7:
			return L"6_7";
		default:
			return L"6_0";
		}
	}

	std::wstring BuildTargetProfile(ShaderCompiler::ShaderDomain domain, ShaderCompiler::ShadingModel model)
	{
		std::wstring profile = ToDomainPrefix(domain);
		profile.push_back(L'_');
		profile.append(ToShadingModelSuffix(model));
		return profile;
	}

	std::wstring GetParentDirectory(const std::wstring& path)
	{
		const size_t slash = path.find_last_of(L"\\/");
		if (slash == std::wstring::npos)
		{
			return L"";
		}
		return path.substr(0, slash);
	}

	std::wstring Utf8ToWide(const char* utf8)
	{
		if (!utf8 || utf8[0] == '\0')
		{
			return L"";
		}

		const int utf8Length = static_cast<int>(strlen(utf8));
		const int wideLength = MultiByteToWideChar(CP_UTF8, 0, utf8, utf8Length, nullptr, 0);
		if (wideLength <= 0)
		{
			return L"";
		}

		std::wstring wide(wideLength, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, utf8, utf8Length, &wide[0], wideLength);
		return wide;
	}

	void AppendCompileErrors(IDxcResult* result, ShaderCompiler::CompileResult& outResult)
	{
		ComPtr<IDxcBlobUtf8> errors;
		if (SUCCEEDED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr)) && errors)
		{
			const char* errorText = errors->GetStringPointer();
			if (errorText && errorText[0] != '\0')
			{
				outResult.errors = Utf8ToWide(errorText);
			}
		}
	}
}

ShaderCompiler::CompileResult ShaderCompiler::CompileFromFile(const std::wstring& filePath, const CompileOptions& options)
{
	CompileResult result;

	if (filePath.empty() || options.entryPoint.empty())
	{
		result.errors = L"ShaderCompiler::CompileFromFile: file path or entry point is empty.";
		return result;
	}

	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcCompiler3> compiler;
	if (!GetDxcInterfaces(utils, compiler))
	{
		result.errors = L"ShaderCompiler::CompileFromFile: failed to create DXC interfaces.";
		return result;
	}

	ComPtr<IDxcBlobEncoding> sourceBlob;
	if (FAILED(utils->LoadFile(filePath.c_str(), nullptr, &sourceBlob)) || !sourceBlob)
	{
		result.errors = L"ShaderCompiler::CompileFromFile: failed to load shader file.";
		return result;
	}

	DxcBuffer sourceBuffer = {};
	sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
	sourceBuffer.Size = sourceBlob->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	ComPtr<IDxcIncludeHandler> includeHandler;
	if (FAILED(utils->CreateDefaultIncludeHandler(&includeHandler)))
	{
		result.errors = L"ShaderCompiler::CompileFromFile: failed to create include handler.";
		return result;
	}

	std::vector<std::wstring> arguments;
	arguments.reserve(10 + options.defines.size() + options.includeDirectories.size() + options.arguments.size());
	arguments.push_back(L"-E");
	arguments.push_back(options.entryPoint);
	arguments.push_back(L"-T");
	arguments.push_back(BuildTargetProfile(options.domain, options.shadingModel));

	const std::wstring shaderDirectory = GetParentDirectory(filePath);
	if (!shaderDirectory.empty())
	{
		arguments.push_back(L"-I");
		arguments.push_back(shaderDirectory);
	}
	for (const auto& includeDir : options.includeDirectories)
	{
		if (!includeDir.empty())
		{
			arguments.push_back(L"-I");
			arguments.push_back(includeDir);
		}
	}

	for (const auto& define : options.defines)
	{
		std::wstring defineArg = define.Name ? define.Name : L"";
		if (define.Value && define.Value[0] != L'\0')
		{
			defineArg.append(L"=");
			defineArg.append(define.Value);
		}
		if (!defineArg.empty())
		{
			arguments.push_back(L"-D");
			arguments.push_back(defineArg);
		}
	}

#if defined(_DEBUG)
	arguments.push_back(L"-Zi");
	arguments.push_back(L"-Qembed_debug");
	arguments.push_back(L"-Od");
#else
	arguments.push_back(L"-O3");
#endif
	for (const auto& arg : options.arguments)
	{
		arguments.push_back(arg);
	}

	std::vector<LPCWSTR> argumentPtrs;
	argumentPtrs.reserve(arguments.size());
	for (const auto& arg : arguments)
	{
		argumentPtrs.push_back(arg.c_str());
	}

	ComPtr<IDxcResult> compileResult;
	const HRESULT compileHr = compiler->Compile(
		&sourceBuffer,
		argumentPtrs.data(),
		static_cast<uint32_t>(argumentPtrs.size()),
		includeHandler.Get(),
		IID_PPV_ARGS(&compileResult));

	if (FAILED(compileHr) || !compileResult)
	{
		result.errors = L"ShaderCompiler::CompileFromFile: DXC compile call failed.";
		return result;
	}

	HRESULT status = S_OK;
	if (FAILED(compileResult->GetStatus(&status)))
	{
		result.errors = L"ShaderCompiler::CompileFromFile: failed to query compile status.";
		return result;
	}

	AppendCompileErrors(compileResult.Get(), result);

	if (FAILED(status))
	{
		if (result.errors.empty())
		{
			result.errors = L"ShaderCompiler::CompileFromFile: shader compilation failed.";
		}
		return result;
	}

	ComPtr<IDxcBlob> objectBlob;
	if (FAILED(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&objectBlob), nullptr)) || !objectBlob)
	{
		result.errors = L"ShaderCompiler::CompileFromFile: failed to fetch compiled bytecode.";
		return result;
	}

	const uint8_t* bytecodePtr = static_cast<const uint8_t*>(objectBlob->GetBufferPointer());
	result.bytecode.assign(bytecodePtr, bytecodePtr + objectBlob->GetBufferSize());
	result.succeeded = true;
	return result;
}
