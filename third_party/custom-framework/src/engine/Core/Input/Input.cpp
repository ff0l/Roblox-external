#include "Input.h"

void CInput::Settle( ) {
    for ( int Button = 0; Button < 3; Button++ ) {
        Pressing[ Button ] = false;

        Releasing[ Button ] = false;
        Doubling[ Button ] = false;
    }

    for ( int Key = 0; Key < 256; Key++ ) {
        Starting[ Key ] = false;

        Stopping[ Key ] = false;
        Ringing[ Key ] = false;
    }

    Typed.clear( );
    WheelDelta = 0.0f;
}

bool CInput::MouseDown( int Button ) const {
    if ( Button < 0 || Button > 2 )
        return false;

    return Buttons[ Button ];
}

bool CInput::MousePressed( int Button ) const {
    if ( Button < 0 || Button > 2 )
        return false;

    return Pressing[ Button ];
}

bool CInput::MouseReleased( int Button ) const {
    if ( Button < 0 || Button > 2 )
        return false;

    return Releasing[ Button ];
}

bool CInput::MouseDouble( int Button ) const {
    if ( Button < 0 || Button > 2 )
        return false;

    return Doubling[ Button ];
}

bool CInput::KeyDownState( int Key ) const {
    if ( Key < 0 || Key > 255 )
        return false;

    return Keys[ Key ];
}

bool CInput::KeyPressed( int Key ) const {
    if ( Key < 0 || Key > 255 )
        return false;

    return Starting[ Key ];
}

bool CInput::KeyReleased( int Key ) const {
    if ( Key < 0 || Key > 255 )
        return false;

    return Stopping[ Key ];
}

bool CInput::KeyRepeated( int Key ) const {
    if ( Key < 0 || Key > 255 )
        return false;

    return Starting[ Key ] || Ringing[ Key ];
}

bool CInput::Shift( ) const {
    return Keys[ KeyShift ];
}

bool CInput::Control( ) const {
    return Keys[ KeyControl ];
}

bool CInput::Alt( ) const {
    return Keys[ KeyAlt ];
}

void CInput::ApplyButton( int Button, bool State ) {
    if ( Button < 0 || Button > 2 )
        return;

    Buttons[ Button ] = State;

    if ( State ) {
        Pressing[ Button ] = true;
        PressPosition = MousePosition;
    } else {
        Releasing[ Button ] = true;
    }
}

void CInput::ApplyDouble( int Button ) {
    if ( Button < 0 || Button > 2 )
        return;

    Doubling[ Button ] = true;
}

void CInput::ApplyPosition( float Across, float Down ) {
    MousePosition = CVector( Across, Down );
}

void CInput::ApplyWheel( float Amount ) {
    WheelDelta += Amount;
}

void CInput::ApplyKey( int Key, bool State ) {
    if ( Key < 0 || Key > 255 )
        return;

    if ( State ) {
        if ( Keys[ Key ] )
            Ringing[ Key ] = true;
        else
            Starting[ Key ] = true;

        Keys[ Key ] = true;
    } else {
        Keys[ Key ] = false;
        Stopping[ Key ] = true;
    }
}

void CInput::ApplyText( unsigned int Unit ) {
    if ( Unit >= 0xD800 && Unit <= 0xDBFF ) {
        Pending = Unit;
        return;
    }

    if ( Unit >= 0xDC00 && Unit <= 0xDFFF ) {
        if ( Pending ) {
            unsigned int Codepoint = 0x10000 + ( ( Pending - 0xD800 ) << 10 ) + ( Unit - 0xDC00 );

            Typed.push_back( Codepoint );
            Pending = 0;
        }

        return;
    }

    Pending = 0;

    if ( Unit >= 32 && Unit != 127 )
        Typed.push_back( Unit );
}

void CInput::ApplyCancel( ) {
    for ( int Button = 0; Button < 3; Button++ )
        Buttons[ Button ] = false;

    for ( int Key = 0; Key < 256; Key++ ) {
        if ( Keys[ Key ] ) {
            Keys[ Key ] = false;
            Stopping[ Key ] = true;
        }
    }
}
