#pragma once

#include "d3d11.h"
#include "d3d11_1.h"
#include "EverydayTools\Exception\CallAndRethrow.h"
#include "WinWrappers\ComPtr.h"
#include "WinWrappers\WinWrappers.h"
#include "Texture.h"
#include "Shader.h"

namespace d3d_tools {
    class Device {
    public:
        struct CreateParams {
            bool debugDevice;
            bool noDeviceMultithreading = false;
        };

        Device(CreateParams params) {
            CallAndRethrowM + [&] {
                unsigned flags = 0;
                if (params.debugDevice) {
                    flags |= D3D11_CREATE_DEVICE_DEBUG;
                }

                if (params.noDeviceMultithreading) {
                    flags |= D3D11_CREATE_DEVICE_SINGLETHREADED;
                    flags |= D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS;
                }

                WinAPI<char>::ThrowIfError(D3D11CreateDevice(
                    nullptr,
                    D3D_DRIVER_TYPE_HARDWARE,
                    nullptr,
                    flags,
                    nullptr,
                    0,
                    D3D11_SDK_VERSION,
                    m_device.Receive(),
                    nullptr,
                    m_deviceContext.Receive()));
            };
        }

        ID3D11Device* GetDevice() const {
            return m_device.Get();
        }

        ID3D11DeviceContext* GetContext() const {
            return m_deviceContext.Get();
        }

        template<ShaderType shaderType>
        decltype(auto) CreateShader(const char* code, const char* entryPoint, ShaderVersion shaderVersion) {
            return CallAndRethrowM + [&] {
                Shader<shaderType> result;
                result.Compile(code, entryPoint, shaderVersion);
                result.Create(m_device.Get());
                return result;
            };
        }

        template<ShaderType shaderType>
        decltype(auto) CreateShader(edt::DenseArrayView<const uint8_t> code, const char* entryPoint, ShaderVersion shaderVersion) {
            return CreateShader<shaderType>((const char*)code.GetData(), entryPoint, shaderVersion);
        }

        template<ShaderType shaderType>
        decltype(auto) CreateShader(std::istream& code, const char* entryPoint, ShaderVersion shaderVersion) {
            return CallAndRethrowM + [&] {
                auto startpos = code.tellg();
                code.seekg(0, std::ios::end);
                auto endpos = code.tellg();
                auto size = endpos - startpos;
                code.seekg(startpos);
                std::string readcode;
                readcode.resize(size);
                code.read(&readcode[0], size);
                return CreateShader<shaderType>(readcode.data(), entryPoint, shaderVersion);
            };
        }

        template<ShaderType shaderType>
        void SetShader(Shader<shaderType>& shader) {
            CallAndRethrowM + [&] {
                using Traits = shader_details::ShaderTraits<shaderType>;
                Traits::Set(
                    m_deviceContext.Get(),
                    shader.shader.Get(),
                    nullptr,
                    0);
            };
        }

        void SetRenderTarget(TextureView<ResourceViewType::RenderTarget>& rtv, TextureView<ResourceViewType::DepthStencil>* dsv = nullptr) {
            auto pRTV = rtv.GetView();
            ID3D11DepthStencilView* pDSV = nullptr;
            if (dsv) {
                pDSV = dsv->GetView();
            }
            m_deviceContext->OMSetRenderTargets(1, &pRTV, pDSV);
        }

        void SetViewports(edt::DenseArrayView<const D3D11_VIEWPORT> viewports) {
            m_deviceContext->RSSetViewports(
                static_cast<UINT>(viewports.GetSize()),
                viewports.GetData());
        }

        void Draw(unsigned vertexCount, unsigned startvert = 0) {
            m_deviceContext->Draw(vertexCount, startvert);
        }

        void SetInputLayout(ID3D11InputLayout* layout) {
            m_deviceContext->IASetInputLayout(layout);
        }

        void SetVertexBuffer(ID3D11Buffer* buffer, unsigned stride, unsigned offset) {
            m_deviceContext->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
        }

        void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topo) {
            m_deviceContext->IASetPrimitiveTopology(topo);
        }

        ComPtr<ID3DUserDefinedAnnotation> CreateAnnotation() const {
            return CallAndRethrowM + [&] {
                ComPtr<ID3DUserDefinedAnnotation> annotation;
                WinAPI<char>::ThrowIfError(m_deviceContext->QueryInterface(
                    __uuidof(ID3DUserDefinedAnnotation),
                    reinterpret_cast<void**>(annotation.Receive())));
                return annotation;
            };
        }

        ComPtr<ID3D11InputLayout> CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* elementDescriptor, unsigned elementsCount, ID3D10Blob* shader) {
            return CallAndRethrowM + [&] {
                ComPtr<ID3D11InputLayout> result;
                WinAPI<char>::ThrowIfError(
                    m_device->CreateInputLayout(
                        elementDescriptor, elementsCount,
                        shader->GetBufferPointer(), shader->GetBufferSize(),
                        result.Receive()));
                return result;
            };
        }

        ComPtr<ID3D11Buffer> CreateBuffer(D3D11_BUFFER_DESC desc, const void* initialData = nullptr) {
            ComPtr<ID3D11Buffer> buffer;
            D3D11_SUBRESOURCE_DATA subresourceData;
            const D3D11_SUBRESOURCE_DATA* pSubresourceData = nullptr;
            if (initialData) {
                subresourceData.pSysMem = initialData;
                subresourceData.SysMemPitch = 0;
                subresourceData.SysMemSlicePitch = 0;
                pSubresourceData = &subresourceData;
            }
            WinAPI<char>::ThrowIfError(m_device->CreateBuffer(&desc, pSubresourceData, buffer.Receive()));
            return buffer;
        }

    private:
        ComPtr<ID3D11Device> m_device;
        ComPtr<ID3D11DeviceContext> m_deviceContext;
    };
}