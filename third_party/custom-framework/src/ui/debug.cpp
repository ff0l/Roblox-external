#include "ur/debug.hpp"

#include "Context.h"
#include "Format.h"
#include "Frames.h"
#include "Style.h"
#include "Widgets.h"

#include <unordered_map>
#include <string>

namespace ur {
namespace debug {

static bool WatchCollisions = false;
static std::unordered_map< unsigned int, std::string > Seen;

void collisions( bool On ) {
    WatchCollisions = On;
}

void ids( bool* Open ) {
    if ( Open && !*Open )
        return;

    if ( !Frames->Begin( "IDs", Open, FrameMove | FrameResize | FrameClose, CVector( 24.0f, 24.0f ), CVector( 320.0f, 220.0f ) ) ) {
        Frames->End( );
        return;
    }

    Widgets->Heading( "Interaction" );
    Widgets->Label( Format->Print( "hovered  %08X", Context->HoveredItem ) );
    Widgets->Label( Format->Print( "active   %08X", Context->ActiveItem ) );
    Widgets->Label( Format->Print( "focused  %08X", Context->FocusedItem ) );
    Widgets->Label( Format->Print( "last     %s", Widgets->LastName( ) ) );

    Widgets->Section( "Faults" );
    Widgets->Label( Format->Print( "%d recorded", ( int )Context->Faults.size( ) ) );
    if ( !Context->Faults.empty( ) )
        Widgets->Faint( Context->Faults.back( ).Message.c_str( ) );

    Widgets->Check( "Watch collisions", WatchCollisions );
    if ( WatchCollisions ) {
        const char* Name = Widgets->LastName( );
        unsigned int Hash = Context->HoveredItem;
        if ( Hash && Name && Name[ 0 ] ) {
            auto Found = Seen.find( Hash );
            if ( Found != Seen.end( ) && Found->second != Name )
                Widgets->Colored( Style->Danger, "ID collision" );
            Seen[ Hash ] = Name;
        }
    }

    Frames->End( );
}

}
}
