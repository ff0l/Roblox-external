#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "Geometry.h"

class CFault {
public:
    std::string Message;
    unsigned int Frame = 0;
};

class CContext {
public:
    bool Create( );
    void Destroy( );

    void Begin( );

    unsigned int Hash( const char* Name ) const;
    const char* Caption( const char* Name );

    void PushOwner( unsigned int Identifier );
    void PopOwner( );

    void PushIdentifier( const char* Name );
    void PushIdentifier( int Number );

    void PopIdentifier( );

    bool Covered( CVector Point ) const;
    void Report( const char* Message );

    unsigned int HoveredItem = 0;
    unsigned int ActiveItem = 0;

    unsigned int FocusedItem = 0;
    unsigned int FrameIndex = 0;

    unsigned int ShieldStamp = 0;

    bool WantKeyboard = false;
    bool FocusClaimed = false;

    float DeltaTime = 0.0f;
    double Elapsed = 0.0;
    float Framerate = 0.0f;

    bool Shielding = false;

    CRectangle Shield;
    CVector Display;

    void ( *Alarm )( const char* Message ) = nullptr;
    std::vector< CFault > Faults;

private:
    std::chrono::steady_clock::time_point Previous;
    bool Started = false;

    double SampleTime = 0.0;
    int SampleCount = 0;

    std::vector< unsigned int > Owners;
};

inline auto Context = std::make_unique< CContext >( );