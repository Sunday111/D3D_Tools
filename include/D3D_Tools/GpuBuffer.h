#pragma once

#include "BufferMapper.h"

namespace d3d_tools {
    class IGpuBuffer
    {
    public:
        virtual ~IGpuBuffer() = default;
        virtual void Activate(Device* device, uint32_t offset = 0) = 0;
    };

    template<typename ElementType>
    class GpuBuffer : public IGpuBuffer
    {
    public:
        GpuBuffer(
            Device* device,
            D3D_PRIMITIVE_TOPOLOGY topology,
            edt::DenseArrayView<const ElementType> elements) :
            m_topology(topology)
        {
            auto count = elements.GetSize();
            if (count == 0) {
                return;
            }

            D3D11_BUFFER_DESC desc{};
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.ByteWidth = static_cast<UINT>(sizeof(ElementType) * count);
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            m_buffer = device->CreateBuffer(desc, elements.GetData());
        }

        virtual void Activate(Device* device, uint32_t offset = 0) override {
            CallAndRethrowM + [&] {
                auto pBuffer = m_buffer.Get();
                auto pContext = device->GetContext();
                UINT stride = sizeof(ElementType);
                pContext->IASetVertexBuffers(0, 1, &pBuffer, &stride, &offset);
                pContext->IASetPrimitiveTopology(m_topology);
            };
        }

        d3d_tools::BufferMapper<ElementType> MakeBufferMapper(Device* device, D3D11_MAP map) {
            return d3d_tools::BufferMapper<ElementType>(m_buffer, device->GetContext(), map);
        }

    private:
        ComPtr<ID3D11Buffer> m_buffer;
        D3D_PRIMITIVE_TOPOLOGY m_topology;
    };
}