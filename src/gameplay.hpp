#pragma once

#include "world.hpp"

#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <string>

namespace play {

inline void PatchXml( bool Uncap ) {
    char Path[ MAX_PATH ] = { };
    char Root[ MAX_PATH ] = { };
    if ( GetEnvironmentVariableA( "LOCALAPPDATA", Root, MAX_PATH ) == 0 || !Root[ 0 ] )
        return;
    snprintf( Path, sizeof( Path ), "%s\\Roblox\\GlobalBasicSettings_13.xml", Root );
    HANDLE File = CreateFileA( Path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( File == INVALID_HANDLE_VALUE )
        return;
    DWORD Size = GetFileSize( File, nullptr );
    if ( Size == 0 || Size > 1u << 20 ) {
        CloseHandle( File );
        return;
    }
    std::string Body;
    Body.resize( Size );
    DWORD Got = 0;
    if ( !ReadFile( File, Body.data( ), Size, &Got, nullptr ) ) {
        CloseHandle( File );
        return;
    }
    CloseHandle( File );

    const char* Key = "<int name=\"FramerateCap\">";
    size_t At = Body.find( Key );
    if ( At == std::string::npos )
        return;
    size_t Start = At + strlen( Key );
    size_t End = Body.find( "</", Start );
    if ( End == std::string::npos )
        return;
    Body.replace( Start, End - Start, Uncap ? "10000" : "240" );

    File = CreateFileA( Path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( File == INVALID_HANDLE_VALUE )
        return;
    DWORD Put = 0;
    WriteFile( File, Body.data( ), ( DWORD )Body.size( ), &Put, nullptr );
    CloseHandle( File );
}

inline void TickAfk( bool On, double Dt ) {
    static double Wait = 0.0;
    if ( !On ) {
        Wait = 0.0;
        return;
    }
    Wait += Dt;
    if ( Wait < 18.0 )
        return;
    Wait = 0.0;

    HWND Window = world::GameWindow( );
    if ( !Window )
        return;

    RECT Box = { };
    GetClientRect( Window, &Box );
    int X = ( Box.right - Box.left ) / 2;
    int Y = ( Box.bottom - Box.top ) / 2;
    LPARAM Spot = MAKELPARAM( X, Y );
    PostMessageW( Window, WM_MOUSEMOVE, 0, Spot );
    PostMessageW( Window, WM_RBUTTONDOWN, MK_RBUTTON, Spot );
    PostMessageW( Window, WM_RBUTTONUP, 0, Spot );
}

inline void TickUncap( bool On ) {
    static int Applied = 0;
    static unsigned Next = 0;
    if ( !On ) {
        if ( Applied && world::Attach( ) ) {
            world::SetFps( 240.0 );
            PatchXml( false );
        }
        Applied = 0;
        Next = 0;
        return;
    }
    unsigned Now = GetTickCount( );
    if ( Applied && Now < Next )
        return;
    if ( !world::Attach( ) )
        return;
    if ( !world::SetFps( 10000.0 ) )
        return;
    if ( !Applied )
        PatchXml( true );
    Applied = 1;
    Next = Now + 2500;
}

}