#pragma once

#include "offsets.hpp"
#include "world.hpp"

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#ifndef CFG_CALL_TARGET_VALID
#define CFG_CALL_TARGET_VALID 0x00000001
#endif

namespace silent {

inline constexpr int Abi = 0;
inline constexpr uintptr_t DescFallback = 0x82012A0;
inline constexpr uintptr_t FnFallback = 0x80;
inline constexpr size_t StubBytes = 0x100;

#pragma pack( push, 4 )
struct RayState {
    uint32_t active = 0;
    uint32_t reserved = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float scale = 1.15f;
    uint64_t calls = 0;
};
#pragma pack( pop )

static_assert( offsetof( RayState, active ) == 0x00, "active" );
static_assert( offsetof( RayState, x ) == 0x08, "target" );
static_assert( offsetof( RayState, calls ) == 0x18, "calls" );

struct Hook {
    uintptr_t thunk = 0;
    uintptr_t state = 0;
    uintptr_t original = 0;
    uintptr_t module = 0;
    uintptr_t slot = 0;
    bool owned = false;
    bool installed = false;
    bool active = false;
    uintptr_t cam = 0;
    int16_t savedView[ 2 ] = { };
    bool haveView = false;
    uintptr_t mouseObj = 0;
    float savedMouse[ 2 ] = { };
    bool haveMouse = false;
    bool pinned = false;
    unsigned lastFail = 0;
};

inline Hook& Live( ) {
    static Hook Store;
    return Store;
}

inline HANDLE Proc( ) {
    return world::Core( ).process;
}

inline bool OkAddr( uintptr_t Addr ) {
    return world::Heap( Addr );
}

inline size_t Page( ) {
    static size_t Size = 0;
    if ( !Size ) {
        SYSTEM_INFO Info = { };
        GetSystemInfo( &Info );
        Size = Info.dwPageSize ? ( size_t )Info.dwPageSize : 0x1000u;
    }
    return Size;
}

inline bool WriteRaw( uintptr_t Addr, const void* Data, size_t Size ) {
    if ( !OkAddr( Addr ) || !Data || !Size || !Proc( ) )
        return false;
    return world::Poke( Addr, Data, Size );
}

inline DWORD QueryProtect( uintptr_t Addr ) {
    MEMORY_BASIC_INFORMATION Info = { };
    if ( !Proc( ) || !VirtualQueryEx( Proc( ), ( void* )Addr, &Info, sizeof( Info ) ) )
        return 0;
    return Info.Protect;
}

inline bool ExecProtect( DWORD Protect ) {
    DWORD Kind = Protect & 0xFF;
    return Kind == PAGE_EXECUTE || Kind == PAGE_EXECUTE_READ
        || Kind == PAGE_EXECUTE_READWRITE || Kind == PAGE_EXECUTE_WRITECOPY;
}

inline bool CodeInImage( uintptr_t Addr ) {
    world::Engine& E = world::Core( );
    if ( !E.base || !E.size || Addr < E.base || Addr >= E.base + E.size )
        return false;
    return ExecProtect( QueryProtect( Addr ) );
}

inline bool Protect( uintptr_t Addr, size_t Size, DWORD Want ) {
    if ( !OkAddr( Addr ) || !Size || !Proc( ) )
        return false;
    uintptr_t Base = Addr & ~( ( uintptr_t )Page( ) - 1 );
    size_t Span = ( size_t )( ( ( Addr + Size + Page( ) - 1 ) & ~( Page( ) - 1 ) ) - Base );
    DWORD Old = 0;
    return VirtualProtectEx( Proc( ), ( void* )Base, Span, Want, &Old ) != 0;
}

inline bool MarkCfg( uintptr_t Target ) {
    auto Resolve = [ ]( ) -> FARPROC {
        const char* Mods[ ] = { "kernelbase.dll", "kernel32.dll", "api-ms-win-core-memory-l1-1-3.dll" };
        for ( int Index = 0; Index < 3; Index++ ) {
            HMODULE Mod = GetModuleHandleA( Mods[ Index ] );
            if ( !Mod )
                Mod = LoadLibraryA( Mods[ Index ] );
            if ( !Mod )
                continue;
            if ( FARPROC Have = GetProcAddress( Mod, "SetProcessValidCallTargets" ) )
                return Have;
        }
        return nullptr;
    };
    FARPROC ProcFn = Resolve( );
    if ( !ProcFn )
        return false;
    struct Info {
        ULONG_PTR Offset;
        ULONG Flags;
    } Item{ };
    Item.Offset = Target & ( Page( ) - 1 );
    Item.Flags = CFG_CALL_TARGET_VALID;
    using Fn = BOOL ( WINAPI* )( HANDLE, PVOID, SIZE_T, ULONG, void* );
    return reinterpret_cast< Fn >( ProcFn )( Proc( ), ( void* )( Target & ~( ( uintptr_t )Page( ) - 1 ) ), Page( ), 1, &Item ) != 0;
}

inline void AppendU64( std::vector< uint8_t >& Code, uint64_t Value ) {
    const uint8_t* Bytes = reinterpret_cast< const uint8_t* >( &Value );
    Code.insert( Code.end( ), Bytes, Bytes + 8 );
}

inline void PatchRel32( std::vector< uint8_t >& Code, size_t At, size_t Till ) {
    int32_t Rel = ( int32_t )( ( ptrdiff_t )Till - ( ptrdiff_t )( At + 4 ) );
    memcpy( Code.data( ) + At, &Rel, 4 );
}

inline std::vector< uint8_t > MakeThunk( uintptr_t State, uintptr_t Orig ) {
    std::vector< uint8_t > Code;
    Code.reserve( 256 );
    std::vector< size_t > Skips;
    auto Jz = [ & ] {
        Code.insert( Code.end( ), { 0x0F, 0x84 } );
        Skips.push_back( Code.size( ) );
        Code.insert( Code.end( ), { 0, 0, 0, 0 } );
    };
    auto Jbe = [ & ] {
        Code.insert( Code.end( ), { 0x0F, 0x86 } );
        Skips.push_back( Code.size( ) );
        Code.insert( Code.end( ), { 0, 0, 0, 0 } );
    };

    Code.insert( Code.end( ), { 0x48, 0x83, 0xEC, 0x68 } );
    Code.insert( Code.end( ), { 0x49, 0xBA } );
    AppendU64( Code, State );
    Code.insert( Code.end( ), { 0x41, 0x83, 0x3A, 0x00 } );
    Jz( );

    if ( Abi == 0 ) {
        Code.insert( Code.end( ), { 0x4D, 0x85, 0xC0 } ); Jz( );
        Code.insert( Code.end( ), { 0x4D, 0x85, 0xC9 } ); Jz( );
    } else {
        Code.insert( Code.end( ), { 0x48, 0x85, 0xD2 } ); Jz( );
        Code.insert( Code.end( ), { 0x4D, 0x85, 0xC0 } ); Jz( );
    }

    if ( Abi == 0 ) {
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x10, 0x29 } );
        Code.insert( Code.end( ), { 0xF3, 0x0F, 0x59, 0xED } );
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x10, 0x61, 0x04 } );
        Code.insert( Code.end( ), { 0xF3, 0x0F, 0x59, 0xE4 } );
        Code.insert( Code.end( ), { 0xF3, 0x0F, 0x58, 0xEC } );
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x10, 0x61, 0x08 } );
    } else {
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x10, 0x28 } );
        Code.insert( Code.end( ), { 0xF3, 0x0F, 0x59, 0xED } );
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x10, 0x60, 0x04 } );
        Code.insert( Code.end( ), { 0xF3, 0x0F, 0x59, 0xE4 } );
        Code.insert( Code.end( ), { 0xF3, 0x0F, 0x58, 0xEC } );
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x10, 0x60, 0x08 } );
    }
    Code.insert( Code.end( ), { 0xF3, 0x0F, 0x59, 0xE4 } );
    Code.insert( Code.end( ), { 0xF3, 0x0F, 0x58, 0xEC } );
    Code.insert( Code.end( ), { 0xF3, 0x0F, 0x51, 0xED } );

    Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x10, 0x42, 0x08 } );
    if ( Abi == 0 )
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x5C, 0x00 } );
    else
        Code.insert( Code.end( ), { 0xF3, 0x0F, 0x5C, 0x02 } );

    Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x10, 0x4A, 0x0C } );
    if ( Abi == 0 )
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x5C, 0x48, 0x04 } );
    else
        Code.insert( Code.end( ), { 0xF3, 0x0F, 0x5C, 0x4A, 0x04 } );

    Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x10, 0x52, 0x10 } );
    if ( Abi == 0 )
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x5C, 0x50, 0x08 } );
    else
        Code.insert( Code.end( ), { 0xF3, 0x0F, 0x5C, 0x52, 0x08 } );

    Code.insert( Code.end( ), { 0x0F, 0x28, 0xD8, 0xF3, 0x0F, 0x59, 0xDB } );
    Code.insert( Code.end( ), { 0x0F, 0x28, 0xE1, 0xF3, 0x0F, 0x59, 0xE4 } );
    Code.insert( Code.end( ), { 0xF3, 0x0F, 0x58, 0xDC } );
    Code.insert( Code.end( ), { 0x0F, 0x28, 0xE2, 0xF3, 0x0F, 0x59, 0xE4 } );
    Code.insert( Code.end( ), { 0xF3, 0x0F, 0x58, 0xDC } );
    Code.insert( Code.end( ), { 0xF3, 0x0F, 0x51, 0xDB } );
    Code.insert( Code.end( ), { 0x0F, 0x57, 0xE4 } );
    Code.insert( Code.end( ), { 0x0F, 0x2E, 0xDC } );
    Jbe( );

    Code.insert( Code.end( ), { 0xF3, 0x0F, 0x5E, 0xEB } );
    Code.insert( Code.end( ), { 0xF3, 0x0F, 0x59, 0xC5 } );
    Code.insert( Code.end( ), { 0xF3, 0x0F, 0x59, 0xCD } );
    Code.insert( Code.end( ), { 0xF3, 0x0F, 0x59, 0xD5 } );

    if ( Abi == 0 ) {
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x11, 0x01 } );
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x11, 0x49, 0x04 } );
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x11, 0x51, 0x08 } );
    } else {
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x11, 0x00 } );
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x11, 0x48, 0x04 } );
        Code.insert( Code.end( ), { 0xF3, 0x41, 0x0F, 0x11, 0x50, 0x08 } );
    }

    Code.insert( Code.end( ), { 0x49, 0xFF, 0x42, 0x18 } );

    const size_t Skip = Code.size( );
    for ( size_t At : Skips )
        PatchRel32( Code, At, Skip );

    if ( Abi == 0 ) {
        Code.insert( Code.end( ), { 0x48, 0x8B, 0x84, 0x24, 0x90, 0x00, 0x00, 0x00 } );
        Code.insert( Code.end( ), { 0x48, 0x89, 0x44, 0x24, 0x20 } );
    }

    Code.insert( Code.end( ), { 0x48, 0xB8 } );
    AppendU64( Code, Orig );
    Code.insert( Code.end( ), { 0xFF, 0xD0 } );
    Code.insert( Code.end( ), { 0x48, 0x83, 0xC4, 0x68 } );
    Code.push_back( 0xC3 );
    return Code;
}

inline bool RegionPad( uintptr_t Addr, size_t Need ) {
    uint8_t Buf[ 256 ];
    if ( Need > sizeof( Buf ) || !world::Pull( Addr, Buf, Need ) )
        return false;
    bool Cc = true;
    bool Zero = true;
    for ( size_t Index = 0; Index < Need; Index++ ) {
        if ( Buf[ Index ] != 0xCC )
            Cc = false;
        if ( Buf[ Index ] != 0x00 )
            Zero = false;
    }
    return Cc || Zero;
}

inline uintptr_t FindCave( size_t Need ) {
    MEMORY_BASIC_INFORMATION Info = { };
    uintptr_t Addr = 0;
    uintptr_t Fallback = 0;
    HANDLE Handle = Proc( );
    if ( !Handle )
        return 0;
    while ( VirtualQueryEx( Handle, ( void* )Addr, &Info, sizeof( Info ) ) ) {
        uintptr_t Base = ( uintptr_t )Info.BaseAddress;
        size_t Size = ( size_t )Info.RegionSize;
        Addr = Base + Size;
        if ( Addr < Base )
            break;
        if ( Info.State != MEM_COMMIT )
            continue;
        if ( Info.Protect & ( PAGE_GUARD | PAGE_NOACCESS ) )
            continue;
        if ( !ExecProtect( Info.Protect ) || Size < Need )
            continue;
        Hook& H = Live( );
        if ( H.thunk && Base <= H.thunk && H.thunk < Addr )
            continue;
        for ( size_t Off = 0; Off + Need <= Size; Off += 0x10 ) {
            uintptr_t Cand = Base + Off;
            if ( RegionPad( Cand, Need ) )
                return Cand;
        }
        if ( !Fallback && Info.Type == MEM_PRIVATE && ( Info.Protect & 0xFF ) == PAGE_EXECUTE_READWRITE && Size >= Need + 0x40 )
            Fallback = Base + Size - Need;
    }
    return Fallback;
}

inline uintptr_t AllocExec( ) {
    HANDLE Handle = Proc( );
    if ( !Handle )
        return 0;
    void* PagePtr = VirtualAllocEx( Handle, nullptr, Page( ), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
    if ( !PagePtr )
        return 0;
    uintptr_t Addr = ( uintptr_t )PagePtr;
    if ( !ExecProtect( QueryProtect( Addr ) ) ) {
        VirtualFreeEx( Handle, PagePtr, 0, MEM_RELEASE );
        return 0;
    }
    return Addr;
}

inline uintptr_t AllocRw( ) {
    HANDLE Handle = Proc( );
    if ( !Handle )
        return 0;
    void* PagePtr = VirtualAllocEx( Handle, nullptr, Page( ), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
    return ( uintptr_t )PagePtr;
}

inline uintptr_t DescRva( ) {
    uintptr_t Have = offsets::Get( "Raycast", "RaycastBoundDesc" );
    if ( !Have )
        Have = offsets::Get( "WorldRoot", "RaycastBoundDesc" );
    return Have;
}

inline uintptr_t FnOff( ) {
    uintptr_t Have = offsets::Get( "Raycast", "RaycastBoundFn" );
    if ( !Have )
        Have = offsets::Get( "WorldRoot", "RaycastBoundFn" );
    if ( !Have )
        Have = FnFallback;
    return Have;
}

inline bool Ready( ) {
    return Live( ).installed;
}

inline uintptr_t Camera( ) {
    world::Engine& E = world::Core( );
    if ( !E.workspace || !E.off.workspaceCam )
        return 0;
    return world::Ptr( E.workspace + E.off.workspaceCam );
}

inline void RestoreMouse( ) {
    Hook& H = Live( );
    if ( !H.haveMouse ) {
        H.pinned = false;
        return;
    }
    if ( H.pinned && H.mouseObj && world::Core( ).off.mousePos )
        world::Poke( H.mouseObj + world::Core( ).off.mousePos, H.savedMouse, sizeof( H.savedMouse ) );
    H.haveMouse = false;
    H.pinned = false;
    H.mouseObj = 0;
}

inline bool MouseObjOk( uintptr_t Obj ) {
    if ( !Obj || ( Obj & 7ull ) || !world::Heap( Obj ) || !world::Core( ).off.mousePos )
        return false;
    float Pos[ 2 ] = { };
    if ( !world::Pull( Obj + world::Core( ).off.mousePos, Pos, sizeof( Pos ) ) )
        return false;
    return Pos[ 0 ] > -200.0f && Pos[ 0 ] < 10000.0f && Pos[ 1 ] > -200.0f && Pos[ 1 ] < 10000.0f;
}

inline void RestoreView( ) {
    Hook& H = Live( );
    RestoreMouse( );
    if ( !H.haveView || !H.cam || !world::Core( ).off.camView )
        return;
    world::Poke( H.cam + world::Core( ).off.camView, H.savedView, sizeof( H.savedView ) );
    H.haveView = false;
    H.cam = 0;
}

inline void Off( ) {
    Hook& H = Live( );
    RestoreView( );
    if ( !H.active || !H.state )
        return;
    uint32_t Zero = 0;
    WriteRaw( H.state, &Zero, sizeof( Zero ) );
    H.active = false;
}

inline void Remove( ) {
    Hook& H = Live( );
    if ( H.installed && OkAddr( H.original ) && H.slot ) {
        Protect( H.slot, 8, PAGE_READWRITE );
        WriteRaw( H.slot, &H.original, sizeof( H.original ) );
    }
    if ( H.thunk && !H.owned ) {
        uint8_t Pad[ StubBytes ];
        memset( Pad, 0xCC, sizeof( Pad ) );
        DWORD Old = QueryProtect( H.thunk );
        Protect( H.thunk, StubBytes, PAGE_EXECUTE_READWRITE );
        WriteRaw( H.thunk, Pad, sizeof( Pad ) );
        if ( Old )
            Protect( H.thunk, StubBytes, Old );
    }
    HANDLE Handle = Proc( );
    if ( Handle && H.thunk && H.owned )
        VirtualFreeEx( Handle, ( void* )H.thunk, 0, MEM_RELEASE );
    if ( Handle && H.state )
        VirtualFreeEx( Handle, ( void* )H.state, 0, MEM_RELEASE );
    H = { };
}

inline bool Install( ) {
    Hook& H = Live( );
    if ( H.installed )
        return true;
    if ( !world::EnsureWrite( ) )
        return false;
    world::Engine& E = world::Core( );
    if ( !E.base || !DescRva( ) ) {
        unsigned Now = GetTickCount( );
        H.lastFail = Now;
        return false;
    }
    unsigned Now = GetTickCount( );
    if ( H.lastFail && Now - H.lastFail < 1500 )
        return false;

    uintptr_t Slot = E.base + DescRva( ) + FnOff( );
    uintptr_t Fn = 0;
    world::Pull( Slot, Fn );
    if ( !CodeInImage( Fn ) ) {
        H.lastFail = Now;
        return false;
    }

    if ( !H.state )
        H.state = AllocRw( );
    if ( !H.state ) {
        H.lastFail = Now;
        return false;
    }

    std::vector< uint8_t > Thunk = MakeThunk( H.state, Fn );
    bool Owned = false;
    uintptr_t Stub = FindCave( StubBytes );
    if ( !Stub ) {
        Stub = AllocExec( );
        Owned = Stub != 0;
    }
    if ( !Stub ) {
        H.lastFail = Now;
        return false;
    }

    DWORD CaveProt = QueryProtect( Stub );
    if ( !Owned )
        Protect( Stub, Thunk.size( ), PAGE_EXECUTE_READWRITE );

    RayState Empty{ };
    if ( !WriteRaw( Stub, Thunk.data( ), Thunk.size( ) ) || !WriteRaw( H.state, &Empty, sizeof( Empty ) ) ) {
        H.lastFail = Now;
        if ( !Owned && CaveProt )
            Protect( Stub, Thunk.size( ), CaveProt );
        if ( Owned )
            VirtualFreeEx( Proc( ), ( void* )Stub, 0, MEM_RELEASE );
        return false;
    }
    if ( !Owned && CaveProt )
        Protect( Stub, Thunk.size( ), CaveProt );
    FlushInstructionCache( Proc( ), ( void* )Stub, Thunk.size( ) );
    MarkCfg( Stub );
    if ( !ExecProtect( QueryProtect( Stub ) ) ) {
        H.lastFail = Now;
        if ( Owned )
            VirtualFreeEx( Proc( ), ( void* )Stub, 0, MEM_RELEASE );
        return false;
    }

    Protect( Slot, 8, PAGE_READWRITE );
    uintptr_t Check = 0;
    if ( !WriteRaw( Slot, &Stub, sizeof( Stub ) ) || !world::Pull( Slot, Check ) || Check != Stub ) {
        H.lastFail = Now;
        if ( Owned )
            VirtualFreeEx( Proc( ), ( void* )Stub, 0, MEM_RELEASE );
        return false;
    }

    H.module = E.base;
    H.original = Fn;
    H.thunk = Stub;
    H.slot = Slot;
    H.owned = Owned;
    H.installed = true;
    H.active = false;
    return true;
}

inline bool PokeMouse( uintptr_t Obj, float X, float Y ) {
    world::Engine& E = world::Core( );
    if ( !Obj || !E.off.mousePos || !world::Heap( Obj ) )
        return false;
    float Pos[ 2 ] = { X, Y };
    return world::Poke( Obj + E.off.mousePos, Pos, sizeof( Pos ) );
}

inline bool MouseAt( float X, float Y ) {
    world::Engine& E = world::Core( );
    if ( !world::EnsureWrite( ) )
        return false;
    if ( !E.mouseSvc || !world::Heap( E.mouseSvc ) ) {
        E.mouseSvc = 0;
        if ( E.dataModel ) {
            E.mouseSvc = world::FindService( E.dataModel, "MouseService" );
            if ( !E.mouseSvc )
                E.mouseSvc = world::FindService( E.dataModel, "UserInputService" );
        }
    }
    if ( !E.mouseSvc || !E.off.mousePos )
        return false;
    uintptr_t A = E.off.mouseObj ? world::Ptr( E.mouseSvc + E.off.mouseObj ) : 0;
    uintptr_t B = E.off.mouseObj2 ? world::Ptr( E.mouseSvc + E.off.mouseObj2 ) : 0;
    bool Ok = false;
    if ( A )
        Ok = PokeMouse( A, X, Y ) || Ok;
    if ( B && B != A )
        Ok = PokeMouse( B, X, Y ) || Ok;
    return Ok;
}

inline bool WarpView( float Tx, float Ty, float Cx, float Cy, float Sw, float Sh ) {
    world::Engine& E = world::Core( );
    if ( !E.off.camView || Sw < 8.0f || Sh < 8.0f )
        return false;
    if ( !world::EnsureWrite( ) )
        return false;
    uintptr_t Cam = Camera( );
    if ( !Cam )
        return false;
    if ( Ty < 1.0f )
        Ty = 1.0f;
    if ( Ty > Sh - 1.0f )
        Ty = Sh - 1.0f;
    if ( Cy < 1.0f )
        Cy = 1.0f;
    if ( Cy > Sh - 1.0f )
        Cy = Sh - 1.0f;
    float Ratio = Cy / Ty;
    float Vy = Sh * Ratio;
    if ( Vy > 32767.0f )
        Vy = 32767.0f;
    if ( Vy < 1.0f )
        Vy = 1.0f;
    Ratio = Vy / Sh;
    float Vx = 2.0f * Cx - Ratio * ( 2.0f * Tx - Sw );
    if ( Vx > 32767.0f )
        Vx = 32767.0f;
    if ( Vx < 1.0f )
        Vx = 1.0f;
    int16_t Next[ 2 ] = { ( int16_t )( Vx + 0.5f ), ( int16_t )( Vy + 0.5f ) };
    Hook& H = Live( );
    if ( !H.haveView || H.cam != Cam ) {
        int16_t Have[ 2 ] = { };
        if ( world::Pull( Cam + E.off.camView, Have, sizeof( Have ) ) ) {
            H.savedView[ 0 ] = Have[ 0 ];
            H.savedView[ 1 ] = Have[ 1 ];
            H.haveView = true;
            H.cam = Cam;
        }
    }
    if ( !world::Poke( Cam + E.off.camView, Next, sizeof( Next ) ) )
        return false;
    return true;
}

inline bool On( const world::Vec3& Target, float ScreenX, float ScreenY, bool Warp = true ) {
    ( void )Warp;
    Hook& H = Live( );
    bool Hooked = false;
    unsigned Now = GetTickCount( );
    if ( H.installed || !H.lastFail || Now - H.lastFail >= 2000 )
        Hooked = Install( );
    if ( Hooked ) {
        float Pos[ 3 ] = { Target.x, Target.y, Target.z };
        uint32_t One = 1;
        WriteRaw( H.state + offsetof( RayState, x ), Pos, sizeof( Pos ) );
        WriteRaw( H.state + offsetof( RayState, active ), &One, sizeof( One ) );
        H.active = true;
    }
    const world::Snap& Snap = world::View( );
    float Sw = ( float )( Snap.viewW > 8 ? Snap.viewW : Snap.clientW );
    float Sh = ( float )( Snap.viewH > 8 ? Snap.viewH : Snap.clientH );
    float Tx = ScreenX;
    float Ty = ScreenY;
    if ( Snap.viewW > 8 && Snap.clientW > 8 && Snap.viewW != Snap.clientW ) {
        Tx *= ( float )Snap.clientW / ( float )Snap.viewW;
        Ty *= ( float )Snap.clientH / ( float )Snap.viewH;
        Sw = ( float )Snap.clientW;
        Sh = ( float )Snap.clientH;
    }
    float Cx = Sw * 0.5f;
    float Cy = Sh * 0.5f;
    bool Mouse = false;
    if ( Tx > 1.0f && Ty > 1.0f && Tx < Sw + 200.0f && Ty < Sh + 200.0f )
        Mouse = MouseAt( Tx, Ty );
    bool View = WarpView( Tx, Ty, Cx, Cy, Sw, Sh );
    return Hooked || Mouse || View;
}

}
