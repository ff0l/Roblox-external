#pragma once

#include <chrono>
#include <cstdio>

inline const char* IceMetal( float Dr, float Dg, float Db, float Mr, float Mg, float Mb, float Hr, float Hg, float Hb ) {
    static char Body[ 2800 ] = { };
    static float ShiftX = 0.0f;
    static float ShiftY = 0.0f;
    static float Twist = 0.0f;
    static float Grain = 1.7f;
    static bool Seeded = false;
    if ( !Seeded ) {
        unsigned long long Tick = ( unsigned long long )std::chrono::high_resolution_clock::now( ).time_since_epoch( ).count( );
        ShiftX = ( float )( Tick % 997ull ) * 0.041f;
        ShiftY = ( float )( ( Tick / 997ull ) % 991ull ) * 0.037f;
        Twist = ( float )( Tick % 628ull ) * 0.01f;
        Grain = 1.7f + ( float )( Tick % 80ull ) * 0.01f;
        Seeded = true;
    }

    snprintf( Body, sizeof( Body ),
        "Float2 Uv = Screen * Float2( 0.0026, 0.0115 ) + Float2( %.4f, %.4f );\n"
        "float Time = Moment * 0.055;\n"
        "Float2 P = Uv;\n"
        "P.y += Time * 0.09;\n"
        "P.x += 0.05 * sin( P.y * 2.1 + Time * 0.28 + %.4f );\n"
        "float Rib = sin( P.x * 6.8 + 0.35 * sin( P.y * 1.6 ) );\n"
        "float Groove = pow( Saturate( 1.0 - abs( Rib ) ), 0.62 );\n"
        "float Pore = sin( P.x * 4.6 + 0.9 ) * sin( P.y * 5.4 - Time * 0.16 );\n"
        "float Hollow = pow( Saturate( 0.18 - Pore ), 1.6 );\n"
        "float Soft = 0.5 + 0.5 * sin( P.y * 1.7 + P.x * 1.1 + Time * 0.2 );\n"
        "float Speck = Fract( sin( dot( Uv, Float2( 127.1, 311.7 ) ) + %.4f ) * 43758.5453 );\n"
        "float Warp = 0.5 + 0.5 * sin( Uv.x * %.3f + Uv.y * 3.3 + Speck * 6.283 );\n"
        "float Mix = Saturate( Groove * 0.46 + Soft * 0.20 + Warp * 0.10 - Hollow * 0.38 - Speck * 0.08 );\n"
        "float Rim = pow( Groove, 3.6 );\n"
        "Float3 Deep = Float3( %.4f, %.4f, %.4f );\n"
        "Float3 Mid = Float3( %.4f, %.4f, %.4f );\n"
        "Float3 High = Float3( %.4f, %.4f, %.4f );\n"
        "Float3 Tint = Lerp( Deep, Mid, Mix );\n"
        "Tint = Lerp( Tint, High, Rim * 0.55 );\n"
        "Final.rgb = Saturate( Tint );\n",
        ( double )ShiftX, ( double )ShiftY, ( double )Twist, ( double )Twist, ( double )Grain,
        ( double )Dr, ( double )Dg, ( double )Db,
        ( double )Mr, ( double )Mg, ( double )Mb,
        ( double )Hr, ( double )Hg, ( double )Hb );

    return Body;
}
