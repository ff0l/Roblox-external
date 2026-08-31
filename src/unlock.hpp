#pragma once

#include "resource.h"
#include "world.hpp"

#include <Windows.h>
#include <ShlObj.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace unlock {

inline constexpr uint64_t RivalsPlace = 17625359962ull;

struct Cfg {
    bool on = false;
};

struct Run {
    bool deployed = false;
    bool signaled = false;
    unsigned lastSignal = 0;
    unsigned lastStatus = 0;
    char status[ 96 ] = "Ready";
};

inline Cfg& Live( ) {
    static Cfg Store;
    return Store;
}

inline Run& State( ) {
    static Run Store;
    return Store;
}

inline bool AppRoot( char* Out, int Cap ) {
    char App[ MAX_PATH ] = { };
    if ( FAILED( SHGetFolderPathA( nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, App ) ) )
        return false;
    snprintf( Out, Cap, "%s\\ff0l", App );
    CreateDirectoryA( Out, nullptr );
    return true;
}

inline bool PathJoin( const char* Root, const char* Tail, char* Out, int Cap ) {
    if ( !Root || !Tail || !Out || Cap < 8 )
        return false;
    snprintf( Out, Cap, "%s\\%s", Root, Tail );
    return true;
}

inline bool RcData( int Id, const void*& Data, DWORD& Size ) {
    HRSRC Res = FindResourceW( nullptr, MAKEINTRESOURCEW( Id ), MAKEINTRESOURCEW( 10 ) );
    if ( !Res )
        return false;
    HGLOBAL Block = LoadResource( nullptr, Res );
    if ( !Block )
        return false;
    Data = LockResource( Block );
    Size = SizeofResource( nullptr, Res );
    return Data && Size > 0;
}

inline bool WriteBytes( const char* Path, const void* Data, DWORD Size ) {
    HANDLE File = CreateFileA( Path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( File == INVALID_HANDLE_VALUE )
        return false;
    DWORD Wrote = 0;
    BOOL Ok = WriteFile( File, Data, Size, &Wrote, nullptr );
    CloseHandle( File );
    return Ok && Wrote == Size;
}

inline bool WriteText( const char* Path, const char* Body ) {
    if ( !Path || !Body )
        return false;
    return WriteBytes( Path, Body, ( DWORD )strlen( Body ) );
}

inline bool DropResource( int Id, const char* Path ) {
    const void* Data = nullptr;
    DWORD Size = 0;
    if ( !RcData( Id, Data, Size ) )
        return false;
    return WriteBytes( Path, Data, Size );
}

inline bool ReadText( const char* Path, char* Out, int Cap ) {
    if ( !Path || !Out || Cap < 2 )
        return false;
    HANDLE File = CreateFileA( Path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( File == INVALID_HANDLE_VALUE )
        return false;
    DWORD Size = GetFileSize( File, nullptr );
    if ( Size == 0 || Size >= ( DWORD )Cap ) {
        CloseHandle( File );
        return false;
    }
    DWORD Got = 0;
    BOOL Ok = ReadFile( File, Out, Size, &Got, nullptr );
    CloseHandle( File );
    if ( !Ok || Got == 0 )
        return false;
    Out[ Got ] = 0;
    while ( Got > 0 && ( Out[ Got - 1 ] == '\r' || Out[ Got - 1 ] == '\n' || Out[ Got - 1 ] == ' ' ) )
        Out[ --Got ] = 0;
    return true;
}

inline void SetStatus( const char* Text ) {
    lstrcpynA( State( ).status, Text ? Text : "", ( int )sizeof( State( ).status ) );
}

inline bool InRivals( ) {
    world::Engine& E = world::Core( );
    return E.placeId == RivalsPlace;
}

inline bool Deploy( ) {
    char Root[ MAX_PATH ] = { };
    if ( !AppRoot( Root, MAX_PATH ) )
        return false;

    char Scripts[ MAX_PATH ] = { };
    char BridgeDir[ MAX_PATH ] = { };
    char UnlockPath[ MAX_PATH ] = { };
    char BridgePath[ MAX_PATH ] = { };
    PathJoin( Root, "scripts", Scripts, MAX_PATH );
    PathJoin( Root, "bridge", BridgeDir, MAX_PATH );
    CreateDirectoryA( Scripts, nullptr );
    CreateDirectoryA( BridgeDir, nullptr );
    PathJoin( Scripts, "rivals_unlock.lua", UnlockPath, MAX_PATH );
    PathJoin( Scripts, "ff0l_bridge.lua", BridgePath, MAX_PATH );

    if ( !DropResource( IDR_RIVALS_UNLOCK, UnlockPath ) )
        return false;
    if ( !DropResource( IDR_FF0L_BRIDGE, BridgePath ) )
        return false;
    State( ).deployed = true;
    return true;
}

inline bool BridgeAlive( ) {
    char Root[ MAX_PATH ] = { };
    char Path[ MAX_PATH ] = { };
    if ( !AppRoot( Root, MAX_PATH ) )
        return false;
    PathJoin( Root, "bridge\\heartbeat", Path, MAX_PATH );

    WIN32_FILE_ATTRIBUTE_DATA Info = { };
    if ( !GetFileAttributesExA( Path, GetFileExInfoStandard, &Info ) )
        return false;
    FILETIME Now = { };
    GetSystemTimeAsFileTime( &Now );
    ULARGE_INTEGER A;
    ULARGE_INTEGER B;
    A.LowPart = Now.dwLowDateTime;
    A.HighPart = Now.dwHighDateTime;
    B.LowPart = Info.ftLastWriteTime.dwLowDateTime;
    B.HighPart = Info.ftLastWriteTime.dwHighDateTime;
    unsigned long long Delta = A.QuadPart > B.QuadPart ? A.QuadPart - B.QuadPart : 0;
    return Delta < 30000000ull;
}

inline void PollStatus( ) {
    char Root[ MAX_PATH ] = { };
    char Path[ MAX_PATH ] = { };
    if ( !AppRoot( Root, MAX_PATH ) )
        return;
    PathJoin( Root, "bridge\\status", Path, MAX_PATH );

    WIN32_FILE_ATTRIBUTE_DATA Info = { };
    if ( !GetFileAttributesExA( Path, GetFileExInfoStandard, &Info ) )
        return;

    unsigned Tick = ( unsigned )( ( ( ULARGE_INTEGER* )&Info.ftLastWriteTime )->QuadPart / 10000ull );
    if ( Tick == State( ).lastStatus )
        return;
    State( ).lastStatus = Tick;

    char Body[ 96 ] = { };
    if ( ReadText( Path, Body, ( int )sizeof( Body ) ) )
        SetStatus( Body );
}

inline bool SignalUnlock( ) {
    char Root[ MAX_PATH ] = { };
    char Path[ MAX_PATH ] = { };
    if ( !AppRoot( Root, MAX_PATH ) )
        return false;
    PathJoin( Root, "bridge\\cmd", Path, MAX_PATH );
    if ( !WriteText( Path, "unlock" ) )
        return false;
    State( ).signaled = true;
    State( ).lastSignal = GetTickCount( );
    SetStatus( BridgeAlive( ) ? "Running unlock..." : "Waiting for bridge" );
    return true;
}

inline void Tick( ) {
    static bool WasOn = false;
    Cfg& C = Live( );
    if ( !C.on ) {
        State( ).signaled = false;
        WasOn = false;
        SetStatus( "Ready" );
        return;
    }

    if ( !WasOn )
        State( ).signaled = false;
    WasOn = true;

    if ( !world::Attach( ) ) {
        SetStatus( "Attach Roblox first" );
        return;
    }

    if ( !InRivals( ) ) {
        SetStatus( "Rivals only" );
        return;
    }

    if ( !State( ).deployed && !Deploy( ) ) {
        SetStatus( "Deploy failed" );
        return;
    }

    PollStatus( );

    if ( !BridgeAlive( ) ) {
        SetStatus( "Add ff0l_bridge.lua to autoexec" );
    }

    if ( !State( ).signaled ) {
        if ( SignalUnlock( ) )
            State( ).signaled = true;
    } else if ( !BridgeAlive( ) ) {
        unsigned Now = GetTickCount( );
        if ( Now - State( ).lastSignal > 4000 ) {
            State( ).signaled = false;
            State( ).lastSignal = Now;
        }
    }
}

inline const char* Status( ) {
    return State( ).status;
}

}
