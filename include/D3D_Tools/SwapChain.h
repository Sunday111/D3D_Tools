#pragma once

#include "Device.h"

namespace d3d_tools {
    class SwapChain {
    public:
        SwapChain(ID3D11Device* device, uint32_t w, uint32_t h, HWND hWnd)
        {
            CallAndRethrowM + [&] {
                ComPtr<IDXGIFactory> factory;
                WinAPI<char>::ThrowIfError(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(factory.Receive())));
    
                DXGI_SWAP_CHAIN_DESC scd{};
                scd.BufferCount = 1;                                    // one back buffer
                scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
                scd.BufferDesc.Width = w;
                scd.BufferDesc.Height = h;
                scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
                scd.OutputWindow = hWnd;                                // the window to be used
                scd.SampleDesc.Count = 1;                               // how many multisamples
                scd.Windowed = TRUE;                                    // windowed/full-screen mode
                scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
                WinAPI<char>::ThrowIfError(factory->CreateSwapChain(device, &scd, m_swapchain.Receive()));
            };
        }

        ~SwapChain() {
            if (m_swapchain.Get()) {
                m_swapchain->SetFullscreenState(false, nullptr);
            }
        }
    
        void* GetNativeInterface() const {
            return m_swapchain.Get();
        }
    
        IDXGISwapChain* GetInterface() const {
            return m_swapchain.Get();
        }

        Texture GetBackBuffer(uint32_t index = 0) {
            return CallAndRethrowM + [&] {
                ComPtr<ID3D11Texture2D> backBuffer;
                // get the address of the back buffer
                WinAPI<char>::ThrowIfError(m_swapchain->GetBuffer(index, __uuidof(ID3D11Texture2D), (void**)backBuffer.Receive()));
                backBuffer->AddRef();
                return Texture(std::move(backBuffer));
            };
        }

        void Present(unsigned interval = 0, unsigned flags = 0) {
            m_swapchain->Present(interval, flags);
        }
    
    private:
        ComPtr<IDXGISwapChain> m_swapchain;
    };
}