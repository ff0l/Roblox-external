#pragma once

#include "Engine.h"

namespace ur {
namespace widget {

struct Item {
    unsigned int id = 0;
    CRectangle bounds;
    bool hovered = false;
    bool held = false;
    bool clicked = false;
    float glow = 0.0f;
};

Item begin( const char* id, CVector size );
void tooltip( const char* message );

}

struct id_scope {
    explicit id_scope( int Number ) {
        Context->PushIdentifier( Number );
    }

    explicit id_scope( const char* Name ) {
        Context->PushIdentifier( Name );
    }

    ~id_scope( ) {
        Context->PopIdentifier( );
    }

    id_scope( const id_scope& ) = delete;
    id_scope& operator=( const id_scope& ) = delete;
};

}
