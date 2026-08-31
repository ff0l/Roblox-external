#include "ur/orbit.hpp"
#include "ur/hear.hpp"

#include "Canvas.h"
#include "Context.h"
#include "Font.h"
#include "Input.h"
#include "Layout.h"
#include "Style.h"
#include "Widgets.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ur {
namespace orbit {

struct Vec3 {
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
};

struct Face {
    Vec3 A;
    Vec3 B;
    Vec3 C;
    Vec3 Normal;
    float Depth = 0.0f;
    float Light = 0.0f;
    float Rim = 0.0f;
    float Spec = 0.0f;
};

static Vec3 Add( Vec3 A, Vec3 B ) {
    return { A.X + B.X, A.Y + B.Y, A.Z + B.Z };
}

static Vec3 Sub( Vec3 A, Vec3 B ) {
    return { A.X - B.X, A.Y - B.Y, A.Z - B.Z };
}

static Vec3 Mul( Vec3 A, float S ) {
    return { A.X * S, A.Y * S, A.Z * S };
}

static float Dot( Vec3 A, Vec3 B ) {
    return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
}

static Vec3 Cross( Vec3 A, Vec3 B ) {
    return { A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X };
}

static Vec3 Unit( Vec3 A ) {
    float Length = sqrtf( Dot( A, A ) );
    if ( Length < 0.0001f )
        return A;
    return Mul( A, 1.0f / Length );
}

static Vec3 Spin( Vec3 A, float Yaw, float Pitch ) {
    float Cy = cosf( Yaw );
    float Sy = sinf( Yaw );
    float Cp = cosf( Pitch );
    float Sp = sinf( Pitch );
    float X = A.X * Cy + A.Z * Sy;
    float Z = -A.X * Sy + A.Z * Cy;
    float Y = A.Y * Cp - Z * Sp;
    float Z2 = A.Y * Sp + Z * Cp;
    return { X, Y, Z2 };
}

static int Shape = 1;
static float Yaw = 0.55f;
static float Pitch = 0.28f;
static float Zoom = 3.05f;
static bool Auto = true;
static bool Lines = false;
static CVector LastMouse;
static bool Drag = false;

static int Built = -1;
static std::vector< Vec3 > Source;
static std::vector< Vec3 > Normals;

static void Push( std::vector< Vec3 >& Out, Vec3 A, Vec3 B, Vec3 C ) {
    Out.push_back( A );
    Out.push_back( B );
    Out.push_back( C );
}

static void Cube( std::vector< Vec3 >& Out ) {
    const float E = 0.78f;
    const float B = 0.18f;
    Vec3 P[ 8 ] = {
        { -E, -E, -E }, { E, -E, -E }, { E, E, -E }, { -E, E, -E },
        { -E, -E, E }, { E, -E, E }, { E, E, E }, { -E, E, E }
    };
    const int F[ 6 ][ 4 ] = {
        { 0, 1, 2, 3 }, { 5, 4, 7, 6 }, { 4, 0, 3, 7 },
        { 1, 5, 6, 2 }, { 4, 5, 1, 0 }, { 3, 2, 6, 7 }
    };
    for ( int f = 0; f < 6; f++ ) {
        Vec3 Ctr = Mul( Add( Add( P[ F[ f ][ 0 ] ], P[ F[ f ][ 1 ] ] ), Add( P[ F[ f ][ 2 ] ], P[ F[ f ][ 3 ] ] ) ), 0.25f );
        Vec3 Q[ 4 ];
        for ( int i = 0; i < 4; i++ )
            Q[ i ] = Add( Mul( P[ F[ f ][ i ] ], 1.0f - B ), Mul( Ctr, B ) );
        Push( Out, Q[ 0 ], Q[ 1 ], Q[ 2 ] );
        Push( Out, Q[ 0 ], Q[ 2 ], Q[ 3 ] );
        for ( int i = 0; i < 4; i++ ) {
            int j = ( i + 1 ) % 4;
            Push( Out, P[ F[ f ][ i ] ], P[ F[ f ][ j ] ], Q[ j ] );
            Push( Out, P[ F[ f ][ i ] ], Q[ j ], Q[ i ] );
        }
    }
}

static void IcosaBase( std::vector< Vec3 >& Faces ) {
    const float T = ( 1.0f + sqrtf( 5.0f ) ) * 0.5f;
    Vec3 Raw[ 12 ] = {
        { -1, T, 0 }, { 1, T, 0 }, { -1, -T, 0 }, { 1, -T, 0 },
        { 0, -1, T }, { 0, 1, T }, { 0, -1, -T }, { 0, 1, -T },
        { T, 0, -1 }, { T, 0, 1 }, { -T, 0, -1 }, { -T, 0, 1 }
    };
    Vec3 V[ 12 ];
    for ( int i = 0; i < 12; i++ )
        V[ i ] = Unit( Raw[ i ] );

    const int I[ 60 ] = {
        0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11,
        1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 7, 1, 8,
        3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9,
        4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1
    };
    for ( int n = 0; n < 60; n++ )
        Faces.push_back( V[ I[ n ] ] );
}

static void Subdivide( std::vector< Vec3 >& Faces, bool Project ) {
    std::vector< Vec3 > Next;
    Next.reserve( Faces.size( ) * 4 );
    for ( size_t i = 0; i + 2 < Faces.size( ); i += 3 ) {
        Vec3 A = Faces[ i ];
        Vec3 B = Faces[ i + 1 ];
        Vec3 C = Faces[ i + 2 ];
        Vec3 AB = Add( A, B );
        Vec3 BC = Add( B, C );
        Vec3 CA = Add( C, A );
        if ( Project ) {
            AB = Unit( AB );
            BC = Unit( BC );
            CA = Unit( CA );
        } else {
            AB = Mul( AB, 0.5f );
            BC = Mul( BC, 0.5f );
            CA = Mul( CA, 0.5f );
        }
        Push( Next, A, AB, CA );
        Push( Next, B, BC, AB );
        Push( Next, C, CA, BC );
        Push( Next, AB, BC, CA );
    }
    Faces.swap( Next );
}

static void Sphere( std::vector< Vec3 >& Out, int Depth ) {
    IcosaBase( Out );
    for ( int i = 0; i < Depth; i++ )
        Subdivide( Out, true );
}

static void Torus( std::vector< Vec3 >& Out, int Rings, int Tubes, float Major, float Minor ) {
    std::vector< Vec3 > Grid( ( size_t )( Rings + 1 ) * ( size_t )( Tubes + 1 ) );
    for ( int i = 0; i <= Rings; i++ ) {
        float Theta = ( float )i * 6.2831853f / ( float )Rings;
        float Ct = cosf( Theta );
        float St = sinf( Theta );
        for ( int j = 0; j <= Tubes; j++ ) {
            float Phi = ( float )j * 6.2831853f / ( float )Tubes;
            float Cp = cosf( Phi );
            float Sp = sinf( Phi );
            Grid[ ( size_t )i * ( size_t )( Tubes + 1 ) + ( size_t )j ] = {
                ( Major + Minor * Cp ) * Ct, Minor * Sp, ( Major + Minor * Cp ) * St
            };
        }
    }
    for ( int i = 0; i < Rings; i++ ) {
        for ( int j = 0; j < Tubes; j++ ) {
            Vec3 A = Grid[ ( size_t )i * ( size_t )( Tubes + 1 ) + ( size_t )j ];
            Vec3 B = Grid[ ( size_t )( i + 1 ) * ( size_t )( Tubes + 1 ) + ( size_t )j ];
            Vec3 C = Grid[ ( size_t )( i + 1 ) * ( size_t )( Tubes + 1 ) + ( size_t )( j + 1 ) ];
            Vec3 D = Grid[ ( size_t )i * ( size_t )( Tubes + 1 ) + ( size_t )( j + 1 ) ];
            Push( Out, A, B, C );
            Push( Out, A, C, D );
        }
    }
}

static void Gem( std::vector< Vec3 >& Out ) {
    const int Sides = 8;
    Vec3 Top{ 0.0f, 1.05f, 0.0f };
    Vec3 Bot{ 0.0f, -1.05f, 0.0f };
    Vec3 Crown[ 8 ];
    Vec3 Girdle[ 8 ];
    for ( int i = 0; i < Sides; i++ ) {
        float A = ( float )i * 6.2831853f / ( float )Sides;
        Crown[ i ] = { cosf( A ) * 0.42f, 0.38f, sinf( A ) * 0.42f };
        Girdle[ i ] = { cosf( A ) * 0.82f, 0.0f, sinf( A ) * 0.82f };
    }
    for ( int i = 0; i < Sides; i++ ) {
        int j = ( i + 1 ) % Sides;
        Push( Out, Top, Crown[ i ], Crown[ j ] );
        Push( Out, Crown[ i ], Girdle[ i ], Girdle[ j ] );
        Push( Out, Crown[ i ], Girdle[ j ], Crown[ j ] );
        Push( Out, Girdle[ i ], Bot, Girdle[ j ] );
    }
}

static void Star( std::vector< Vec3 >& Out ) {
    std::vector< Vec3 > Base;
    IcosaBase( Base );
    for ( size_t i = 0; i + 2 < Base.size( ); i += 3 ) {
        Vec3 A = Base[ i ];
        Vec3 B = Base[ i + 1 ];
        Vec3 C = Base[ i + 2 ];
        Vec3 Apex = Mul( Unit( Add( Add( A, B ), C ) ), 1.55f );
        Push( Out, A, B, Apex );
        Push( Out, B, C, Apex );
        Push( Out, C, A, Apex );
    }
}

static Vec3 KnotPoint( float T ) {
    float Q = 3.0f;
    float P = 2.0f;
    float R = 0.38f;
    float Tube = 0.78f + R * cosf( Q * T );
    return { Tube * cosf( P * T ), R * 1.15f * sinf( Q * T ), Tube * sinf( P * T ) };
}

static void Knot( std::vector< Vec3 >& Out ) {
    const int Steps = 80;
    const int Ring = 8;
    const float Radius = 0.16f;
    std::vector< Vec3 > Tube( ( size_t )( Steps + 1 ) * ( size_t )( Ring + 1 ) );

    for ( int i = 0; i <= Steps; i++ ) {
        float T = ( float )i * 6.2831853f / ( float )Steps;
        float T2 = T + 0.04f;
        Vec3 P = KnotPoint( T );
        Vec3 Tan = Unit( Sub( KnotPoint( T2 ), P ) );
        Vec3 N = Unit( Cross( Tan, { 0.0f, 1.0f, 0.0f } ) );
        if ( Dot( N, N ) < 0.2f )
            N = Unit( Cross( Tan, { 1.0f, 0.0f, 0.0f } ) );
        Vec3 B = Unit( Cross( Tan, N ) );
        for ( int j = 0; j <= Ring; j++ ) {
            float A = ( float )j * 6.2831853f / ( float )Ring;
            Vec3 Offset = Add( Mul( N, cosf( A ) * Radius ), Mul( B, sinf( A ) * Radius ) );
            Tube[ ( size_t )i * ( size_t )( Ring + 1 ) + ( size_t )j ] = Add( P, Offset );
        }
    }

    for ( int i = 0; i < Steps; i++ ) {
        for ( int j = 0; j < Ring; j++ ) {
            Vec3 A = Tube[ ( size_t )i * ( size_t )( Ring + 1 ) + ( size_t )j ];
            Vec3 B = Tube[ ( size_t )( i + 1 ) * ( size_t )( Ring + 1 ) + ( size_t )j ];
            Vec3 C = Tube[ ( size_t )( i + 1 ) * ( size_t )( Ring + 1 ) + ( size_t )( j + 1 ) ];
            Vec3 D = Tube[ ( size_t )i * ( size_t )( Ring + 1 ) + ( size_t )( j + 1 ) ];
            Push( Out, A, B, C );
            Push( Out, A, C, D );
        }
    }
}

static void Spike( std::vector< Vec3 >& Out ) {
    std::vector< Vec3 > Ball;
    Sphere( Ball, 2 );
    for ( size_t i = 0; i + 2 < Ball.size( ); i += 3 ) {
        Vec3 A = Ball[ i ];
        Vec3 B = Ball[ i + 1 ];
        Vec3 C = Ball[ i + 2 ];
        float Ha = 0.72f + 0.38f * fabsf( A.Y );
        float Hb = 0.72f + 0.38f * fabsf( B.Y );
        float Hc = 0.72f + 0.38f * fabsf( C.Y );
        Push( Out, Mul( A, Ha ), Mul( B, Hb ), Mul( C, Hc ) );
    }
}

static void Build( int Kind ) {
    if ( Built == Kind && !Source.empty( ) )
        return;

    Source.clear( );
    if ( Kind == 0 )
        Cube( Source );
    else if ( Kind == 2 )
        Torus( Source, 28, 14, 0.72f, 0.28f );
    else if ( Kind == 3 )
        Gem( Source );
    else if ( Kind == 4 )
        Star( Source );
    else if ( Kind == 5 )
        Knot( Source );
    else if ( Kind == 6 )
        Spike( Source );
    else
        Sphere( Source, 2 );

    Normals.assign( Source.size( ), Vec3{ } );
    for ( size_t i = 0; i + 2 < Source.size( ); i += 3 ) {
        Vec3 N = Unit( Cross( Sub( Source[ i + 1 ], Source[ i ] ), Sub( Source[ i + 2 ], Source[ i ] ) ) );
        Normals[ i ] = N;
        Normals[ i + 1 ] = N;
        Normals[ i + 2 ] = N;
    }

    if ( Kind == 1 || Kind == 2 || Kind == 5 || Kind == 6 ) {
        for ( size_t i = 0; i < Source.size( ); i++ )
            Normals[ i ] = Unit( Source[ i ] );
        if ( Kind == 2 || Kind == 5 ) {
            for ( size_t i = 0; i + 2 < Source.size( ); i += 3 ) {
                Vec3 N = Unit( Cross( Sub( Source[ i + 1 ], Source[ i ] ), Sub( Source[ i + 2 ], Source[ i ] ) ) );
                Normals[ i ] = N;
                Normals[ i + 1 ] = N;
                Normals[ i + 2 ] = N;
            }
        }
    }

    Built = Kind;
}

void draw( const Options& Wanted ) {
    int& MeshShape = Wanted.shape ? *Wanted.shape : Shape;
    bool& Spinning = Wanted.spin ? *Wanted.spin : Auto;
    bool& Wired = Wanted.wire ? *Wanted.wire : Lines;

    const char* Names[ 7 ] = { "Cube", "Orb", "Ring", "Gem", "Star", "Knot", "Spike" };
    if ( MeshShape < 0 || MeshShape > 6 )
        MeshShape = 1;
    Widgets->Choice( "Mesh", MeshShape, Names, 7 );
    Widgets->Check( "Spin", Spinning );
    Layout->SameLine( );
    Widgets->Check( "Edges", Wired );

    float ViewH = Wanted.height > 0.0f ? Wanted.height : ( Layout->Remaining( ) - Font->LineSpan - Style->Spacing * 2.0f );
    if ( ViewH < 160.0f * Style->Scale )
        ViewH = 240.0f * Style->Scale;
    CRectangle View = Layout->Place( CVector( Layout->Width( ), ViewH ) );
    Canvas->PushClip( View );
    Canvas->Gradient( View, Style->Backdrop.Blend( Style->Accent, 0.08f ), Style->Backdrop, 10.0f * Style->Scale, false );

    CVector Point = Input->MousePosition;
    bool Over = View.Contains( Point ) && Canvas->Visible( Point );
    if ( Over && Input->MousePressed( 0 ) ) {
        Drag = true;
        LastMouse = Point;
        Spinning = false;
    }
    if ( Drag && Input->MouseDown( 0 ) ) {
        Yaw += ( Point.Horizontal - LastMouse.Horizontal ) * 0.012f;
        Pitch += ( Point.Vertical - LastMouse.Vertical ) * 0.012f;
        if ( Pitch < -1.15f )
            Pitch = -1.15f;
        if ( Pitch > 1.15f )
            Pitch = 1.15f;
        LastMouse = Point;
    }
    if ( !Input->MouseDown( 0 ) )
        Drag = false;
    if ( Over && fabsf( Input->WheelDelta ) > 0.0f ) {
        Zoom -= Input->WheelDelta * 0.18f;
        if ( Zoom < 2.0f )
            Zoom = 2.0f;
        if ( Zoom > 5.4f )
            Zoom = 5.4f;
    }

    if ( Spinning )
        Yaw += Context->DeltaTime * ( 0.42f + hear::bass( ) * 1.6f );

    Build( MeshShape );

    float Pulse = 1.0f + hear::bass( ) * 0.06f;
    Vec3 Key = Unit( { -0.45f, 0.82f, -0.55f } );
    Vec3 FillL = Unit( { 0.55f, 0.15f, -0.25f } );
    Vec3 ViewDir{ 0.0f, 0.0f, -1.0f };

    std::vector< Face > Faces;
    Faces.reserve( Source.size( ) / 3 );
    for ( size_t i = 0; i + 2 < Source.size( ); i += 3 ) {
        Face Next;
        Next.A = Spin( Mul( Source[ i ], Pulse ), Yaw, Pitch );
        Next.B = Spin( Mul( Source[ i + 1 ], Pulse ), Yaw, Pitch );
        Next.C = Spin( Mul( Source[ i + 2 ], Pulse ), Yaw, Pitch );
        Vec3 Mid = Mul( Add( Add( Next.A, Next.B ), Next.C ), 1.0f / 3.0f );
        Next.Depth = Mid.Z;
        Next.Normal = Unit( Spin( Normals[ i ], Yaw, Pitch ) );
        if ( Dot( Next.Normal, ViewDir ) <= 0.02f )
            continue;

        float Diffuse = fmaxf( 0.0f, Dot( Next.Normal, Key ) );
        float Fill = fmaxf( 0.0f, Dot( Next.Normal, FillL ) ) * 0.28f;
        Vec3 Half = Unit( Add( Key, ViewDir ) );
        float Spec = powf( fmaxf( 0.0f, Dot( Next.Normal, Half ) ), 28.0f );
        float Rim = powf( fmaxf( 0.0f, 1.0f + Dot( Next.Normal, ViewDir ) ), 2.2f );
        float Ambient = 0.14f;
        Next.Light = Ambient + Diffuse * 0.78f + Fill;
        Next.Rim = Rim * 0.45f;
        Next.Spec = Spec * 0.55f;
        Faces.push_back( Next );
    }

    std::sort( Faces.begin( ), Faces.end( ), [ ]( const Face& A, const Face& B ) {
        return A.Depth > B.Depth;
    } );

    CVector Center = View.Center( );
    float Span = ( View.Width < View.Height ? View.Width : View.Height ) * 0.40f;
    auto Project = [ & ]( Vec3 V ) {
        float Depth = V.Z + Zoom;
        if ( Depth < 0.35f )
            Depth = 0.35f;
        float ScaleP = Span / Depth;
        return CVector( Center.Horizontal + V.X * ScaleP, Center.Vertical - V.Y * ScaleP );
    };

    CRectangle Shadow( Center.Horizontal - Span * 0.42f, Center.Vertical + Span * 0.62f, Span * 0.84f, Span * 0.16f );
    Canvas->Rectangle( Shadow, Style->Shade.Fade( 0.35f ), Shadow.Height * 0.5f );

    for ( const Face& Item : Faces ) {
        CVector Poly[ 3 ] = { Project( Item.A ), Project( Item.B ), Project( Item.C ) };
        float Fog = 1.0f - ( Item.Depth + 1.2f ) * 0.12f;
        if ( Fog < 0.55f )
            Fog = 0.55f;
        if ( Fog > 1.0f )
            Fog = 1.0f;

        CColor Dark = Style->Elevated.Blend( CColor( 8, 8, 12 ), 0.45f );
        CColor Lit = Style->Accent.Blend( CColor( 255, 255, 255 ), 0.18f );
        CColor Fill = Dark.Blend( Lit, Item.Light ).Blend( Style->AccentSoft, Item.Rim );
        Fill = Fill.Blend( CColor( 255, 255, 255 ), Item.Spec );
        Fill = Fill.Fade( Fog );
        Canvas->Polygon( Poly, 3, Fill );
        if ( Wired )
            Canvas->PolygonBorder( Poly, 3, Style->Text.Fade( 0.22f ), 1.0f );
    }

    Canvas->PopClip( );
    Widgets->Faint( "Drag to orbit  ·  scroll to zoom" );
}

}
}
