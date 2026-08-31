#pragma once

#include "Input.h"

#include <unordered_map>
#include <string>

namespace ur {

enum class Key : int {
    None = 0,
    Backspace = KeyBackspace,
    Tab = KeyTab,
    Enter = KeyEnter,
    Shift = KeyShift,
    Control = KeyControl,
    Alt = KeyAlt,
    Escape = KeyEscape,
    Space = KeySpace,
    PageUp = KeyPageUp,
    PageDown = KeyPageDown,
    End = KeyEnd,
    Home = KeyHome,
    Left = KeyLeft,
    Up = KeyUp,
    Right = KeyRight,
    Down = KeyDown,
    Delete = KeyDelete,
    A = KeyLetterA,
    C = KeyLetterC,
    S = 83,
    V = KeyLetterV,
    X = KeyLetterX,
    Y = KeyLetterY,
    Z = KeyLetterZ,
    F1 = KeyF1,
    F2 = KeyF2,
    F3 = KeyF3,
    F4 = KeyF4,
    F5 = KeyF5,
    F6 = KeyF6,
    F7 = KeyF7,
    F8 = KeyF8,
    F9 = KeyF9,
    F10 = KeyF10,
    F11 = KeyF11,
    F12 = KeyF12
};

inline bool down( Key Wanted ) {
    return Input->KeyDownState( ( int )Wanted );
}

inline bool pressed( Key Wanted ) {
    return Input->KeyPressed( ( int )Wanted );
}

inline bool released( Key Wanted ) {
    return Input->KeyReleased( ( int )Wanted );
}

namespace bind {

void set( const char* Name, int Key );
int get( const char* Name, int Fallback = 0 );
bool pressed( const char* Name );
bool down( const char* Name );

}

}
