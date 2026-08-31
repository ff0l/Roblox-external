#pragma once

#include <Windows.h>
#include <ShlObj.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace sense {

enum TeamHow : int {
    TeamAuto = 0,
    TeamNone,
    TeamPtr,
    TeamName,
    TeamColor
};

enum VisHow : int {
    VisAuto = 0,
    VisNone,
    VisRay
};

struct Card {
    uint64_t place = 0;
    int teamHow = TeamAuto;
    int visHow = VisAuto;
    bool teamReady = false;
    bool visReady = false;
    unsigned nextTeam = 0;
    unsigned nextVis = 0;
};

inline Card& Live( ) {
    static Card Store;
    return Store;
}

inline int TeamHow( ) {
    return Live( ).teamReady ? Live( ).teamHow : TeamAuto;
}

inline int VisHow( ) {
    return Live( ).visReady ? Live( ).visHow : VisAuto;
}

inline bool TeamWorks( ) {
    int How = TeamHow( );
    return How != TeamNone;
}

inline bool VisWorks( ) {
    return VisHow( ) != VisNone;
}

inline bool Folder( char* Out, int Cap ) {
    char App[ MAX_PATH ] = { };
    if ( FAILED( SHGetFolderPathA( nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, App ) ) )
        return false;
    snprintf( Out, Cap, "%s\\ff0l", App );
    CreateDirectoryA( Out, nullptr );
    snprintf( Out, Cap, "%s\\ff0l\\games", App );
    CreateDirectoryA( Out, nullptr );
    return true;
}

inline bool PathOf( uint64_t Place, const char* Kind, char* Out, int Cap ) {
    char Dir[ MAX_PATH ] = { };
    if ( !Folder( Dir, MAX_PATH ) || !Place || !Kind )
        return false;
    snprintf( Out, Cap, "%s\\%llu.%s", Dir, ( unsigned long long )Place, Kind );
    return true;
}

inline const char* TeamWord( int How ) {
    if ( How == TeamPtr )
        return "ptr";
    if ( How == TeamName )
        return "name";
    if ( How == TeamColor )
        return "color";
    if ( How == TeamNone )
        return "none";
    return "auto";
}

inline const char* VisWord( int How ) {
    if ( How == VisRay )
        return "ray";
    if ( How == VisNone )
        return "none";
    return "auto";
}

inline int TeamFrom( const char* Word ) {
    if ( Word && !_stricmp( Word, "ptr" ) )
        return TeamPtr;
    if ( Word && !_stricmp( Word, "name" ) )
        return TeamName;
    if ( Word && !_stricmp( Word, "color" ) )
        return TeamColor;
    if ( Word && !_stricmp( Word, "none" ) )
        return TeamNone;
    return TeamAuto;
}

inline int VisFrom( const char* Word ) {
    if ( Word && !_stricmp( Word, "ray" ) )
        return VisRay;
    if ( Word && !_stricmp( Word, "none" ) )
        return VisNone;
    return VisAuto;
}

inline bool LoadKind( uint64_t Place, const char* Kind, char* Method, int Cap ) {
    if ( Method && Cap > 0 )
        Method[ 0 ] = 0;
    char Path[ MAX_PATH ] = { };
    if ( !PathOf( Place, Kind, Path, MAX_PATH ) )
        return false;
    FILE* File = nullptr;
    if ( fopen_s( &File, Path, "rb" ) != 0 || !File )
        return false;
    char Body[ 256 ] = { };
    size_t Got = fread( Body, 1, sizeof( Body ) - 1, File );
    fclose( File );
    Body[ Got ] = 0;
    const char* At = strstr( Body, "method=" );
    if ( !At || !Method )
        return false;
    At += 7;
    int Write = 0;
    while ( At[ Write ] && At[ Write ] != '\r' && At[ Write ] != '\n' && Write < Cap - 1 ) {
        Method[ Write ] = At[ Write ];
        Write++;
    }
    Method[ Write ] = 0;
    return Method[ 0 ] != 0;
}

inline void SaveKind( uint64_t Place, const char* Kind, const char* Method, const char* Extra ) {
    char Path[ MAX_PATH ] = { };
    if ( !PathOf( Place, Kind, Path, MAX_PATH ) )
        return;
    FILE* File = nullptr;
    if ( fopen_s( &File, Path, "wb" ) != 0 || !File )
        return;
    fprintf( File, "place=%llu\r\nmethod=%s\r\n", ( unsigned long long )Place, Method ? Method : "none" );
    if ( Extra && Extra[ 0 ] )
        fprintf( File, "%s\r\n", Extra );
    fclose( File );
}

inline void BindPlace( uint64_t Place ) {
    Card& S = Live( );
    if ( Place == S.place && ( S.teamReady || S.visReady ) )
        return;
    S.place = Place;
    S.teamHow = TeamAuto;
    S.visHow = VisAuto;
    S.teamReady = false;
    S.visReady = false;
    S.nextTeam = 0;
    S.nextVis = 0;
    if ( !Place )
        return;
    char Method[ 24 ] = { };
    if ( LoadKind( Place, "team", Method, ( int )sizeof( Method ) ) ) {
        S.teamHow = TeamFrom( Method );
        S.teamReady = S.teamHow != TeamAuto;
    }
    if ( LoadKind( Place, "vis", Method, ( int )sizeof( Method ) ) ) {
        int How = VisFrom( Method );
        if ( How == VisNone )
            How = VisAuto;
        S.visHow = How;
        S.visReady = How == VisRay;
    }
}

inline void DecideTeam( uint64_t Place, int PtrN, int NameN, int ColorN, int Teamed ) {
    Card& S = Live( );
    if ( !Place )
        return;
    BindPlace( Place );
    unsigned Now = GetTickCount( );
    if ( S.teamReady && Now < S.nextTeam )
        return;
    S.nextTeam = Now + 8000;
    int How = TeamNone;
    if ( PtrN >= 2 || ( Teamed > 0 && PtrN >= 1 ) )
        How = TeamPtr;
    else if ( NameN >= 2 )
        How = TeamName;
    else if ( ColorN >= 2 )
        How = TeamColor;
    S.teamHow = How;
    S.teamReady = true;
    char Extra[ 80 ];
    snprintf( Extra, sizeof( Extra ), "ptrs=%d names=%d colors=%d teamed=%d", PtrN, NameN, ColorN, Teamed );
    SaveKind( Place, "team", TeamWord( How ), Extra );
}

inline void DecideVis( uint64_t Place, int Walls ) {
    Card& S = Live( );
    if ( !Place )
        return;
    BindPlace( Place );
    unsigned Now = GetTickCount( );
    if ( S.visReady && Now < S.nextVis )
        return;
    S.nextVis = Now + 8000;
    if ( Walls >= 6 ) {
        S.visHow = VisRay;
        S.visReady = true;
        char Extra[ 48 ];
        snprintf( Extra, sizeof( Extra ), "walls=%d", Walls );
        SaveKind( Place, "vis", VisWord( VisRay ), Extra );
        return;
    }
    S.visHow = VisAuto;
    S.visReady = false;
}

}
