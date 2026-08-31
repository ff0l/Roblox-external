#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "ur/config.hpp"

#include <Windows.h>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace ur {
namespace config {

static std::unordered_map< std::string, std::string > Values;
static std::string AppDir;
static std::string AssetRoot;

static std::string Trim( const std::string& Text ) {
    size_t Start = 0;
    while ( Start < Text.size( ) && ( Text[ Start ] == ' ' || Text[ Start ] == '\t' || Text[ Start ] == '\r' ) )
        Start++;

    size_t Stop = Text.size( );
    while ( Stop > Start && ( Text[ Stop - 1 ] == ' ' || Text[ Stop - 1 ] == '\t' || Text[ Stop - 1 ] == '\r' ) )
        Stop--;

    return Text.substr( Start, Stop - Start );
}

static void ParseFile( const std::string& Path ) {
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

static std::string DirOf( const std::string& Path ) {
    size_t Slash = Path.find_last_of( "\\/" );
    if ( Slash == std::string::npos )
        return ".";
    return Path.substr( 0, Slash );
}

static bool Exists( const std::string& Path ) {
    DWORD Attr = GetFileAttributesA( Path.c_str( ) );
    return Attr != INVALID_FILE_ATTRIBUTES;
}

void load( ) {
    Values.clear( );

    char Module[ MAX_PATH ] = { };
    GetModuleFileNameA( nullptr, Module, MAX_PATH );
    AppDir = DirOf( Module );

    char Cwd[ MAX_PATH ] = { };
    GetCurrentDirectoryA( MAX_PATH, Cwd );

    ParseFile( std::string( Cwd ) + "\\.env.example" );
    ParseFile( std::string( Cwd ) + "\\.env" );
    ParseFile( AppDir + "\\.env" );
    ParseFile( AppDir + "\\..\\.env" );

    wchar_t* Block = GetEnvironmentStringsW( );
    if ( Block ) {
        for ( wchar_t* Scan = Block; *Scan; Scan += wcslen( Scan ) + 1 ) {
            char Narrow[ 1024 ] = { };
            WideCharToMultiByte( CP_UTF8, 0, Scan, -1, Narrow, 1024, nullptr, nullptr );
            std::string Pair = Narrow;
            size_t Split = Pair.find( '=' );
            if ( Split == std::string::npos )
                continue;
            std::string Key = Pair.substr( 0, Split );
            if ( Key.rfind( "UR_", 0 ) == 0 && Values[ Key ].empty( ) )
                Values[ Key ] = Pair.substr( Split + 1 );
        }
        FreeEnvironmentStringsW( Block );
    }
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

void set_asset_root( const char* Path ) {
    AssetRoot = Path ? Path : "";
}

std::string app_dir( ) {
    return AppDir;
}

std::string asset( const char* Relative ) {
    std::vector< std::string > Roots;
    if ( !AssetRoot.empty( ) )
        Roots.push_back( AssetRoot );
    Roots.push_back( AppDir );
    Roots.push_back( AppDir + "\\.." );
    Roots.push_back( AppDir + "\\..\\.." );

    char Cwd[ MAX_PATH ] = { };
    GetCurrentDirectoryA( MAX_PATH, Cwd );
    Roots.push_back( Cwd );

    for ( const std::string& Root : Roots ) {
        std::string Path = Root + "\\" + ( Relative ? Relative : "" );
        if ( Exists( Path ) )
            return Path;
    }

    return std::string( Relative ? Relative : "" );
}

}
}
