#pragma once

#include "Geometry.h"

namespace ur {
namespace desk {

void clock( );
void mix( );
void wave( const float* values, int count );
void spectrum( const float* values, int count );
void meter( float level );

}
}
