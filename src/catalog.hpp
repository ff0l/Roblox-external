#pragma once

#include "ice.hpp"

#include "Shaders.h"

namespace skin {

inline constexpr int ToneCount = 3;
inline constexpr int LookCount = 6;

inline int& tone( ) {
    static int Selected = 0;
    return Selected;
}

inline int& look( ) {
    static int Selected = 0;
    return Selected;
}

inline const char* toneName( int Index ) {
    static const char* Names[ ToneCount ] = { "Dark Knight", "Coffee", "Matcha" };
    if ( Index < 0 || Index >= ToneCount )
        Index = 0;
    return Names[ Index ];
}

inline const char* lookName( int Index ) {
    static const char* Names[ LookCount ] = {
        "Ice", "Thunder", "Ether", "Snow", "Bends", "Clouds"
    };
    if ( Index < 0 || Index >= LookCount )
        Index = 0;
    return Names[ Index ];
}

inline const float* deep( int Index ) {
    static const float Set[ ToneCount ][ 3 ] = {
        { 0.018f, 0.024f, 0.038f },
        { 0.055f, 0.028f, 0.016f },
        { 0.020f, 0.038f, 0.018f }
    };
    if ( Index < 0 || Index >= ToneCount )
        Index = 0;
    return Set[ Index ];
}

inline const float* mid( int Index ) {
    static const float Set[ ToneCount ][ 3 ] = {
        { 0.07f, 0.11f, 0.17f },
        { 0.22f, 0.12f, 0.06f },
        { 0.08f, 0.16f, 0.07f }
    };
    if ( Index < 0 || Index >= ToneCount )
        Index = 0;
    return Set[ Index ];
}

inline const float* high( int Index ) {
    static const float Set[ ToneCount ][ 3 ] = {
        { 0.26f, 0.36f, 0.48f },
        { 0.55f, 0.38f, 0.20f },
        { 0.28f, 0.48f, 0.26f }
    };
    if ( Index < 0 || Index >= ToneCount )
        Index = 0;
    return Set[ Index ];
}

inline const char* Atmosphere( int Index ) {
    static const char* Bodies[ LookCount ] = {
        nullptr,
        R"(
Float2 Uv = Screen * Float2( 0.00115, 0.00115 );
Uv = Uv * 2.0 - 1.0;
Uv.x *= 1.55;
float Hash = Fract( sin( Moment * 1.65 ) * 43758.5453 );
Float2 P = Uv;
float Amp = 0.5;
float N = 0.0;
for ( int i = 0; i < 10; i++ )
{
    Float2 Ip = floor( P );
    Float2 Fp = Fract( P );
    float A = Fract( sin( dot( Ip, Float2( 12.9898, 78.233 ) ) ) * 43758.5453 );
    float B = Fract( sin( dot( Ip + Float2( 1.0, 0.0 ), Float2( 12.9898, 78.233 ) ) ) * 43758.5453 );
    float C = Fract( sin( dot( Ip + Float2( 0.0, 1.0 ), Float2( 12.9898, 78.233 ) ) ) * 43758.5453 );
    float D = Fract( sin( dot( Ip + Float2( 1.0, 1.0 ), Float2( 12.9898, 78.233 ) ) ) * 43758.5453 );
    Float2 T = Fp * Fp * ( 3.0 - 2.0 * Fp );
    N += Amp * Lerp( Lerp( A, B, T.x ), Lerp( C, D, T.x ), T.y );
    float Cs = cos( 0.45 );
    float Sn = sin( 0.45 );
    P = Float2( P.x * Cs - P.y * Sn, P.x * Sn + P.y * Cs ) * 2.0;
    Amp *= 0.5;
}
float Bolt = 0.0;
for ( int b = 0; b < 5; b++ )
{
    float Shift = -0.72 + float( b ) * 0.36;
    float Twist = ( 2.0 * N - 1.0 ) * ( 0.28 + 0.06 * float( b ) );
    float Dist = abs( Uv.x + Shift + Twist + 0.04 * sin( Moment * ( 0.55 + float( b ) * 0.17 ) ) );
    float Pulse = Fract( sin( Hash * ( 12.1 + float( b ) * 7.3 ) ) * 43758.5453 );
    Bolt += pow( ( 0.028 + 0.012 * Pulse ) / max( Dist, 0.0016 ), 1.0 );
}
Float3 Col = Float3( 0.70, 0.84, 1.0 ) * Saturate( Bolt * 0.55 );
Final.rgb = Saturate( Col + Float3( 0.03, 0.04, 0.07 ) );
)",
        R"(
Float2 Uv = Screen * Float2( 0.00115, 0.00115 );
Float2 Q = Uv * 2.4;
float n = 0.0;
float Amp = 0.55;
for ( int i = 0; i < 5; i++ )
{
    n += Amp * sin( Q.x + Moment * 0.4 ) * sin( Q.y - Moment * 0.33 );
    Q = Float2( Q.x * 1.7 - Q.y * 0.4, Q.x * 0.4 + Q.y * 1.7 );
    Amp *= 0.55;
}
Float3 Tone = Float3( 0.12 + n * 0.25, 0.28 + n * 0.35, 0.42 + n * 0.28 );
Final.rgb = Saturate( Tone );
)",
        R"(
Float2 Uv = Screen * Float2( 0.00115, 0.00115 );
Float3 Color = Float3( 0.035, 0.04, 0.055 );
for ( int Layer = 0; Layer < 3; Layer++ )
{
    float Depth = 8.0 + float( Layer ) * 9.0;
    float Drift = Moment * ( 0.08 + float( Layer ) * 0.05 );
    Float2 Q = Uv * Depth;
    Q.x += sin( Moment * 0.3 + float( Layer ) ) * 0.35;
    Q.y -= Drift;
    Float2 Cell = floor( Q );
    Float2 Fr = Fract( Q );
    float N = Fract( sin( dot( Cell, Float2( 12.9898, 78.233 ) ) ) * 43758.5453 );
    float Keep = step( 0.82 + float( Layer ) * 0.04, N );
    Float2 Center = Float2( 0.35 + 0.3 * Fract( N * 13.0 ), 0.35 + 0.3 * Fract( N * 7.0 ) );
    float Dist = length( Fr - Center );
    float Soft = Saturate( 1.0 - Dist * ( 14.0 - float( Layer ) * 2.0 ) );
    float Flake = pow( Soft, 1.6 ) * Keep;
    Color += Float3( 0.86, 0.90, 1.0 ) * Flake * ( 0.35 + 0.25 * float( Layer ) );
}
Final.rgb = Saturate( Color );
)",
        R"(
Float2 Uv = Screen * Float2( 0.00115, 0.00115 );
Float2 P = Uv;
for ( int i = 0; i < 4; i++ )
{
    P.x += 0.22 * sin( P.y * 3.4 + Moment * 0.6 + float( i ) );
    P.y += 0.22 * cos( P.x * 3.1 - Moment * 0.5 + float( i ) * 0.7 );
}
float Mix = 0.5 + 0.5 * sin( P.x * 2.0 + P.y * 2.4 );
Final.rgb = Lerp( Float3( 0.12, 0.18, 0.38 ), Float3( 0.72, 0.32, 0.48 ), Mix );
)",
        R"(
Float2 Uv = Screen * Float2( 0.00085, 0.00125 );
float T = Moment * 0.035;
float N = 0.0;
float Amp = 0.52;
Float2 P = Uv + Float2( T * 0.18, T * 0.05 );
for ( int i = 0; i < 5; i++ )
{
    Float2 Ip = floor( P );
    Float2 Fp = Fract( P );
    float A = Fract( sin( dot( Ip, Float2( 27.1, 61.7 ) ) ) * 43758.5453 );
    float B = Fract( sin( dot( Ip + Float2( 1.0, 0.0 ), Float2( 27.1, 61.7 ) ) ) * 43758.5453 );
    float C = Fract( sin( dot( Ip + Float2( 0.0, 1.0 ), Float2( 27.1, 61.7 ) ) ) * 43758.5453 );
    float D = Fract( sin( dot( Ip + Float2( 1.0, 1.0 ), Float2( 27.1, 61.7 ) ) ) * 43758.5453 );
    Float2 S = Fp * Fp * ( 3.0 - 2.0 * Fp );
    N += Amp * Lerp( Lerp( A, B, S.x ), Lerp( C, D, S.x ), S.y );
    P = P * 2.08 + Float2( 9.0, 4.0 );
    Amp *= 0.5;
}
float Soft = pow( Saturate( N * 1.2 ), 1.55 );
Final.rgb = Saturate( Lerp( Float3( 0.08, 0.11, 0.16 ), Float3( 0.78, 0.84, 0.92 ), Soft ) );
)"
    };
    if ( Index < 1 || Index >= LookCount )
        return Bodies[ 1 ];
    return Bodies[ Index ];
}

inline const char* Tinted( const char* Source, int Tone ) {
    static char Body[ 8192 ] = { };
    const float* Deep = deep( Tone );
    const float* Mid = mid( Tone );
    const float* High = high( Tone );
    snprintf( Body, sizeof( Body ),
        "%s\n"
        "float Lum = dot( Final.rgb, Float3( 0.30, 0.54, 0.16 ) );\n"
        "Float3 Deep = Float3( %.4f, %.4f, %.4f );\n"
        "Float3 Mid = Float3( %.4f, %.4f, %.4f );\n"
        "Float3 High = Float3( %.4f, %.4f, %.4f );\n"
        "Float3 Tint = Lerp( Deep, Mid, Saturate( Lum * 1.55 ) );\n"
        "Final.rgb = Saturate( Lerp( Tint, High, pow( Saturate( Lum ), 2.2 ) ) );\n",
        Source,
        ( double )Deep[ 0 ], ( double )Deep[ 1 ], ( double )Deep[ 2 ],
        ( double )Mid[ 0 ], ( double )Mid[ 1 ], ( double )Mid[ 2 ],
        ( double )High[ 0 ], ( double )High[ 1 ], ( double )High[ 2 ] );
    return Body;
}

inline unsigned int effect( ) {
    static unsigned int Handles[ LookCount ][ ToneCount ] = { };
    int Look = look( );
    int Tone = tone( );
    if ( Look < 0 || Look >= LookCount )
        Look = 0;
    if ( Tone < 0 || Tone >= ToneCount )
        Tone = 0;
    if ( Handles[ Look ][ Tone ] == 0 ) {
        const float* Deep = deep( Tone );
        const float* Mid = mid( Tone );
        const float* High = high( Tone );
        const char* Body = Look == 0
            ? IceMetal( Deep[ 0 ], Deep[ 1 ], Deep[ 2 ], Mid[ 0 ], Mid[ 1 ], Mid[ 2 ], High[ 0 ], High[ 1 ], High[ 2 ] )
            : Tinted( Atmosphere( Look ), Tone );
        Handles[ Look ][ Tone ] = Shaders->Compose( lookName( Look ), Body );
    }
    return Handles[ Look ][ Tone ];
}

}
