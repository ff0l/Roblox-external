#pragma once

#include "Engine.h"

namespace ur {
namespace effects {

enum class Quality {
    Off,
    Low,
    High
};

void bind( CGraphics* graphics );
void set_quality( Quality quality );
Quality quality( );

void set_background( int index );
int background( );
const char* const* background_names( int& count );

void compose( );
void draw_atmosphere( float width, float height );
void draw_particles( float width, float height, float dt );

}
}
