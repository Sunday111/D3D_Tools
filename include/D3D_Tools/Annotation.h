#pragma once

#include "EverydayTools\Exception\CallAndRethrow.h"
#include "WinWrappers\ComPtr.h"
#include "Device.h"

namespace d3d_tools {
    class ScopedAnnotation {
    public:
        ScopedAnnotation(ComPtr<ID3DUserDefinedAnnotation> ptr, const wchar_t* title) :
            m_p(ptr)
        {
            m_p->BeginEvent(title);
        }

        ~ScopedAnnotation() {
            m_p->EndEvent();
        }

    private:
        ComPtr<ID3DUserDefinedAnnotation> m_p;
    };

    inline ScopedAnnotation CreateScopedAnnotation(Device* device, const wchar_t* title) {
        return ScopedAnnotation(device->CreateAnnotation(), title);
    }


    template<typename F>
	inline decltype(auto) Annotate(Device* device, const wchar_t* title, F&& f) {
        ScopedAnnotation annotation(device->CreateAnnotation(), title);
        return static_cast<decltype(f.operator()())>(f());
    }
}