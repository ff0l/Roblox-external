#pragma once

#include <string>

namespace ur {
namespace config {

void load( );
const char* get( const char* key, const char* fallback = "" );
void set( const char* key, const char* value );
void set_asset_root( const char* path );
std::string app_dir( );
std::string asset( const char* relative );

}
}
