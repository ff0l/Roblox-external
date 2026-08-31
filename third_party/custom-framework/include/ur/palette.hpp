#pragma once

#include <functional>

namespace ur {
namespace palette {

void add( const char* name, const char* hint, std::function< void( ) > action );
void open( );
void close( );
bool visible( );
void draw( );
void clear( );

}
}
