#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_5.h>

#include "DirectX12.h"
#include "TwelveHost.h"

bool CTwelveHost::Create( void* Window, int Width, int Height ) {
    Destroy( );

    if ( FAILED( D3D12CreateDevice( nullptr, D3D_FEATURE_LEVEL_11_0, __uuidof( ID3D12Device ), ( void** )&Hardware ) ) )
        return false;

    D3D12_COMMAND_QUEUE_DESC Task = { };
    Task.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    Hardware->CreateCommandQueue( &Task, __uuidof( ID3D12CommandQueue ), ( void** )&Queue );

    if ( !Queue ) {
        Destroy( );
        return false;
    }

    IDXGIFactory5* Factory = nullptr;
    CreateDXGIFactory1( __uuidof( IDXGIFactory5 ), ( void** )&Factory );

    if ( !Factory ) {
        Destroy( );
        return false;
    }

    BOOL Allowed = FALSE;

    if ( SUCCEEDED( Factory->CheckFeatureSupport( DXGI_FEATURE_PRESENT_ALLOW_TEARING, &Allowed, sizeof( Allowed ) ) ) )
        Tearing = Allowed != FALSE;

    SwapOptions = Tearing ? ( unsigned int )DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

    DXGI_SWAP_CHAIN_DESC1 Description = { };

    Description.Width = ( unsigned int )Width;
    Description.Height = ( unsigned int )Height;

    Description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Description.SampleDesc.Count = 1;

    Description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    Description.BufferCount = 2;

    Description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    Description.Flags = SwapOptions;

    IDXGISwapChain1* Fresh = nullptr;

    HRESULT Outcome = Factory->CreateSwapChainForHwnd( Queue, ( HWND )Window, &Description, nullptr, nullptr, &Fresh );
    Factory->MakeWindowAssociation( ( HWND )Window, DXGI_MWA_NO_ALT_ENTER );

    Factory->Release( );

    if ( FAILED( Outcome ) || !Fresh ) {
        Destroy( );
        return false;
    }

    Fresh->QueryInterface( __uuidof( IDXGISwapChain3 ), ( void** )&Swapchain );
    Fresh->Release( );

    if ( !Swapchain ) {
        Destroy( );
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC Slots = { };

    Slots.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    Slots.NumDescriptors = 2;

    Hardware->CreateDescriptorHeap( &Slots, __uuidof( ID3D12DescriptorHeap ), ( void** )&Targets );
    Increment = Hardware->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

    if ( !Targets || !CreateTargets( ) ) {
        Destroy( );
        return false;
    }

    for ( int Frame = 0; Frame < 2; Frame++ ) {
        Hardware->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof( ID3D12CommandAllocator ), ( void** )&Allocators[ Frame ] );

        if ( !Allocators[ Frame ] ) {
            Destroy( );
            return false;
        }
    }

    Hardware->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, Allocators[ 0 ], nullptr, __uuidof( ID3D12GraphicsCommandList ), ( void** )&Orders );

    if ( !Orders ) {
        Destroy( );
        return false;
    }

    Orders->Close( );
    Hardware->CreateFence( 0, D3D12_FENCE_FLAG_NONE, __uuidof( ID3D12Fence ), ( void** )&Fence );

    Signal = CreateEventA( nullptr, FALSE, FALSE, nullptr );

    if ( !Fence || !Signal ) {
        Destroy( );
        return false;
    }

    SurfaceWidth = Width;
    SurfaceHeight = Height;

    if ( !Twelve->Create( Hardware, 2, ( int )DXGI_FORMAT_R8G8B8A8_UNORM ) ) {
        Destroy( );
        return false;
    }

    return true;
}

void CTwelveHost::Flush( ) {
    if ( !Queue || !Fence || !Signal )
        return;

    NextValue++;
    Queue->Signal( Fence, NextValue );

    if ( Fence->GetCompletedValue( ) < NextValue ) {
        Fence->SetEventOnCompletion( NextValue, Signal );
        WaitForSingleObject( Signal, 5000 );
    }
}

void CTwelveHost::Destroy( ) {
    Flush( );

    Twelve->Destroy( );
    ReleaseTargets( );

    for ( int Frame = 0; Frame < 2; Frame++ ) {
        if ( Allocators[ Frame ] ) {
            Allocators[ Frame ]->Release( );
            Allocators[ Frame ] = nullptr;
        }

        Values[ Frame ] = 0;
    }

    if ( Orders ) {
        Orders->Release( );
        Orders = nullptr;
    }

    if ( Fence ) {
        Fence->Release( );
        Fence = nullptr;
    }

    if ( Signal ) {
        CloseHandle( Signal );
        Signal = nullptr;
    }

    if ( Targets ) {
        Targets->Release( );
        Targets = nullptr;
    }

    if ( Swapchain ) {
        Swapchain->Release( );
        Swapchain = nullptr;
    }

    if ( Queue ) {
        Queue->Release( );
        Queue = nullptr;
    }

    if ( Hardware ) {
        Hardware->Release( );
        Hardware = nullptr;
    }

    NextValue = 0;
    FrameIndex = 0;

    SurfaceWidth = 0;
    SurfaceHeight = 0;

    Tearing = false;
    SwapOptions = 0;
}

bool CTwelveHost::CreateTargets( ) {
    D3D12_CPU_DESCRIPTOR_HANDLE Handle = Targets->GetCPUDescriptorHandleForHeapStart( );

    for ( int Frame = 0; Frame < 2; Frame++ ) {
        if ( FAILED( Swapchain->GetBuffer( ( unsigned int )Frame, __uuidof( ID3D12Resource ), ( void** )&Buffers[ Frame ] ) ) )
            return false;

        Hardware->CreateRenderTargetView( Buffers[ Frame ], nullptr, Handle );
        Handle.ptr += Increment;
    }

    return true;
}

void CTwelveHost::ReleaseTargets( ) {
    for ( int Frame = 0; Frame < 2; Frame++ ) {
        if ( Buffers[ Frame ] ) {
            Buffers[ Frame ]->Release( );
            Buffers[ Frame ] = nullptr;
        }
    }
}

void CTwelveHost::Resize( int Width, int Height ) {
    if ( !Swapchain || Width <= 0 || Height <= 0 )
        return;

    if ( Width == SurfaceWidth && Height == SurfaceHeight )
        return;

    Flush( );
    ReleaseTargets( );

    if ( FAILED( Swapchain->ResizeBuffers( 0, ( unsigned int )Width, ( unsigned int )Height, DXGI_FORMAT_UNKNOWN, SwapOptions ) ) )
        return;

    SurfaceWidth = Width;
    SurfaceHeight = Height;

    CreateTargets( );
}

void CTwelveHost::Begin( CColor Backdrop ) {
    if ( !Swapchain || !Orders )
        return;

    FrameIndex = ( int )Swapchain->GetCurrentBackBufferIndex( );

    if ( Fence->GetCompletedValue( ) < Values[ FrameIndex ] ) {
        Fence->SetEventOnCompletion( Values[ FrameIndex ], Signal );
        WaitForSingleObject( Signal, 5000 );
    }

    Allocators[ FrameIndex ]->Reset( );
    Orders->Reset( Allocators[ FrameIndex ], nullptr );

    D3D12_RESOURCE_BARRIER Barrier = { };

    Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Transition.pResource = Buffers[ FrameIndex ];

    Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;

    Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    Orders->ResourceBarrier( 1, &Barrier );

    D3D12_CPU_DESCRIPTOR_HANDLE Handle = Targets->GetCPUDescriptorHandleForHeapStart( );
    Handle.ptr += ( size_t )FrameIndex * Increment;

    float Channels[ 4 ] = { Backdrop.Red / 255.0f, Backdrop.Green / 255.0f, Backdrop.Blue / 255.0f, Backdrop.Alpha / 255.0f };

    Orders->ClearRenderTargetView( Handle, Channels, 0, nullptr );
    Orders->OMSetRenderTargets( 1, &Handle, FALSE, nullptr );
}

void* CTwelveHost::Stream( ) {
    return Orders;
}

CGraphics* CTwelveHost::Graphics( ) {
    return Twelve.get( );
}

void CTwelveHost::End( bool VerticalSync ) {
    if ( !Swapchain || !Orders )
        return;

    D3D12_RESOURCE_BARRIER Barrier = { };

    Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Transition.pResource = Buffers[ FrameIndex ];

    Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;

    Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    Orders->ResourceBarrier( 1, &Barrier );
    Orders->Close( );

    ID3D12CommandList* Bundle[ 1 ] = { Orders };
    Queue->ExecuteCommandLists( 1, Bundle );

    unsigned int Options = !VerticalSync && Tearing ? DXGI_PRESENT_ALLOW_TEARING : 0u;
    Swapchain->Present( VerticalSync ? 1 : 0, Options );

    NextValue++;
    Queue->Signal( Fence, NextValue );

    Values[ FrameIndex ] = NextValue;
}