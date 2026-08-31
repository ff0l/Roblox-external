#pragma once

#include "Engine.h"

namespace ur {
namespace image {

void bind( CGraphics* graphics );
unsigned long long svg( const char* markup, int size );
unsigned long long file( const char* path, int longest = 0 );
void sweep( );

}
}
