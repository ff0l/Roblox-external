#pragma once

#include <string>

#include "Geometry.h"

namespace ur {
namespace view {

float& number( const char* id, float fallback = 0.0f );
int& index( const char* id, int fallback = 0 );
bool& flag( const char* id, bool fallback = false );
std::string& text( const char* id, const char* fallback = "" );
void clear( );

unsigned int card( );

class Board {
public:
    void begin( int columns = 0 );
    void cell( const char* title, float guess, CVector& origin, CVector& extent, int span = 1 );

private:
    int Columns = 3;
    float Pad = 12.0f;
    float Gap = 10.0f;
    float Width = 280.0f;
    float Floor = 720.0f;
    float ColumnX[ 8 ] = { };
    float ColumnY[ 8 ] = { };
};

}
}
