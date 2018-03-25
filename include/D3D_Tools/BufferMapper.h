#pragma once

#include "d3d11.h"
#include "EverydayTools/Array/ArrayView.h"
#include "WinWrappers/ComPtr.h"

namespace d3d_tools {
    template<typename Element>
    class BufferMapper {
    public:
        BufferMapper(ComPtr<ID3D11Buffer> buffer, ComPtr<ID3D11DeviceContext> deviceContext, D3D11_MAP mapType, unsigned mapFlags = 0) :
            m_buffer(buffer),
            m_deviceContext(deviceContext)
        {
            Map(mapType, mapFlags);
        }

        void Write(const Element* elements, size_t count) {
            std::copy(elements, elements + count, GetDataPtr());
        }

        Element& At(size_t index) {
            return GetDataPtr()[index];
        }

        const Element& At(size_t index) const {
            return *(GetDataPtr()[index]);
        }

        Element* GetDataPtr() const {
            return (Element*)m_subresource.pData;
        }

        ~BufferMapper() {
            Unmap();
        }

    protected:
        void Map(D3D11_MAP mapType, unsigned mapFlags) {
            WinAPI<char>::ThrowIfError(m_deviceContext->Map(m_buffer.Get(), 0, mapType, mapFlags, &m_subresource));
        }

        void Unmap() {
            m_deviceContext->Unmap(m_buffer.Get(), 0);
        }

    private:
        D3D11_MAPPED_SUBRESOURCE m_subresource;
        ComPtr<ID3D11Buffer> m_buffer = nullptr;
        ComPtr<ID3D11DeviceContext> m_deviceContext = nullptr;
    };
}