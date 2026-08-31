#include "ur/keys.hpp"

#include "Input.h"

#include <string>
#include <unordered_map>

namespace ur {
namespace bind {

static std::unordered_map< std::string, int > Keys;

void set( const char* Name, int Key ) {
    Keys[ Name ? Name : "" ] = Key;
}

int get( const char* Name, int Fallback ) {
    auto Found = Keys.find( Name ? Name : "" );
    if ( Found == Keys.end( ) )
        return Fallback;
    return Found->second;
}

bool pressed( const char* Name ) {
    int Key = get( Name, 0 );
    return Key != 0 && Input->KeyPressed( Key );
}

bool down( const char* Name ) {
    int Key = get( Name, 0 );
    return Key != 0 && Input->KeyDownState( Key );
}

}
}
