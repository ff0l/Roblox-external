#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "Canvas.h"

class COpenImage {
public:
    unsigned int Texture = 0;

    int Width = 0;
    int Height = 0;

    bool Owned = false;
};

class COpenSlots {
public:
    unsigned int Program = 0;

    int Projection = -1;
    int Origin = -1;

    int Moment = -1;
};

class COpenGL : public CGraphics {
public:
    ~COpenGL( );

    bool Create( );
    void Destroy( ) override;

    void Render( const CDrawData& Data, void* Stream ) override;

    unsigned long long CreateImage( const unsigned char* Pixels, int Width, int Height ) override;
    void UpdateImage( unsigned long long Image, const unsigned char* Pixels ) override;

    unsigned long long AdoptImage( void* Native ) override;
    void DestroyImage( unsigned long long Image ) override;

private:
    void Refresh( );

    const COpenSlots* Program( unsigned int Effect );
    void Apply( const CDrawData& Data, const COpenSlots& Slots, unsigned int Offset );

    std::vector< COpenSlots > Programs;

    unsigned int Vertices = 0;
    unsigned int Storage = 0;

    unsigned int SheetTexture = 0;
    unsigned int SheetVersion = 0;

    int SheetDepth = 0;
    int Capacity = 0;

    std::unordered_map< unsigned long long, COpenImage > Images;
    unsigned long long NextImage = 1;
};

inline auto OpenGL = std::make_unique< COpenGL >( );