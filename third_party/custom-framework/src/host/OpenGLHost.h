#pragma once

#include <memory>

#include "Hosts.h"

class COpenGLHost : public CHost {
public:
    bool Create( void* Window, int Width, int Height ) override;
    void Destroy( ) override;

    void Resize( int Width, int Height ) override;
    void Begin( CColor Backdrop ) override;

    void* Stream( ) override;
    CGraphics* Graphics( ) override;

    void End( bool VerticalSync ) override;

private:
    void* Handle = nullptr;
    void* Owner = nullptr;
    void* Surface = nullptr;

    void* Setting = nullptr;

    int SurfaceWidth = 0;
    int SurfaceHeight = 0;

    int Interval = -1;
    bool Clipped = false;
};

inline auto OpenGLHost = std::make_unique< COpenGLHost >( );