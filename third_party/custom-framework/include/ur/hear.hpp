#pragma once

#include "Geometry.h"

namespace ur {
namespace hear {

enum class Source {
    Output,
    Mic,
    Both
};

void start( );
void shutdown( );
void tick( );

void set_source( Source source );
Source source( );
const char* const* source_names( int& count );

void set_gain( float gain );
float gain( );

bool armed( );
bool live( );
const char* status( );
float level( );
float peak( );
float bass( );
float mid( );
float treble( );
void bands( float* dest, int count );
void wave( float* dest, int count );
CColor tint( );

void draw_wave( );
void draw_spectrum( );
void draw_meter( );

}
}
