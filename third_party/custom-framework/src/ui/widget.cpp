#include "ur/widget.hpp"

#include "Layout.h"
#include "Widgets.h"

namespace ur {
namespace widget {

Item begin( const char* Id, CVector Size ) {
    Item Next;
    Next.bounds = Layout->Place( Size );
    Next.clicked = Widgets->Hit( Id, Next.bounds, Next.hovered, Next.held );
    Next.id = Context->Hash( Id );
    Next.glow = Widgets->LastItem( ).Width > 0.0f && Next.hovered ? 1.0f : 0.0f;
    if ( Next.hovered )
        Next.glow = 1.0f;
    return Next;
}

void tooltip( const char* Message ) {
    Widgets->Tooltip( Message );
}

}
}
