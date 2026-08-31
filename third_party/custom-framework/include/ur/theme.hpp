#pragma once

namespace ur {
namespace theme {

inline constexpr int Count = 12;

const char* const* names( );
void apply( int index );
int current( );
bool load_file( const char* path );

}
}
