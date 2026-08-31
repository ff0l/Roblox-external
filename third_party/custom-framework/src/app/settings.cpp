#include "ur/settings.hpp"

#include <cstdlib>
#include <fstream>
#include <string>
#include <unordered_map>

namespace ur {
namespace settings {

static std::unordered_map< std::string, std::string > Values;
static std::string Path;

static std::string Trim( const std::string& Text ) {
    size_t Start = 0;
    while ( Start < Text.size( ) && ( Text[ Start ] == ' ' || Text[ Start ] == '\t' || Text[ Start ] == '\r' ) )
        Start++;

    size_t Stop = Text.size( );
    while ( Stop > Start && ( Text[ Stop - 1 ] == ' ' || Text[ Stop - 1 ] == '\t' || Text[ Stop - 1 ] == '\r' ) )
        Stop--;

    return Text.substr( Start, Stop - Start );
}

void load( const char* File ) {
    Values.clear( );
    Path = File ? File : "ur.settings";

    std::ifstream Stream( Path );
    if ( !Stream )
        return;

    std::string Line;
    while ( std::getline( Stream, Line ) ) {
        Line = Trim( Line );
        if ( Line.empty( ) || Line[ 0 ] == '#' )
            continue;

        size_t Split = Line.find( '=' );
        if ( Split == std::string::npos )
            continue;

        Values[ Trim( Line.substr( 0, Split ) ) ] = Trim( Line.substr( Split + 1 ) );
    }
}

void save( const char* File ) {
    if ( File && File[ 0 ] )
        Path = File;
    if ( Path.empty( ) )
        Path = "ur.settings";

    std::ofstream Stream( Path );
    if ( !Stream )
        return;

    for ( const auto& Entry : Values )
        Stream << Entry.first << '=' << Entry.second << '\n';
}

const char* get( const char* Key, const char* Fallback ) {
    auto Found = Values.find( Key ? Key : "" );
    if ( Found == Values.end( ) || Found->second.empty( ) )
        return Fallback ? Fallback : "";
    return Found->second.c_str( );
}

void set( const char* Key, const char* Value ) {
    Values[ Key ? Key : "" ] = Value ? Value : "";
}

int get_int( const char* Key, int Fallback ) {
    const char* Text = get( Key, "" );
    if ( !Text[ 0 ] )
        return Fallback;
    return atoi( Text );
}

void set_int( const char* Key, int Value ) {
    set( Key, std::to_string( Value ).c_str( ) );
}

float get_float( const char* Key, float Fallback ) {
    const char* Text = get( Key, "" );
    if ( !Text[ 0 ] )
        return Fallback;
    return ( float )atof( Text );
}

void set_float( const char* Key, float Value ) {
    set( Key, std::to_string( Value ).c_str( ) );
}

bool get_bool( const char* Key, bool Fallback ) {
    const char* Text = get( Key, "" );
    if ( !Text[ 0 ] )
        return Fallback;
    return Text[ 0 ] == '1' || Text[ 0 ] == 't' || Text[ 0 ] == 'T';
}

void set_bool( const char* Key, bool Value ) {
    set( Key, Value ? "1" : "0" );
}

}
}
