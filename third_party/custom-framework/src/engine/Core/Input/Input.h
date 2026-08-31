#pragma once

#include <memory>
#include <vector>

#include "Geometry.h"

inline constexpr int PointerArrow = 0;
inline constexpr int PointerAcross = 1;
inline constexpr int PointerDown = 2;
inline constexpr int PointerSlant = 3;
inline constexpr int PointerMove = 4;
inline constexpr int PointerText = 5;
inline constexpr int PointerHand = 6;

inline constexpr int KeyBackspace = 8;
inline constexpr int KeyTab = 9;

inline constexpr int KeyEnter = 13;
inline constexpr int KeyShift = 16;

inline constexpr int KeyControl = 17;
inline constexpr int KeyAlt = 18;

inline constexpr int KeyEscape = 27;
inline constexpr int KeySpace = 32;

inline constexpr int KeyPageUp = 33;
inline constexpr int KeyPageDown = 34;

inline constexpr int KeyEnd = 35;
inline constexpr int KeyHome = 36;

inline constexpr int KeyLeft = 37;
inline constexpr int KeyUp = 38;

inline constexpr int KeyRight = 39;
inline constexpr int KeyDown = 40;

inline constexpr int KeyDelete = 46;

inline constexpr int KeyLetterA = 65;
inline constexpr int KeyLetterC = 67;

inline constexpr int KeyLetterV = 86;
inline constexpr int KeyLetterX = 88;

inline constexpr int KeyLetterY = 89;
inline constexpr int KeyLetterZ = 90;

inline constexpr int KeyF1 = 112;
inline constexpr int KeyF2 = 113;
inline constexpr int KeyF3 = 114;
inline constexpr int KeyF4 = 115;
inline constexpr int KeyF5 = 116;
inline constexpr int KeyF6 = 117;
inline constexpr int KeyF7 = 118;
inline constexpr int KeyF8 = 119;
inline constexpr int KeyF9 = 120;
inline constexpr int KeyF10 = 121;
inline constexpr int KeyF11 = 122;
inline constexpr int KeyF12 = 123;

class CInput {
public:
    void Settle( );

    bool MouseDown( int Button ) const;
    bool MousePressed( int Button ) const;

    bool MouseReleased( int Button ) const;
    bool MouseDouble( int Button ) const;

    bool KeyDownState( int Key ) const;
    bool KeyPressed( int Key ) const;

    bool KeyReleased( int Key ) const;
    bool KeyRepeated( int Key ) const;

    bool Shift( ) const;
    bool Control( ) const;

    bool Alt( ) const;

    void ApplyButton( int Button, bool State );
    void ApplyDouble( int Button );

    void ApplyPosition( float Across, float Down );
    void ApplyWheel( float Amount );

    void ApplyKey( int Key, bool State );
    void ApplyText( unsigned int Unit );

    void ApplyCancel( );

    CVector MousePosition;
    CVector PressPosition;

    float WheelDelta = 0.0f;
    int Pointer = PointerArrow;

    std::vector< unsigned int > Typed;

private:
    bool Buttons[ 3 ] = { };
    bool Pressing[ 3 ] = { };

    bool Releasing[ 3 ] = { };
    bool Doubling[ 3 ] = { };

    bool Keys[ 256 ] = { };
    bool Starting[ 256 ] = { };

    bool Stopping[ 256 ] = { };
    bool Ringing[ 256 ] = { };

    unsigned int Pending = 0;
};

inline auto Input = std::make_unique< CInput >( );
