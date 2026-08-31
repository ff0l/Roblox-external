#define _CRT_SECURE_NO_WARNINGS

#include "ur/theme.hpp"

#include "Style.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

namespace ur {
namespace theme {

static const char* Names[ Count ] = {
    "Dark", "Light", "Mocha", "Nord", "Midnight", "Rose",
    "Dracula", "Tokyo", "Grove", "Forest", "Solar", "Latte"
};

const char* const* names( ) {
    return Names;
}

void apply( int Index ) {
    if ( Index < 0 || Index >= Count )
        Index = 0;

    switch ( Index ) {
    case 1: Style->Light( ); break;
    case 2: Style->Mocha( ); break;
    case 3: Style->Nord( ); break;
    case 4: Style->Midnight( ); break;
    case 5: Style->Rose( ); break;
    case 6: Style->Dracula( ); break;
    case 7: Style->Tokyo( ); break;
    case 8: Style->Grove( ); break;
    case 9: Style->Forest( ); break;
    case 10: Style->Solar( ); break;
    case 11: Style->Latte( ); break;
    default: Style->Dark( ); break;
    }
}

int current( ) {
    return Style->Theme( );
}

static bool ParseColor( const std::string& Text, CColor& Tint ) {
    int Red = 0;
    int Green = 0;
    int Blue = 0;
    int Alpha = 255;
    int Got = std::sscanf( Text.c_str( ), "%d %d %d %d", &Red, &Green, &Blue, &Alpha );
    if ( Got < 3 )
        return false;
    Tint = CColor( Red, Green, Blue, Alpha );
    return true;
}

bool load_file( const char* Path ) {
    if ( !Path )
        return false;

    std::ifstream Stream( Path );
    if ( !Stream )
        return false;

    std::string Line;
    while ( std::getline( Stream, Line ) ) {
        if ( Line.empty( ) || Line[ 0 ] == '#' )
            continue;

        size_t Split = Line.find( '=' );
        if ( Split == std::string::npos )
            Split = Line.find( ' ' );
        if ( Split == std::string::npos )
            continue;

        std::string Key = Line.substr( 0, Split );
        std::string Rest = Line.substr( Split + 1 );
        while ( !Rest.empty( ) && ( Rest[ 0 ] == ' ' || Rest[ 0 ] == '=' ) )
            Rest.erase( Rest.begin( ) );

        CColor Tint;
        if ( !ParseColor( Rest, Tint ) )
            continue;

        if ( Key == "backdrop" ) Style->Backdrop = Tint;
        else if ( Key == "surface" ) Style->Surface = Tint;
        else if ( Key == "elevated" ) Style->Elevated = Tint;
        else if ( Key == "header" ) Style->Header = Tint;
        else if ( Key == "outline" ) Style->Outline = Tint;
        else if ( Key == "text" ) Style->Text = Tint;
        else if ( Key == "faint" ) Style->Faint = Tint;
        else if ( Key == "accent" ) Style->Accent = Tint;
        else if ( Key == "accent_soft" ) Style->AccentSoft = Tint;
        else if ( Key == "control" ) Style->Control = Tint;
        else if ( Key == "danger" ) Style->Danger = Tint;
        else if ( Key == "success" ) Style->Success = Tint;
        else if ( Key == "warning" ) Style->Warning = Tint;
        else if ( Key == "popup" ) Style->Popup = Tint;
    }

    return true;
}

}
}
