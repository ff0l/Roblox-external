#include "ur/toast.hpp"

#include "Canvas.h"
#include "Context.h"
#include "Font.h"
#include "Style.h"

#include <string>
#include <vector>

namespace ur {
namespace toast {

struct Note {
    std::string Text;
    CColor Tint;
    float Life = 2.8f;
};

static std::vector< Note > Notes;

void push( const char* Message, float Seconds ) {
    push( Message, Style->Text, Seconds );
}

void push( const char* Message, CColor Tint, float Seconds ) {
    if ( !Message || !Message[ 0 ] )
        return;

    Note Next;
    Next.Text = Message;
    Next.Tint = Tint;
    Next.Life = Seconds > 0.2f ? Seconds : 0.2f;
    Notes.push_back( Next );
    if ( Notes.size( ) > 8 )
        Notes.erase( Notes.begin( ) );
}

void clear( ) {
    Notes.clear( );
}

void draw( ) {
    if ( Notes.empty( ) )
        return;

    int Former = Canvas->Route( OverlayRoute );
    float Left = Context->Display.Horizontal - 16.0f * Style->Scale;
    float Top = 16.0f * Style->Scale;

    for ( size_t Index = Notes.size( ); Index > 0; Index-- ) {
        Note& Item = Notes[ Index - 1 ];
        Item.Life -= Context->DeltaTime;

        CVector Size = Font->Measure( Item.Text.c_str( ) );
        float Pad = 10.0f * Style->Scale;
        CRectangle Box( Left - Size.Horizontal - Pad * 2.0f, Top, Size.Horizontal + Pad * 2.0f, Size.Vertical + Pad );

        float Fade = Item.Life < 0.35f ? Item.Life / 0.35f : 1.0f;
        if ( Fade < 0.0f )
            Fade = 0.0f;

        float FormerOpacity = Canvas->Opacity;
        Canvas->Opacity = Fade;
        if ( Style->Shadows )
            Canvas->Shadow( Box, Style->Shade, Style->ControlRounding, Style->Softness * 0.4f );
        Canvas->Rectangle( Box, Style->Popup, Style->ControlRounding );
        if ( Style->Borders )
            Canvas->Border( Box, Style->Outline, Style->ControlRounding, Style->Thickness );
        Canvas->Text( CVector( Box.Left + Pad, Box.Top + Pad * 0.45f ), Item.Tint, Item.Text.c_str( ) );
        Canvas->Opacity = FormerOpacity;

        Top += Box.Height + 8.0f * Style->Scale;
    }

    Canvas->Route( Former );

    while ( !Notes.empty( ) && Notes.front( ).Life <= 0.0f )
        Notes.erase( Notes.begin( ) );
}

}
}
