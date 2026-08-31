#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "Geometry.h"

class CGlyph {
public:
    CRectangle Source;

    CVector Offset;
    CVector Span;

    float Advance = 0.0f;
};

class CFont {
public:
    ~CFont( );

    bool Create( const char* const* Names, int Count, float Height, int Boldness = 400 );
    void Destroy( );

    void Rescale( float Factor );

    const CGlyph* Fetch( unsigned int Codepoint );
    CVector Measure( const char* Message );

    CRectangle Bounds( const char* Message );

    float Span( const char* Message, int Bytes );
    int Hit( const char* Message, float Across );

    float LineSpan = 0.0f;
    float Ascent = 0.0f;

    float Leading = 0.0f;

private:
    bool Prepare( );
    void Release( );

    bool Rasterize( unsigned int Codepoint, CGlyph& Glyph );

    std::vector< std::string > Families;
    std::vector< void* > Handles;

    std::unordered_map< unsigned int, CGlyph > Glyphs;
    const CGlyph* Quick[ 128 ] = { };

    void* Surface = nullptr;

    float BaseHeight = 0.0f;
    float Scale = 1.0f;

    int Weight = 400;
};

inline auto Font = std::make_unique< CFont >( );
inline auto Heading = std::make_unique< CFont >( );