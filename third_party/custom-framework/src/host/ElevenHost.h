#pragma once

#include <memory>

#include "Hosts.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

struct ID3D11RenderTargetView;
struct IDXGISwapChain1;

class CElevenHost : public CHost {
public:
    bool Create( void* Window, int Width, int Height ) override;
    void Destroy( ) override;

    void Resize( int Width, int Height ) override;
    void Begin( CColor Backdrop ) override;

    void* Stream( ) override;
    CGraphics* Graphics( ) override;

    void End( bool VerticalSync ) override;

private:
    bool CreateTarget( );

    ID3D11Device* Hardware = nullptr;
    ID3D11DeviceContext* Commands = nullptr;

    IDXGISwapChain1* Swapchain = nullptr;
    ID3D11RenderTargetView* Target = nullptr;

    unsigned int SwapOptions = 0;

    int SurfaceWidth = 0;
    int SurfaceHeight = 0;

    bool Tearing = false;
};

inline auto ElevenHost = std::make_unique< CElevenHost >( );
