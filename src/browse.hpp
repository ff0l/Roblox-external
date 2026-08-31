#pragma once

#include "world.hpp"
#include "explorer.hpp"
#include <cstdio>
#include <cstring>

namespace browse {

inline constexpr int NodeMax = 2048;
inline constexpr int KidCap = 256;
inline constexpr int PathCap = 256;

struct Node {
    uintptr_t addr = 0;
    uintptr_t parent = 0;
    char name[ 48 ] = { };
    char klass[ 40 ] = { };
    int kidN = 0;
    int extra = 0;
    bool fetched = false;
    bool open = false;
};

struct Prop {
    char label[ 24 ] = { };
    char text[ 72 ] = { };
    int write = 0;
};

struct Session {
    Node nodes[ NodeMax ];
    int used = 0;
    uintptr_t root = 0;
    uintptr_t pick = 0;
    unsigned next = 0;
    bool live = false;
    char query[ 48 ] = { };
    char path[ PathCap ] = { };
    char note[ 80 ] = { };
    Prop props[ 16 ];
    int propN = 0;
};

inline Session& Core( ) {
    static Session Store;
    return Store;
}

inline bool SkipClass( const char* Klass ) {
    if ( !Klass || !Klass[ 0 ] )
        return false;
    return !_stricmp( Klass, "CoreGui" ) || !_stricmp( Klass, "CorePackages" )
        || !_stricmp( Klass, "CoreReplicator" ) || !_stricmp( Klass, "HttpRbxApiService" );
}

inline bool IsKind( const char* Klass, const char* Want ) {
    return Klass && Want && !_stricmp( Klass, Want );
}

inline bool IsPart( const char* Klass ) {
    if ( !Klass || !Klass[ 0 ] )
        return false;
    return IsKind( Klass, "Part" ) || IsKind( Klass, "MeshPart" ) || IsKind( Klass, "SpawnLocation" )
        || IsKind( Klass, "WedgePart" ) || IsKind( Klass, "CornerWedgePart" ) || IsKind( Klass, "TrussPart" )
        || IsKind( Klass, "UnionOperation" ) || strstr( Klass, "Part" ) != nullptr;
}

inline bool IsValue( const char* Klass ) {
    if ( !Klass )
        return false;
    return strstr( Klass, "Value" ) != nullptr;
}

inline bool IContains( const char* Hay, const char* Needle ) {
    if ( !Needle || !Needle[ 0 ] )
        return true;
    if ( !Hay )
        return false;
    size_t Need = strlen( Needle );
    for ( const char* At = Hay; *At; At++ ) {
        if ( _strnicmp( At, Needle, Need ) == 0 )
            return true;
    }
    return false;
}

inline bool Hits( const Node& Item, const char* Query ) {
    if ( !Query || !Query[ 0 ] )
        return true;
    return IContains( Item.name, Query ) || IContains( Item.klass, Query );
}

inline int Find( uintptr_t Addr ) {
    Session& S = Core( );
    if ( !Addr )
        return -1;
    for ( int Index = 0; Index < S.used; Index++ ) {
        if ( S.nodes[ Index ].addr == Addr )
            return Index;
    }
    return -1;
}

inline Node* Get( uintptr_t Addr ) {
    int Index = Find( Addr );
    return Index >= 0 ? &Core( ).nodes[ Index ] : nullptr;
}

inline int Add( uintptr_t Addr, uintptr_t Parent ) {
    Session& S = Core( );
    int Have = Find( Addr );
    if ( Have >= 0 ) {
        S.nodes[ Have ].parent = Parent;
        return Have;
    }
    if ( S.used >= NodeMax )
        return -1;
    Node& Item = S.nodes[ S.used ];
    Item = { };
    Item.addr = Addr;
    Item.parent = Parent;
    world::ClassName( Addr, Item.klass, ( int )sizeof( Item.klass ) );
    if ( SkipClass( Item.klass ) )
        return -1;
    world::TagName( Addr, Item.klass[ 0 ] ? Item.klass : "Inst", Item.name, ( int )sizeof( Item.name ) );
    return S.used++;
}

inline void Fill( Node& Item ) {
    world::ClassName( Item.addr, Item.klass, ( int )sizeof( Item.klass ) );
    world::TagName( Item.addr, Item.klass[ 0 ] ? Item.klass : "Inst", Item.name, ( int )sizeof( Item.name ) );
    uintptr_t Have = world::ParentOf( Item.addr );
    if ( Have )
        Item.parent = Have;
}

inline void Expand( uintptr_t Addr ) {
    Node* Item = Get( Addr );
    if ( !Item )
        return;
    uintptr_t List[ KidCap ];
    int Count = world::Kids( Addr, List, KidCap );
    Item->kidN = 0;
    Item->extra = 0;
    Item->fetched = true;
    for ( int Index = 0; Index < Count; Index++ ) {
        char Kind[ 40 ] = { };
        world::ClassName( List[ Index ], Kind, ( int )sizeof( Kind ) );
        if ( SkipClass( Kind ) )
            continue;
        if ( Item->kidN >= KidCap ) {
            Item->extra++;
            continue;
        }
        int Slot = Add( List[ Index ], Addr );
        if ( Slot < 0 )
            continue;
        Item->kidN++;
    }
    if ( Count >= KidCap )
        Item->extra += 1;
}

inline void Close( ) {
    Session& S = Core( );
    S = { };
}

inline bool Open( ) {
    Session& S = Core( );
    Close( );
    if ( !world::Attach( ) || !world::Resolve( ) ) {
        lstrcpynA( S.note, "Roblox not attached", ( int )sizeof( S.note ) );
        return false;
    }
    uintptr_t Data = world::DataModel( );
    if ( !Data ) {
        lstrcpynA( S.note, "DataModel missing", ( int )sizeof( S.note ) );
        return false;
    }
    int Root = Add( Data, 0 );
    if ( Root < 0 )
        return false;
    S.root = Data;
    S.pick = Data;
    S.nodes[ Root ].open = true;
    lstrcpynA( S.nodes[ Root ].name, "game", ( int )sizeof( S.nodes[ Root ].name ) );
    lstrcpynA( S.nodes[ Root ].klass, "DataModel", ( int )sizeof( S.nodes[ Root ].klass ) );
    Expand( Data );
    S.live = true;
    S.note[ 0 ] = 0;
    return true;
}

inline void Toggle( uintptr_t Addr ) {
    Node* Item = Get( Addr );
    if ( !Item )
        return;
    Item->open = !Item->open;
    if ( Item->open && !Item->fetched )
        Expand( Addr );
}

inline void Select( uintptr_t Addr ) {
    Core( ).pick = Addr;
}

inline uintptr_t Pick( ) {
    return Core( ).pick;
}

inline uintptr_t Root( ) {
    return Core( ).root;
}

inline bool Live( ) {
    return Core( ).live;
}

inline void PathOf( uintptr_t Addr, char* Out, int Cap ) {
    if ( !Out || Cap < 8 )
        return;
    Out[ 0 ] = 0;
    uintptr_t Stack[ 24 ];
    int Depth = 0;
    uintptr_t At = Addr;
    Session& S = Core( );
    while ( At && Depth < 24 ) {
        Stack[ Depth++ ] = At;
        if ( At == S.root )
            break;
        uintptr_t Up = world::ParentOf( At );
        if ( !Up || Up == At )
            break;
        At = Up;
    }
    for ( int Index = Depth - 1; Index >= 0; Index-- ) {
        Node* Item = Get( Stack[ Index ] );
        const char* Name = Item ? Item->name : "Inst";
        if ( Stack[ Index ] == S.root )
            Name = "game";
        if ( Out[ 0 ] )
            strncat_s( Out, ( size_t )Cap, ".", _TRUNCATE );
        strncat_s( Out, ( size_t )Cap, Name, _TRUNCATE );
    }
}

inline void Note( const char* Text ) {
    lstrcpynA( Core( ).note, Text ? Text : "", ( int )sizeof( Core( ).note ) );
}

inline uintptr_t AimPart( uintptr_t Addr ) {
    Node* Item = Get( Addr );
    if ( !Item )
        return 0;
    world::Vec3 Pos;
    if ( IsPart( Item->klass ) && world::PartPos( Addr, Pos ) )
        return Addr;
    if ( IsKind( Item->klass, "Model" ) || IsKind( Item->klass, "Player" ) ) {
        uintptr_t Model = Addr;
        if ( IsKind( Item->klass, "Player" ) )
            Model = world::Ptr( Addr + world::Core( ).off.playerModel );
        if ( Model ) {
            uintptr_t Primary = world::Core( ).off.modelPrimary ? world::Ptr( Model + world::Core( ).off.modelPrimary ) : 0;
            if ( Primary && world::PartPos( Primary, Pos ) )
                return Primary;
            uintptr_t Root = world::ChildNamed( Model, "HumanoidRootPart" );
            if ( Root && world::PartPos( Root, Pos ) )
                return Root;
        }
    }
    if ( IsKind( Item->klass, "Humanoid" ) ) {
        uintptr_t Root = world::Core( ).off.humanoidRoot ? world::Ptr( Addr + world::Core( ).off.humanoidRoot ) : 0;
        if ( Root && world::PartPos( Root, Pos ) )
            return Root;
    }
    return 0;
}

inline bool Goto( uintptr_t Addr ) {
    uintptr_t Part = AimPart( Addr );
    world::Vec3 Pos;
    if ( !Part || !world::PartPos( Part, Pos ) ) {
        Note( "No position" );
        return false;
    }
    uintptr_t Local = world::LocalRoot( );
    if ( !Local ) {
        Note( "No local root" );
        return false;
    }
    if ( !world::EnsureWrite( ) || !world::WritePartPos( Local, Pos ) ) {
        Note( "Goto write failed" );
        return false;
    }
    Note( "Teleported" );
    return true;
}

inline bool Destroy( uintptr_t Addr ) {
    Session& S = Core( );
    if ( !Addr || Addr == S.root ) {
        Note( "Cannot destroy root" );
        return false;
    }
    if ( !world::Core( ).off.instParent ) {
        Note( "No Parent offset" );
        return false;
    }
    if ( !world::EnsureWrite( ) ) {
        Note( "Write access failed" );
        return false;
    }
    uintptr_t Zero = 0;
    if ( !world::Poke( Addr + world::Core( ).off.instParent, Zero ) ) {
        Note( "Destroy failed" );
        return false;
    }
    Note( "Destroyed" );
    if ( S.pick == Addr )
        S.pick = S.root;
    return true;
}

inline bool Nudge( uintptr_t Addr, int Which, float Delta ) {
    Node* Item = Get( Addr );
    if ( !Item || !IsKind( Item->klass, "Humanoid" ) )
        return false;
    world::Off& O = world::Core( ).off;
    uintptr_t Field = 0;
    if ( Which == 1 )
        Field = O.humanoidHealth;
    else if ( Which == 2 )
        Field = O.humanoidWalk;
    else if ( Which == 3 )
        Field = O.humanoidJump;
    else if ( Which == 4 )
        Field = O.humanoidHip;
    if ( !Field ) {
        Note( "Offset missing" );
        return false;
    }
    float Have = 0.0f;
    if ( !world::Pull( Addr + Field, Have ) ) {
        Note( "Read failed" );
        return false;
    }
    Have += Delta;
    if ( Have < 0.0f )
        Have = 0.0f;
    if ( !world::EnsureWrite( ) || !world::Poke( Addr + Field, Have ) ) {
        Note( "Write failed" );
        return false;
    }
    Note( "Updated" );
    return true;
}

inline void RefreshProps( uintptr_t Addr ) {
    Session& S = Core( );
    S.propN = 0;
    Node* Item = Get( Addr );
    if ( !Item )
        return;
    auto Push = [ & ]( const char* Label, const char* Text, int Write ) {
        if ( S.propN >= 16 )
            return;
        Prop& Row = S.props[ S.propN++ ];
        lstrcpynA( Row.label, Label, ( int )sizeof( Row.label ) );
        lstrcpynA( Row.text, Text ? Text : "", ( int )sizeof( Row.text ) );
        Row.write = Write;
    };
    Push( "Name", Item->name, 0 );
    Push( "Class", Item->klass, 0 );
    char Hex[ 32 ];
    snprintf( Hex, sizeof( Hex ), "0x%llx", ( unsigned long long )Item->parent );
    Push( "Parent", Hex, 0 );

    world::Off& O = world::Core( ).off;
    if ( IsPart( Item->klass ) ) {
        world::Vec3 Pos{ };
        world::Vec3 Size{ };
        world::Vec3 Vel{ };
        if ( world::PartPos( Addr, Pos ) ) {
            char Line[ 72 ];
            snprintf( Line, sizeof( Line ), "%.2f, %.2f, %.2f", Pos.x, Pos.y, Pos.z );
            Push( "Position", Line, 0 );
        }
        if ( world::PartSize( Addr, Size ) ) {
            char Line[ 72 ];
            snprintf( Line, sizeof( Line ), "%.2f, %.2f, %.2f", Size.x, Size.y, Size.z );
            Push( "Size", Line, 0 );
        }
        if ( world::PartVel( Addr, Vel ) ) {
            char Line[ 72 ];
            snprintf( Line, sizeof( Line ), "%.2f, %.2f, %.2f", Vel.x, Vel.y, Vel.z );
            Push( "Velocity", Line, 0 );
        }
    }
    if ( IsKind( Item->klass, "Humanoid" ) ) {
        auto Num = [ & ]( const char* Label, uintptr_t Field, int Write ) {
            if ( !Field )
                return;
            float Have = 0.0f;
            if ( !world::Pull( Addr + Field, Have ) )
                return;
            char Line[ 32 ];
            snprintf( Line, sizeof( Line ), "%.2f", Have );
            Push( Label, Line, Write );
        };
        Num( "Health", O.humanoidHealth, 1 );
        Num( "MaxHealth", O.humanoidMax, 0 );
        Num( "WalkSpeed", O.humanoidWalk, 2 );
        Num( "JumpPower", O.humanoidJump, 3 );
        Num( "HipHeight", O.humanoidHip, 4 );
    }
    if ( IsKind( Item->klass, "Player" ) ) {
        if ( O.playerUser ) {
            int64_t Id = 0;
            if ( world::Pull( Addr + O.playerUser, Id ) ) {
                char Line[ 32 ];
                snprintf( Line, sizeof( Line ), "%lld", ( long long )Id );
                Push( "UserId", Line, 0 );
            }
        }
        if ( O.playerDisplay ) {
            char Line[ 48 ] = { };
            if ( world::LabelOf( Addr, O.playerDisplay, Line, ( int )sizeof( Line ) ) )
                Push( "DisplayName", Line, 0 );
        }
    }
    if ( IsKind( Item->klass, "Model" ) && O.modelPrimary ) {
        uintptr_t Primary = world::Ptr( Addr + O.modelPrimary );
        char Line[ 32 ];
        snprintf( Line, sizeof( Line ), "0x%llx", ( unsigned long long )Primary );
        Push( "PrimaryPart", Line, 0 );
    }
    if ( IsValue( Item->klass ) ) {
        uintptr_t Field = offsets::Get( Item->klass, "Value" );
        if ( !Field )
            Field = offsets::Get( "ValueBase", "Value" );
        if ( Field ) {
            if ( strstr( Item->klass, "String" ) ) {
                char Line[ 48 ] = { };
                if ( world::LabelOf( Addr, Field, Line, ( int )sizeof( Line ) ) )
                    Push( "Value", Line, 0 );
            } else if ( strstr( Item->klass, "Bool" ) ) {
                bool Have = false;
                if ( world::Pull( Addr + Field, Have ) )
                    Push( "Value", Have ? "true" : "false", 0 );
            } else {
                double Have = 0.0;
                float Small = 0.0f;
                int32_t Whole = 0;
                char Line[ 32 ] = { };
                if ( world::Pull( Addr + Field, Have ) )
                    snprintf( Line, sizeof( Line ), "%.4f", Have );
                else if ( world::Pull( Addr + Field, Small ) )
                    snprintf( Line, sizeof( Line ), "%.4f", Small );
                else if ( world::Pull( Addr + Field, Whole ) )
                    snprintf( Line, sizeof( Line ), "%d", Whole );
                if ( Line[ 0 ] )
                    Push( "Value", Line, 0 );
            }
        }
    }
}

inline void RefreshOpen( ) {
    Session& S = Core( );
    for ( int Index = 0; Index < S.used; Index++ ) {
        Node& Item = S.nodes[ Index ];
        if ( !Item.addr )
            continue;
        if ( Item.addr == S.root ) {
            if ( Item.open )
                Expand( Item.addr );
            continue;
        }
        Fill( Item );
        if ( Item.open )
            Expand( Item.addr );
        if ( Item.parent && Item.addr != S.root ) {
            uintptr_t Have = world::ParentOf( Item.addr );
            if ( Have && Item.parent && Have != Item.parent && Have != S.root ) {
                Item.parent = Have;
            }
        }
    }
}

inline void Tick( ) {
    Session& S = Core( );
    if ( !S.live )
        return;
    if ( !world::Attach( ) || !world::Resolve( ) ) {
        Note( "Roblox not attached" );
        return;
    }
    if ( !S.root || S.root != world::DataModel( ) ) {
        Open( );
        return;
    }
    unsigned Now = GetTickCount( );
    if ( Now >= S.next ) {
        RefreshOpen( );
        S.next = Now + 500;
    }
    PathOf( S.pick, S.path, PathCap );
    RefreshProps( S.pick );
}

inline int Children( uintptr_t Parent, uintptr_t* Out, int Cap ) {
    Session& S = Core( );
    int Count = 0;
    for ( int Index = 0; Index < S.used && Count < Cap; Index++ ) {
        if ( !S.nodes[ Index ].addr || S.nodes[ Index ].addr == Parent )
            continue;
        if ( S.nodes[ Index ].parent == Parent )
            Out[ Count++ ] = S.nodes[ Index ].addr;
    }
    return Count;
}

inline bool HasKids( uintptr_t Addr ) {
    Node* Item = Get( Addr );
    if ( !Item )
        return false;
    if ( !Item->fetched )
        return true;
    return Item->kidN > 0;
}

inline bool Visible( uintptr_t Addr ) {
    Session& S = Core( );
    if ( !S.query[ 0 ] )
        return true;
    Node* Item = Get( Addr );
    if ( !Item )
        return false;
    if ( Hits( *Item, S.query ) )
        return true;
    uintptr_t List[ KidCap ];
    int Count = Children( Addr, List, KidCap );
    for ( int Index = 0; Index < Count; Index++ ) {
        if ( Visible( List[ Index ] ) )
            return true;
    }
    return false;
}

inline TreeIcon Glyph( uintptr_t Addr ) {
    Node* Item = Get( Addr );
    return IconFor( Item ? Item->klass : "" );
}

}