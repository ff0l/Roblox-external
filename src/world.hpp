#pragma once

#include <Windows.h>
#include <TlHelp32.h>

#include <cmath>
#include <cstdint>
#include <cstring>

#include "offsets.hpp"
#include "sense.hpp"

namespace world {

inline constexpr int ActorMax = 64;
inline constexpr int BoneMax = 16;
inline constexpr int KidMax = 256;
inline constexpr int WallMax = 256;

enum Bone : int {
    BoneHead = 0,
    BoneRoot,
    BoneUpper,
    BoneLower,
    BoneLArmU,
    BoneLArmL,
    BoneLHand,
    BoneRArmU,
    BoneRArmL,
    BoneRHand,
    BoneLLegU,
    BoneLLegL,
    BoneLFoot,
    BoneRLegU,
    BoneRLegL,
    BoneRFoot
};

inline const char* BoneName[ BoneMax ] = {
    "Head", "HumanoidRootPart", "UpperTorso", "LowerTorso",
    "LeftUpperArm", "LeftLowerArm", "LeftHand",
    "RightUpperArm", "RightLowerArm", "RightHand",
    "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
    "RightUpperLeg", "RightLowerLeg", "RightFoot"
};

inline const char* BoneNameR6[ BoneMax ] = {
    "Head", "HumanoidRootPart", "Torso", "Torso",
    "Left Arm", "Left Arm", "Left Arm",
    "Right Arm", "Right Arm", "Right Arm",
    "Left Leg", "Left Leg", "Left Leg",
    "Right Leg", "Right Leg", "Right Leg"
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Dot {
    float x = 0.0f;
    float y = 0.0f;
    bool ok = false;
};

struct Actor {
    uintptr_t player = 0;
    uintptr_t character = 0;
    uintptr_t humanoid = 0;
    uintptr_t team = 0;
    int teamColor = 0;
    char teamName[ 32 ] = { };
    uintptr_t part[ BoneMax ] = { };
    char name[ 32 ] = { };
    float health = 0.0f;
    float maxHealth = 100.0f;
    float dist = 0.0f;
    Vec3 head;
    Vec3 root;
    Vec3 world[ BoneMax ];
    bool boneOk[ BoneMax ] = { };
    bool r15 = true;
    bool self = false;
    bool mate = false;
    bool vis = true;
    int visBad = 0;
    int visGood = 0;
    int mateBad = 0;
    int mateGood = 0;
    float ping = 0.0f;
    Vec3 high;
    Vec3 low;
    Vec3 vel;
};

struct Snap {
    bool ready = false;
    bool attached = false;
    int count = 0;
    float view[ 16 ] = { };
    int viewW = 0;
    int viewH = 0;
    int clientX = 0;
    int clientY = 0;
    int clientW = 0;
    int clientH = 0;
    uintptr_t local = 0;
    uintptr_t localTeam = 0;
    Vec3 camera;
    Vec3 right;
    float localPing = 0.0f;
    Actor list[ ActorMax ];
    char note[ 80 ] = { };
};

struct Off {
    uintptr_t fakeDm = 0;
    uintptr_t realDm = 0;
    uintptr_t visPtr = 0;
    uintptr_t visView = 0;
    uintptr_t visDim = 0;
    uintptr_t dmPlace = 0;
    uintptr_t dmWorkspace = 0;
    uintptr_t workspaceCam = 0;
    uintptr_t playerLocal = 0;
    uintptr_t playerModel = 0;
    uintptr_t playerTeam = 0;
    uintptr_t playerTeamColor = 0;
    uintptr_t teamBrick = 0;
    uintptr_t playerDisplay = 0;
    uintptr_t playerUser = 0;
    uintptr_t modelPrimary = 0;
    uintptr_t instName = 0;
    uintptr_t instKids = 0;
    uintptr_t instKidEnd = 0;
    uintptr_t instClass = 0;
    uintptr_t instClassName = 0;
    uintptr_t instParent = 0;
    uintptr_t strLen = 0;
    uintptr_t humanoidHealth = 0;
    uintptr_t humanoidMax = 0;
    uintptr_t humanoidRoot = 0;
    uintptr_t humanoidRig = 0;
    uintptr_t humanoidName = 0;
    uintptr_t humanoidWalk = 0;
    uintptr_t humanoidWalkCheck = 0;
    uintptr_t humanoidJump = 0;
    uintptr_t humanoidJumpH = 0;
    uintptr_t humanoidJumpReq = 0;
    uintptr_t humanoidHip = 0;
    uintptr_t humanoidStand = 0;
    uintptr_t humanoidUseJump = 0;
    uintptr_t humanoidState = 0;
    uintptr_t humanoidStateId = 0;
    uintptr_t partPrim = 0;
    uintptr_t primPos = 0;
    uintptr_t primSize = 0;
    uintptr_t primRot = 0;
    uintptr_t primVel = 0;
    uintptr_t primFlags = 0;
    uintptr_t wsWorld = 0;
    uintptr_t wsGravity = 0;
    uintptr_t worldGravity = 0;
    uintptr_t taskPtr = 0;
    uintptr_t taskFps = 0;
    uintptr_t taskJobs = 0;
    uintptr_t taskJobEnd = 0;
    uintptr_t taskJobName = 0;
    uintptr_t camPos = 0;
    uintptr_t camRot = 0;
    uintptr_t camType = 0;
    uintptr_t camView = 0;
    uintptr_t camViewSize = 0;
    uintptr_t statsVal = 0;
    uintptr_t playerPing = 0;
    uintptr_t mouseObj = 0;
    uintptr_t mouseObj2 = 0;
    uintptr_t mousePos = 0;
    uintptr_t camMode = 0;
};

using NtReadFn = LONG ( WINAPI* )( HANDLE, void*, void*, SIZE_T, SIZE_T* );
using NtWriteFn = LONG ( WINAPI* )( HANDLE, void*, void*, SIZE_T, SIZE_T* );

struct Engine {
    HANDLE process = nullptr;
    DWORD pid = 0;
    HWND window = nullptr;
    uintptr_t base = 0;
    uintptr_t size = 0;
    NtReadFn ntRead = nullptr;
    NtWriteFn ntWrite = nullptr;
    Off off;
    uintptr_t dataModel = 0;
    uintptr_t players = 0;
    uintptr_t workspace = 0;
    uintptr_t visual = 0;
    uintptr_t local = 0;
    uintptr_t localTeam = 0;
    int localTeamColor = 0;
    char localTeamName[ 32 ] = { };
    bool colorUseful = false;
    uintptr_t localRoot = 0;
    uintptr_t localHum = 0;
    uintptr_t localModel = 0;
    bool writeOk = false;
    uint64_t placeId = 0;
    uintptr_t mouseSvc = 0;
    unsigned nextTeamLog = 0;
    unsigned nextAttach = 0;
    unsigned nextDiscover = 0;
    unsigned nextResolve = 0;
    unsigned nextBeat = 0;
    unsigned nextWalls = 0;
    unsigned reads = 0;
    unsigned fails = 0;
    int nameMode = -1;
    int kidMode = -1;
    uintptr_t walls[ WallMax ] = { };
    Vec3 wallC[ WallMax ];
    Vec3 wallH[ WallMax ];
    float wallR[ WallMax ][ 9 ];
    int wallN = 0;
    Vec3 wallCam;
    uintptr_t stats = 0;
    uintptr_t pingItem = 0;
    float localPing = 0.0f;
    unsigned nextPing = 0;
    unsigned nextBind = 0;
    Snap front;
    Actor track[ ActorMax ];
    int tracked = 0;
};

inline Engine& Core( ) {
    static Engine Store;
    return Store;
}

inline Snap& View( ) {
    return Core( ).front;
}

inline bool Heap( uintptr_t Addr ) {
    return Addr >= 0x10000ull && Addr < 0x00007FFFFFFFFFFFull;
}

inline void BindNt( ) {
    Engine& E = Core( );
    if ( E.ntRead )
        return;
    HMODULE Ntdll = GetModuleHandleW( L"ntdll.dll" );
    if ( Ntdll ) {
        E.ntRead = ( NtReadFn )GetProcAddress( Ntdll, "NtReadVirtualMemory" );
        E.ntWrite = ( NtWriteFn )GetProcAddress( Ntdll, "NtWriteVirtualMemory" );
    }
}

inline bool Poke( uintptr_t Addr, const void* In, size_t Size ) {
    Engine& E = Core( );
    if ( !E.process || !Heap( Addr ) || !In || !Size )
        return false;
    SIZE_T Put = 0;
    if ( E.ntWrite ) {
        LONG Status = E.ntWrite( E.process, ( void* )Addr, ( void* )In, Size, &Put );
        if ( Status >= 0 && Put == Size )
            return true;
    }
    Put = 0;
    if ( WriteProcessMemory( E.process, ( LPVOID )Addr, In, Size, &Put ) && Put == Size )
        return true;
    return false;
}

template< typename T >
inline bool Poke( uintptr_t Addr, const T& Value ) {
    return Poke( Addr, &Value, sizeof( T ) );
}

inline bool Pull( uintptr_t Addr, void* Out, size_t Size ) {
    Engine& E = Core( );
    if ( !E.process || !Heap( Addr ) || !Out || !Size ) {
        E.fails++;
        return false;
    }
    SIZE_T Got = 0;
    if ( E.ntRead ) {
        LONG Status = E.ntRead( E.process, ( void* )Addr, Out, Size, &Got );
        if ( Status >= 0 && Got == Size ) {
            E.reads++;
            return true;
        }
    }
    SIZE_T Rpm = 0;
    if ( ReadProcessMemory( E.process, ( LPCVOID )Addr, Out, Size, &Rpm ) && Rpm == Size ) {
        E.reads++;
        return true;
    }
    E.fails++;
    return false;
}

template < typename T >
inline bool Pull( uintptr_t Addr, T& Out ) {
    return Pull( Addr, &Out, sizeof( T ) );
}

inline uintptr_t Ptr( uintptr_t Addr ) {
    uintptr_t Value = 0;
    if ( !Pull( Addr, Value ) )
        return 0;
    return Heap( Value ) ? Value : 0;
}

inline void LoadOff( ) {
    Off& O = Core( ).off;
    O.fakeDm = offsets::Get( "FakeDataModel", "Pointer" );
    O.realDm = offsets::Get( "FakeDataModel", "RealDataModel" );
    O.visPtr = offsets::Get( "VisualEngine", "Pointer" );
    O.visView = offsets::Get( "VisualEngine", "ViewMatrix" );
    O.visDim = offsets::Get( "VisualEngine", "Dimensions" );
    O.dmPlace = offsets::Get( "DataModel", "PlaceId" );
    O.dmWorkspace = offsets::Get( "DataModel", "Workspace" );
    O.workspaceCam = offsets::Get( "Workspace", "CurrentCamera" );
    O.playerLocal = offsets::Get( "Player", "LocalPlayer" );
    O.playerModel = offsets::Get( "Player", "ModelInstance" );
    O.playerTeam = offsets::Get( "Player", "Team" );
    O.playerTeamColor = offsets::Get( "Player", "TeamColor" );
    O.teamBrick = offsets::Get( "Team", "BrickColor" );
    O.playerDisplay = offsets::Get( "Player", "DisplayName" );
    O.playerUser = offsets::Get( "Player", "UserId" );
    O.modelPrimary = offsets::Get( "Model", "PrimaryPart" );
    O.instName = offsets::Get( "Instance", "NameContainer" );
    O.instKids = offsets::Get( "Instance", "ChildrenStart" );
    O.instKidEnd = offsets::Get( "Instance", "ChildrenEnd" );
    O.instClass = offsets::Get( "Instance", "ClassDescriptor" );
    O.instClassName = offsets::Get( "Instance", "ClassName" );
    O.instParent = offsets::Get( "Instance", "Parent" );
    O.strLen = offsets::Get( "Misc", "StringLength" );
    O.humanoidHealth = offsets::Get( "Humanoid", "Health" );
    O.humanoidMax = offsets::Get( "Humanoid", "MaxHealth" );
    O.humanoidRoot = offsets::Get( "Humanoid", "HumanoidRootPart" );
    O.humanoidRig = offsets::Get( "Humanoid", "RigType" );
    O.humanoidName = offsets::Get( "Humanoid", "DisplayName" );
    O.humanoidWalk = offsets::Get( "Humanoid", "WalkSpeed" );
    if ( !O.humanoidWalk )
        O.humanoidWalk = offsets::Get( "Humanoid", "Walkspeed" );
    O.humanoidWalkCheck = offsets::Get( "Humanoid", "WalkSpeedCheck" );
    if ( !O.humanoidWalkCheck )
        O.humanoidWalkCheck = offsets::Get( "Humanoid", "WalkspeedCheck" );
    O.humanoidJump = offsets::Get( "Humanoid", "JumpPower" );
    O.humanoidJumpH = offsets::Get( "Humanoid", "JumpHeight" );
    O.humanoidJumpReq = offsets::Get( "Humanoid", "Jump" );
    O.humanoidHip = offsets::Get( "Humanoid", "HipHeight" );
    O.humanoidStand = offsets::Get( "Humanoid", "PlatformStand" );
    O.humanoidUseJump = offsets::Get( "Humanoid", "UseJumpPower" );
    O.humanoidState = offsets::Get( "Humanoid", "HumanoidState" );
    O.humanoidStateId = offsets::Get( "Humanoid", "HumanoidStateID" );
    O.wsWorld = offsets::Get( "Workspace", "World" );
    O.wsGravity = offsets::Get( "Workspace", "ReadOnlyGravity" );
    O.worldGravity = offsets::Get( "World", "Gravity" );
    O.partPrim = offsets::Get( "BasePart", "Primitive" );
    O.primPos = offsets::Get( "Primitive", "Position" );
    O.primSize = offsets::Get( "Primitive", "Size" );
    O.primRot = offsets::Get( "Primitive", "Rotation" );
    O.primVel = offsets::Get( "Primitive", "AssemblyLinearVelocity" );
    O.primFlags = offsets::Get( "Primitive", "Flags" );
    if ( !O.primFlags )
        O.primFlags = 0x1B6;
    O.taskPtr = offsets::Get( "TaskScheduler", "Pointer" );
    O.taskFps = offsets::Get( "TaskScheduler", "MaxFPS" );
    O.taskJobs = offsets::Get( "TaskScheduler", "JobStart" );
    O.taskJobEnd = offsets::Get( "TaskScheduler", "JobEnd" );
    O.taskJobName = offsets::Get( "TaskScheduler", "JobName" );
    O.camPos = offsets::Get( "Camera", "Position" );
    O.camRot = offsets::Get( "Camera", "Rotation" );
    O.camType = offsets::Get( "Camera", "CameraType" );
    O.camView = offsets::Get( "Camera", "Viewport" );
    O.camViewSize = offsets::Get( "Camera", "ViewportSize" );
    O.statsVal = offsets::Get( "StatsItem", "Value" );
    if ( !O.statsVal )
        O.statsVal = 200;
    O.playerPing = offsets::Get( "Player", "Ping" );
    if ( !O.playerPing )
        O.playerPing = offsets::Get( "Player", "NetworkPing" );
    O.mouseObj = offsets::Get( "MouseService", "InputObject" );
    O.mouseObj2 = offsets::Get( "MouseService", "InputObject2" );
    O.mousePos = offsets::Get( "MouseService", "MousePosition" );
    O.camMode = offsets::Get( "Player", "CameraMode" );
}

inline bool Printable( const char* Text, int Len ) {
    if ( Len <= 0 || Len > 64 )
        return false;
    for ( int Index = 0; Index < Len; Index++ ) {
        unsigned char Byte = ( unsigned char )Text[ Index ];
        if ( Byte < 32 || Byte > 126 )
            return false;
    }
    return true;
}

inline bool DigitOnly( const char* Text ) {
    if ( !Text || !Text[ 0 ] )
        return true;
    for ( int Index = 0; Text[ Index ]; Index++ ) {
        unsigned char Byte = ( unsigned char )Text[ Index ];
        if ( Byte < '0' || Byte > '9' )
            return false;
    }
    return true;
}

inline bool NameOk( const char* Text ) {
    if ( !Text || !Text[ 0 ] )
        return false;
    int Len = ( int )strlen( Text );
    if ( !Printable( Text, Len ) || DigitOnly( Text ) )
        return false;
    return Len >= 1 && Len <= 64;
}

inline bool Ident( const char* Text ) {
    if ( !Text || !Text[ 0 ] )
        return false;
    unsigned char First = ( unsigned char )Text[ 0 ];
    if ( !( ( First >= 'A' && First <= 'Z' ) || ( First >= 'a' && First <= 'z' ) ) )
        return false;
    int Len = 0;
    int Letters = 0;
    for ( ; Text[ Len ]; Len++ ) {
        unsigned char Byte = ( unsigned char )Text[ Len ];
        bool Letter = ( Byte >= 'A' && Byte <= 'Z' ) || ( Byte >= 'a' && Byte <= 'z' );
        bool Ok = Letter || ( Byte >= '0' && Byte <= '9' ) || Byte == '_' || Byte == ' ';
        if ( !Ok )
            return false;
        if ( Letter )
            Letters++;
    }
    return Len >= 2 && Len <= 40 && Letters >= 2;
}

inline bool ReadChars( uintptr_t Addr, char* Out, int Cap, int Len ) {
    if ( Len <= 0 || Len >= Cap )
        return false;
    if ( !Pull( Addr, Out, ( size_t )Len ) )
        return false;
    Out[ Len ] = 0;
    return Printable( Out, Len );
}

inline bool ReadRoblox( uintptr_t Object, char* Out, int Cap ) {
    Engine& E = Core( );
    if ( !Object || Cap < 2 )
        return false;
    Out[ 0 ] = 0;
    uintptr_t LenOff = E.off.strLen ? E.off.strLen : 16;
    int32_t Len = 0;
    if ( Pull( Object + LenOff, Len ) && Len > 0 && Len < 80 ) {
        uintptr_t Data = ( Len >= 16 ) ? Ptr( Object ) : Object;
        if ( !Data )
            Data = Object;
        if ( ReadChars( Data, Out, Cap, Len ) && NameOk( Out ) )
            return true;
    }
    int32_t Alt = 0;
    if ( LenOff != 16 && Pull( Object + 16, Alt ) && Alt > 0 && Alt < 80 ) {
        uintptr_t Data = ( Alt >= 16 ) ? Ptr( Object ) : Object;
        if ( ReadChars( Data, Out, Cap, Alt ) && NameOk( Out ) )
            return true;
    }
    uintptr_t HeapStr = Ptr( Object );
    if ( HeapStr ) {
        char Raw[ 64 ] = { };
        if ( Pull( HeapStr, Raw, 63 ) ) {
            Raw[ 63 ] = 0;
            int Have = ( int )strnlen( Raw, 63 );
            if ( Printable( Raw, Have ) && NameOk( Raw ) ) {
                lstrcpynA( Out, Raw, Cap );
                return true;
            }
        }
        int32_t InnerLen = 0;
        if ( Pull( HeapStr + LenOff, InnerLen ) && InnerLen > 0 && InnerLen < 80 ) {
            uintptr_t Data = ( InnerLen >= 16 ) ? Ptr( HeapStr ) : HeapStr;
            if ( ReadChars( Data, Out, Cap, InnerLen ) && NameOk( Out ) )
                return true;
        }
    }
    return false;
}

inline bool ReadName( uintptr_t Object, char* Out, int Cap ) {
    Engine& E = Core( );
    if ( !Object || Cap < 2 )
        return false;
    Out[ 0 ] = 0;
    if ( ReadRoblox( Object, Out, Cap ) )
        return true;

    auto Try = [ & ]( int Mode ) -> bool {
        char Hold[ 64 ] = { };
        Hold[ 0 ] = 0;
        if ( Mode == 0 ) {
            int32_t Len = 0;
            if ( !Pull( Object + E.off.strLen, Len ) || Len <= 0 || Len > 48 )
                return false;
            uintptr_t Data = ( Len >= 16 ) ? Ptr( Object ) : Object;
            if ( !ReadChars( Data, Hold, 64, Len ) )
                return false;
        } else if ( Mode == 1 ) {
            uintptr_t Data = Ptr( Object + 8 );
            int32_t Len = 0;
            if ( !Pull( Object + E.off.strLen, Len ) || Len <= 0 || Len > 48 )
                return false;
            if ( !ReadChars( Data ? Data : Object + 8, Hold, 64, Len ) )
                return false;
        } else if ( Mode == 2 ) {
            char Raw[ 48 ] = { };
            if ( !Pull( Object, Raw, sizeof( Raw ) ) )
                return false;
            Raw[ 47 ] = 0;
            int Len = ( int )strnlen( Raw, 47 );
            if ( !Printable( Raw, Len ) )
                return false;
            memcpy( Hold, Raw, ( size_t )Len + 1 );
        } else {
            return false;
        }
        lstrcpynA( Out, Hold, Cap );
        return Out[ 0 ] != 0;
    };

    int Order[ 3 ] = { E.nameMode >= 0 ? E.nameMode : 0, 1, 2 };
    if ( Order[ 0 ] == 1 ) {
        Order[ 1 ] = 0;
        Order[ 2 ] = 2;
    } else if ( Order[ 0 ] == 2 ) {
        Order[ 1 ] = 0;
        Order[ 2 ] = 1;
    }

    char Best[ 64 ] = { };
    int BestMode = -1;
    for ( int Index = 0; Index < 3; Index++ ) {
        char Hold[ 64 ] = { };
        if ( !Try( Order[ Index ] ) )
            continue;
        lstrcpynA( Hold, Out, 64 );
        if ( Ident( Hold ) ) {
            if ( E.nameMode < 0 ) {
                E.nameMode = Order[ Index ];
            }
            return true;
        }
        if ( BestMode < 0 ) {
            lstrcpynA( Best, Hold, 64 );
            BestMode = Order[ Index ];
        }
    }
    if ( BestMode >= 0 ) {
        lstrcpynA( Out, Best, Cap );
        return true;
    }
    Out[ 0 ] = 0;
    return false;
}

inline bool ReadLoose( uintptr_t Addr, char* Out, int Cap ) {
    if ( !Addr || Cap < 3 )
        return false;
    unsigned char Raw[ 96 ] = { };
    if ( !Pull( Addr, Raw, sizeof( Raw ) ) )
        return false;
    int Best = -1;
    int BestLen = 0;
    for ( int Index = 0; Index < 90; Index++ ) {
        unsigned char Byte = Raw[ Index ];
        if ( !( ( Byte >= 'A' && Byte <= 'Z' ) || ( Byte >= 'a' && Byte <= 'z' ) ) )
            continue;
        int Len = 0;
        while ( Index + Len < 95 ) {
            unsigned char Next = Raw[ Index + Len ];
            bool Ok = ( Next >= 'A' && Next <= 'Z' ) || ( Next >= 'a' && Next <= 'z' ) || ( Next >= '0' && Next <= '9' ) || Next == '_' || Next == ' ';
            if ( !Ok )
                break;
            Len++;
        }
        if ( Len >= 3 && Len > BestLen && Len < 28 ) {
            Best = Index;
            BestLen = Len;
        }
    }
    if ( Best < 0 )
        return false;
    if ( BestLen >= Cap )
        BestLen = Cap - 1;
    memcpy( Out, Raw + Best, ( size_t )BestLen );
    Out[ BestLen ] = 0;
    while ( BestLen && Out[ BestLen - 1 ] == ' ' )
        Out[ --BestLen ] = 0;
    return BestLen >= 2 && Ident( Out );
}

inline bool LabelOf( uintptr_t Inst, uintptr_t Field, char* Out, int Cap ) {
    if ( !Inst || !Field )
        return false;
    if ( ReadName( Inst + Field, Out, Cap ) && NameOk( Out ) )
        return true;
    uintptr_t PtrName = Ptr( Inst + Field );
    if ( PtrName && ReadName( PtrName, Out, Cap ) && NameOk( Out ) )
        return true;
    if ( PtrName && ReadLoose( PtrName, Out, Cap ) && NameOk( Out ) )
        return true;
    return ReadLoose( Inst + Field, Out, Cap ) && NameOk( Out );
}

inline bool InstName( uintptr_t Inst, char* Out, int Cap ) {
    if ( !Inst || !Out || Cap < 2 )
        return false;
    Out[ 0 ] = 0;
    if ( LabelOf( Inst, Core( ).off.instName, Out, Cap ) )
        return true;
    uintptr_t Inner = Ptr( Inst + Core( ).off.instName );
    if ( Inner && ReadRoblox( Inner, Out, Cap ) )
        return true;
    return false;
}

inline uintptr_t ParentOf( uintptr_t Inst ) {
    if ( !Inst || !Core( ).off.instParent )
        return 0;
    return Ptr( Inst + Core( ).off.instParent );
}

inline void TagName( uintptr_t Inst, const char* Klass, char* Out, int Cap ) {
    if ( InstName( Inst, Out, Cap ) && Out[ 0 ] )
        return;
    snprintf( Out, Cap, "%s_%04x", ( Klass && Klass[ 0 ] ) ? Klass : "Inst", ( unsigned )( Inst & 0xffff ) );
}

inline bool WritePartPos( uintptr_t Part, const Vec3& In ) {
    if ( !Part || !Core( ).off.partPrim || !Core( ).off.primPos )
        return false;
    uintptr_t Prim = Ptr( Part + Core( ).off.partPrim );
    if ( !Prim )
        return false;
    return Poke( Prim + Core( ).off.primPos, In );
}

inline bool WritePartVel( uintptr_t Part, const Vec3& In ) {
    if ( !Part || !Core( ).off.partPrim || !Core( ).off.primVel )
        return false;
    uintptr_t Prim = Ptr( Part + Core( ).off.partPrim );
    if ( !Prim )
        return false;
    return Poke( Prim + Core( ).off.primVel, In );
}

inline bool WritePartSize( uintptr_t Part, const Vec3& In ) {
    if ( !Part || !Core( ).off.partPrim || !Core( ).off.primSize )
        return false;
    uintptr_t Prim = Ptr( Part + Core( ).off.partPrim );
    if ( !Prim )
        return false;
    return Poke( Prim + Core( ).off.primSize, In );
}

inline bool PartRot( uintptr_t Part, float* Out ) {
    if ( !Part || !Out || !Core( ).off.partPrim || !Core( ).off.primRot )
        return false;
    uintptr_t Prim = Ptr( Part + Core( ).off.partPrim );
    if ( !Prim )
        return false;
    return Pull( Prim + Core( ).off.primRot, Out, sizeof( float ) * 9 );
}

inline bool WritePartRot( uintptr_t Part, const float* In ) {
    if ( !Part || !In || !Core( ).off.partPrim || !Core( ).off.primRot )
        return false;
    uintptr_t Prim = Ptr( Part + Core( ).off.partPrim );
    if ( !Prim )
        return false;
    return Poke( Prim + Core( ).off.primRot, In, sizeof( float ) * 9 );
}

inline uintptr_t CurrentCam( ) {
    Engine& E = Core( );
    if ( !E.workspace || !E.off.workspaceCam )
        return 0;
    return Ptr( E.workspace + E.off.workspaceCam );
}

inline bool SetCollide( uintptr_t Part, bool On ) {
    Engine& E = Core( );
    if ( !Part || !E.off.partPrim || !E.off.primFlags )
        return false;
    uintptr_t Prim = Ptr( Part + E.off.partPrim );
    if ( !Prim )
        return false;
    uint8_t Flags = 0;
    if ( !Pull( Prim + E.off.primFlags, Flags ) )
        return false;
    if ( On )
        Flags = ( uint8_t )( Flags | 0x8 );
    else
        Flags = ( uint8_t )( Flags & ~0x8 );
    return Poke( Prim + E.off.primFlags, Flags );
}

inline uintptr_t DataModel( ) {
    return Core( ).dataModel;
}

inline uintptr_t LocalRoot( ) {
    return Core( ).localRoot;
}

inline bool ClassName( uintptr_t Inst, char* Out, int Cap ) {
    uintptr_t Desc = Ptr( Inst + Core( ).off.instClass );
    if ( !Desc )
        return false;
    uintptr_t Name = Ptr( Desc + Core( ).off.instClassName );
    if ( Name && ReadName( Name, Out, Cap ) )
        return true;
    return ReadName( Desc + Core( ).off.instClassName, Out, Cap );
}

inline bool IsClass( uintptr_t Inst, const char* Want ) {
    char Have[ 40 ] = { };
    return ClassName( Inst, Have, ( int )sizeof( Have ) ) && _stricmp( Have, Want ) == 0;
}

inline bool IsNamed( uintptr_t Inst, const char* Want ) {
    char Have[ 40 ] = { };
    return InstName( Inst, Have, ( int )sizeof( Have ) ) && _stricmp( Have, Want ) == 0;
}

inline int Kids( uintptr_t Inst, uintptr_t* Out, int Cap ) {
    Engine& E = Core( );
    if ( !Inst || !Out || Cap <= 0 )
        return 0;

    auto Walk = [ & ]( uintptr_t Start, uintptr_t Finish, int Stride ) -> int {
        if ( !Heap( Start ) || !Heap( Finish ) || Finish <= Start )
            return 0;
        uintptr_t Span = Finish - Start;
        if ( Span > 0x4000 )
            return 0;
        int Count = 0;
        for ( uintptr_t At = Start; At + sizeof( uintptr_t ) <= Finish && Count < Cap; At += ( uintptr_t )Stride ) {
            uintptr_t Child = Ptr( At );
            if ( Child )
                Out[ Count++ ] = Child;
        }
        return Count;
    };

    auto Run = [ & ]( int Mode ) -> int {
        if ( Mode == 0 ) {
            uintptr_t Start = Ptr( Inst + E.off.instKids );
            uintptr_t Finish = Ptr( Inst + E.off.instKids + E.off.instKidEnd );
            int Count = Walk( Start, Finish, 16 );
            return Count ? Count : Walk( Start, Finish, 8 );
        }
        uintptr_t List = Ptr( Inst + E.off.instKids );
        if ( !List )
            return 0;
        uintptr_t Start = Ptr( List );
        uintptr_t Finish = Ptr( List + E.off.instKidEnd );
        int Count = Walk( Start, Finish, 16 );
        return Count ? Count : Walk( Start, Finish, 8 );
    };

    if ( E.kidMode >= 0 )
        return Run( E.kidMode );

    for ( int Mode = 0; Mode < 2; Mode++ ) {
        int Count = Run( Mode );
        if ( Count > 0 ) {
            E.kidMode = Mode;
            return Count;
        }
    }
    return 0;
}

inline uintptr_t ChildNamed( uintptr_t Inst, const char* Name ) {
    uintptr_t List[ KidMax ];
    int Count = Kids( Inst, List, KidMax );
    for ( int Index = 0; Index < Count; Index++ ) {
        char Have[ 40 ] = { };
        if ( InstName( List[ Index ], Have, ( int )sizeof( Have ) ) && strcmp( Have, Name ) == 0 )
            return List[ Index ];
    }
    return 0;
}

inline uintptr_t ChildClass( uintptr_t Inst, const char* Name ) {
    uintptr_t List[ KidMax ];
    int Count = Kids( Inst, List, KidMax );
    for ( int Index = 0; Index < Count; Index++ ) {
        if ( IsClass( List[ Index ], Name ) )
            return List[ Index ];
    }
    return 0;
}

inline void BindLocal( ) {
    Engine& E = Core( );
    unsigned Now = GetTickCount( );
    if ( E.localRoot && E.localHum && E.localModel && Now < E.nextBind )
        return;
    E.nextBind = Now + 250;
    uintptr_t Model = 0;
    uintptr_t Hum = 0;
    uintptr_t Root = 0;
    if ( E.local && E.off.playerModel )
        Model = Ptr( E.local + E.off.playerModel );
    if ( Model ) {
        Hum = ChildClass( Model, "Humanoid" );
        if ( !Hum )
            Hum = ChildNamed( Model, "Humanoid" );
        if ( Hum && E.off.humanoidRoot )
            Root = Ptr( Hum + E.off.humanoidRoot );
        if ( !Root )
            Root = ChildNamed( Model, "HumanoidRootPart" );
    }
    for ( int Index = 0; Index < E.tracked; Index++ ) {
        if ( !E.track[ Index ].self )
            continue;
        if ( E.track[ Index ].character )
            Model = E.track[ Index ].character;
        if ( E.track[ Index ].humanoid )
            Hum = E.track[ Index ].humanoid;
        if ( E.track[ Index ].part[ BoneRoot ] )
            Root = E.track[ Index ].part[ BoneRoot ];
        break;
    }
    E.localModel = Model;
    E.localHum = Hum;
    if ( Root )
        E.localRoot = Root;
}

inline bool TagTeam( uintptr_t Inst, char* Out, int Cap ) {
    if ( !Inst || !Out || Cap <= 1 )
        return false;
    uintptr_t Field = offsets::Get( "Misc", "Value" );
    if ( !Field )
        Field = offsets::Get( "StatsItem", "Value" );
    uintptr_t List[ KidMax ];
    int Count = Kids( Inst, List, KidMax );
    for ( int Index = 0; Index < Count; Index++ ) {
        char Have[ 40 ] = { };
        if ( !InstName( List[ Index ], Have, ( int )sizeof( Have ) ) )
            continue;
        if ( _stricmp( Have, "TeamID" ) && _stricmp( Have, "TeamId" ) && _stricmp( Have, "teamId" )
            && _stricmp( Have, "Squad" ) && _stricmp( Have, "Party" ) )
            continue;
        if ( Field && LabelOf( List[ Index ], Field, Out, Cap ) && Out[ 0 ] )
            return true;
        int Whole = 0;
        if ( Field && Pull( List[ Index ] + Field, Whole ) && Whole ) {
            snprintf( Out, Cap, "%d", Whole );
            return true;
        }
    }
    return false;
}

inline void FillTeam( uintptr_t Player, uintptr_t Character, uintptr_t& Team, int& Color, char* Name, int Cap, bool Deep ) {
    Team = 0;
    Color = 0;
    if ( Name && Cap > 0 )
        Name[ 0 ] = 0;
    Engine& E = Core( );
    if ( Player && E.off.playerTeam )
        Team = Ptr( Player + E.off.playerTeam );
    if ( Player && E.off.playerTeamColor )
        Pull( Player + E.off.playerTeamColor, Color );
    if ( Team ) {
        if ( Name && Cap > 0 )
            InstName( Team, Name, Cap );
        if ( E.off.teamBrick ) {
            int Brick = 0;
            if ( Pull( Team + E.off.teamBrick, Brick ) && Brick )
                Color = Brick;
        }
    }
    if ( Deep && Name && Cap > 0 && !Name[ 0 ] && Player )
        TagTeam( Player, Name, Cap );
    if ( Deep && Name && Cap > 0 && !Name[ 0 ] && Character )
        TagTeam( Character, Name, Cap );
}

inline bool IsMate( const Actor& Item ) {
    if ( Item.self )
        return false;
    int How = sense::TeamHow( );
    if ( How == sense::TeamNone )
        return false;
    Engine& E = Core( );
    bool PtrOk = E.localTeam && Item.team && E.localTeam == Item.team;
    bool NameOk = E.localTeamName[ 0 ] && Item.teamName[ 0 ] && !_stricmp( E.localTeamName, Item.teamName );
    bool ColorOk = E.colorUseful && E.localTeamColor && Item.teamColor && E.localTeamColor == Item.teamColor;
    if ( How == sense::TeamPtr )
        return PtrOk;
    if ( How == sense::TeamName )
        return NameOk;
    if ( How == sense::TeamColor )
        return ColorOk;
    return PtrOk || NameOk || ColorOk;
}

inline uintptr_t FindService( uintptr_t Inst, const char* Want ) {
    uintptr_t List[ KidMax ];
    int Count = Kids( Inst, List, KidMax );
    uintptr_t Named = 0;
    for ( int Index = 0; Index < Count; Index++ ) {
        if ( IsClass( List[ Index ], Want ) )
            return List[ Index ];
        if ( !Named && IsNamed( List[ Index ], Want ) )
            Named = List[ Index ];
    }
    return Named;
}

inline bool PartPos( uintptr_t Part, Vec3& Out ) {
    if ( !Part )
        return false;
    uintptr_t Prim = Ptr( Part + Core( ).off.partPrim );
    if ( !Prim )
        return false;
    return Pull( Prim + Core( ).off.primPos, Out );
}

inline bool PartVel( uintptr_t Part, Vec3& Out ) {
    if ( !Part )
        return false;
    uintptr_t Prim = Ptr( Part + Core( ).off.partPrim );
    if ( !Prim || !Core( ).off.primVel )
        return false;
    return Pull( Prim + Core( ).off.primVel, Out );
}

inline bool PartSize( uintptr_t Part, Vec3& Out ) {
    if ( !Part )
        return false;
    uintptr_t Prim = Ptr( Part + Core( ).off.partPrim );
    if ( !Prim )
        return false;
    return Pull( Prim + Core( ).off.primSize, Out );
}

inline bool SlabHit( const Vec3& Origin, const Vec3& Dir, float MaxT, const Vec3& Center, const Vec3& Half, const float* Rot ) {
    Vec3 Rel;
    Rel.x = Origin.x - Center.x;
    Rel.y = Origin.y - Center.y;
    Rel.z = Origin.z - Center.z;
    Vec3 LocO = Rel;
    Vec3 LocD = Dir;
    if ( Rot ) {
        LocO.x = Rel.x * Rot[ 0 ] + Rel.y * Rot[ 3 ] + Rel.z * Rot[ 6 ];
        LocO.y = Rel.x * Rot[ 1 ] + Rel.y * Rot[ 4 ] + Rel.z * Rot[ 7 ];
        LocO.z = Rel.x * Rot[ 2 ] + Rel.y * Rot[ 5 ] + Rel.z * Rot[ 8 ];
        LocD.x = Dir.x * Rot[ 0 ] + Dir.y * Rot[ 3 ] + Dir.z * Rot[ 6 ];
        LocD.y = Dir.x * Rot[ 1 ] + Dir.y * Rot[ 4 ] + Dir.z * Rot[ 7 ];
        LocD.z = Dir.x * Rot[ 2 ] + Dir.y * Rot[ 5 ] + Dir.z * Rot[ 8 ];
    }
    float TMin = 0.0f;
    float TMax = MaxT;
    float O[ 3 ] = { LocO.x, LocO.y, LocO.z };
    float D[ 3 ] = { LocD.x, LocD.y, LocD.z };
    float H[ 3 ] = { Half.x, Half.y, Half.z };
    for ( int Axis = 0; Axis < 3; Axis++ ) {
        if ( fabsf( D[ Axis ] ) < 1.0e-7f ) {
            if ( O[ Axis ] < -H[ Axis ] || O[ Axis ] > H[ Axis ] )
                return false;
            continue;
        }
        float Inv = 1.0f / D[ Axis ];
        float T1 = ( -H[ Axis ] - O[ Axis ] ) * Inv;
        float T2 = ( H[ Axis ] - O[ Axis ] ) * Inv;
        if ( T1 > T2 ) {
            float Swap = T1;
            T1 = T2;
            T2 = Swap;
        }
        if ( T1 > TMin )
            TMin = T1;
        if ( T2 < TMax )
            TMax = T2;
        if ( TMin > TMax )
            return false;
    }
    return TMin > 0.10f && TMax >= TMin;
}

inline bool PartFrame( uintptr_t Part, Vec3& Center, Vec3& Half, float* Rot ) {
    uintptr_t Prim = Ptr( Part + Core( ).off.partPrim );
    if ( !Prim )
        return false;
    Vec3 Size;
    if ( !Pull( Prim + Core( ).off.primPos, Center ) || !Pull( Prim + Core( ).off.primSize, Size ) )
        return false;
    Half.x = Size.x * 0.5f;
    Half.y = Size.y * 0.5f;
    Half.z = Size.z * 0.5f;
    if ( Rot ) {
        Rot[ 0 ] = 1.0f; Rot[ 1 ] = 0.0f; Rot[ 2 ] = 0.0f;
        Rot[ 3 ] = 0.0f; Rot[ 4 ] = 1.0f; Rot[ 5 ] = 0.0f;
        Rot[ 6 ] = 0.0f; Rot[ 7 ] = 0.0f; Rot[ 8 ] = 1.0f;
        if ( Core( ).off.primRot )
            Pull( Prim + Core( ).off.primRot, Rot, sizeof( float ) * 9 );
    }
    return true;
}

inline bool RayHit( const Vec3& Origin, const Vec3& Dir, float MaxT, uintptr_t Part, float Pad ) {
    Vec3 Center;
    Vec3 Half;
    float Rot[ 9 ];
    if ( !PartFrame( Part, Center, Half, Rot ) )
        return false;
    Half.x += Pad;
    Half.y += Pad;
    Half.z += Pad;
    return SlabHit( Origin, Dir, MaxT, Center, Half, Rot );
}

inline DWORD FindPid( const wchar_t* Name ) {
    HANDLE Snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if ( Snap == INVALID_HANDLE_VALUE )
        return 0;
    PROCESSENTRY32W Entry = { };
    Entry.dwSize = sizeof( Entry );
    DWORD Pid = 0;
    if ( Process32FirstW( Snap, &Entry ) ) {
        do {
            if ( _wcsicmp( Entry.szExeFile, Name ) == 0 )
                Pid = Entry.th32ProcessID;
        } while ( Process32NextW( Snap, &Entry ) );
    }
    CloseHandle( Snap );
    return Pid;
}

inline uintptr_t ModuleBase( DWORD Pid, const wchar_t* Name, uintptr_t* Size = nullptr ) {
    HANDLE Snap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, Pid );
    if ( Snap == INVALID_HANDLE_VALUE )
        return 0;
    MODULEENTRY32W Entry = { };
    Entry.dwSize = sizeof( Entry );
    uintptr_t Base = 0;
    if ( Module32FirstW( Snap, &Entry ) ) {
        do {
            if ( _wcsicmp( Entry.szModule, Name ) == 0 ) {
                Base = ( uintptr_t )Entry.modBaseAddr;
                if ( Size )
                    *Size = ( uintptr_t )Entry.modBaseSize;
                break;
            }
        } while ( Module32NextW( Snap, &Entry ) );
    }
    CloseHandle( Snap );
    return Base;
}

inline HWND FindHwnd( DWORD Pid ) {
    struct Hunt {
        DWORD pid;
        HWND hwnd;
    } Want{ Pid, nullptr };
    EnumWindows( []( HWND Hwnd, LPARAM Param ) -> BOOL {
        Hunt* Need = ( Hunt* )Param;
        DWORD WindowPid = 0;
        GetWindowThreadProcessId( Hwnd, &WindowPid );
        if ( WindowPid != Need->pid || !IsWindowVisible( Hwnd ) )
            return TRUE;
        if ( GetWindow( Hwnd, GW_OWNER ) )
            return TRUE;
        RECT Box = { };
        if ( !GetClientRect( Hwnd, &Box ) || Box.right < 64 || Box.bottom < 64 )
            return TRUE;
        Need->hwnd = Hwnd;
        return FALSE;
    }, ( LPARAM )&Want );
    return Want.hwnd;
}

inline void Detach( const char* Why ) {
    ( void )Why;
    Engine& E = Core( );
    if ( E.process ) {
        CloseHandle( E.process );
    }
    E.process = nullptr;
    E.pid = 0;
    E.window = nullptr;
    E.base = 0;
    E.size = 0;
    E.dataModel = 0;
    E.players = 0;
    E.workspace = 0;
    E.stats = 0;
    E.pingItem = 0;
    E.localPing = 0.0f;
    E.visual = 0;
    E.local = 0;
    E.localTeam = 0;
    E.localRoot = 0;
    E.localHum = 0;
    E.localModel = 0;
    E.writeOk = false;
    E.tracked = 0;
    E.front.ready = false;
    E.front.attached = false;
    E.front.count = 0;
    lstrcpynA( E.front.note, Why, ( int )sizeof( E.front.note ) );
}

inline bool Attach( ) {
    Engine& E = Core( );
    unsigned Now = GetTickCount( );
    if ( E.process ) {
        DWORD Code = 0;
        if ( GetExitCodeProcess( E.process, &Code ) && Code == STILL_ACTIVE ) {
            if ( !E.off.fakeDm && offsets::Ready( ) )
                LoadOff( );
            return true;
        }
        Detach( "process exited" );
    }
    if ( Now < E.nextAttach )
        return false;
    E.nextAttach = Now + 1500;

    static const wchar_t* Names[ ] = { L"RobloxPlayerBeta.exe", L"RobloxPlayer.exe", L"Windows10Universal.exe" };
    DWORD Pid = 0;
    const wchar_t* Used = nullptr;
    for ( const wchar_t* Name : Names ) {
        Pid = FindPid( Name );
        if ( Pid ) {
            Used = Name;
            break;
        }
    }
    if ( !Pid ) {
        lstrcpynA( E.front.note, "roblox not found", ( int )sizeof( E.front.note ) );
        return false;
    }

    BindNt( );
    bool CanWrite = true;
    HANDLE Handle = OpenProcess( PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, Pid );
    if ( !Handle ) {
        CanWrite = false;
        Handle = OpenProcess( PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, Pid );
    }
    if ( !Handle )
        Handle = OpenProcess( PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, Pid );
    if ( !Handle ) {
        lstrcpynA( E.front.note, "access denied", ( int )sizeof( E.front.note ) );
        return false;
    }

    uintptr_t Size = 0;
    uintptr_t Base = ModuleBase( Pid, Used, &Size );
    if ( !Base )
        Base = ModuleBase( Pid, L"RobloxPlayerBeta.exe", &Size );
    if ( !Base ) {
        CloseHandle( Handle );
        return false;
    }

    E.process = Handle;
    E.writeOk = CanWrite;
    E.pid = Pid;
    E.base = Base;
    E.size = Size;
    E.window = FindHwnd( Pid );
    E.reads = 0;
    E.fails = 0;
    LoadOff( );
    return true;
}

inline bool Resolve( ) {
    Engine& E = Core( );
    if ( !E.process || !E.base || !E.off.fakeDm )
        return false;

    unsigned Now = GetTickCount( );
    if ( E.dataModel && Now < E.nextResolve )
        return E.players != 0 && E.workspace != 0;

    uintptr_t Fake = Ptr( E.base + E.off.fakeDm );
    uintptr_t Data = Fake ? Ptr( Fake + E.off.realDm ) : 0;
    if ( !Data )
        return false;

    uintptr_t Visual = E.off.visPtr ? Ptr( E.base + E.off.visPtr ) : 0;
    uintptr_t Workspace = E.workspace;
    uintptr_t Players = E.players;
    if ( Data != E.dataModel || !Workspace || !Players ) {
        Workspace = E.off.dmWorkspace ? Ptr( Data + E.off.dmWorkspace ) : 0;
        if ( !Workspace )
            Workspace = FindService( Data, "Workspace" );
        Players = FindService( Data, "Players" );
        if ( !Players && Workspace ) {
            uintptr_t Parent = E.off.instParent ? Ptr( Workspace + E.off.instParent ) : 0;
            if ( Parent && Parent != Data )
                Players = FindService( Parent, "Players" );
        }
        if ( !E.mouseSvc )
            E.mouseSvc = FindService( Data, "MouseService" );
    }

    uintptr_t Local = 0;
    if ( Players && E.off.playerLocal )
        Local = Ptr( Players + E.off.playerLocal );

    if ( Local != E.local ) {
        E.localRoot = 0;
        E.localHum = 0;
        E.localModel = 0;
        E.nextBind = 0;
    }
    E.dataModel = Data;
    E.workspace = Workspace;
    E.players = Players;
    E.visual = Visual;
    E.local = Local;
    E.nextResolve = Now + ( Players && Workspace && Local ? 800 : 400 );
    if ( E.off.dmPlace ) {
        int64_t Place = 0;
        if ( Pull( Data + E.off.dmPlace, Place ) && Place > 0 )
            E.placeId = ( uint64_t )Place;
    }
    if ( E.placeId )
        sense::BindPlace( E.placeId );
    if ( Local )
        FillTeam( Local, E.off.playerModel ? Ptr( Local + E.off.playerModel ) : 0, E.localTeam, E.localTeamColor, E.localTeamName, ( int )sizeof( E.localTeamName ), false );
    BindLocal( );
    return Players != 0;
}

inline void BindBones( Actor& Item, bool NeedSkel ) {
    if ( !Item.character )
        return;
    Engine& E = Core( );
    auto TakeRoot = [ & ]( uintptr_t Guess ) {
        Vec3 Test;
        if ( Guess && PartPos( Guess, Test ) )
            Item.part[ BoneRoot ] = Guess;
    };
    if ( !Item.part[ BoneRoot ] && Item.humanoid && E.off.humanoidRoot )
        TakeRoot( Ptr( Item.humanoid + E.off.humanoidRoot ) );
    if ( !Item.part[ BoneRoot ] && E.off.modelPrimary )
        TakeRoot( Ptr( Item.character + E.off.modelPrimary ) );

    uintptr_t List[ KidMax ];
    int Count = Kids( Item.character, List, KidMax );
    uintptr_t Parts[ 28 ];
    Vec3 Spot[ 28 ];
    int Used = 0;
    uintptr_t HitHead = 0;
    uintptr_t HitBody = 0;
    for ( int Index = 0; Index < Count && Used < 28; Index++ ) {
        if ( !E.off.partPrim || !Ptr( List[ Index ] + E.off.partPrim ) )
            continue;
        char Have[ 40 ] = { };
        if ( !InstName( List[ Index ], Have, ( int )sizeof( Have ) ) )
            continue;
        if ( !_stricmp( Have, "Handle" ) || !_stricmp( Have, "Accessory" ) || !_stricmp( Have, "Hat" ) )
            continue;
        if ( !_stricmp( Have, "PhysicalHitboxHead" ) || !_stricmp( Have, "HitboxHead" ) || !_stricmp( Have, "HitboxHeadSmall" ) ) {
            HitHead = List[ Index ];
            continue;
        }
        if ( !_stricmp( Have, "PhysicalHitboxBody" ) || !_stricmp( Have, "HitboxBody" ) || !_stricmp( Have, "HitboxBodySmall" ) ) {
            HitBody = List[ Index ];
            continue;
        }
        if ( !PartPos( List[ Index ], Spot[ Used ] ) )
            continue;
        Parts[ Used++ ] = List[ Index ];
    }
    if ( !Used ) {
        if ( !Item.part[ BoneHead ] )
            Item.part[ BoneHead ] = Item.part[ BoneRoot ] ? Item.part[ BoneRoot ] : ( HitHead ? HitHead : HitBody );
        ( void )NeedSkel;
        return;
    }

    int Head = 0;
    int Root = 0;
    float MidX = 0.0f;
    float MidZ = 0.0f;
    for ( int Index = 0; Index < Used; Index++ ) {
        MidX += Spot[ Index ].x;
        MidZ += Spot[ Index ].z;
        if ( Spot[ Index ].y > Spot[ Head ].y )
            Head = Index;
        if ( E.off.modelPrimary && Parts[ Index ] == Item.part[ BoneRoot ] )
            Root = Index;
    }
    MidX /= ( float )Used;
    MidZ /= ( float )Used;
    if ( !Item.part[ BoneRoot ] ) {
        float Best = 1.0e9f;
        for ( int Index = 0; Index < Used; Index++ ) {
            float Dx = Spot[ Index ].x - MidX;
            float Dz = Spot[ Index ].z - MidZ;
            float Score = Dx * Dx + Dz * Dz + ( Spot[ Index ].y - Spot[ Head ].y + 3.0f ) * ( Spot[ Index ].y - Spot[ Head ].y + 3.0f ) * 0.15f;
            if ( Score < Best ) {
                Best = Score;
                Root = Index;
            }
        }
    }
    for ( int Index = 0; Index < Used; Index++ ) {
        char Have[ 40 ] = { };
        if ( !InstName( Parts[ Index ], Have, ( int )sizeof( Have ) ) )
            continue;
        if ( _stricmp( Have, "Left Leg" ) == 0 ) {
            if ( !Item.part[ BoneLLegU ] )
                Item.part[ BoneLLegU ] = Parts[ Index ];
            if ( !Item.part[ BoneLLegL ] )
                Item.part[ BoneLLegL ] = Parts[ Index ];
            if ( !Item.part[ BoneLFoot ] )
                Item.part[ BoneLFoot ] = Parts[ Index ];
            continue;
        }
        if ( _stricmp( Have, "Right Leg" ) == 0 ) {
            if ( !Item.part[ BoneRLegU ] )
                Item.part[ BoneRLegU ] = Parts[ Index ];
            if ( !Item.part[ BoneRLegL ] )
                Item.part[ BoneRLegL ] = Parts[ Index ];
            if ( !Item.part[ BoneRFoot ] )
                Item.part[ BoneRFoot ] = Parts[ Index ];
            continue;
        }
        for ( int Slot = 0; Slot < BoneMax; Slot++ ) {
            if ( Item.part[ Slot ] )
                continue;
            if ( _stricmp( Have, BoneName[ Slot ] ) == 0 || _stricmp( Have, BoneNameR6[ Slot ] ) == 0 )
                Item.part[ Slot ] = Parts[ Index ];
        }
        if ( !_stricmp( Have, "UpperTorso" ) )
            Item.r15 = true;
        if ( !_stricmp( Have, "Torso" ) )
            Item.r15 = false;
    }
    if ( !Item.part[ BoneHead ] && HitHead )
        Item.part[ BoneHead ] = HitHead;
    if ( !Item.part[ BoneUpper ] && HitBody )
        Item.part[ BoneUpper ] = HitBody;
    if ( !Item.part[ BoneHead ] )
        Item.part[ BoneHead ] = Parts[ Head ];
    if ( !Item.part[ BoneRoot ] )
        Item.part[ BoneRoot ] = Parts[ Root ];
    Item.head = Spot[ Head ];
    Item.root = Spot[ Root ];
    Item.high = Spot[ Head ];
    Item.low = Spot[ Root ];
    for ( int Index = 0; Index < Used; Index++ ) {
        if ( Spot[ Index ].y < Item.low.y )
            Item.low = Spot[ Index ];
        if ( Spot[ Index ].y > Item.high.y )
            Item.high = Spot[ Index ];
    }
    Item.high.y += 0.12f;
    Item.low.y -= 0.08f;
    if ( !Item.part[ BoneUpper ] )
        Item.part[ BoneUpper ] = Item.part[ BoneRoot ];
    if ( !Item.part[ BoneLower ] )
        Item.part[ BoneLower ] = Item.part[ BoneUpper ] ? Item.part[ BoneUpper ] : Item.part[ BoneRoot ];

    auto ShareLimb = [ & ]( int Upper, int Lower, int Foot ) {
        if ( Item.part[ Upper ] && !Item.part[ Lower ] )
            Item.part[ Lower ] = Item.part[ Upper ];
        if ( Item.part[ Lower ] && !Item.part[ Foot ] )
            Item.part[ Foot ] = Item.part[ Lower ];
        if ( Item.part[ Upper ] && !Item.part[ Foot ] )
            Item.part[ Foot ] = Item.part[ Upper ];
    };
    ShareLimb( BoneLLegU, BoneLLegL, BoneLFoot );
    ShareLimb( BoneRLegU, BoneRLegL, BoneRFoot );

    auto FillNamedBone = [ & ]( const char* Name, int Slot ) {
        if ( Item.part[ Slot ] || !Name || !Name[ 0 ] )
            return;
        for ( int Index = 0; Index < Count; Index++ ) {
            if ( !IsNamed( List[ Index ], Name ) )
                continue;
            Vec3 Test;
            if ( !PartPos( List[ Index ], Test ) )
                continue;
            Item.part[ Slot ] = List[ Index ];
            return;
        }
    };
    FillNamedBone( "RightUpperLeg", BoneRLegU );
    FillNamedBone( "RightLowerLeg", BoneRLegL );
    FillNamedBone( "RightFoot", BoneRFoot );
    FillNamedBone( "LeftUpperLeg", BoneLLegU );
    FillNamedBone( "LeftLowerLeg", BoneLLegL );
    FillNamedBone( "LeftFoot", BoneLFoot );
    if ( !Item.part[ BoneRLegU ] )
        FillNamedBone( "Right Leg", BoneRLegU );
    if ( !Item.part[ BoneLLegU ] )
        FillNamedBone( "Left Leg", BoneLLegU );
    ShareLimb( BoneLLegU, BoneLLegL, BoneLFoot );
    ShareLimb( BoneRLegU, BoneRLegL, BoneRFoot );

    if ( NeedSkel ) {
        Vec3 LowerPos = Item.root;
        if ( Item.part[ BoneLower ] )
            PartPos( Item.part[ BoneLower ], LowerPos );
        Vec3 SplitOrigin = Item.root;
        if ( Item.part[ BoneUpper ] )
            PartPos( Item.part[ BoneUpper ], SplitOrigin );
        Vec3 Right = { 1.0f, 0.0f, 0.0f };
        float Rot[ 9 ] = { };
        uintptr_t RotPart = Item.part[ BoneRoot ] ? Item.part[ BoneRoot ] : Item.part[ BoneUpper ];
        if ( RotPart && PartRot( RotPart, Rot ) ) {
            Right.x = Rot[ 0 ];
            Right.z = Rot[ 6 ];
            float Len = sqrtf( Right.x * Right.x + Right.z * Right.z );
            if ( Len > 0.01f ) {
                Right.x /= Len;
                Right.z /= Len;
            }
        }

        struct LegCand {
            uintptr_t part = 0;
            float y = 0.0f;
        };
        LegCand RightLeg[ 8 ] = { };
        LegCand LeftLeg[ 8 ] = { };
        int RightN = 0;
        int LeftN = 0;

        for ( int Index = 0; Index < Used; Index++ ) {
            bool Taken = false;
            for ( int Slot = 0; Slot < BoneMax; Slot++ ) {
                if ( Item.part[ Slot ] == Parts[ Index ] ) {
                    Taken = true;
                    break;
                }
            }
            if ( Taken )
                continue;
            if ( Spot[ Index ].y > LowerPos.y - 0.35f )
                continue;
            float Dx = Spot[ Index ].x - SplitOrigin.x;
            float Dz = Spot[ Index ].z - SplitOrigin.z;
            float Side = Dx * Right.x + Dz * Right.z;
            if ( Side >= 0.05f && RightN < 8 ) {
                RightLeg[ RightN ].part = Parts[ Index ];
                RightLeg[ RightN ].y = Spot[ Index ].y;
                RightN++;
            } else if ( Side <= -0.05f && LeftN < 8 ) {
                LeftLeg[ LeftN ].part = Parts[ Index ];
                LeftLeg[ LeftN ].y = Spot[ Index ].y;
                LeftN++;
            }
        }

        auto SortLegs = []( LegCand* List, int Count ) {
            for ( int A = 0; A < Count; A++ ) {
                for ( int B = A + 1; B < Count; B++ ) {
                    if ( List[ B ].y > List[ A ].y ) {
                        LegCand Swap = List[ A ];
                        List[ A ] = List[ B ];
                        List[ B ] = Swap;
                    }
                }
            }
        };
        auto AssignLeg = [ & ]( LegCand* List, int Count, int Upper, int Lower, int Foot ) {
            if ( Count < 1 )
                return;
            SortLegs( List, Count );
            if ( !Item.part[ Upper ] )
                Item.part[ Upper ] = List[ 0 ].part;
            if ( !Item.part[ Lower ] ) {
                if ( Count > 1 )
                    Item.part[ Lower ] = List[ 1 ].part;
                else
                    Item.part[ Lower ] = List[ 0 ].part;
            }
            if ( !Item.part[ Foot ] ) {
                if ( Count > 2 )
                    Item.part[ Foot ] = List[ 2 ].part;
                else
                    Item.part[ Foot ] = List[ Count - 1 ].part;
            }
        };
        AssignLeg( LeftLeg, LeftN, BoneLLegU, BoneLLegL, BoneLFoot );
        AssignLeg( RightLeg, RightN, BoneRLegU, BoneRLegL, BoneRFoot );
    }
}

inline void Discover( bool NeedSkel ) {
    Engine& E = Core( );
    if ( !E.players )
        return;

    uintptr_t List[ KidMax ];
    int Count = Kids( E.players, List, KidMax );
    int Used = 0;
    for ( int Index = 0; Index < Count && Used < ActorMax; Index++ ) {
        uintptr_t Player = List[ Index ];
        if ( !IsClass( Player, "Player" ) )
            continue;

        Actor Item = { };
        Item.player = Player;
        Item.character = E.off.playerModel ? Ptr( Player + E.off.playerModel ) : 0;
        if ( !Item.character )
            continue;
        if ( !IsClass( Item.character, "Model" ) && !ChildNamed( Item.character, "Humanoid" ) )
            continue;

        Item.humanoid = ChildClass( Item.character, "Humanoid" );
        if ( !Item.humanoid )
            Item.humanoid = ChildNamed( Item.character, "Humanoid" );
        FillTeam( Player, Item.character, Item.team, Item.teamColor, Item.teamName, ( int )sizeof( Item.teamName ), true );
        Item.self = ( Player == E.local );
        Item.mate = IsMate( Item );

        if ( E.off.playerDisplay )
            LabelOf( Player, E.off.playerDisplay, Item.name, ( int )sizeof( Item.name ) );
        if ( !Item.name[ 0 ] )
            InstName( Player, Item.name, ( int )sizeof( Item.name ) );
        if ( !Item.name[ 0 ] && Item.humanoid && E.off.humanoidName )
            LabelOf( Item.humanoid, E.off.humanoidName, Item.name, ( int )sizeof( Item.name ) );
        if ( !Item.name[ 0 ] )
            InstName( Item.character, Item.name, ( int )sizeof( Item.name ) );
        if ( !Item.name[ 0 ] )
            lstrcpynA( Item.name, "Player", ( int )sizeof( Item.name ) );

        int Rig = 1;
        if ( Item.humanoid && E.off.humanoidRig )
            Pull( Item.humanoid + E.off.humanoidRig, Rig );
        Item.r15 = Rig != 0;
        BindBones( Item, NeedSkel );
        E.track[ Used++ ] = Item;
    }
    E.tracked = Used;
    E.colorUseful = false;
    int SeenColor = 0;
    bool Spread = false;
    for ( int Index = 0; Index < E.tracked; Index++ ) {
        int Have = E.track[ Index ].teamColor;
        if ( !Have )
            continue;
        if ( !SeenColor )
            SeenColor = Have;
        else if ( Have != SeenColor )
            Spread = true;
    }
    E.colorUseful = Spread;
    int PtrN = 0;
    int NameN = 0;
    int ColorN = 0;
    int Teamed = 0;
    uintptr_t Ptrs[ 12 ] = { };
    char Names[ 12 ][ 32 ] = { };
    int Colors[ 12 ] = { };
    auto PushU64 = [ ]( uintptr_t* List, int& Used, uintptr_t Have ) {
        if ( !Have )
            return;
        for ( int Index = 0; Index < Used; Index++ )
            if ( List[ Index ] == Have )
                return;
        if ( Used < 12 )
            List[ Used++ ] = Have;
    };
    auto PushName = [ ]( char List[ ][ 32 ], int& Used, const char* Have ) {
        if ( !Have || !Have[ 0 ] )
            return;
        for ( int Index = 0; Index < Used; Index++ )
            if ( !_stricmp( List[ Index ], Have ) )
                return;
        if ( Used < 12 )
            lstrcpynA( List[ Used++ ], Have, 32 );
    };
    auto PushInt = [ ]( int* List, int& Used, int Have ) {
        if ( !Have )
            return;
        for ( int Index = 0; Index < Used; Index++ )
            if ( List[ Index ] == Have )
                return;
        if ( Used < 12 )
            List[ Used++ ] = Have;
    };
    if ( E.localTeam )
        Teamed++;
    PushU64( Ptrs, PtrN, E.localTeam );
    PushName( Names, NameN, E.localTeamName );
    PushInt( Colors, ColorN, E.localTeamColor );
    for ( int Index = 0; Index < E.tracked; Index++ ) {
        if ( E.track[ Index ].self )
            continue;
        if ( E.track[ Index ].team )
            Teamed++;
        PushU64( Ptrs, PtrN, E.track[ Index ].team );
        PushName( Names, NameN, E.track[ Index ].teamName );
        PushInt( Colors, ColorN, E.track[ Index ].teamColor );
    }
    if ( E.placeId )
        sense::DecideTeam( E.placeId, PtrN, NameN, ColorN, Teamed );
    for ( int Index = 0; Index < E.tracked; Index++ )
        E.track[ Index ].mate = IsMate( E.track[ Index ] );
}

inline bool Project( const Vec3& World, const float* M, int Wide, int Tall, Dot& Out ) {
    float X = World.x * M[ 0 ] + World.y * M[ 1 ] + World.z * M[ 2 ] + M[ 3 ];
    float Y = World.x * M[ 4 ] + World.y * M[ 5 ] + World.z * M[ 6 ] + M[ 7 ];
    float W = World.x * M[ 12 ] + World.y * M[ 13 ] + World.z * M[ 14 ] + M[ 15 ];
    if ( W < 0.05f )
        return false;
    float Inv = 1.0f / W;
    Out.x = ( Wide * 0.5f ) + ( X * Inv ) * ( Wide * 0.5f );
    Out.y = ( Tall * 0.5f ) - ( Y * Inv ) * ( Tall * 0.5f );
    Out.ok = Out.x > -200.0f && Out.x < ( float )Wide + 200.0f && Out.y > -200.0f && Out.y < ( float )Tall + 200.0f;
    return Out.ok;
}

inline float Dist( const Vec3& A, const Vec3& B ) {
    float X = A.x - B.x;
    float Y = A.y - B.y;
    float Z = A.z - B.z;
    return sqrtf( X * X + Y * Y + Z * Z );
}

inline void ClientBox( ) {
    Engine& E = Core( );
    Snap& S = E.front;
    S.clientX = 0;
    S.clientY = 0;
    S.clientW = GetSystemMetrics( SM_CXSCREEN );
    S.clientH = GetSystemMetrics( SM_CYSCREEN );
    if ( !E.window || !IsWindow( E.window ) ) {
        E.window = FindHwnd( E.pid );
        if ( !E.window )
            return;
    }
    RECT Box = { };
    if ( !GetClientRect( E.window, &Box ) )
        return;
    POINT Origin{ 0, 0 };
    ClientToScreen( E.window, &Origin );
    S.clientX = Origin.x;
    S.clientY = Origin.y;
    S.clientW = Box.right - Box.left;
    S.clientH = Box.bottom - Box.top;
    if ( S.clientW < 64 )
        S.clientW = GetSystemMetrics( SM_CXSCREEN );
    if ( S.clientH < 64 )
        S.clientH = GetSystemMetrics( SM_CYSCREEN );
}

inline bool OwnPart( uintptr_t Part ) {
    Engine& E = Core( );
    for ( int Index = 0; Index < E.tracked; Index++ ) {
        for ( int Bone = 0; Bone < BoneMax; Bone++ ) {
            if ( E.track[ Index ].part[ Bone ] == Part )
                return true;
        }
        if ( E.track[ Index ].character == Part )
            return true;
    }
    return false;
}

inline bool PartBlocks( uintptr_t Part ) {
    Engine& E = Core( );
    if ( !E.off.primFlags || !E.off.partPrim )
        return true;
    uintptr_t Prim = Ptr( Part + E.off.partPrim );
    if ( !Prim )
        return false;
    uint16_t Flags = 0;
    if ( !Pull( Prim + E.off.primFlags, Flags ) )
        return true;
    return ( Flags & 0x28 ) != 0;
}

inline bool IsSolid( const char* Kind ) {
    if ( !Kind || !Kind[ 0 ] )
        return false;
    return !_stricmp( Kind, "Part" ) || !_stricmp( Kind, "MeshPart" ) || !_stricmp( Kind, "WedgePart" )
        || !_stricmp( Kind, "CornerWedgePart" ) || !_stricmp( Kind, "TrussPart" ) || !_stricmp( Kind, "UnionOperation" )
        || !_stricmp( Kind, "Ball" ) || !_stricmp( Kind, "Cylinder" ) || !_stricmp( Kind, "TriangleMeshPart" )
        || !_stricmp( Kind, "PartOperation" ) || !_stricmp( Kind, "SmoothVoxelPart" );
}

inline bool IsHolder( const char* Kind ) {
    if ( !Kind || !Kind[ 0 ] )
        return false;
    return !_stricmp( Kind, "Folder" ) || !_stricmp( Kind, "Model" ) || !_stricmp( Kind, "Actor" )
        || !_stricmp( Kind, "WorldModel" ) || !_stricmp( Kind, "WorldRoot" );
}

inline void RefreshWalls( ) {
    Engine& E = Core( );
    unsigned Now = GetTickCount( );
    Vec3 Cam = E.front.camera;
    float Moved = Dist( Cam, E.wallCam );
    if ( Now < E.nextWalls )
        return;
    if ( E.wallN > 0 && Moved < 220.0f ) {
        E.nextWalls = Now + 1200;
        return;
    }
    E.nextWalls = Now + 1400;
    E.wallCam = Cam;
    E.wallN = 0;
    if ( !E.workspace )
        return;

    uintptr_t Queue[ 128 ];
    int Depth[ 128 ];
    int Used = 0;
    Queue[ Used ] = E.workspace;
    Depth[ Used ] = 0;
    Used++;
    int Seen = 0;
    for ( int Cursor = 0; Cursor < Used && Seen < 96 && E.wallN < WallMax; Cursor++ ) {
        Seen++;
        if ( Depth[ Cursor ] > 6 )
            continue;
        uintptr_t List[ KidMax ];
        int Count = Kids( Queue[ Cursor ], List, KidMax );
        for ( int Index = 0; Index < Count && E.wallN < WallMax; Index++ ) {
            char Kind[ 40 ] = { };
            if ( !ClassName( List[ Index ], Kind, ( int )sizeof( Kind ) ) )
                continue;
            if ( IsSolid( Kind ) ) {
                if ( OwnPart( List[ Index ] ) )
                    continue;
                if ( !PartBlocks( List[ Index ] ) )
                    continue;
                Vec3 Center;
                Vec3 Half;
                float Rot[ 9 ];
                if ( !PartFrame( List[ Index ], Center, Half, Rot ) )
                    continue;
                float Sx = Half.x * 2.0f;
                float Sy = Half.y * 2.0f;
                float Sz = Half.z * 2.0f;
                float Mn = Sx < Sy ? Sx : Sy;
                if ( Sz < Mn )
                    Mn = Sz;
                float Mx = Sx > Sy ? Sx : Sy;
                if ( Sz > Mx )
                    Mx = Sz;
                if ( !( Mn >= 0.14f && Mx <= 640.0f ) )
                    continue;
                if ( Sx > 36.0f && Sy > 36.0f && Sz > 36.0f )
                    continue;
                E.walls[ E.wallN ] = List[ Index ];
                E.wallC[ E.wallN ] = Center;
                E.wallH[ E.wallN ] = Half;
                memcpy( E.wallR[ E.wallN ], Rot, sizeof( Rot ) );
                E.wallN++;
                continue;
            }
            if ( !IsHolder( Kind ) || Used >= 128 )
                continue;
            bool Character = false;
            for ( int Track = 0; Track < E.tracked && !Character; Track++ )
                if ( E.track[ Track ].character == List[ Index ] )
                    Character = true;
            if ( Character )
                continue;
            Queue[ Used ] = List[ Index ];
            Depth[ Used ] = Depth[ Cursor ] + 1;
            Used++;
        }
    }
}

inline bool EnsureWrite( ) {
    Engine& E = Core( );
    if ( E.writeOk && E.process )
        return true;
    if ( !E.process || !E.pid )
        return false;
    HANDLE Handle = OpenProcess( PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, E.pid );
    if ( !Handle )
        return false;
    CloseHandle( E.process );
    E.process = Handle;
    E.writeOk = true;
    return true;
}

inline bool WriteFps( uintptr_t Addr, double Cap ) {
    double Have = 0.0;
    if ( Pull( Addr, Have ) && Have > 0.5 && Have < 20000.0 )
        return Poke( Addr, Cap );
    if ( Have > 0.0004 && Have < 0.08 )
        return Poke( Addr, 1.0 / Cap );
    float Small = 0.0f;
    if ( Pull( Addr, Small ) && Small > 0.5f && Small < 20000.0f )
        return Poke( Addr, ( float )Cap );
    return false;
}

inline bool SetFps( double Cap ) {
    Engine& E = Core( );
    if ( Cap < 1.0 )
        Cap = 10000.0;
    if ( !Attach( ) )
        return false;
    if ( !E.off.taskPtr || !E.off.taskFps )
        return false;
    uintptr_t Sched = Ptr( E.base + E.off.taskPtr );
    if ( !Sched )
        return false;
    if ( !EnsureWrite( ) )
        return false;
    bool Ok = WriteFps( Sched + E.off.taskFps, Cap );
    if ( E.off.taskJobs && E.off.taskJobEnd ) {
        uintptr_t Begin = 0;
        uintptr_t End = 0;
        Pull( Sched + E.off.taskJobs, Begin );
        Pull( Sched + E.off.taskJobEnd, End );
        int Seen = 0;
        for ( uintptr_t At = Begin; Heap( At ) && End && At < End && Seen < 48; At += 16, Seen++ ) {
            uintptr_t Job = Ptr( At );
            if ( Job )
                WriteFps( Job + E.off.taskFps, Cap );
        }
    }
    return Ok;
}

inline HWND GameWindow( ) {
    Engine& E = Core( );
    if ( E.window && IsWindow( E.window ) )
        return E.window;
    E.window = FindHwnd( E.pid );
    return E.window;
}

inline bool RayBlocked( const Vec3& Eye, const Vec3& Point ) {
    Engine& E = Core( );
    float Reach = Dist( Eye, Point );
    if ( Reach < 0.35f )
        return false;
    Vec3 Dir;
    Dir.x = ( Point.x - Eye.x ) / Reach;
    Dir.y = ( Point.y - Eye.y ) / Reach;
    Dir.z = ( Point.z - Eye.z ) / Reach;
    float Limit = Reach - 1.55f;
    if ( Limit < 0.20f )
        return false;
    for ( int Wall = 0; Wall < E.wallN; Wall++ ) {
        if ( Dist( E.wallC[ Wall ], Point ) < 2.4f )
            continue;
        Vec3 Half = E.wallH[ Wall ];
        if ( Half.x > 0.06f ) Half.x -= 0.06f;
        if ( Half.y > 0.06f ) Half.y -= 0.06f;
        if ( Half.z > 0.06f ) Half.z -= 0.06f;
        float Ext = Half.x;
        if ( Half.y > Ext )
            Ext = Half.y;
        if ( Half.z > Ext )
            Ext = Half.z;
        if ( Dist( Eye, E.wallC[ Wall ] ) > Reach + Ext )
            continue;
        if ( SlabHit( Eye, Dir, Limit, E.wallC[ Wall ], Half, E.wallR[ Wall ] ) )
            return true;
    }
    return false;
}

inline bool ActorClear( const Actor& Item, const Vec3& Eye, const Vec3& ) {
    if ( !RayBlocked( Eye, Item.head ) )
        return true;
    Vec3 Mid;
    Mid.x = ( Item.head.x + Item.root.x ) * 0.5f;
    Mid.y = ( Item.head.y + Item.root.y ) * 0.5f;
    Mid.z = ( Item.head.z + Item.root.z ) * 0.5f;
    if ( !RayBlocked( Eye, Mid ) )
        return true;
    if ( Item.root.x != Item.head.x || Item.root.y != Item.head.y || Item.root.z != Item.head.z )
        return !RayBlocked( Eye, Item.root );
    return false;
}

inline bool FrameView( ) {
    Engine& E = Core( );
    Snap& S = E.front;
    if ( !Attach( ) || !Resolve( ) ) {
        S.attached = E.process != nullptr;
        return false;
    }
    S.attached = true;
    ClientBox( );
    S.viewW = S.clientW;
    S.viewH = S.clientH;
    if ( E.visual && E.off.visDim ) {
        int Dim[ 2 ] = { };
        if ( Pull( E.visual + E.off.visDim, Dim ) && Dim[ 0 ] > 64 && Dim[ 1 ] > 64 && Dim[ 0 ] < 8000 && Dim[ 1 ] < 8000 ) {
            S.viewW = Dim[ 0 ];
            S.viewH = Dim[ 1 ];
        }
    }
    if ( E.visual && E.off.visView )
        Pull( E.visual + E.off.visView, S.view );
    if ( E.workspace && E.off.workspaceCam ) {
        uintptr_t Cam = Ptr( E.workspace + E.off.workspaceCam );
        if ( Cam && E.off.camPos )
            Pull( Cam + E.off.camPos, S.camera );
    }
    S.ready = true;
    return true;
}

inline float NormPing( float Raw ) {
    if ( !( Raw >= 0.0f ) || Raw > 2000.0f )
        return 0.0f;
    if ( Raw > 1.0f )
        return Raw / 1000.0f;
    return Raw;
}

inline float ReadPingValue( uintptr_t Item ) {
    if ( !Item || !Core( ).off.statsVal )
        return 0.0f;
    float Small = 0.0f;
    if ( Pull( Item + Core( ).off.statsVal, Small ) ) {
        float Have = NormPing( Small );
        if ( Have > 0.0f )
            return Have;
    }
    double Wide = 0.0;
    if ( Pull( Item + Core( ).off.statsVal, Wide ) )
        return NormPing( ( float )Wide );
    return 0.0f;
}

inline uintptr_t FindPingItem( uintptr_t Network ) {
    if ( !Network )
        return 0;
    uintptr_t Named = ChildNamed( Network, "ServerStatsItem" );
    if ( Named )
        return Named;
    uintptr_t List[ 64 ];
    int Count = Kids( Network, List, 64 );
    uintptr_t Any = 0;
    for ( int Index = 0; Index < Count; Index++ ) {
        char Kind[ 40 ] = { };
        char Name[ 40 ] = { };
        ClassName( List[ Index ], Kind, ( int )sizeof( Kind ) );
        InstName( List[ Index ], Name, ( int )sizeof( Name ) );
        if ( !_stricmp( Kind, "ServerStatsItem" ) || !_stricmp( Name, "ServerStatsItem" ) )
            return List[ Index ];
        if ( !Any && ( strstr( Name, "Ping" ) || strstr( Name, "ping" ) ) )
            Any = List[ Index ];
    }
    return Any;
}

inline void BindPing( ) {
    Engine& E = Core( );
    unsigned Now = GetTickCount( );
    if ( !E.pingItem || Now >= E.nextPing ) {
        E.nextPing = Now + 800;
        if ( E.dataModel ) {
            if ( !E.stats )
                E.stats = FindService( E.dataModel, "Stats" );
            uintptr_t Net = E.stats ? ChildNamed( E.stats, "Network" ) : 0;
            if ( !Net && E.stats )
                Net = ChildClass( E.stats, "NetworkClient" );
            E.pingItem = FindPingItem( Net );
        }
    }
    float Have = ReadPingValue( E.pingItem );
    if ( Have > 0.0f )
        E.localPing = Have;
    E.front.localPing = E.localPing;
}

inline float PlayerPing( uintptr_t Player ) {
    Engine& E = Core( );
    if ( Player && E.off.playerPing ) {
        float Raw = 0.0f;
        if ( Pull( Player + E.off.playerPing, Raw ) ) {
            float Have = NormPing( Raw );
            if ( Have > 0.0f )
                return Have;
        }
    }
    return E.localPing;
}

inline void Pulse( bool Want, bool NeedAim, bool Skel, float Range, bool NeedVis ) {
    Engine& E = Core( );
    Snap& S = E.front;
    S.attached = E.process != nullptr;
    if ( !Want ) {
        S.count = 0;
        return;
    }
    if ( !E.off.fakeDm && offsets::Ready( ) )
        LoadOff( );
    if ( !Attach( ) || !Resolve( ) ) {
        S.ready = false;
        S.count = 0;
        return;
    }
    if ( !E.workspace || !E.players ) {
        S.ready = false;
        S.count = 0;
        return;
    }
    BindLocal( );
    BindPing( );

    unsigned Now = GetTickCount( );
    if ( Now >= E.nextDiscover || E.tracked == 0 ) {
        Discover( Skel );
        E.nextDiscover = Now + 700;
    }
    BindLocal( );

    ClientBox( );
    S.viewW = S.clientW;
    S.viewH = S.clientH;
    if ( E.visual && E.off.visDim ) {
        int Dim[ 2 ] = { };
        if ( Pull( E.visual + E.off.visDim, Dim ) && Dim[ 0 ] > 64 && Dim[ 1 ] > 64 && Dim[ 0 ] < 8000 && Dim[ 1 ] < 8000 ) {
            S.viewW = Dim[ 0 ];
            S.viewH = Dim[ 1 ];
        }
    }
    if ( E.visual && E.off.visView )
        Pull( E.visual + E.off.visView, S.view );
    if ( E.workspace && E.off.workspaceCam ) {
        uintptr_t Cam = Ptr( E.workspace + E.off.workspaceCam );
        if ( Cam && E.off.camPos )
            Pull( Cam + E.off.camPos, S.camera );
        if ( Cam && E.off.camRot ) {
            float Rot[ 9 ] = { };
            if ( Pull( Cam + E.off.camRot, Rot, sizeof( Rot ) ) ) {
                S.right.x = Rot[ 0 ];
                S.right.y = Rot[ 3 ];
                S.right.z = Rot[ 6 ];
                float Len = sqrtf( S.right.x * S.right.x + S.right.y * S.right.y + S.right.z * S.right.z );
                if ( Len > 0.01f ) {
                    S.right.x /= Len;
                    S.right.y /= Len;
                    S.right.z /= Len;
                }
                if ( S.camera.x == 0.0f && S.camera.y == 0.0f && S.camera.z == 0.0f && E.off.camPos )
                    Pull( Cam + E.off.camPos, S.camera );
            }
        }
    }

    Vec3 Origin = S.camera;
    Vec3 Root;
    if ( PartPos( E.localRoot, Root ) )
        Origin = Root;
    if ( NeedVis )
        RefreshWalls( );
    if ( E.placeId && NeedVis )
        sense::DecideVis( E.placeId, E.wallN );
    bool DoVis = NeedVis;

    int Used = 0;
    for ( int Index = 0; Index < E.tracked && Used < ActorMax; Index++ ) {
        Actor Item = E.track[ Index ];
        memset( Item.world, 0, sizeof( Item.world ) );
        memset( Item.boneOk, 0, sizeof( Item.boneOk ) );
        FillTeam( Item.player, Item.character, Item.team, Item.teamColor, Item.teamName, ( int )sizeof( Item.teamName ), false );
        bool NowMate = IsMate( Item );
        int MateGood = E.track[ Index ].mateGood;
        int MateBad = E.track[ Index ].mateBad;
        if ( NowMate ) {
            MateGood++;
            MateBad = 0;
            if ( MateGood > 10 )
                MateGood = 10;
        } else {
            MateBad++;
            MateGood = 0;
            if ( MateBad > 10 )
                MateBad = 10;
        }
        if ( MateGood >= 4 )
            Item.mate = true;
        else if ( MateBad >= 4 )
            Item.mate = false;
        else
            Item.mate = E.track[ Index ].mate;
        Item.mateGood = MateGood;
        Item.mateBad = MateBad;
        E.track[ Index ].team = Item.team;
        E.track[ Index ].teamColor = Item.teamColor;
        E.track[ Index ].mate = Item.mate;
        E.track[ Index ].mateGood = MateGood;
        E.track[ Index ].mateBad = MateBad;
        lstrcpynA( E.track[ Index ].teamName, Item.teamName, ( int )sizeof( E.track[ Index ].teamName ) );
        if ( Item.self )
            continue;
        if ( !Item.character || !Heap( Item.character ) )
            continue;

        if ( !PartPos( Item.part[ BoneHead ], Item.head ) ) {
            continue;
        }
        if ( !PartPos( Item.part[ BoneRoot ], Item.root ) )
            Item.root = Item.head;
        Item.vel = { };
        if ( Item.part[ BoneRoot ] && Item.part[ BoneRoot ] != Item.part[ BoneHead ] )
            PartVel( Item.part[ BoneRoot ], Item.vel );
        if ( Item.vel.x == 0.0f && Item.vel.y == 0.0f && Item.vel.z == 0.0f )
            PartVel( Item.part[ BoneHead ], Item.vel );
        float Speed2 = Item.vel.x * Item.vel.x + Item.vel.y * Item.vel.y + Item.vel.z * Item.vel.z;
        if ( !( Speed2 >= 0.0f ) || Speed2 > 16000.0f )
            Item.vel = { };

        Item.health = 100.0f;
        Item.maxHealth = 100.0f;
        if ( Item.humanoid ) {
            Pull( Item.humanoid + E.off.humanoidHealth, Item.health );
            Pull( Item.humanoid + E.off.humanoidMax, Item.maxHealth );
        }
        if ( !( Item.maxHealth > 1.0f && Item.maxHealth < 20000.0f ) )
            Item.maxHealth = 100.0f;
        if ( !( Item.health >= 0.0f && Item.health <= Item.maxHealth * 1.5f ) )
            Item.health = Item.maxHealth;
        if ( Item.health <= 0.05f ) {
            continue;
        }

        Item.dist = Dist( Origin, Item.root );
        Item.ping = PlayerPing( Item.player );
        if ( Range > 1.0f && Item.dist > Range )
            continue;
        float Hip = 0.0f;
        if ( Item.humanoid && E.off.humanoidHip )
            Pull( Item.humanoid + E.off.humanoidHip, Hip );
        if ( !( Hip > 0.35f && Hip < 6.5f ) )
            Hip = Item.r15 ? 2.0f : 2.0f;
        Vec3 HeadSz;
        if ( !PartSize( Item.part[ BoneHead ], HeadSz ) || !( HeadSz.y > 0.15f && HeadSz.y < 3.5f ) ) {
            HeadSz.x = 1.2f;
            HeadSz.y = 1.2f;
            HeadSz.z = 1.2f;
        }
        Item.high = Item.head;
        Item.high.y += HeadSz.y * 0.5f + 0.12f;
        Item.low = Item.root;
        float Foot = Item.root.y - Hip - 0.45f;
        if ( Item.head.y - Foot < 2.8f )
            Foot = Item.head.y - 5.2f;
        Item.low.y = Foot;
        if ( NeedAim || Skel ) {
            static const int AimBones[ ] = { BoneUpper, BoneLower, BoneLLegU, BoneRLegU, BoneLFoot, BoneRFoot };
            for ( int Slot = 0; Slot < 6; Slot++ ) {
                int Bone = AimBones[ Slot ];
                Item.boneOk[ Bone ] = Item.part[ Bone ] && PartPos( Item.part[ Bone ], Item.world[ Bone ] );
                if ( !Item.boneOk[ Bone ] )
                    continue;
                if ( Item.world[ Bone ].y > Item.high.y )
                    Item.high = Item.world[ Bone ];
                if ( Item.world[ Bone ].y < Item.low.y )
                    Item.low = Item.world[ Bone ];
            }
        }
        if ( Skel ) {
            for ( int BoneIndex = 0; BoneIndex < BoneMax; BoneIndex++ ) {
                if ( Item.boneOk[ BoneIndex ] )
                    continue;
                Item.boneOk[ BoneIndex ] = Item.part[ BoneIndex ] && PartPos( Item.part[ BoneIndex ], Item.world[ BoneIndex ] );
                if ( !Item.boneOk[ BoneIndex ] )
                    continue;
                if ( Item.world[ BoneIndex ].y > Item.high.y )
                    Item.high = Item.world[ BoneIndex ];
                if ( Item.world[ BoneIndex ].y < Item.low.y )
                    Item.low = Item.world[ BoneIndex ];
            }
        }
        Item.world[ BoneHead ] = Item.head;
        Item.boneOk[ BoneHead ] = true;
        Item.world[ BoneRoot ] = Item.root;
        Item.boneOk[ BoneRoot ] = true;

        Item.vis = true;
        Item.visBad = 0;
        Item.visGood = 0;
        if ( DoVis ) {
            bool Was = E.track[ Index ].vis;
            int Good = E.track[ Index ].visGood;
            int Bad = E.track[ Index ].visBad;
            Dot Screen = { };
            bool Drawn = S.viewW > 8 && Project( Item.head, S.view, S.viewW, S.viewH, Screen )
                && Screen.x > -80.0f && Screen.y > -80.0f
                && Screen.x < ( float )S.viewW + 80.0f && Screen.y < ( float )S.viewH + 80.0f;
            if ( E.wallN < 1 || !Drawn ) {
                Item.vis = Was;
                Item.visGood = Good;
                Item.visBad = Bad;
            } else {
                Vec3 Eye = S.camera;
                if ( Eye.x == 0.0f && Eye.y == 0.0f && Eye.z == 0.0f )
                    Eye = Origin;
                bool Clear = ActorClear( Item, Eye, S.right );
                if ( Clear ) {
                    Good++;
                    Bad = 0;
                    if ( Good > 16 )
                        Good = 16;
                } else {
                    Bad++;
                    Good = 0;
                    if ( Bad > 28 )
                        Bad = 28;
                }
                if ( Good >= 6 )
                    Item.vis = true;
                else if ( Bad >= 20 )
                    Item.vis = false;
                else
                    Item.vis = Was;
                Item.visGood = Good;
                Item.visBad = Bad;
            }
        }

        E.track[ Index ] = Item;
        S.list[ Used++ ] = Item;
    }
    S.count = Used;
    if ( Used == 0 && E.tracked > 1 )
        E.nextDiscover = Now;
    S.local = E.local;
    S.localTeam = E.localTeam;
    S.ready = true;
    snprintf( S.note, sizeof( S.note ), "%d players", Used );
}

inline bool ToView( const Vec3& World, Dot& Out ) {
    const Snap& S = View( );
    if ( !S.ready || S.viewW < 8 || S.viewH < 8 )
        return false;
    return Project( World, S.view, S.viewW, S.viewH, Out );
}

inline bool ToScreen( const Vec3& World, Dot& Out ) {
    const Snap& S = View( );
    if ( !S.ready || S.viewW < 8 || S.viewH < 8 )
        return false;
    if ( !Project( World, S.view, S.viewW, S.viewH, Out ) )
        return false;
    if ( S.viewW != S.clientW && S.viewW > 0 ) {
        Out.x *= ( float )S.clientW / ( float )S.viewW;
        Out.y *= ( float )S.clientH / ( float )S.viewH;
    }
    Out.x += ( float )S.clientX;
    Out.y += ( float )S.clientY;
    return true;
}

inline void Boot( ) {
    LoadOff( );
}

}
