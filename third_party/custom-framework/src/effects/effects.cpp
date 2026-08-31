#include "ur/effects.hpp"
#include "ur/hear.hpp"

#include "Canvas.h"
#include "Context.h"
#include "Input.h"
#include "Layout.h"
#include "Shaders.h"
#include "Style.h"
#include "Widgets.h"

#include <cmath>
#include <vector>

namespace ur {
namespace effects {

static Quality Level = Quality::Low;
static int Background = 1;
static CGraphics* Gfx = nullptr;
static unsigned long long Plate = 0;
static unsigned int Effects[ 12 ] = { };
static bool Ready = false;

struct Speck {
    float X = 0.0f;
    float Y = 0.0f;
    float Vx = 0.0f;
    float Vy = 0.0f;
    float Size = 1.5f;
    float Life = 1.0f;
    float Hue = 0.0f;
};

static std::vector< Speck > Cloud;

static const char* Names[ ] = {
    "None",
    "Plasma",
    "Light Rays",
    "Galaxy",
    "Lightning",
    "Line Waves",
    "Liquid Chrome",
    "Ether",
    "Pixel Snow",
    "Prism",
    "Color Bends",
    "Prismatic Burst"
};

static const char* Bodies[ ] = {
    nullptr,
    R"(
Float2 Uv = Local / max( Extent, Float2( 1.0, 1.0 ) );
float T = Moment * 0.45;
float A = sin( Uv.x * 7.0 + T ) + sin( Uv.y * 6.2 - T * 1.15 );
float B = sin( ( Uv.x + Uv.y ) * 4.4 + T * 0.8 );
float Wave = A + B;
Float3 Tone = Float3( 0.16 + 0.28 * sin( Wave ), 0.14 + 0.22 * sin( Wave + 1.9 ), 0.32 + 0.30 * sin( Wave + 3.4 ) );
Final.rgb = Tone;
)",
    R"(
Float2 Uv = Local / max( Extent, Float2( 1.0, 1.0 ) ) + Float2( 0.5, 0.5 );
Float2 Origin = Float2( 0.5, 0.12 );
Float2 Dir = normalize( Uv - Origin );
float Spread = pow( Saturate( Dir.y ), 1.35 );
float Dist = length( Uv - Origin );
float Pulse = 0.75 + 0.25 * sin( Moment * 2.2 + Dist * 18.0 );
float Shaft = Spread * Saturate( 1.0 - Dist * 1.15 ) * Pulse;
float Noise = Fract( sin( dot( Uv, Float2( 12.9898, 78.233 ) ) ) * 43758.5453 );
Shaft *= 0.85 + 0.15 * Noise;
Final.rgb = Float3( 0.95, 0.86, 0.62 ) * Shaft + Float3( 0.03, 0.035, 0.06 );
)",
    R"(
Float2 Uv = Local / max( Extent.y, 1.0 );
float T = Moment * 0.04;
Float2 P = Uv;
float Arm = P.x * 3.1 + P.y * 2.2 + length( P ) * 2.4 - T * 1.6;
float Spiral = 0.5 + 0.5 * sin( Arm * 3.0 );
float Dust = exp( -length( P ) * 1.15 );
Float3 Nebula = Float3( 0.18, 0.08, 0.32 ) * Spiral * Dust;
Nebula += Float3( 0.06, 0.12, 0.28 ) * ( 1.0 - Spiral ) * Dust;
Float3 Color = Float3( 0.015, 0.018, 0.04 ) + Nebula;
for ( int Layer = 0; Layer < 5; Layer++ )
{
    float Depth = 0.55 + float( Layer ) * 0.28;
    Float2 Q = Uv * Depth + Float2( T * ( 0.08 + float( Layer ) * 0.03 ), T * 0.04 );
    Float2 Cell = floor( Q * 18.0 );
    Float2 Fr = Fract( Q * 18.0 ) - Float2( 0.5, 0.5 );
    float N = Fract( sin( dot( Cell, Float2( 127.1, 311.7 ) ) ) * 43758.5453 );
    float Core = 0.006 / max( length( Fr ), 0.0018 );
    float Star = Core * step( 0.965 - float( Layer ) * 0.012, N );
    float Twinkle = 0.45 + 0.55 * abs( sin( Moment * ( 1.2 + N * 4.0 ) + N * 20.0 ) );
    Color += Float3( 0.92, 0.94, 1.0 ) * Star * Twinkle;
}
float Glow = exp( -length( P ) * 2.4 ) * ( 0.18 + 0.08 * sin( Moment * 0.7 ) );
Color += Float3( 0.35, 0.22, 0.55 ) * Glow;
Final.rgb = Saturate( Color );
)",
    R"(
Float2 Uv = Local / max( Extent, Float2( 1.0, 1.0 ) );
float Bolt = 0.0;
float Y = Uv.y;
float X = Uv.x + 0.18 * sin( Y * 14.0 + Moment * 6.0 ) + 0.08 * sin( Y * 37.0 - Moment * 11.0 );
Bolt = 0.012 / max( abs( X ), 0.0015 );
float Flash = pow( Saturate( sin( Moment * 9.0 ) ), 18.0 );
Final.rgb = Float3( 0.55, 0.75, 1.0 ) * Bolt * ( 0.25 + Flash ) + Float3( 0.02, 0.03, 0.05 );
)",
    R"(
Float2 Uv = Local / max( Extent, Float2( 1.0, 1.0 ) ) + Float2( 0.5, 0.5 );
float Sum = 0.0;
for ( int i = 0; i < 6; i++ )
{
    float Fi = float( i );
    float Wave = sin( Uv.x * ( 6.0 + Fi ) + Moment * ( 0.7 + Fi * 0.13 ) + Fi );
    Sum += 0.08 / max( abs( Uv.y - 0.5 - Wave * 0.12 * ( 1.0 - Fi * 0.08 ) ), 0.004 );
}
Final.rgb = Float3( 0.25, 0.55, 0.95 ) * Saturate( Sum ) + Float3( 0.03, 0.04, 0.07 );
)",
    R"(
Float2 Uv = Local / max( Extent, Float2( 1.0, 1.0 ) );
Float2 P = Uv * 0.85;
float Angle = Moment * 0.18;
float C = cos( Angle );
float S = sin( Angle );
P = Float2( P.x * C - P.y * S, P.x * S + P.y * C );
for ( int i = 0; i < 4; i++ )
{
    P = Float2( P.x, P.y ) + Float2( 0.18 * sin( P.y * 3.2 + Moment * 0.35 ), 0.18 * cos( P.x * 3.0 - Moment * 0.28 ) );
}
float Field = 0.5 + 0.5 * sin( P.x * 4.0 + P.y * 2.6 );
float Spec = pow( Saturate( 0.72 + 0.28 * sin( P.x * 6.0 - Moment * 0.5 ) ), 8.0 );
Float3 Cool = Float3( 0.62, 0.68, 0.76 );
Float3 Warm = Float3( 0.88, 0.86, 0.82 );
Float3 Tone = Lerp( Cool, Warm, Field );
Tone += Float3( 1.0, 1.0, 1.0 ) * Spec * 0.35;
Tone *= 0.22 + 0.18 * ( 1.0 - length( Uv ) * 0.35 );
Final.rgb = Saturate( Tone );
)",
    R"(
Float2 Uv = Local / max( Extent, Float2( 1.0, 1.0 ) );
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
Float2 Uv = Local / max( Extent, Float2( 1.0, 1.0 ) ) + Float2( 0.5, 0.5 );
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
Float2 Uv = Local / max( Extent, Float2( 1.0, 1.0 ) );
float A = atan( Uv.y, Uv.x );
float R = length( Uv );
float Band = 0.5 + 0.5 * sin( A * 6.0 + R * 8.0 - Moment * 1.4 );
Final.rgb = Float3( Band, 0.4 + 0.4 * sin( Band * 5.0 + 1.2 ), 0.7 - Band * 0.35 );
)",
    R"(
Float2 Uv = Local / max( Extent, Float2( 1.0, 1.0 ) );
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
Float2 Uv = Local / max( Extent, Float2( 1.0, 1.0 ) );
float R = length( Uv );
float Burst = pow( Saturate( 1.0 - R * 0.85 ), 2.2 );
float Ring = pow( Saturate( 1.0 - abs( R - 0.35 - 0.08 * sin( Moment ) ) * 6.0 ), 3.0 );
Float3 Core = Float3( 0.95, 0.55, 0.85 ) * Burst;
Float3 Edge = Float3( 0.35, 0.55, 1.0 ) * Ring;
Final.rgb = Core + Edge + Float3( 0.03, 0.03, 0.05 );
)"
};

static void SeedCloud( float Width, float Height, int Count ) {
    Cloud.resize( Count );
    for ( int i = 0; i < Count; i++ ) {
        float Seed = ( float )i * 12.9898f;
        Speck Item;
        Item.X = fmodf( Seed * 17.13f, Width );
        Item.Y = fmodf( Seed * 9.77f, Height );
        Item.Vx = sinf( Seed ) * 12.0f;
        Item.Vy = cosf( Seed * 0.7f ) * 8.0f;
        Item.Size = 1.1f + fmodf( Seed, 2.8f );
        Item.Life = 0.22f + fmodf( Seed * 0.13f, 0.65f );
        Item.Hue = fmodf( Seed * 0.07f, 1.0f );
        Cloud[ i ] = Item;
    }
}

void bind( CGraphics* Graphics ) {
    Gfx = Graphics;
    unsigned char White[ 16 ] = {
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255
    };
    if ( Gfx )
        Plate = Gfx->CreateImage( White, 2, 2 );
}

void set_quality( Quality Value ) {
    Level = Value;
}

Quality quality( ) {
    return Level;
}

void set_background( int Index ) {
    if ( Index < 0 )
        Index = 0;
    if ( Index > 11 )
        Index = 11;
    Background = Index;
}

int background( ) {
    return Background;
}

const char* const* background_names( int& Count ) {
    Count = 12;
    return Names;
}

void compose( ) {
    if ( Ready )
        return;
    for ( int i = 1; i < 12; i++ )
        Effects[ i ] = Shaders->Compose( Names[ i ], Bodies[ i ] );
    Ready = true;
}

void draw_atmosphere( float Width, float Height ) {
    if ( Level == Quality::Off || Background <= 0 || !Plate )
        return;
    compose( );
    unsigned int Former = Canvas->Effect( Effects[ Background ] );
    Canvas->Image( CRectangle( 0.0f, 0.0f, Width, Height ), Plate, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), CColor( 255, 255, 255 ), 0.0f );
    Canvas->Effect( Former );
    float Pulse = hear::level( );
    if ( Pulse > 0.02f )
        Canvas->Rectangle( CRectangle( 0.0f, 0.0f, Width, Height ), hear::tint( ).Fade( 0.06f + Pulse * 0.20f ), 0.0f );
}

void draw_particles( float Width, float Height, float Dt ) {
    if ( Level == Quality::Off || Background <= 0 )
        return;
    if ( Background != 3 && Background != 8 )
        return;

    int Count = Level == Quality::High ? 80 : 32;
    if ( ( int )Cloud.size( ) != Count )
        SeedCloud( Width, Height, Count );

    float Pulse = 0.15f + hear::level( ) * 1.35f;
    CColor Tint = Style->Text.Fade( 0.28f ).Blend( hear::tint( ), hear::level( ) * 0.55f );
    for ( Speck& Item : Cloud ) {
        Item.X += Item.Vx * Dt * Pulse;
        Item.Y -= ( 10.0f + Item.Size ) * Dt * ( 0.85f + hear::bass( ) * 1.4f );
        if ( Item.X < 0.0f ) Item.X += Width;
        if ( Item.X > Width ) Item.X -= Width;
        if ( Item.Y < 0.0f ) Item.Y += Height;
        if ( Item.Y > Height ) Item.Y -= Height;
        Canvas->Rectangle( CRectangle( Item.X, Item.Y, Item.Size, Item.Size ), Tint, 0.0f );
    }
}

}
}
