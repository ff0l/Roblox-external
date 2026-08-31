#include "ur/image.hpp"

#include "Pictures.h"

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace ur {
namespace image {

struct Entry {
    unsigned long long Handle = 0;
    std::string Key;
};

static CGraphics* Gfx = nullptr;
static std::unordered_map< std::string, unsigned long long > Cache;

void bind( CGraphics* Graphics ) {
    if ( Gfx && Gfx != Graphics ) {
        for ( auto& Item : Cache ) {
            if ( Item.second )
                Gfx->DestroyImage( Item.second );
        }
        Cache.clear( );
    }
    Gfx = Graphics;
}

void sweep( ) {
    if ( Gfx ) {
        for ( auto& Item : Cache ) {
            if ( Item.second )
                Gfx->DestroyImage( Item.second );
        }
    }
    Cache.clear( );
    Gfx = nullptr;
}

unsigned long long svg( const char* Markup, int Size ) {
    if ( !Gfx || !Markup || Size <= 0 )
        return 0;

    std::string Key = "svg:";
    Key += std::to_string( Size );
    Key += ':';
    Key += Markup;

    auto Found = Cache.find( Key );
    if ( Found != Cache.end( ) )
        return Found->second;

    std::vector< unsigned char > Pixels;
    int Width = 0;
    int Height = 0;
    if ( !Pictures->VectorBytes( ( const unsigned char* )Markup, strlen( Markup ), Pixels, Width, Height, Size ) )
        return 0;

    unsigned long long Handle = Gfx->CreateImage( Pixels.data( ), Width, Height );
    Cache[ Key ] = Handle;
    return Handle;
}

unsigned long long file( const char* Path, int Longest ) {
    if ( !Gfx || !Path )
        return 0;

    std::string Key = "file:";
    Key += std::to_string( Longest );
    Key += ':';
    Key += Path;

    auto Found = Cache.find( Key );
    if ( Found != Cache.end( ) )
        return Found->second;

    std::vector< unsigned char > Pixels;
    int Width = 0;
    int Height = 0;
    bool Ok = Longest > 0
        ? Pictures->Load( Path, Pixels, Width, Height, Longest )
        : Pictures->Load( Path, Pixels, Width, Height );
    if ( !Ok )
        return 0;

    unsigned long long Handle = Gfx->CreateImage( Pixels.data( ), Width, Height );
    Cache[ Key ] = Handle;
    return Handle;
}

}
}
