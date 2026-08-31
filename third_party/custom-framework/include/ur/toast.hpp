#pragma once

#include "Geometry.h"

namespace ur {
namespace toast {

void push( const char* message, float seconds = 2.8f );
void push( const char* message, CColor tint, float seconds = 2.8f );
void draw( );
void clear( );

}
}
