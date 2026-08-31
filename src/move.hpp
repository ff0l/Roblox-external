#pragma once

#include <Windows.h>

#include "world.hpp"

namespace move {

inline constexpr int ClipMax = 48;

struct Cfg {
    bool jump = false;
    bool infJump = false;
    bool noclip = false;
    float jumpPower = 90.0f;
};

inline Cfg& Live( ) {
    static Cfg Store;
    return Store;
}

inline bool Want( ) {
    const Cfg& C = Live( );
    return C.jump || C.infJump || C.noclip;
}

inline bool Down( int Key ) {
    return ( GetAsyncKeyState( Key ) & 0x8000 ) != 0;
}

struct State {
    bool jumpOn = false;
    bool clipOn = false;
    bool spaceWas = false;
    float savedJump = 50.0f;
    float savedJumpH = 7.2f;
    uintptr_t clipPart[ ClipMax ] = { };
    int clipN = 0;
};

inline State& Run( ) {
    static State Store;
    return Store;
}

inline void WriteJump( uintptr_t Hum, float Power ) {
    world::Off& O = world::Core( ).off;
    if ( !Hum || !O.humanoidJump )
        return;
    if ( O.humanoidUseJump ) {
        bool Use = true;
        world::Poke( Hum + O.humanoidUseJump, Use );
    }
    world::Poke( Hum + O.humanoidJump, Power );
    if ( O.humanoidJumpH )
        world::Poke( Hum + O.humanoidJumpH, Power * 0.144f );
}

inline void RequestJump( uintptr_t Hum ) {
    world::Off& O = world::Core( ).off;
    if ( !Hum )
        return;
    if ( O.humanoidJumpReq ) {
        bool On = true;
        world::Poke( Hum + O.humanoidJumpReq, On );
    }
    if ( O.humanoidState && O.humanoidStateId ) {
        uintptr_t State = world::Ptr( Hum + O.humanoidState );
        if ( State ) {
            int Jumping = 3;
            world::Poke( State + O.humanoidStateId, Jumping );
        }
    }
}

inline void RestoreJump( ) {
    State& S = Run( );
    if ( !S.jumpOn )
        return;
    WriteJump( world::Core( ).localHum, S.savedJump );
    S.jumpOn = false;
}

inline void RestoreClip( ) {
    State& S = Run( );
    for ( int Index = 0; Index < S.clipN; Index++ )
        world::SetCollide( S.clipPart[ Index ], true );
    S.clipN = 0;
    S.clipOn = false;
}

inline void RestoreAll( ) {
    RestoreJump( );
    RestoreClip( );
}

inline void ScanClip( uintptr_t Model ) {
    State& S = Run( );
    S.clipN = 0;
    if ( !Model )
        return;
    uintptr_t List[ world::KidMax ];
    int Count = world::Kids( Model, List, world::KidMax );
    for ( int Index = 0; Index < Count && S.clipN < ClipMax; Index++ ) {
        char Kind[ 40 ] = { };
        if ( !world::ClassName( List[ Index ], Kind, ( int )sizeof( Kind ) ) )
            continue;
        if ( !world::IsSolid( Kind ) )
            continue;
        S.clipPart[ S.clipN++ ] = List[ Index ];
    }
}

inline void TickJump( bool Active ) {
    State& S = Run( );
    Cfg& C = Live( );
    world::Engine& E = world::Core( );
    if ( !Active ) {
        RestoreJump( );
        return;
    }
    if ( !E.localHum )
        return;
    if ( !S.jumpOn ) {
        if ( E.off.humanoidJump )
            world::Pull( E.localHum + E.off.humanoidJump, S.savedJump );
        if ( E.off.humanoidJumpH )
            world::Pull( E.localHum + E.off.humanoidJumpH, S.savedJumpH );
        S.jumpOn = true;
    }
    WriteJump( E.localHum, C.jumpPower );
}

inline void TickInfJump( ) {
    State& S = Run( );
    Cfg& C = Live( );
    world::Engine& E = world::Core( );
    bool Space = Down( VK_SPACE );
    bool Edge = Space && !S.spaceWas;
    S.spaceWas = Space;
    if ( !E.localRoot )
        return;
    bool Repeat = C.infJump && Space;
    bool Once = C.jump && !C.infJump && Edge;
    if ( !Repeat && !Once )
        return;
    RequestJump( E.localHum );
    world::Vec3 Vel;
    if ( !world::PartVel( E.localRoot, Vel ) )
        return;
    float Power = C.jump ? C.jumpPower : 50.0f;
    if ( Vel.y < Power )
        Vel.y = Power;
    world::WritePartVel( E.localRoot, Vel );
}

inline void TickClip( bool Active ) {
    State& S = Run( );
    world::Engine& E = world::Core( );
    if ( !Active ) {
        RestoreClip( );
        return;
    }
    if ( !E.localModel )
        return;
    static unsigned Last = 0;
    unsigned Now = GetTickCount( );
    if ( !S.clipOn || Now - Last > 350 ) {
        ScanClip( E.localModel );
        Last = Now;
        S.clipOn = true;
    }
    for ( int Index = 0; Index < S.clipN; Index++ )
        world::SetCollide( S.clipPart[ Index ], false );
}

inline void Tick( float Dt, bool Busy ) {
    ( void )Dt;
    if ( !Want( ) ) {
        RestoreAll( );
        return;
    }
    world::EnsureWrite( );
    world::Engine& E = world::Core( );
    if ( !E.localRoot || !E.localHum || !E.localModel )
        world::BindLocal( );
    TickJump( Live( ).jump && !Busy );
    if ( !Busy )
        TickInfJump( );
    TickClip( Live( ).noclip && !Busy );
}

inline void Clamp( ) {
    Cfg& C = Live( );
    if ( C.jumpPower < 1.0f )
        C.jumpPower = 1.0f;
    if ( C.jumpPower > 500.0f )
        C.jumpPower = 500.0f;
}

}
