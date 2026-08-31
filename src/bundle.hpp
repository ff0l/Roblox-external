#pragma once

#include "resource.h"
#include "ur/config.hpp"

#include <Windows.h>

#include <string>

namespace bundle {

inline HANDLE FaceMem[ 3 ] = { };

inline bool Rc( int Id, const void*& Data, DWORD& Size ) {
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

inline bool WriteIfNeeded( const std::string& Path, const void* Data, DWORD Size ) {
    HANDLE Have = CreateFileA( Path.c_str( ), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( Have != INVALID_HANDLE_VALUE ) {
        LARGE_INTEGER Len = { };
        GetFileSizeEx( Have, &Len );
        CloseHandle( Have );
        if ( Len.QuadPart == ( LONGLONG )Size )
            return true;
    }
    HANDLE File = CreateFileA( Path.c_str( ), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( File == INVALID_HANDLE_VALUE )
        return false;
    DWORD Wrote = 0;
    BOOL Ok = WriteFile( File, Data, Size, &Wrote, nullptr );
    CloseHandle( File );
    return Ok && Wrote == Size;
}

inline bool Drop( int Id, const std::string& Path ) {
    const void* Data = nullptr;
    DWORD Size = 0;
    if ( !Rc( Id, Data, Size ) )
        return false;
    return WriteIfNeeded( Path, Data, Size );
}

inline bool Boot( ) {
    char Temp[ MAX_PATH ] = { };
    if ( !GetTempPathA( MAX_PATH, Temp ) )
        return false;

    std::string Root = std::string( Temp ) + "ff0l-rt";
    std::string Fonts = Root + "\\assets\\fonts";
    std::string Icons = Root + "\\assets\\icons\\fontawesome";
    CreateDirectoryA( Root.c_str( ), nullptr );
    CreateDirectoryA( ( Root + "\\assets" ).c_str( ), nullptr );
    CreateDirectoryA( Fonts.c_str( ), nullptr );
    CreateDirectoryA( ( Root + "\\assets\\icons" ).c_str( ), nullptr );
    CreateDirectoryA( Icons.c_str( ), nullptr );

    Drop( IDR_POPPINS_REGULAR, Fonts + "\\Poppins-Regular.ttf" );
    Drop( IDR_POPPINS_MEDIUM, Fonts + "\\Poppins-Medium.ttf" );
    Drop( IDR_POPPINS_SEMIBOLD, Fonts + "\\Poppins-SemiBold.ttf" );
    Drop( IDR_FA_SOLID, Icons + "\\fa-solid-900.woff2" );
    Drop( IDR_FA_REGULAR, Icons + "\\fa-regular-400.woff2" );
    Drop( IDR_FA_LIGHT, Icons + "\\fa-light-300.woff2" );

    ur::config::set_asset_root( Root.c_str( ) );

    static const int FaceId[ 3 ] = { IDR_POPPINS_REGULAR, IDR_POPPINS_MEDIUM, IDR_POPPINS_SEMIBOLD };
    for ( int Index = 0; Index < 3; Index++ ) {
        const void* Data = nullptr;
        DWORD Size = 0;
        if ( !Rc( FaceId[ Index ], Data, Size ) )
            continue;
        DWORD Count = 0;
        FaceMem[ Index ] = AddFontMemResourceEx( ( void* )Data, Size, nullptr, &Count );
    }
    return true;
}

inline void Shutdown( ) {
    for ( int Index = 0; Index < 3; Index++ ) {
        if ( FaceMem[ Index ] ) {
            RemoveFontMemResourceEx( FaceMem[ Index ] );
            FaceMem[ Index ] = nullptr;
        }
    }
}

}
