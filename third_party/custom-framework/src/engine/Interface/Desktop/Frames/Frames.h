#pragma once

#include <iosfwd>
#include <memory>
#include <vector>
#include <unordered_map>

#include "Geometry.h"

inline constexpr unsigned int FrameMove = 1;
inline constexpr unsigned int FrameResize = 2;
inline constexpr unsigned int FrameCollapse = 4;
inline constexpr unsigned int FrameClose = 8;
inline constexpr unsigned int FrameDock = 16;
inline constexpr unsigned int FrameFit = 32;
inline constexpr unsigned int FramePin = 64;
inline constexpr unsigned int FrameDefault = 31;

class CFrameState {
public:
    CVector Position;
    CVector Size;

    CVector Grab;

    CRectangle Bounds;
    CRectangle Inner;

    char Title[ 64 ] = { };

    unsigned int Options = FrameDefault;
    unsigned int Home = 0;

    unsigned int Group = 0;
    unsigned int Stamp = 0;

    unsigned int Touch = 0;
    unsigned int HomeStamp = 0;

    int Channel = 0;

    float Reveal = 0.0f;
    float Collapse = 0.0f;

    float Scroll = 0.0f;
    float Aim = 0.0f;

    float Reach = 0.0f;
    float View = 0.0f;
    float Ceiling = 0.0f;

    bool DockFront = false;
    bool Collapsed = false;

    bool Dragging = false;
    bool Ready = false;
    bool Loose = false;
};

class CFrames {
public:
    void Update( );

    bool Begin( const char* Title, bool* Open = nullptr, unsigned int Options = FrameDefault, CVector Anchor = CVector( 60.0f, 60.0f ), CVector Extent = CVector( 380.0f, 320.0f ) );
    void End( );

    void Destroy( );
    void Raise( unsigned int Identifier );

    unsigned int Topmost( CVector Point ) const;
    CFrameState* Find( unsigned int Identifier );

    const std::vector< unsigned int >& Order( ) const;

    void Save( std::ostream& Stream ) const;
    void Load( std::istream& Stream );

    unsigned int Current = 0;
    unsigned int Beneath = 0;

private:
    bool Tap( unsigned int Identifier, CRectangle Bounds, bool& Hovered );

    void Resize( unsigned int Identifier, CFrameState& State );
    void Content( unsigned int Identifier, CFrameState& State, CRectangle Region, float Tall );

    std::unordered_map< unsigned int, CFrameState > States;

    std::vector< unsigned int > Stack;
    std::vector< int > Spares;

    int NextChannel = 1;
    bool Producing = false;
};

inline auto Frames = std::make_unique< CFrames >( );