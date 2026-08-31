#include "Arena.h"
#include "Context.h"

#include <cstdio>

#if defined( _WIN32 )

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#endif

bool CContext::Create( ) {
    Previous = std::chrono::steady_clock::now( );
    Started = true;

    return Started;
}

void CContext::Destroy( ) {
    Owners.clear( );
    Faults.clear( );

    HoveredItem = 0;
    ActiveItem = 0;

    FrameIndex = 0;
    Shielding = false;

    Alarm = nullptr;
}

void CContext::Begin( ) {
    if ( !Started )
        return;

    std::chrono::steady_clock::time_point Moment = std::chrono::steady_clock::now( );

    double Passed = std::chrono::duration< double >( Moment - Previous ).count( );
    Previous = Moment;

    Elapsed += Passed;
    DeltaTime = ( float )Passed;

    if ( DeltaTime > 0.1f )
        DeltaTime = 0.1f;

    if ( DeltaTime < 0.0f )
        DeltaTime = 0.0f;

    SampleTime += Passed;
    SampleCount++;

    if ( SampleTime >= 0.25 ) {
        Framerate = ( float )( ( double )SampleCount / SampleTime );

        SampleTime = 0.0;
        SampleCount = 0;
    }

    FrameIndex++;
    HoveredItem = 0;

    WantKeyboard = false;
    FocusClaimed = false;

    Owners.clear( );
}

unsigned int CContext::Hash( const char* Name ) const {
    unsigned int Value = Owners.empty( ) ? 2166136261u : Owners.back( );
    if ( !Name )
        return Value;

    const char* Cursor = Name;

    for ( const char* Scan = Name; *Scan; Scan++ ) {
        if ( Scan[ 0 ] == '#' && Scan[ 1 ] == '#' && Scan[ 2 ] == '#' )
            Cursor = Scan + 3;
    }

    while ( *Cursor ) {
        Value = ( Value ^ ( unsigned int )( unsigned char )*Cursor ) * 16777619u;
        Cursor++;
    }

    return Value;
}

const char* CContext::Caption( const char* Name ) {
    if ( !Name )
        return "";

    for ( const char* Scan = Name; *Scan; Scan++ ) {
        if ( Scan[ 0 ] == '#' && Scan[ 1 ] == '#' ) {
            int Length = ( int )( Scan - Name );
            char* Copy = ( char* )Arena->Allocate( ( size_t )Length + 1 );

            for ( int Position = 0; Position < Length; Position++ )
                Copy[ Position ] = Name[ Position ];

            Copy[ Length ] = 0;
            return Copy;
        }
    }

    return Name;
}

void CContext::PushOwner( unsigned int Identifier ) {
    Owners.push_back( Identifier );
}

void CContext::PopOwner( ) {
    if ( !Owners.empty( ) )
        Owners.pop_back( );
}

void CContext::PushIdentifier( const char* Name ) {
    Owners.push_back( Hash( Name ) );
}

void CContext::PushIdentifier( int Number ) {
    unsigned int Value = Owners.empty( ) ? 2166136261u : Owners.back( );
    Value = ( Value ^ ( unsigned int )Number ) * 16777619u;

    Owners.push_back( Value );
}

void CContext::PopIdentifier( ) {
    if ( !Owners.empty( ) )
        Owners.pop_back( );
}

bool CContext::Covered( CVector Point ) const {
    return Shielding && FrameIndex - ShieldStamp <= 1 && Shield.Contains( Point );
}

void CContext::Report( const char* Message ) {
    if ( !Message )
        return;

    CFault Fault;

    Fault.Message = Message;
    Fault.Frame = FrameIndex;

    Faults.push_back( Fault );
    if ( Faults.size( ) > 64 )
        Faults.erase( Faults.begin( ) );

#if defined( _WIN32 )
    OutputDebugStringA( Message );
    OutputDebugStringA( "\n" );
#else
    std::fprintf( stderr, "%s\n", Message );
#endif

    if ( Alarm )
        Alarm( Message );
}