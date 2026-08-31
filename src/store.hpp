#pragma once

#include <Windows.h>
#include <ShlObj.h>
#include <Shellapi.h>
#include <cstdio>
#include <cstring>

namespace store {

inline constexpr int SlotMax = 32;
inline constexpr int NameCap = 28;
inline constexpr int BodyCap = 8192;

inline bool ValidChar( char Mark ) {
    return ( Mark >= 'A' && Mark <= 'Z' )
        || ( Mark >= 'a' && Mark <= 'z' )
        || ( Mark >= '0' && Mark <= '9' )
        || Mark == ' '
        || Mark == '-'
        || Mark == '_';
}

inline void Sanitize( char* Name ) {
    if ( !Name )
        return;
    int Write = 0;
    for ( int Read = 0; Name[ Read ] && Write < NameCap - 1; Read++ ) {
        if ( ValidChar( Name[ Read ] ) )
            Name[ Write++ ] = Name[ Read ];
    }
    while ( Write > 0 && Name[ Write - 1 ] == ' ' )
        Write--;
    Name[ Write ] = 0;
}

inline bool Valid( const char* Name ) {
    if ( !Name || !Name[ 0 ] )
        return false;
    for ( int Index = 0; Name[ Index ]; Index++ ) {
        if ( !ValidChar( Name[ Index ] ) )
            return false;
    }
    return true;
}

inline bool Root( char* Out, int Cap ) {
    char App[ MAX_PATH ] = { };
    if ( FAILED( SHGetFolderPathA( nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, App ) ) )
        return false;
    snprintf( Out, Cap, "%s\\ff0l", App );
    CreateDirectoryA( Out, nullptr );
    snprintf( Out, Cap, "%s\\ff0l\\configs", App );
    CreateDirectoryA( Out, nullptr );
    return true;
}

inline bool PathOf( const char* Name, char* Out, int Cap ) {
    char Folder[ MAX_PATH ] = { };
    if ( !Root( Folder, MAX_PATH ) || !Valid( Name ) )
        return false;
    snprintf( Out, Cap, "%s\\%s.cfg", Folder, Name );
    return true;
}

inline bool ActivePath( char* Out, int Cap ) {
    char Folder[ MAX_PATH ] = { };
    if ( !Root( Folder, MAX_PATH ) )
        return false;
    snprintf( Out, Cap, "%s\\current.txt", Folder );
    return true;
}

inline bool ReadFile( const char* Path, char* Body, int Cap ) {
    if ( !Path || !Body || Cap < 2 )
        return false;
    FILE* File = nullptr;
    if ( fopen_s( &File, Path, "rb" ) != 0 || !File )
        return false;
    size_t Got = fread( Body, 1, ( size_t )( Cap - 1 ), File );
    fclose( File );
    Body[ Got ] = 0;
    return Got > 0;
}

inline bool WriteFile( const char* Path, const char* Body ) {
    if ( !Path || !Body )
        return false;
    FILE* File = nullptr;
    if ( fopen_s( &File, Path, "wb" ) != 0 || !File )
        return false;
    fputs( Body, File );
    fclose( File );
    return true;
}

inline int List( char Names[ SlotMax ][ NameCap ] ) {
    char Folder[ MAX_PATH ] = { };
    if ( !Root( Folder, MAX_PATH ) )
        return 0;
    char Find[ MAX_PATH ] = { };
    snprintf( Find, MAX_PATH, "%s\\*.cfg", Folder );
    WIN32_FIND_DATAA Data;
    HANDLE Hunt = FindFirstFileA( Find, &Data );
    if ( Hunt == INVALID_HANDLE_VALUE )
        return 0;
    int Count = 0;
    do {
        if ( Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            continue;
        char Stem[ NameCap ] = { };
        lstrcpynA( Stem, Data.cFileName, NameCap );
        char* Dot = strstr( Stem, ".cfg" );
        if ( Dot )
            *Dot = 0;
        Sanitize( Stem );
        if ( !Valid( Stem ) || Count >= SlotMax )
            continue;
        lstrcpynA( Names[ Count++ ], Stem, NameCap );
    } while ( FindNextFileA( Hunt, &Data ) );
    FindClose( Hunt );
    for ( int Left = 0; Left < Count; Left++ ) {
        for ( int Right = Left + 1; Right < Count; Right++ ) {
            if ( _stricmp( Names[ Left ], Names[ Right ] ) > 0 ) {
                char Swap[ NameCap ];
                lstrcpynA( Swap, Names[ Left ], NameCap );
                lstrcpynA( Names[ Left ], Names[ Right ], NameCap );
                lstrcpynA( Names[ Right ], Swap, NameCap );
            }
        }
    }
    return Count;
}

inline bool Read( const char* Name, char* Body, int Cap ) {
    char Path[ MAX_PATH ] = { };
    return PathOf( Name, Path, MAX_PATH ) && ReadFile( Path, Body, Cap );
}

inline bool Write( const char* Name, const char* Body ) {
    char Path[ MAX_PATH ] = { };
    return PathOf( Name, Path, MAX_PATH ) && WriteFile( Path, Body );
}

inline bool Remove( const char* Name ) {
    char Path[ MAX_PATH ] = { };
    return PathOf( Name, Path, MAX_PATH ) && DeleteFileA( Path ) != 0;
}

inline bool Current( char* Out, int Cap ) {
    char Path[ MAX_PATH ] = { };
    if ( !ActivePath( Path, MAX_PATH ) || !ReadFile( Path, Out, Cap ) )
        return false;
    Sanitize( Out );
    return Valid( Out );
}

inline bool SetCurrent( const char* Name ) {
    char Path[ MAX_PATH ] = { };
    if ( !ActivePath( Path, MAX_PATH ) || !Valid( Name ) )
        return false;
    return WriteFile( Path, Name );
}

inline bool OpenFolder( ) {
    char Folder[ MAX_PATH ] = { };
    if ( !Root( Folder, MAX_PATH ) )
        return false;
    return ( INT_PTR )ShellExecuteA( nullptr, "open", Folder, nullptr, nullptr, SW_SHOWNORMAL ) > 32;
}

inline bool Take( const char* Body, const char* Key, int& Out ) {
    if ( !Body || !Key )
        return false;
    char Needle[ 64 ];
    snprintf( Needle, sizeof( Needle ), "%s ", Key );
    const char* Hit = strstr( Body, Needle );
    if ( !Hit )
        return false;
    Out = atoi( Hit + strlen( Needle ) );
    return true;
}

inline bool TakeF( const char* Body, const char* Key, float& Out ) {
    int Whole = 0;
    if ( !Take( Body, Key, Whole ) )
        return false;
    Out = ( float )Whole;
    return true;
}

inline bool TakeB( const char* Body, const char* Key, bool& Out ) {
    int Whole = 0;
    if ( !Take( Body, Key, Whole ) )
        return false;
    Out = Whole != 0;
    return true;
}

}
