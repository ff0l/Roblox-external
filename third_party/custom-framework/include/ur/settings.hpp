#pragma once

#include <string>

namespace ur {
namespace settings {

void load( const char* path );
void save( const char* path );

const char* get( const char* key, const char* fallback = "" );
void set( const char* key, const char* value );

int get_int( const char* key, int fallback = 0 );
void set_int( const char* key, int value );

float get_float( const char* key, float fallback = 0.0f );
void set_float( const char* key, float value );

bool get_bool( const char* key, bool fallback = false );
void set_bool( const char* key, bool value );

}
}
