#pragma once

namespace ur {
namespace overlay {

struct Options {
    bool topmost = false;
    bool click_through = false;
    bool layered = false;
    bool borderless = false;
    bool transparent = false;
    int alpha = 255;
};

void apply( void* window, const Options& options );
void attach( void* window );
void seal( void* window );
bool glass( );

}
}
