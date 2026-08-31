#pragma once

#include <memory>
#include <vector>

#include "Geometry.h"

class CFont;

inline constexpr int OverlayRoute = 200;

class CInstance {
public:
    CRectangle Bounds;

    float ClipLeft = 0.0f;
    float ClipTop = 0.0f;

    float ClipRight = 0.0f;
    float ClipBottom = 0.0f;

    CRectangle Source;

    unsigned int ColorStart = 0;
    unsigned int ColorFinish = 0;

    float Rounding = 0.0f;
    float Thickness = 0.0f;

    unsigned int Flags = 0;
    float Softness = 0.75f;

    float Inflate = 0.0f;
    float Spare = 0.0f;
};

class CBatch {
public:
    int Offset = 0;
    int Count = 0;

    unsigned long long Image = 0;
    unsigned int Effect = 0;

    void ( *Callback )( void* Stream, void* Detail ) = nullptr;
    void* Detail = nullptr;
};

class CChannel {
public:
    std::vector< CInstance > Items;
    std::vector< CBatch > Marks;
};

class CDrawData {
public:
    const CInstance* Items = nullptr;
    int Count = 0;

    const CBatch* Batches = nullptr;
    int BatchCount = 0;

    CVector Extent;
    float Moment = 0.0f;
    CVector Mouse;
    float PressLeft = 0.0f;
    float PressRight = 0.0f;
};

class CGraphics {
public:
    virtual ~CGraphics( ) = default;

    virtual void Destroy( ) = 0;
    virtual void Render( const CDrawData& Data, void* Stream ) = 0;

    virtual unsigned long long CreateImage( const unsigned char* Pixels, int Width, int Height ) = 0;
    virtual void UpdateImage( unsigned long long Image, const unsigned char* Pixels ) = 0;

    virtual unsigned long long AdoptImage( void* Native ) = 0;
    virtual void DestroyImage( unsigned long long Image ) = 0;
};

class CCanvas {
public:
    void Begin( CVector Display );

    int Route( int Channel );
    unsigned int Effect( unsigned int Wanted );

    void PushClip( CRectangle Bounds, bool Absolute = false );
    void PopClip( );

    bool Visible( CVector Point ) const;

    void Rectangle( CRectangle Bounds, CColor Fill, float Rounding );
    void Gradient( CRectangle Bounds, CColor First, CColor Second, float Rounding, bool Horizontal );

    void Border( CRectangle Bounds, CColor Stroke, float Rounding, float Thickness );
    void Shadow( CRectangle Bounds, CColor Tint, float Rounding, float Softness );

    void Circle( CVector Middle, float Radius, CColor Fill );
    void Wedge( CRectangle Bounds, CColor Fill, int Turn );

    void Sector( CVector Middle, float Radius, float Start, float Sweep, float Inner, CColor Fill );

    void Polygon( const CVector* Points, int Count, CColor Fill );
    void PolygonBorder( const CVector* Points, int Count, CColor Tint, float Thickness );

    void Stroke( CRectangle Bounds, CVector From, CVector Till, CColor Tint, float Thickness );
    void Line( CVector From, CVector Till, CColor Tint, float Thickness );

    void Text( CVector Anchor, CColor Tint, const char* Message );
    void Write( CFont* Face, CVector Anchor, CColor Tint, const char* Message );

    void Outlined( CVector Anchor, CColor Tint, CColor Border, float Thickness, const char* Message );
    void WriteOutlined( CFont* Face, CVector Anchor, CColor Tint, CColor Border, float Thickness, const char* Message );

    void Image( CRectangle Bounds, unsigned long long Picture, CRectangle Source, CColor Tint, float Rounding = 0.0f );
    void Custom( CRectangle Bounds, void ( *Callback )( void* Stream, void* Detail ), void* Detail );

    void Arrange( const int* Order, int Count );
    const CDrawData& Data( ) const;

    float Opacity = 1.0f;

    int LastCount = 0;
    int LastBatches = 0;

private:
    CInstance* Emit( CRectangle Bounds, unsigned long long Picture );

    std::vector< CChannel > Channels;
    std::vector< CInstance > Combined;

    std::vector< CBatch > Joined;
    std::vector< CRectangle > Clips;

    CDrawData Product;
    CVector Screen;

    int Active = 0;
    unsigned int Shader = 0;
};

inline auto Canvas = std::make_unique< CCanvas >( );