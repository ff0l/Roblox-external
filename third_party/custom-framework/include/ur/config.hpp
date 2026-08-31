#pragma once

#include <string>

namespace ur {
namespace config {

void load( );
const char* get( const char* key, const char* fallback = "" );
void set( const char* key, const char* value );
std::string app_dir( );
std::string asset( const char* relative );

}
}
