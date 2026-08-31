#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_5.h>

#include "DirectX11.h"
#include "ElevenHost.h"
#include "ur/overlay.hpp"

#ifndef WS_EX_NOREDIRECTIONBITMAP
#define WS_EX_NOREDIRECTIONBITMAP 0x00200000L
#endif

static bool MakeSwap( IDXGIFactory2* Factory, ID3D11Device* Hardware, HWND Window, const DXGI_SWAP_CHAIN_DESC1& Description, IDXGISwapChain1** Out ) {
    *Out = nullptr;
    return SUCCEEDED( Factory->CreateSwapChainForHwnd( Hardware, Window, &Description, nullptr, nullptr, Out ) ) && *Out;
}

bool CElevenHost::Create( void* Window, int Width, int Height ) {
    Destroy( );

    HWND Handle = ( HWND )Window;
    D3D_FEATURE_LEVEL Wanted = D3D_FEATURE_LEVEL_11_0;
    UINT DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if ( FAILED( D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, DeviceFlags, &Wanted, 1, D3D11_SDK_VERSION, &Hardware, nullptr, &Commands ) ) ) {
        if ( FAILED( D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &Wanted, 1, D3D11_SDK_VERSION, &Hardware, nullptr, &Commands ) ) )
            return false;
    }

    IDXGIDevice* Bridge = nullptr;
    IDXGIAdapter* Adapter = nullptr;
    IDXGIFactory2* Factory = nullptr;

    Hardware->QueryInterface( __uuidof( IDXGIDevice ), ( void** )&Bridge );
    if ( Bridge )
        Bridge->GetAdapter( &Adapter );
    if ( Adapter )
        Adapter->GetParent( __uuidof( IDXGIFactory2 ), ( void** )&Factory );

    if ( !Factory ) {
        if ( Adapter )
            Adapter->Release( );
        if ( Bridge )
            Bridge->Release( );
        Destroy( );
        return false;
    }

    IDXGIFactory5* Modern = nullptr;
    Factory->QueryInterface( __uuidof( IDXGIFactory5 ), ( void** )&Modern );
    if ( Modern ) {
        BOOL Allowed = FALSE;
        if ( SUCCEEDED( Modern->CheckFeatureSupport( DXGI_FEATURE_PRESENT_ALLOW_TEARING, &Allowed, sizeof( Allowed ) ) ) )
            Tearing = Allowed != FALSE;
        Modern->Release( );
    }

    bool Glass = ur::overlay::glass( );
    if ( Glass )
        Tearing = false;

    SwapOptions = Tearing ? ( unsigned int )DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

    DXGI_SWAP_CHAIN_DESC1 Description = { };
    Description.Width = ( unsigned int )Width;
    Description.Height = ( unsigned int )Height;
    Description.Format = Glass ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
    Description.SampleDesc.Count = 1;
    Description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    Description.Flags = SwapOptions;

    LONG Extra = GetWindowLongW( Handle, GWL_EXSTYLE );
    HRESULT Outcome = E_FAIL;

    if ( Glass ) {
        Description.BufferCount = 2;
        Description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        Description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
        SetWindowLongW( Handle, GWL_EXSTYLE, Extra | WS_EX_NOREDIRECTIONBITMAP );
        if ( MakeSwap( Factory, Hardware, Handle, Description, &Swapchain ) ) {
            Outcome = S_OK;
        } else {
            SetWindowLongW( Handle, GWL_EXSTYLE, Extra );
            Description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            if ( MakeSwap( Factory, Hardware, Handle, Description, &Swapchain ) ) {
                SetWindowLongW( Handle, GWL_EXSTYLE, Extra | WS_EX_NOREDIRECTIONBITMAP );
                Outcome = S_OK;
            } else {
                Description.BufferCount = 1;
                Description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                Description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
                if ( MakeSwap( Factory, Hardware, Handle, Description, &Swapchain ) )
                    Outcome = S_OK;
                else {
                    Description.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
                    if ( MakeSwap( Factory, Hardware, Handle, Description, &Swapchain ) )
                        Outcome = S_OK;
                }
            }
        }
    } else {
        Description.BufferCount = 2;
        Description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        Description.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        if ( MakeSwap( Factory, Hardware, Handle, Description, &Swapchain ) )
            Outcome = S_OK;
    }

    Factory->MakeWindowAssociation( Handle, DXGI_MWA_NO_ALT_ENTER );
    Factory->Release( );
    Adapter->Release( );
    if ( Bridge )
        Bridge->Release( );

    if ( FAILED( Outcome ) ) {
        Destroy( );
        return false;
    }

    SurfaceWidth = Width;
    SurfaceHeight = Height;

    if ( !CreateTarget( ) ) {
        Destroy( );
        return false;
    }

    if ( !Eleven->Create( Hardware, Commands ) ) {
        Destroy( );
        return false;
    }

    if ( Glass )
        ur::overlay::seal( Handle );

    return true;
}

void CElevenHost::Destroy( ) {
    Eleven->Destroy( );

    if ( Target ) {
        Target->Release( );
        Target = nullptr;
    }

    if ( Swapchain ) {
        Swapchain->Release( );
        Swapchain = nullptr;
    }

    if ( Commands ) {
        Commands->ClearState( );
        Commands->Flush( );

        Commands->Release( );
        Commands = nullptr;
    }

    if ( Hardware ) {
        Hardware->Release( );
        Hardware = nullptr;
    }

    SurfaceWidth = 0;
    SurfaceHeight = 0;

    Tearing = false;
    SwapOptions = 0;
}

bool CElevenHost::CreateTarget( ) {
    ID3D11Texture2D* Backbuffer = nullptr;
    Swapchain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( void** )&Backbuffer );

    if ( !Backbuffer )
        return false;

    HRESULT Outcome = Hardware->CreateRenderTargetView( Backbuffer, nullptr, &Target );
    Backbuffer->Release( );

    return SUCCEEDED( Outcome );
}

void CElevenHost::Resize( int Width, int Height ) {
    if ( !Swapchain || Width <= 0 || Height <= 0 )
        return;

    if ( Width == SurfaceWidth && Height == SurfaceHeight )
        return;

    Commands->OMSetRenderTargets( 0, nullptr, nullptr );

    if ( Target ) {
        Target->Release( );
        Target = nullptr;
    }

    if ( FAILED( Swapchain->ResizeBuffers( 0, ( unsigned int )Width, ( unsigned int )Height, DXGI_FORMAT_UNKNOWN, SwapOptions ) ) )
        return;

    SurfaceWidth = Width;
    SurfaceHeight = Height;

    CreateTarget( );
}

void CElevenHost::Begin( CColor Backdrop ) {
    if ( !Target )
        return;

    float Channels[ 4 ] = { Backdrop.Red / 255.0f, Backdrop.Green / 255.0f, Backdrop.Blue / 255.0f, Backdrop.Alpha / 255.0f };

    Commands->ClearRenderTargetView( Target, Channels );
    Commands->OMSetRenderTargets( 1, &Target, nullptr );
}

void* CElevenHost::Stream( ) {
    return nullptr;
}

CGraphics* CElevenHost::Graphics( ) {
    return Eleven.get( );
}

void CElevenHost::End( bool VerticalSync ) {
    if ( !Swapchain )
        return;

    unsigned int Options = 0u;
    if ( !VerticalSync && Tearing )
        Options = DXGI_PRESENT_ALLOW_TEARING;
    else if ( !VerticalSync )
        Options = DXGI_PRESENT_DO_NOT_WAIT;
    Swapchain->Present( VerticalSync ? 1 : 0, Options );
}
