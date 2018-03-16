#pragma once

#include "d3d11.h"
#include "d3dcompiler.h"
#include "EverydayTools\Exception\CallAndRethrow.h"
#include "EverydayTools\Array\ArrayView.h"
#include "WinWrappers\ComPtr.h"
#include "WinWrappers\WinWrappers.h"

namespace d3d_tools {
    enum class ShaderType {
        Compute,
        Domain,
        Geometry,
        Hull,
        Pixel,
        Vertex
    };

    enum class ShaderVersion {
        _5_0,
        _4_1,
        _4_0,
    };

	struct ShaderMacro {
		std::string_view name;
		std::string_view value;
	};

    namespace shader_details {
    
        template<typename Interface>
        using CreateShaderMethodType = HRESULT(ID3D11Device::*)(const void*, SIZE_T, ID3D11ClassLinkage*, Interface**);
    
        template<typename Interface>
        using SetShaderMethodType = void(ID3D11DeviceContext::*)(Interface*, ID3D11ClassInstance*const*, unsigned);
    
        template
        <
            typename InterfaceType,
            CreateShaderMethodType<InterfaceType> createMethod,
            SetShaderMethodType<InterfaceType> setMethod
        >
        struct ShaderTraitsBase
        {
            using Interface = InterfaceType;
            static ComPtr<Interface> Create(ID3D11Device* device, const void* bytecode, SIZE_T size, ID3D11ClassLinkage* linkage) {
                return CallAndRethrowM + [&] {
                    ComPtr<Interface> result;
                    WinAPI<char>::ThrowIfError((device->*createMethod)(bytecode, size, linkage, result.Receive()));
                    return result;
                };
            }
        
            static void Set(ID3D11DeviceContext* context, Interface* shader, ID3D11ClassInstance*const* instances, uint32_t count) {
                (context->*setMethod)(shader, instances, count);
            }
        };
    
        template<ShaderType shaderType>
        struct ShaderTraits;
        
        template<>
        struct ShaderTraits<ShaderType::Vertex> :
            public ShaderTraitsBase<
                ID3D11VertexShader,
                &ID3D11Device::CreateVertexShader,
                &ID3D11DeviceContext::VSSetShader>
        {
        };
        
        template<>
        struct ShaderTraits<ShaderType::Pixel> :
            public ShaderTraitsBase<
                ID3D11PixelShader,
                &ID3D11Device::CreatePixelShader,
                &ID3D11DeviceContext::PSSetShader>
        {
        };
        
        template<>
        struct ShaderTraits<ShaderType::Geometry> :
            public ShaderTraitsBase<
                ID3D11GeometryShader,
                &ID3D11Device::CreateGeometryShader,
                &ID3D11DeviceContext::GSSetShader>
        {
        };
        
        template<>
        struct ShaderTraits<ShaderType::Hull> :
            public ShaderTraitsBase<
                ID3D11HullShader,
                &ID3D11Device::CreateHullShader,
                &ID3D11DeviceContext::HSSetShader>
        {
        };
        
        template<>
        struct ShaderTraits<ShaderType::Domain> :
            public ShaderTraitsBase<
                ID3D11DomainShader,
                &ID3D11Device::CreateDomainShader,
                &ID3D11DeviceContext::DSSetShader>
        {
        };
        
        template<>
        struct ShaderTraits<ShaderType::Compute> :
            public ShaderTraitsBase<
                ID3D11ComputeShader,
                &ID3D11Device::CreateComputeShader,
                &ID3D11DeviceContext::CSSetShader>
        {
        };
    
    }
    
    inline const char* ShaderTypeToShaderTarget(ShaderType shaderType, ShaderVersion shaderVersion) {
        return CallAndRethrowM + [&] {
            switch (shaderVersion)
            {
            case ShaderVersion::_5_0:
                switch (shaderType)
                {
                case ShaderType::Compute: return "cs_5_0";
                case ShaderType::Domain:        return "ds_5_0";
                case ShaderType::Geometry:      return "gs_5_0";
                case ShaderType::Hull:          return "hs_5_0";
                case ShaderType::Pixel:         return "ps_5_0";
                case ShaderType::Vertex:        return "vs_5_0";
                default: throw std::invalid_argument("This version (5.0) does not support this shader type");
                }
            case ShaderVersion::_4_1:
                switch (shaderType)
                {
                case ShaderType::Compute: return "cs_4_1";
                case ShaderType::Geometry:      return "gs_4_1";
                case ShaderType::Pixel:         return "ps_4_1";
                case ShaderType::Vertex:        return "vs_4_1";
                default: throw std::invalid_argument("This version (4.1) does not support this shader type");
                }
            case ShaderVersion::_4_0:
                switch (shaderType)
                {
                case ShaderType::Compute: return "cs_4_0";
                case ShaderType::Geometry:      return "gs_4_0";
                case ShaderType::Pixel:         return "ps_4_0";
                case ShaderType::Vertex:        return "vs_4_0";
                default: throw std::invalid_argument("This version (4.0) does not support this shader type");
                }
            default:
                throw std::invalid_argument("This version is not supported");
            }
        };
    }

    inline ComPtr<ID3DBlob> CompileShaderToBlob(const char* code, const char* entryPoint, ShaderType shaderType, ShaderVersion shaderVersion,
		edt::SparseArrayView<const ShaderMacro> definitionsView = edt::SparseArrayView<const ShaderMacro>()) {
        return CallAndRethrowM + [&] {
            auto datasize = std::char_traits<char>::length(code);
            auto shaderTarget = ShaderTypeToShaderTarget(shaderType, shaderVersion);
            ComPtr<ID3DBlob> shaderBlob;
            ComPtr<ID3DBlob> errorBlob;
            // This lambda combines described windows error code with d3d compiler error
            // and throws and exception
            auto onCompileError = [&](std::string errorMessage) {
                if (errorBlob.Get()) {
                    errorMessage += ": ";
                    errorMessage.append(
                        (char*)errorBlob->GetBufferPointer(),
                        errorBlob->GetBufferSize());
                }
                throw std::runtime_error(std::move(errorMessage));
            };
    
            UINT flags1 = 0
            #ifdef _DEBUG
                | D3DCOMPILE_DEBUG
                | D3DCOMPILE_SKIP_OPTIMIZATION
                | D3DCOMPILE_WARNINGS_ARE_ERRORS
                | D3DCOMPILE_ALL_RESOURCES_BOUND
            #endif;
                ;

			std::vector<D3D_SHADER_MACRO> definitions;
			D3D_SHADER_MACRO nullDefinition {};
			const D3D_SHADER_MACRO* definitionsPtr = &nullDefinition;

			auto definitionsCount = definitionsView.GetSize();
			if (definitionsCount > 0) {
				definitions.reserve(definitionsCount + 1);
				for (auto& definition : definitionsView) {
					definitions.push_back(D3D_SHADER_MACRO { definition.name.data(), definition.value.data()});
				}
				definitions.push_back(nullDefinition);
				definitionsPtr = definitions.data();
			}
    
            WinAPI<char>::HandleError(D3DCompile(
                code,
                datasize,
                nullptr,              // May be used for debugging
				definitionsPtr,       // Null-terminated array of macro definitions
                nullptr,              // Includes
                entryPoint,           // Main function of shader
                shaderTarget,         // shader target
                flags1,               // Flags for compile constants
                0,                    // Flags for compile effects constants
                shaderBlob.Receive(), // Output compiled shader
                errorBlob.Receive()   // Compile error messages
            ), onCompileError);
            return shaderBlob;
        };
    }

    template<ShaderType shaderType>
    class Shader
    {
    public:
        using Traits = shader_details::ShaderTraits<shaderType>;
        using Interface = typename Traits::Interface;
    
        void Compile(const char* code, const char* entryPoint, ShaderVersion shaderVersion, edt::SparseArrayView<const ShaderMacro> definitions =
			edt::SparseArrayView<const ShaderMacro>()) {
            CallAndRethrowM + [&] {
                bytecode = CompileShaderToBlob(code, entryPoint, shaderType, shaderVersion, definitions);
            };
        }
    
        void Create(ID3D11Device* device) {
            shader = Traits::Create(device,
                bytecode->GetBufferPointer(),
                bytecode->GetBufferSize(),
                nullptr);
        }
    
        ComPtr<ID3D10Blob> bytecode;
        ComPtr<Interface> shader;
    };
}