#pragma once

#include <functional>

#include "Engine.h"
#include "ur/overlay.hpp"

namespace ur {

enum class Backend {
    Auto = -1,
    DX11 = 0,
    DX12 = 1,
    OpenGL = 2,
    Vulkan = 3
};

namespace app {

struct Config {
    const char* title = "UR";
    int width = 1280;
    int height = 720;
    Backend backend = Backend::Auto;
    bool vsync = true;
    bool docking = false;
    bool persist = true;
    const char* layout = "ur.layout";
    const char* settings = "ur.settings";
    bool media = false;
    bool hear = false;
    bool discord = false;
    bool overlay = false;
    const char* const* fonts = nullptr;
    int font_count = 0;
    float font_size = 16.0f;
};

int run( const Config& config, std::function< void( ) > tick );

void quit( );
void* window( );
int width( );
int height( );
CGraphics* graphics( );
Backend backend( );
void set_backend( Backend backend );
bool vsync( );
void set_vsync( bool on );
void on_graphics( void ( *fn )( CGraphics* ) );
void save_layout( );
void load_layout( );
const Config& config( );
overlay::Options& overlay_options( );

}

}
