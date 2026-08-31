#pragma once

#include <memory>

#include "Hosts.h"

struct ID3D12Device;
struct ID3D12CommandQueue;

struct ID3D12DescriptorHeap;
struct ID3D12Resource;

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;

struct ID3D12Fence;
struct IDXGISwapChain3;

class CTwelveHost : public CHost {
public:
    bool Create( void* Window, int Width, int Height ) override;
    void Destroy( ) override;

    void Resize( int Width, int Height ) override;
    void Begin( CColor Backdrop ) override;

    void* Stream( ) override;
    CGraphics* Graphics( ) override;

    void End( bool VerticalSync ) override;

private:
    bool CreateTargets( );
    void ReleaseTargets( );

    void Flush( );

    ID3D12Device* Hardware = nullptr;
    ID3D12CommandQueue* Queue = nullptr;

    IDXGISwapChain3* Swapchain = nullptr;
    ID3D12DescriptorHeap* Targets = nullptr;

    ID3D12Resource* Buffers[ 2 ] = { };
    ID3D12CommandAllocator* Allocators[ 2 ] = { };

    ID3D12GraphicsCommandList* Orders = nullptr;
    ID3D12Fence* Fence = nullptr;

    void* Signal = nullptr;

    unsigned long long Values[ 2 ] = { };
    unsigned long long NextValue = 0;

    unsigned int Increment = 0;
    unsigned int SwapOptions = 0;

    int FrameIndex = 0;

    int SurfaceWidth = 0;
    int SurfaceHeight = 0;

    bool Tearing = false;
};

inline auto TwelveHost = std::make_unique< CTwelveHost >( );