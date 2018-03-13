#pragma once

#include "GpuBuffer.h"

namespace d3d_tools {
    class ICrossDeviceBuffer
    {
    public:
        virtual ~ICrossDeviceBuffer() = default;
        virtual std::shared_ptr<IGpuBuffer> GetGpuBuffer() const = 0;
        virtual void BeginUpdate() = 0;
        virtual void EndUpdate() = 0;
        virtual void Sync(Device* device) = 0;
    };

    template<typename ElementType>
    class CrossDeviceBuffer : public ICrossDeviceBuffer
    {
    public:
        CrossDeviceBuffer(
            Device* device, D3D_PRIMITIVE_TOPOLOGY topology,
            edt::DenseArrayView<const ElementType> elements) :
            m_gpuBuffer(std::make_shared<GpuBuffer<ElementType>>(device, topology, elements))
        {
            auto count = elements.GetSize();
            if (count == 0) {
                return;
            }

            m_cpuMirror.reserve(count);
            for (auto& element : elements) {
                m_cpuMirror.push_back(element);
            }
        }

        edt::DenseArrayView<const ElementType> MakeView() const {
            return const_cast<CrossDeviceBuffer<ElementType>*>(this)->MakeView();
        }

        virtual std::shared_ptr<IGpuBuffer> GetGpuBuffer() const override {
            return m_gpuBuffer;
        }

        virtual void BeginUpdate() override {
            m_dirty = true;
        }

        edt::DenseArrayView<ElementType> MakeView()
        {
            return CallAndRethrowM + [&] {
                return edt::DenseArrayView<ElementType>(&m_cpuMirror[0], m_cpuMirror.size());
            };
        }

        virtual void EndUpdate() override {
            // For now
        }

        virtual void Sync(Device* device) override {
            if (!m_dirty) {
                return;
            }

            auto mapper = m_gpuBuffer->MakeBufferMapper(device, D3D11_MAP_WRITE_DISCARD);
            mapper.Write(m_cpuMirror.data(), m_cpuMirror.size());
            m_dirty = false;
        }

    private:
        bool m_dirty = true;
        std::shared_ptr<GpuBuffer<ElementType>> m_gpuBuffer;
        std::vector<ElementType> m_cpuMirror;
    };
}