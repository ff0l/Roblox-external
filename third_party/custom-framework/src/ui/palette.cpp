#include "ur/palette.hpp"

#include "Context.h"
#include "Input.h"
#include "Layout.h"
#include "Style.h"
#include "Widgets.h"
#include "Frames.h"

#include <cstring>
#include <string>
#include <vector>

namespace ur {
namespace palette {

struct Command {
    std::string Name;
    std::string Hint;
    std::function< void( ) > Action;
};

static std::vector< Command > Commands;
static bool Open = false;
static char Query[ 96 ] = "";
static int Picked = 0;

void add( const char* Name, const char* Hint, std::function< void( ) > Action ) {
    if ( !Name || !Name[ 0 ] || !Action )
        return;

    Command Next;
    Next.Name = Name;
    Next.Hint = Hint ? Hint : "";
    Next.Action = std::move( Action );
    Commands.push_back( std::move( Next ) );
}

void open( ) {
    Open = true;
    Query[ 0 ] = 0;
    Picked = 0;
}

void close( ) {
    Open = false;
}

bool visible( ) {
    return Open;
}

void clear( ) {
    Commands.clear( );
    Open = false;
}

void draw( ) {
    if ( Input->Control( ) && Input->KeyPressed( KeyLetterA + ( 'K' - 'A' ) ) )
        Open = !Open;

    if ( !Open )
        return;

    if ( Input->KeyPressed( KeyEscape ) ) {
        Open = false;
        return;
    }

    std::vector< int > Hits;
    for ( int Index = 0; Index < ( int )Commands.size( ); Index++ ) {
        if ( Query[ 0 ] == 0 || strstr( Commands[ ( size_t )Index ].Name.c_str( ), Query ) )
            Hits.push_back( Index );
    }

    if ( Picked >= ( int )Hits.size( ) )
        Picked = ( int )Hits.size( ) - 1;
    if ( Picked < 0 )
        Picked = 0;

    if ( Input->KeyPressed( KeyDown ) && Picked + 1 < ( int )Hits.size( ) )
        Picked++;
    if ( Input->KeyPressed( KeyUp ) && Picked > 0 )
        Picked--;

    CVector Size( 460.0f * Style->Scale, 320.0f * Style->Scale );
    CVector Anchor( ( Context->Display.Horizontal - Size.Horizontal ) * 0.5f, 72.0f * Style->Scale );

    if ( !Frames->Begin( "Command", nullptr, 0, Anchor, Size ) ) {
        Frames->End( );
        return;
    }

    Widgets->Field( "##palette", Query, 96, "Type a command" );

    if ( Input->KeyPressed( KeyEnter ) && !Hits.empty( ) ) {
        int Index = Hits[ ( size_t )Picked ];
        Open = false;
        Commands[ ( size_t )Index ].Action( );
        Frames->End( );
        return;
    }

    int First = 0;
    int Last = 0;
    if ( Widgets->BeginVirtual( "##palette-list", ( int )Hits.size( ), Style->ControlHeight, First, Last, 220.0f * Style->Scale ) ) {
        for ( int Slot = First; Slot < Last; Slot++ ) {
            int Index = Hits[ ( size_t )Slot ];
            Context->PushIdentifier( Index );
            bool Active = Slot == Picked;
            if ( Widgets->Selectable( Commands[ ( size_t )Index ].Name.c_str( ), Active, Style->ControlHeight ) ) {
                Open = false;
                Commands[ ( size_t )Index ].Action( );
                Context->PopIdentifier( );
                Widgets->EndVirtual( );
                Frames->End( );
                return;
            }
            if ( Active && !Commands[ ( size_t )Index ].Hint.empty( ) )
                Widgets->Tooltip( Commands[ ( size_t )Index ].Hint.c_str( ) );
            Context->PopIdentifier( );
        }
        Widgets->EndVirtual( );
    }

    Widgets->Faint( "Ctrl+K  ·  Enter runs  ·  Esc closes" );
    Frames->End( );
}

}
}
