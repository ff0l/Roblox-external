#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Font.h"
#include "Input.h"
#include "Arena.h"
#include "Canvas.h"
#include "Format.h"
#include "Frames.h"
#include "Layout.h"
#include "Native.h"
#include "Context.h"
#include "Widgets.h"

#include "Style.h"

static int NextBoundary( const char* Buffer, int Position ) {
    int Length = ( int )strlen( Buffer );
    if ( Position >= Length )
        return Length;

    Position++;

    while ( Position < Length && ( ( unsigned char )Buffer[ Position ] & 0xC0 ) == 0x80 )
        Position++;

    return Position;
}

static int PriorBoundary( const char* Buffer, int Position ) {
    if ( Position <= 0 )
        return 0;

    Position--;

    while ( Position > 0 && ( ( unsigned char )Buffer[ Position ] & 0xC0 ) == 0x80 )
        Position--;

    return Position;
}

static int Encode( unsigned int Codepoint, char* Output ) {
    if ( Codepoint < 0x80 ) {
        Output[ 0 ] = ( char )Codepoint;
        return 1;
    }

    if ( Codepoint < 0x800 ) {
        Output[ 0 ] = ( char )( 0xC0 | ( Codepoint >> 6 ) );
        Output[ 1 ] = ( char )( 0x80 | ( Codepoint & 0x3F ) );

        return 2;
    }

    if ( Codepoint < 0x10000 ) {
        Output[ 0 ] = ( char )( 0xE0 | ( Codepoint >> 12 ) );
        Output[ 1 ] = ( char )( 0x80 | ( ( Codepoint >> 6 ) & 0x3F ) );

        Output[ 2 ] = ( char )( 0x80 | ( Codepoint & 0x3F ) );
        return 3;
    }

    Output[ 0 ] = ( char )( 0xF0 | ( Codepoint >> 18 ) );
    Output[ 1 ] = ( char )( 0x80 | ( ( Codepoint >> 12 ) & 0x3F ) );

    Output[ 2 ] = ( char )( 0x80 | ( ( Codepoint >> 6 ) & 0x3F ) );
    Output[ 3 ] = ( char )( 0x80 | ( Codepoint & 0x3F ) );

    return 4;
}

void CWidgets::Destroy( ) {
    Motions.clear( );
    Editors.clear( );

    Opens.clear( );
    Picks.clear( );

    Extents.clear( );
    Panels.clear( );

    Twigs.clear( );
    MenuChain.clear( );

    Reveal = 0;
    Popup = 0;

    Latest = 0;
    Aimed = 0;

    Dwell = 0.0f;
    MenuTouched = false;

    DisableStack.clear( );
    AlphaStack.clear( );
    VirtualCount = 0;
    LatestBounds = CRectangle( );
    LatestName = "";
}

void CWidgets::Sweep( ) {
    auto Entry = Motions.begin( );

    while ( Entry != Motions.end( ) ) {
        if ( Context->FrameIndex - Entry->second.Stamp > 2 )
            Entry = Motions.erase( Entry );
        else
            Entry++;
    }

    auto Scribe = Editors.begin( );

    while ( Scribe != Editors.end( ) ) {
        if ( Context->FrameIndex - Scribe->second.Stamp > 4 )
            Scribe = Editors.erase( Scribe );
        else
            Scribe++;
    }

    MenuTouched = false;
    Twigs.clear( );
}

bool CWidgets::Behavior( unsigned int Identifier, CRectangle Bounds, bool& Hovered, bool& Held ) {
    LatestBounds = Bounds;
    CVector Point = Input->MousePosition;

    if ( Disabled( ) ) {
        Hovered = false;
        Held = false;
        Latest = Identifier;
        return false;
    }

    Hovered = Bounds.Contains( Point ) && Canvas->Visible( Point ) && Frames->Beneath == Frames->Current && !Context->Covered( Point );
    if ( Hovered )
        Context->HoveredItem = Identifier;

    if ( Hovered && Input->MousePressed( 0 ) && Context->ActiveItem == 0 )
        Context->ActiveItem = Identifier;

    Held = Context->ActiveItem == Identifier;
    bool Clicked = Held && Hovered && Input->MouseReleased( 0 );

    if ( Held && Input->MouseReleased( 0 ) )
        Context->ActiveItem = 0;

    Latest = Identifier;
    return Clicked;
}

bool CWidgets::Disabled( ) const {
    for ( unsigned char Flag : DisableStack ) {
        if ( Flag )
            return true;
    }

    return false;
}

void CWidgets::BeginDisabled( bool Off ) {
    DisableStack.push_back( Off ? 1 : 0 );
    if ( Off )
        PushAlpha( 0.45f );
}

void CWidgets::EndDisabled( ) {
    if ( DisableStack.empty( ) )
        return;

    bool Off = DisableStack.back( ) != 0;
    DisableStack.pop_back( );
    if ( Off )
        PopAlpha( );
}

void CWidgets::PushAlpha( float Amount ) {
    AlphaStack.push_back( Canvas->Opacity );
    if ( Amount < 0.0f )
        Amount = 0.0f;
    if ( Amount > 1.0f )
        Amount = 1.0f;
    Canvas->Opacity *= Amount;
}

void CWidgets::PopAlpha( ) {
    if ( AlphaStack.empty( ) )
        return;

    Canvas->Opacity = AlphaStack.back( );
    AlphaStack.pop_back( );
}

CRectangle CWidgets::LastItem( ) const {
    return LatestBounds;
}

const char* CWidgets::LastName( ) const {
    return LatestName ? LatestName : "";
}

bool CWidgets::Hit( const char* Title, CRectangle Bounds, bool& Hovered, bool& Held ) {
    LatestName = Title ? Title : "";
    return Behavior( Context->Hash( Title ), Bounds, Hovered, Held );
}

CMotion& CWidgets::Motion( unsigned int Identifier ) {
    CMotion& Movement = Motions[ Identifier ];
    Movement.Stamp = Context->FrameIndex;

    return Movement;
}

float CWidgets::Ease( unsigned int Identifier, bool Hovered ) {
    CMotion& Movement = Motion( Identifier );

    float Speed = Style->FadeSpeed * Context->DeltaTime;
    if ( Speed > 1.0f )
        Speed = 1.0f;

    Movement.Glow += ( ( Hovered ? 1.0f : 0.0f ) - Movement.Glow ) * Speed;
    return Movement.Glow;
}

void CWidgets::Label( const char* Message ) {
    CVector Extent = Font->Measure( Message );
    CRectangle Bounds = Layout->Place( Extent );

    Canvas->Text( Bounds.Origin( ), Style->Text, Message );
}

void CWidgets::Faint( const char* Message ) {
    CVector Extent = Font->Measure( Message );
    CRectangle Bounds = Layout->Place( Extent );

    Canvas->Text( Bounds.Origin( ), Style->Faint, Message );
}

void CWidgets::Heading( const char* Message ) {
    CVector Extent = ::Heading->Measure( Message );
    CRectangle Bounds = Layout->Place( Extent );

    Canvas->Write( ::Heading.get( ), Bounds.Origin( ), Style->Text, Message );
}

void CWidgets::Section( const char* Message ) {
    Layout->Skip( Style->Spacing * 0.5f );

    CVector Extent = Font->Measure( Message );
    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Extent.Vertical ) );

    Canvas->Text( Bounds.Origin( ), Style->Faint, Message );

    float Start = Bounds.Left + Extent.Horizontal + Style->Spacing;
    float Middle = Bounds.Top + Extent.Vertical * 0.55f;

    if ( Start < Bounds.Right( ) )
        Canvas->Rectangle( CRectangle( Start, Middle, Bounds.Right( ) - Start, Style->Thickness ), Style->Outline.Fade( 0.7f ), 0.0f );
}

bool CWidgets::Button( const char* Title, float Width ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );
    LatestName = Shown;

    float Span = Width > 0.0f ? Width : Layout->Width( );
    CRectangle Bounds = Layout->Place( CVector( Span, Style->ControlHeight ) );

    bool Hovered = false;
    bool Held = false;

    bool Clicked = Behavior( Identifier, Bounds, Hovered, Held );
    float Glow = Ease( Identifier, Hovered );

    bool Sunk = Held && Hovered;
    CMotion& Movement = Motion( Identifier );

    if ( Sunk ) {
        Movement.Slide = 1.0f;
    } else {
        float Speed = Style->FadeSpeed * 1.6f * Context->DeltaTime;

        if ( Speed > 1.0f )
            Speed = 1.0f;

        Movement.Slide += ( 0.0f - Movement.Slide ) * Speed;
    }

    CColor Fill = Style->Control.Blend( Style->Hovered, Glow ).Blend( Style->Pressed, Movement.Slide );

    if ( Style->Glass )
        Canvas->Gradient( Bounds, Fill.Blend( CColor( 255, 255, 255 ), 0.04f ), Fill, Style->ControlRounding, false );
    else
        Canvas->Rectangle( Bounds, Fill, Style->ControlRounding );

    if ( Style->Borders )
        Canvas->Border( Bounds, Style->Outline, Style->ControlRounding, Style->Thickness );

    CVector Naming = Font->Measure( Shown );
    Canvas->Text( CVector( Bounds.Left + ( Bounds.Width - Naming.Horizontal ) * 0.5f, Bounds.Top + ( Bounds.Height - Font->LineSpan ) * 0.5f ), Style->Text, Shown );

    return Clicked;
}

bool CWidgets::IconButton( const char* Title, unsigned long long Image, float Size, bool Active ) {
    unsigned int Identifier = Context->Hash( Title );
    CRectangle Bounds = Layout->Place( CVector( Size, Size ) );

    bool Hovered = false;
    bool Held = false;
    bool Clicked = Behavior( Identifier, Bounds, Hovered, Held );
    float Glow = Ease( Identifier, Hovered );
    bool Sunk = Held && Hovered;

    CColor Fill = Active ? Style->Accent.Fade( 0.22f ) : Style->Control;
    Fill = Fill.Blend( Style->Hovered, Glow ).Blend( Style->Pressed, Sunk ? 1.0f : 0.0f );

    float Round = Size * 0.5f;
    Canvas->Rectangle( Bounds, Fill, Round );
    if ( Style->Borders )
        Canvas->Border( Bounds, Active ? Style->Accent.Fade( 0.55f ) : Style->Outline, Round, Style->Thickness );

    if ( Image ) {
        float Mark = Size * 0.42f;
        CRectangle Icon( Bounds.Left + ( Size - Mark ) * 0.5f, Bounds.Top + ( Size - Mark ) * 0.5f, Mark, Mark );
        CColor Tint = Active ? Style->AccentSoft : Style->Text.Blend( Style->Accent, Glow * 0.35f );
        Canvas->Image( Icon, Image, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), Tint, 0.0f );
    }

    return Clicked;
}

bool CWidgets::Check( const char* Title, bool& State ) {
    unsigned int Identifier = Context->Hash( Title );
    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );

    bool Hovered = false;
    bool Held = false;

    bool Clicked = Behavior( Identifier, Bounds, Hovered, Held );
    if ( Clicked )
        State = !State;

    float Glow = Ease( Identifier, Hovered );

    CMotion& Movement = Motion( Identifier );
    if ( !Movement.Seen ) {
        Movement.Seen = true;
        Movement.Slide = State ? 1.0f : 0.0f;
    }

    float Speed = Style->FadeSpeed * Context->DeltaTime;
    if ( Speed > 1.0f )
        Speed = 1.0f;

    Movement.Slide += ( ( State ? 1.0f : 0.0f ) - Movement.Slide ) * Speed;
    float Box = Style->CheckSize;

    CRectangle Mark( Bounds.Left, Bounds.Top + ( Bounds.Height - Box ) * 0.5f, Box, Box );
    CColor Fill = Style->Control.Blend( Style->Hovered, Glow ).Blend( Style->Accent, Movement.Slide );

    Canvas->Rectangle( Mark, Fill, Style->ControlRounding * 0.65f );
    if ( Style->Borders )
        Canvas->Border( Mark, Style->Outline.Blend( Style->Accent, Movement.Slide ), Style->ControlRounding * 0.65f, Style->Thickness );

    if ( Movement.Slide > 0.02f ) {
        CColor Ink = CColor( 255, 255, 255 ).Fade( Movement.Slide );
        float Line = Style->IconStroke;

        CRectangle Inner = Mark.Shrink( Box * 0.26f );
        CVector Turn( 0.38f, 0.86f );

        Canvas->Stroke( Inner, CVector( 0.05f, 0.56f ), Turn, Ink, Line );
        Canvas->Stroke( Inner, Turn, CVector( 0.97f, 0.18f ), Ink, Line );
    }

    Canvas->Text( CVector( Mark.Right( ) + Style->Spacing, Bounds.Top + ( Bounds.Height - Font->LineSpan ) * 0.5f ), Style->Text, Context->Caption( Title ) );
    return Clicked;
}

bool CWidgets::Radio( const char* Title, int& Selected, int Option ) {
    unsigned int Identifier = Context->Hash( Title );
    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );

    bool Hovered = false;
    bool Held = false;

    bool Clicked = Behavior( Identifier, Bounds, Hovered, Held );
    if ( Clicked )
        Selected = Option;

    bool Chosen = Selected == Option;
    float Glow = Ease( Identifier, Hovered );

    CMotion& Movement = Motion( Identifier );
    if ( !Movement.Seen ) {
        Movement.Seen = true;
        Movement.Slide = Chosen ? 1.0f : 0.0f;
    }

    float Speed = Style->FadeSpeed * Context->DeltaTime;
    if ( Speed > 1.0f )
        Speed = 1.0f;

    Movement.Slide += ( ( Chosen ? 1.0f : 0.0f ) - Movement.Slide ) * Speed;

    float Round = Style->CheckSize * 0.5f;
    CVector Middle( Bounds.Left + Round, Bounds.Top + Bounds.Height * 0.5f );

    Canvas->Circle( Middle, Round, Style->Control.Blend( Style->Hovered, Glow ) );
    if ( Style->Borders ) {
        CRectangle Ring( Middle.Horizontal - Round, Middle.Vertical - Round, Round * 2.0f, Round * 2.0f );
        Canvas->Border( Ring, Style->Outline.Blend( Style->Accent, Movement.Slide ), Round, Style->Thickness );
    }

    if ( Movement.Slide > 0.02f )
        Canvas->Circle( Middle, Round * 0.5f * Movement.Slide, Style->Accent );

    Canvas->Text( CVector( Bounds.Left + Round * 2.0f + Style->Spacing, Bounds.Top + ( Bounds.Height - Font->LineSpan ) * 0.5f ), Style->Text, Context->Caption( Title ) );
    return Clicked;
}

bool CWidgets::Slider( const char* Title, float& Current, float Minimum, float Maximum ) {
    unsigned int Identifier = Context->Hash( Title );
    float Depth = Font->LineSpan + Style->KnobSize * 2.0f + 8.0f * Style->Scale;

    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Depth ) );

    bool Hovered = false;
    bool Held = false;

    Behavior( Identifier, Bounds, Hovered, Held );
    float Before = Current;

    if ( Held && Maximum > Minimum && Bounds.Width > 0.0f ) {
        float Ratio = ( Input->MousePosition.Horizontal - Bounds.Left ) / Bounds.Width;

        if ( Ratio < 0.0f )
            Ratio = 0.0f;

        if ( Ratio > 1.0f )
            Ratio = 1.0f;

        Current = Minimum + ( Maximum - Minimum ) * Ratio;
    }

    if ( Current < Minimum )
        Current = Minimum;

    if ( Current > Maximum )
        Current = Maximum;

    float Glow = Ease( Identifier, Hovered || Held );

    char* Reading = Format->Print( "%.2f", ( double )Current );
    CVector Extent = Font->Measure( Reading );

    const char* Shown = Context->Caption( Title );
    CVector Titled = Font->Measure( Shown );

    float Gap = 8.0f * Style->Scale;
    if ( Shown[ 0 ] && Shown[ 0 ] != ' ' )
        Canvas->Text( Bounds.Origin( ), Style->Text, Shown );

    if ( Shown[ 0 ] && Shown[ 0 ] != ' ' )
        Canvas->Text( CVector( Bounds.Left + Titled.Horizontal + Gap, Bounds.Top ), Style->Faint.Blend( Style->Text, Glow ), Reading );

    float Line = Style->GrooveWidth;
    float Middle = Bounds.Bottom( ) - Style->KnobSize - 1.0f;

    CRectangle Groove( Bounds.Left, Middle - Line * 0.5f, Bounds.Width, Line );
    Canvas->Rectangle( Groove, Style->Groove, Line * 0.5f );

    float Portion = Maximum > Minimum ? ( Current - Minimum ) / ( Maximum - Minimum ) : 0.0f;
    float Reach = Groove.Width * Portion;

    if ( Reach > 0.0f && Reach < Line )
        Reach = Line;

    Canvas->Rectangle( CRectangle( Groove.Left, Groove.Top, Reach, Line ), Style->Accent, Line * 0.5f );

    float Radius = Style->KnobSize + ( Held ? 1.5f : Glow * 0.75f ) * Style->Scale;
    float Center = Groove.Left + Groove.Width * Portion;

    if ( Center < Groove.Left + Radius )
        Center = Groove.Left + Radius;

    if ( Center > Groove.Right( ) - Radius )
        Center = Groove.Right( ) - Radius;

    Canvas->Circle( CVector( Center, Middle ), Radius, Style->Knob );

    if ( Style->Borders ) {
        CRectangle Ring( Center - Radius, Middle - Radius, Radius * 2.0f, Radius * 2.0f );
        Canvas->Border( Ring, Style->Outline.Blend( Style->Accent, Glow ), Radius, Style->Thickness );
    }

    return Current != Before;
}

bool CWidgets::Choice( const char* Title, int& Selected, const char* const* Names, int Count ) {
    if ( !Names || Count <= 0 )
        return false;

    if ( Selected < 0 )
        Selected = 0;

    if ( Selected >= Count )
        Selected = Count - 1;

    unsigned int Identifier = Context->Hash( Title );
    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );

    bool Hovered = false;
    bool Held = false;

    bool Clicked = Behavior( Identifier, Bounds, Hovered, Held );

    if ( Clicked )
        Reveal = Reveal == Identifier ? 0 : Identifier;

    float Glow = Ease( Identifier, Hovered );
    CColor Fill = Style->Control.Blend( Style->Hovered, Glow );

    if ( Style->Glass )
        Canvas->Gradient( Bounds, Fill.Blend( CColor( 255, 255, 255 ), 0.05f ), Fill, Style->ControlRounding, false );
    else
        Canvas->Rectangle( Bounds, Fill, Style->ControlRounding );

    if ( Style->Borders )
        Canvas->Border( Bounds, Reveal == Identifier ? Style->Accent : Style->Outline.Blend( Style->Accent, Glow * 0.35f ), Style->ControlRounding, Style->Thickness );

    float TextTop = Bounds.Top + ( Bounds.Height - Font->LineSpan ) * 0.5f;
    float Inset = Style->PaddingWide * 0.75f;
    float Well = 22.0f * Style->Scale;
    CRectangle Chevron( Bounds.Right( ) - Inset - Well, Bounds.Top + ( Bounds.Height - Well ) * 0.5f, Well, Well );
    Canvas->Rectangle( Chevron, Style->Elevated.Blend( Style->Accent, Glow * 0.18f ), Well * 0.34f );

    CRectangle Caret( Chevron.Left + Well * 0.28f, Chevron.Top + Well * 0.34f, Well * 0.44f, Well * 0.32f );
    CColor Ink = Style->Text.Blend( Style->Accent, Glow );
    Canvas->Stroke( Caret, CVector( 0.08f, 0.28f ), CVector( 0.5f, 0.78f ), Ink, Style->IconStroke );
    Canvas->Stroke( Caret, CVector( 0.5f, 0.78f ), CVector( 0.92f, 0.28f ), Ink, Style->IconStroke );

    CVector Titled = Font->Measure( Title );
    float TitleLeft = Bounds.Left + Inset;
    if ( Title[ 0 ] )
        Canvas->Text( CVector( TitleLeft, TextTop ), Style->Faint, Title );

    float ClipLeft = Title[ 0 ] ? TitleLeft + Titled.Horizontal + 10.0f * Style->Scale : TitleLeft;
    Canvas->PushClip( CRectangle( ClipLeft, Bounds.Top, Chevron.Left - 8.0f * Style->Scale - ClipLeft, Bounds.Height ) );
    Canvas->Text( CVector( ClipLeft, TextTop ), Style->Text, Names[ Selected ] );
    Canvas->PopClip( );

    bool Showing = Reveal == Identifier;
    CMotion& Movement = Motion( Identifier );

    float Speed = Style->FadeSpeed * 1.4f * Context->DeltaTime;
    if ( Speed > 1.0f )
        Speed = 1.0f;

    Movement.Slide += ( ( Showing ? 1.0f : 0.0f ) - Movement.Slide ) * Speed;

    if ( !Showing && Movement.Slide < 0.01f ) {
        Movement.Slide = 0.0f;
        return false;
    }

    float Grown = Movement.Slide;

    float ItemHeight = Style->ControlHeight + 2.0f * Style->Scale;
    float Lip = 7.0f * Style->Scale;

    CRectangle Zone( Bounds.Left, Bounds.Bottom( ) + 5.0f * Style->Scale, Bounds.Width, Count * ItemHeight + Lip * 2.0f );

    if ( Context->Display.Vertical > 0.0f && Zone.Bottom( ) > Context->Display.Vertical )
        Zone.Top = Bounds.Top - Zone.Height - 6.0f * Style->Scale;

    if ( Showing || Grown > 0.15f ) {
        Context->Shielding = true;
        Context->Shield = Zone;

        Context->ShieldStamp = Context->FrameIndex;
    }

    Zone.Top += ( 1.0f - Grown ) * 6.0f * Style->Scale;

    bool Changed = false;

    int Former = Canvas->Route( OverlayRoute );
    float Former2 = Canvas->Opacity;

    Canvas->Opacity = Grown;
    Canvas->PushClip( Zone.Inflate( Style->Softness + 8.0f ), true );

    if ( Style->Shadows )
        Canvas->Shadow( Zone, Style->Shade, Style->ControlRounding + 4.0f, Style->Softness );

    Canvas->Gradient( Zone, Style->Popup.Blend( CColor( 255, 255, 255 ), 0.03f ), Style->Popup, Style->ControlRounding + 3.0f, false );
    if ( Style->Borders )
        Canvas->Border( Zone, Style->Outline.Blend( Style->Accent, 0.18f ), Style->ControlRounding + 3.0f, Style->Thickness );

    CVector Point = Input->MousePosition;
    Canvas->PushClip( Zone );

    for ( int Position = 0; Position < Count; Position++ ) {
        CRectangle Item( Zone.Left + Lip, Zone.Top + Lip + Position * ItemHeight, Zone.Width - Lip * 2.0f, ItemHeight );

        bool Over = Showing && Item.Contains( Point );

        if ( Position == Selected )
            Canvas->Rectangle( Item, Style->Accent.Fade( 0.16f ), Style->ControlRounding * 0.7f );

        if ( Over && Position != Selected )
            Canvas->Rectangle( Item, Style->Hovered, Style->ControlRounding * 0.7f );

        if ( Over && Input->MousePressed( 0 ) ) {
            Changed = Selected != Position;
            Selected = Position;

            Reveal = 0;
        }

        if ( Position == Selected ) {
            Canvas->Rectangle( CRectangle( Item.Left + 3.0f * Style->Scale, Item.Top + 6.0f * Style->Scale, 3.0f * Style->Scale, Item.Height - 12.0f * Style->Scale ), Style->Accent, 1.5f * Style->Scale );
            float Tick = 8.0f * Style->Scale;
            CRectangle Mark( Item.Right( ) - Tick - 8.0f * Style->Scale, Item.Top + ( Item.Height - Tick ) * 0.5f, Tick, Tick );
            Canvas->Stroke( Mark, CVector( 0.1f, 0.55f ), CVector( 0.38f, 0.86f ), Style->Accent, Style->IconStroke );
            Canvas->Stroke( Mark, CVector( 0.38f, 0.86f ), CVector( 0.92f, 0.18f ), Style->Accent, Style->IconStroke );
        }

        Canvas->Text( CVector( Item.Left + 12.0f * Style->Scale, Item.Top + ( Item.Height - Font->LineSpan ) * 0.5f ), Position == Selected ? Style->Accent : Style->Text, Names[ Position ] );
    }

    Canvas->PopClip( );
    Canvas->PopClip( );
    Canvas->Opacity = Former2;

    Canvas->Route( Former );

    if ( Showing && Input->MousePressed( 0 ) && !Zone.Contains( Point ) && !Bounds.Contains( Point ) )
        Reveal = 0;

    return Changed;
}

bool CWidgets::Segments( const char* Title, int& Selected, const char* const* Names, int Count ) {
    if ( !Names || Count <= 0 )
        return false;

    if ( Count > 16 )
        Count = 16;

    if ( Selected < 0 )
        Selected = 0;

    if ( Selected >= Count )
        Selected = Count - 1;

    unsigned int Identifier = Context->Hash( Title );
    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );

    bool Hovered = false;
    bool Held = false;

    Behavior( Identifier, Bounds, Hovered, Held );

    float Edges[ 17 ];
    Edges[ 0 ] = 0.0f;

    if ( Style->Adaptive ) {
        float Sizes[ 16 ];
        bool Fixed[ 16 ] = { };

        for ( int Position = 0; Position < Count; Position++ )
            Sizes[ Position ] = Font->Measure( Names[ Position ] ).Horizontal + Style->PaddingWide * 1.3f;

        float Remaining = Bounds.Width;
        int Flexible = Count;

        bool Settled = false;

        while ( !Settled ) {
            Settled = true;
            float Fair = Flexible > 0 ? Remaining / ( float )Flexible : 0.0f;

            for ( int Position = 0; Position < Count; Position++ ) {
                if ( !Fixed[ Position ] && Sizes[ Position ] > Fair ) {
                    Fixed[ Position ] = true;
                    Remaining -= Sizes[ Position ];

                    Flexible--;
                    Settled = false;
                }
            }
        }

        float Fair = Flexible > 0 ? Remaining / ( float )Flexible : 0.0f;

        for ( int Position = 0; Position < Count; Position++ )
            Edges[ Position + 1 ] = Edges[ Position ] + ( Fixed[ Position ] ? Sizes[ Position ] : Fair );
    } else {
        for ( int Position = 0; Position < Count; Position++ )
            Edges[ Position + 1 ] = Bounds.Width * ( float )( Position + 1 ) / ( float )Count;
    }

    CVector Point = Input->MousePosition;
    bool Changed = false;

    if ( Hovered && Input->MousePressed( 0 ) ) {
        for ( int Position = 0; Position < Count; Position++ ) {
            if ( Point.Horizontal >= Bounds.Left + Edges[ Position ] && Point.Horizontal < Bounds.Left + Edges[ Position + 1 ] ) {
                Changed = Position != Selected;
                Selected = Position;

                break;
            }
        }
    }

    CMotion& Movement = Motion( Identifier );
    if ( !Movement.Seen ) {
        Movement.Seen = true;
        Movement.Slide = ( float )Selected;
    }

    float Speed = Style->FadeSpeed * Context->DeltaTime;
    if ( Speed > 1.0f )
        Speed = 1.0f;

    Movement.Slide += ( ( float )Selected - Movement.Slide ) * Speed;

    if ( fabsf( ( float )Selected - Movement.Slide ) < 0.01f )
        Movement.Slide = ( float )Selected;

    Canvas->Rectangle( Bounds, Style->Groove, Style->ControlRounding );
    float Pad = 3.0f * Style->Scale;

    int Lower = ( int )Movement.Slide;

    if ( Lower < 0 )
        Lower = 0;

    if ( Lower > Count - 1 )
        Lower = Count - 1;

    int Upper = Lower + 1 < Count ? Lower + 1 : Lower;
    float Amount = Movement.Slide - ( float )Lower;

    if ( Amount < 0.0f )
        Amount = 0.0f;

    if ( Amount > 1.0f )
        Amount = 1.0f;

    float LeftEdge = Edges[ Lower ] + ( Edges[ Upper ] - Edges[ Lower ] ) * Amount;
    float RightEdge = Edges[ Lower + 1 ] + ( Edges[ Upper + 1 ] - Edges[ Lower + 1 ] ) * Amount;

    CRectangle Pill( Bounds.Left + LeftEdge + Pad, Bounds.Top + Pad, RightEdge - LeftEdge - Pad * 2.0f, Bounds.Height - Pad * 2.0f );

    Canvas->Rectangle( Pill, Style->TabActive.Blend( Style->Hovered, 0.5f ), Style->ControlRounding - Pad * 0.5f );
    if ( Style->Borders )
        Canvas->Border( Pill, Style->Outline, Style->ControlRounding - Pad * 0.5f, Style->Thickness );

    for ( int Position = 0; Position < Count; Position++ ) {
        CVector Naming = Font->Measure( Names[ Position ] );

        float Piece = Edges[ Position + 1 ] - Edges[ Position ];
        float Center = Bounds.Left + ( Edges[ Position ] + Edges[ Position + 1 ] ) * 0.5f;

        float LabelFade = ( Piece - Naming.Horizontal - 6.0f * Style->Scale ) / ( 14.0f * Style->Scale ) + 1.0f;

        if ( LabelFade > 1.0f )
            LabelFade = 1.0f;

        if ( LabelFade <= 0.02f )
            continue;

        CColor Ink = Position == Selected ? Style->Text : Style->Faint;

        Canvas->PushClip( CRectangle( Bounds.Left + Edges[ Position ], Bounds.Top, Piece, Bounds.Height ) );
        Canvas->Text( CVector( Center - Naming.Horizontal * 0.5f, Bounds.Top + ( Bounds.Height - Font->LineSpan ) * 0.5f ), Ink.Fade( LabelFade ), Names[ Position ] );

        Canvas->PopClip( );
    }

    return Changed;
}

CRectangle CWidgets::Picture( unsigned long long Image, CVector Extent ) {
    CRectangle Bounds = Layout->Place( Extent );

    Canvas->Image( Bounds, Image, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), CColor( 255, 255, 255 ), Style->ControlRounding + 2.0f );
    if ( Style->Borders )
        Canvas->Border( Bounds, Style->Outline, Style->ControlRounding + 2.0f, Style->Thickness );

    return Bounds;
}

void CWidgets::Progress( float Fraction ) {
    if ( Fraction < 0.0f )
        Fraction = 0.0f;

    if ( Fraction > 1.0f )
        Fraction = 1.0f;

    float Line = Style->GrooveWidth + 2.0f * Style->Scale;
    CRectangle Bounds = Layout->Stretch( Line );

    Canvas->Rectangle( Bounds, Style->Groove, Line * 0.5f );
    float Reach = Bounds.Width * Fraction;

    if ( Reach > 0.5f ) {
        if ( Reach < Line )
            Reach = Line;

        Canvas->Rectangle( CRectangle( Bounds.Left, Bounds.Top, Reach, Line ), Style->Accent, Line * 0.5f );
    }
}

void CWidgets::Separator( ) {
    Layout->Skip( Style->Spacing * 0.3f );

    CRectangle Bounds = Layout->Stretch( Style->Thickness );
    Canvas->Rectangle( Bounds, Style->Outline.Fade( 0.6f ), 0.0f );

    Layout->Skip( Style->Spacing * 0.3f );
}

void CWidgets::Tooltip( const char* Message ) {
    if ( !Message || Latest == 0 || Context->HoveredItem != Latest )
        return;

    if ( Latest != Aimed ) {
        Aimed = Latest;
        Dwell = 0.0f;

        return;
    }

    Dwell += Context->DeltaTime;
    if ( Dwell < 0.45f )
        return;

    float Grown = Dwell - 0.45f;
    if ( Grown > 0.15f )
        Grown = 0.15f;

    Grown /= 0.15f;

    CVector Extent = Font->Measure( Message );
    CVector Point = Input->MousePosition;

    float Lip = 8.0f * Style->Scale;
    CRectangle Zone( Point.Horizontal + 14.0f * Style->Scale, Point.Vertical + 20.0f * Style->Scale, Extent.Horizontal + Lip * 2.0f, Extent.Vertical + Lip );

    if ( Context->Display.Horizontal > 0.0f && Zone.Right( ) > Context->Display.Horizontal )
        Zone.Left = Context->Display.Horizontal - Zone.Width - 4.0f;

    if ( Context->Display.Vertical > 0.0f && Zone.Bottom( ) > Context->Display.Vertical )
        Zone.Top = Point.Vertical - Zone.Height - 8.0f;

    int Former = Canvas->Route( OverlayRoute );
    float Former2 = Canvas->Opacity;

    Canvas->Opacity = Grown;
    Canvas->PushClip( Zone.Inflate( Style->Softness ), true );

    if ( Style->Shadows )
        Canvas->Shadow( Zone, Style->Shade, Style->ControlRounding, Style->Softness * 0.5f );

    Canvas->Rectangle( Zone, Style->Popup, Style->ControlRounding );
    if ( Style->Borders )
        Canvas->Border( Zone, Style->Outline, Style->ControlRounding, Style->Thickness );

    Canvas->Text( CVector( Zone.Left + Lip, Zone.Top + Lip * 0.5f ), Style->Text, Message );

    Canvas->PopClip( );
    Canvas->Opacity = Former2;

    Canvas->Route( Former );
}

void CWidgets::Wrapped( const char* Message ) {
    if ( !Message )
        return;

    float Wide = Layout->Width( );
    const char* Cursor = Message;

    while ( *Cursor ) {
        const char* LineStart = Cursor;
        const char* Fit = Cursor;

        const char* Word = Cursor;
        float Reach = 0.0f;

        while ( *Cursor && *Cursor != '\n' ) {
            const char* Step = Cursor;
            unsigned int Codepoint = DecodeCharacter( Step );

            float Advance = Font->Fetch( Codepoint )->Advance;

            if ( Reach + Advance > Wide && Fit != LineStart ) {
                Cursor = Fit;
                break;
            }

            Reach += Advance;
            Cursor = Step;

            if ( Codepoint == ' ' )
                Fit = Cursor;

            Word = Cursor;
        }

        int Length = ( int )( ( Cursor == Fit && Fit != LineStart ? Fit : Word ) - LineStart );
        if ( Cursor >= Word )
            Length = ( int )( Word - LineStart );

        char* Slice = ( char* )Arena->Allocate( ( size_t )Length + 1 );

        for ( int Position = 0; Position < Length; Position++ )
            Slice[ Position ] = LineStart[ Position ];

        Slice[ Length ] = 0;

        CRectangle Bounds = Layout->Place( CVector( Wide, Font->LineSpan ) );
        Canvas->Text( Bounds.Origin( ), Style->Text, Slice );

        while ( *Cursor == ' ' )
            Cursor++;

        if ( *Cursor == '\n' )
            Cursor++;
    }
}

void CWidgets::Bullet( const char* Message ) {
    CVector Extent = Font->Measure( Message );
    CRectangle Bounds = Layout->Place( CVector( Extent.Horizontal + Style->PaddingWide, Font->LineSpan ) );

    float Wide = 8.0f * Style->Scale;
    float Tall = 2.0f * Style->Scale;

    Canvas->Rectangle( CRectangle( Bounds.Left + 3.0f * Style->Scale, Bounds.Top + Font->LineSpan * 0.5f - Tall * 0.5f, Wide, Tall ), Style->Faint, Tall * 0.5f );
    Canvas->Text( CVector( Bounds.Left + Style->PaddingWide, Bounds.Top ), Style->Text, Message );
}

void CWidgets::Colored( CColor Tint, const char* Message ) {
    CVector Extent = Font->Measure( Message );
    CRectangle Bounds = Layout->Place( Extent );

    Canvas->Text( Bounds.Origin( ), Tint, Message );
}

bool CWidgets::Small( const char* Title ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    CVector Naming = Font->Measure( Shown );
    CRectangle Bounds = Layout->Place( CVector( Naming.Horizontal + Style->PaddingWide, Font->LineSpan + 6.0f * Style->Scale ) );

    bool Hovered = false;
    bool Held = false;

    bool Clicked = Behavior( Identifier, Bounds, Hovered, Held );
    float Glow = Ease( Identifier, Hovered );

    CMotion& Movement = Motion( Identifier );

    if ( Held && Hovered ) {
        Movement.Slide = 1.0f;
    } else {
        float Speed = Style->FadeSpeed * 1.6f * Context->DeltaTime;
        if ( Speed > 1.0f )
            Speed = 1.0f;

        Movement.Slide += ( 0.0f - Movement.Slide ) * Speed;
    }

    CColor Fill = Style->Control.Blend( Style->Hovered, Glow ).Blend( Style->Pressed, Movement.Slide );

    Canvas->Rectangle( Bounds, Fill, Style->ControlRounding * 0.7f );
    if ( Style->Borders )
        Canvas->Border( Bounds, Style->Outline, Style->ControlRounding * 0.7f, Style->Thickness );

    Canvas->Text( CVector( Bounds.Left + ( Bounds.Width - Naming.Horizontal ) * 0.5f, Bounds.Top + ( Bounds.Height - Font->LineSpan ) * 0.5f ), Style->Text, Shown );
    return Clicked;
}

bool CWidgets::Toggle( const char* Title, bool& State ) {
    unsigned int Identifier = Context->Hash( Title );
    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );

    float Track = Style->ControlHeight * 0.54f;
    float Wide = Track * 1.9f;

    CRectangle Switch( Bounds.Left, Bounds.Top + ( Bounds.Height - Track ) * 0.5f, Wide, Track );

    bool Hovered = false;
    bool Held = false;

    bool Clicked = Behavior( Identifier, Bounds, Hovered, Held );
    if ( Clicked )
        State = !State;

    CMotion& Movement = Motion( Identifier );

    if ( !Movement.Seen ) {
        Movement.Seen = true;
        Movement.Slide = State ? 1.0f : 0.0f;
    }

    float Speed = Style->FadeSpeed * Context->DeltaTime;
    if ( Speed > 1.0f )
        Speed = 1.0f;

    Movement.Slide += ( ( State ? 1.0f : 0.0f ) - Movement.Slide ) * Speed;
    float Glow = Ease( Identifier, Hovered );

    CColor Idle = Style->Groove.Blend( Style->Hovered, Glow * 0.6f );
    CColor Rail = Idle.Blend( Style->Accent, Movement.Slide );

    Canvas->Rectangle( Switch, Rail, Track * 0.5f );
    if ( Style->Borders )
        Canvas->Border( Switch, Style->Outline.Blend( Style->Accent, Movement.Slide ), Track * 0.5f, Style->Thickness );

    float Radius = Track * 0.5f - 3.0f * Style->Scale;
    float Travel = Wide - Radius * 2.0f - 6.0f * Style->Scale;

    float Center = Switch.Left + 3.0f * Style->Scale + Radius + Travel * Movement.Slide;
    CVector Middle( Center, Switch.Center( ).Vertical );

    if ( Style->Shadows )
        Canvas->Shadow( CRectangle( Center - Radius, Middle.Vertical - Radius, Radius * 2.0f, Radius * 2.0f ), Style->Shade.Fade( 0.55f ), Radius, 3.0f * Style->Scale );

    Canvas->Circle( Middle, Radius, CColor( 255, 255, 255 ) );
    Canvas->Text( CVector( Switch.Right( ) + Style->Spacing, Bounds.Top + ( Bounds.Height - Font->LineSpan ) * 0.5f ), Style->Text, Context->Caption( Title ) );
    
    return Clicked;
}

static int RemoveRange( char* Buffer, int From, int To ) {
    if ( From >= To )
        return 0;

    int Length = ( int )strlen( Buffer );
    memmove( Buffer + From, Buffer + To, ( size_t )( Length - To + 1 ) );

    return To - From;
}

static int InsertText( char* Buffer, int Capacity, int At, const char* Text, int Count ) {
    int Length = ( int )strlen( Buffer );

    if ( Count <= 0 )
        return 0;

    if ( Length + Count >= Capacity )
        Count = Capacity - 1 - Length;

    if ( Count <= 0 )
        return 0;

    memmove( Buffer + At + Count, Buffer + At, ( size_t )( Length - At + 1 ) );
    memcpy( Buffer + At, Text, ( size_t )Count );

    return Count;
}

static int WordLeft( const char* Buffer, int Position ) {
    while ( Position > 0 && Buffer[ Position - 1 ] == ' ' )
        Position--;

    while ( Position > 0 && Buffer[ Position - 1 ] != ' ' && Buffer[ Position - 1 ] != '\n' )
        Position = PriorBoundary( Buffer, Position );

    return Position;
}

static int WordRight( const char* Buffer, int Position ) {
    int Length = ( int )strlen( Buffer );

    while ( Position < Length && Buffer[ Position ] != ' ' && Buffer[ Position ] != '\n' )
        Position = NextBoundary( Buffer, Position );

    while ( Position < Length && Buffer[ Position ] == ' ' )
        Position++;

    return Position;
}

static int LineHead( const char* Buffer, int Position ) {
    while ( Position > 0 && Buffer[ Position - 1 ] != '\n' )
        Position--;

    return Position;
}

static int LineTail( const char* Buffer, int Position ) {
    int Length = ( int )strlen( Buffer );

    while ( Position < Length && Buffer[ Position ] != '\n' )
        Position++;

    return Position;
}

CEditor& CWidgets::Editor( unsigned int Identifier ) {
    CEditor& State = Editors[ Identifier ];
    State.Stamp = Context->FrameIndex;

    return State;
}

void CWidgets::Snapshot( CEditor& State, const char* Buffer ) {
    if ( State.At + 1 < ( int )State.History.size( ) ) {
        State.History.resize( ( size_t )State.At + 1 );
        State.Carets.resize( ( size_t )State.At + 1 );
    }

    if ( !State.History.empty( ) && State.History.back( ) == Buffer ) {
        State.Carets.back( ) = State.Caret;
        return;
    }

    State.History.push_back( Buffer );
    State.Carets.push_back( State.Caret );

    if ( State.History.size( ) > 64 ) {
        State.History.erase( State.History.begin( ) );
        State.Carets.erase( State.Carets.begin( ) );
    }

    State.At = ( int )State.History.size( ) - 1;
}

bool CWidgets::Edit( CEditor& State, char* Buffer, int Capacity, bool Multiline ) {
    int Length = ( int )strlen( Buffer );

    if ( State.Caret > Length )
        State.Caret = Length;

    if ( State.Mark > Length )
        State.Mark = Length;

    if ( State.Caret < 0 )
        State.Caret = 0;

    if ( State.Mark < 0 )
        State.Mark = 0;

    if ( State.History.empty( ) )
        Snapshot( State, Buffer );

    bool Shift = Input->Shift( );
    bool Control = Input->Control( );

    bool Changed = false;
    bool Moved = false;

    int Low = State.Caret < State.Mark ? State.Caret : State.Mark;
    int High = State.Caret < State.Mark ? State.Mark : State.Caret;

    bool Selected = Low != High;

    if ( Control && Input->KeyPressed( KeyLetterZ ) ) {
        if ( State.At > 0 ) {
            State.At--;

            strncpy_s( Buffer, ( size_t )Capacity, State.History[ State.At ].c_str( ), _TRUNCATE );
            State.Caret = State.Carets[ State.At ];

            State.Mark = State.Caret;
            State.Blink = 0.0f;
        }

        return true;
    }

    if ( Control && Input->KeyPressed( KeyLetterY ) ) {
        if ( State.At + 1 < ( int )State.History.size( ) ) {
            State.At++;

            strncpy_s( Buffer, ( size_t )Capacity, State.History[ State.At ].c_str( ), _TRUNCATE );
            State.Caret = State.Carets[ State.At ];

            State.Mark = State.Caret;
            State.Blink = 0.0f;
        }

        return true;
    }

    if ( Input->KeyRepeated( KeyLeft ) ) {
        if ( Selected && !Shift )
            State.Caret = Low;
        else
            State.Caret = Control ? WordLeft( Buffer, State.Caret ) : PriorBoundary( Buffer, State.Caret );

        Moved = true;
    } else if ( Input->KeyRepeated( KeyRight ) ) {
        if ( Selected && !Shift )
            State.Caret = High;
        else
            State.Caret = Control ? WordRight( Buffer, State.Caret ) : NextBoundary( Buffer, State.Caret );

        Moved = true;
    } else if ( Input->KeyRepeated( KeyHome ) ) {
        State.Caret = Multiline ? LineHead( Buffer, State.Caret ) : 0;
        Moved = true;
    } else if ( Input->KeyRepeated( KeyEnd ) ) {
        State.Caret = Multiline ? LineTail( Buffer, State.Caret ) : Length;
        Moved = true;
    } else if ( Multiline && Input->KeyRepeated( KeyUp ) ) {
        int Head = LineHead( Buffer, State.Caret );
        int Column = State.Caret - Head;

        if ( Head > 0 ) {
            int PriorHead = LineHead( Buffer, Head - 1 );
            int PriorTail = Head - 1;

            State.Caret = PriorHead + Column < PriorTail ? PriorHead + Column : PriorTail;
        }

        Moved = true;
    } else if ( Multiline && Input->KeyRepeated( KeyDown ) ) {
        int Head = LineHead( Buffer, State.Caret );
        int Column = State.Caret - Head;

        int Tail = LineTail( Buffer, State.Caret );

        if ( Tail < Length ) {
            int NextHead = Tail + 1;
            int NextTail = LineTail( Buffer, NextHead );

            State.Caret = NextHead + Column < NextTail ? NextHead + Column : NextTail;
        }

        Moved = true;
    }

    if ( Moved ) {
        if ( !Shift )
            State.Mark = State.Caret;

        State.Blink = 0.0f;
    }

    if ( Control && Input->KeyPressed( KeyLetterA ) ) {
        State.Mark = 0;
        State.Caret = Length;
    }

    Low = State.Caret < State.Mark ? State.Caret : State.Mark;
    High = State.Caret < State.Mark ? State.Mark : State.Caret;

    Selected = Low != High;

    if ( Control && Selected && ( Input->KeyPressed( KeyLetterC ) || Input->KeyPressed( KeyLetterX ) ) ) {
        std::string Piece( Buffer + Low, Buffer + High );
        Native->ClipboardSet( Piece.c_str( ) );

        if ( Input->KeyPressed( KeyLetterX ) ) {
            RemoveRange( Buffer, Low, High );

            State.Caret = Low;
            State.Mark = Low;

            Changed = true;
        }
    }

    if ( Control && Input->KeyPressed( KeyLetterV ) ) {
        std::string Paste = Native->ClipboardGet( );
        std::string Clean;

        for ( char Letter : Paste ) {
            if ( ( unsigned char )Letter >= 32 || ( Multiline && Letter == '\n' ) )
                Clean.push_back( Letter );
        }

        if ( Selected ) {
            RemoveRange( Buffer, Low, High );

            State.Caret = Low;
            State.Mark = Low;
        }

        int Put = InsertText( Buffer, Capacity, State.Caret, Clean.c_str( ), ( int )Clean.size( ) );

        State.Caret += Put;
        State.Mark = State.Caret;

        if ( Put > 0 )
            Changed = true;
    }

    if ( Input->KeyRepeated( KeyBackspace ) ) {
        if ( Selected ) {
            RemoveRange( Buffer, Low, High );

            State.Caret = Low;
            State.Mark = Low;

            Changed = true;
        } else if ( State.Caret > 0 ) {
            int Prior = PriorBoundary( Buffer, State.Caret );
            RemoveRange( Buffer, Prior, State.Caret );

            State.Caret = Prior;
            State.Mark = Prior;

            Changed = true;
        }
    } else if ( Input->KeyRepeated( KeyDelete ) ) {
        if ( Selected ) {
            RemoveRange( Buffer, Low, High );

            State.Caret = Low;
            State.Mark = Low;

            Changed = true;
        } else if ( State.Caret < Length ) {
            int Next = NextBoundary( Buffer, State.Caret );
            RemoveRange( Buffer, State.Caret, Next );

            Changed = true;
        }
    }

    if ( Multiline && Input->KeyRepeated( KeyEnter ) ) {
        Low = State.Caret < State.Mark ? State.Caret : State.Mark;
        High = State.Caret < State.Mark ? State.Mark : State.Caret;

        if ( Low != High ) {
            RemoveRange( Buffer, Low, High );

            State.Caret = Low;
            State.Mark = Low;
        }

        int Put = InsertText( Buffer, Capacity, State.Caret, "\n", 1 );

        State.Caret += Put;
        State.Mark = State.Caret;

        if ( Put > 0 )
            Changed = true;
    }

    for ( unsigned int Codepoint : Input->Typed ) {
        if ( Codepoint < 32 )
            continue;

        int Anchor = State.Caret < State.Mark ? State.Caret : State.Mark;
        int Reach = State.Caret < State.Mark ? State.Mark : State.Caret;

        if ( Anchor != Reach ) {
            RemoveRange( Buffer, Anchor, Reach );

            State.Caret = Anchor;
            State.Mark = Anchor;
        }

        char Bytes[ 4 ];
        int Count = Encode( Codepoint, Bytes );

        int Put = InsertText( Buffer, Capacity, State.Caret, Bytes, Count );

        State.Caret += Put;
        State.Mark = State.Caret;

        if ( Put > 0 )
            Changed = true;
    }

    if ( Changed ) {
        State.Blink = 0.0f;
        Snapshot( State, Buffer );
    }

    return Changed;
}

bool CWidgets::Field( const char* Title, char* Buffer, int Capacity, const char* Hint ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    CRectangle Row = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );
    float Label = Shown[ 0 ] ? Font->Measure( Shown ).Horizontal + Style->Spacing : 0.0f;

    CRectangle Box( Row.Left, Row.Top, Row.Width - Label, Row.Height );
    float Inset = Style->PaddingWide * 0.5f;

    CVector Point = Input->MousePosition;

    bool Hovered = Box.Contains( Point ) && Canvas->Visible( Point ) && Frames->Beneath == Frames->Current && !Context->Covered( Point );
    if ( Hovered )
        Input->Pointer = PointerText;

    CEditor& State = Editor( Identifier );

    if ( Hovered && Input->MousePressed( 0 ) ) {
        Context->FocusedItem = Identifier;
        Context->ActiveItem = Identifier;

        Context->FocusClaimed = true;
        int Hit = Font->Hit( Buffer, Point.Horizontal - ( Box.Left + Inset ) + State.Scroll );

        State.Caret = Hit;
        State.Blink = 0.0f;

        if ( !Input->Shift( ) )
            State.Mark = Hit;

        if ( Input->MouseDouble( 0 ) ) {
            State.Mark = WordLeft( Buffer, Hit );
            State.Caret = WordRight( Buffer, Hit );
        }

        State.Pen = Font->Span( Buffer, State.Caret );
    }

    bool Focused = Context->FocusedItem == Identifier;
    bool Changed = false;

    if ( Context->ActiveItem == Identifier ) {
        if ( Input->MouseDown( 0 ) )
            State.Caret = Font->Hit( Buffer, Point.Horizontal - ( Box.Left + Inset ) + State.Scroll );
        else
            Context->ActiveItem = 0;
    }

    if ( Focused ) {
        Context->WantKeyboard = true;

        Changed = Edit( State, Buffer, Capacity, false );

        if ( Input->KeyPressed( KeyEnter ) || Input->KeyPressed( KeyEscape ) )
            Context->FocusedItem = 0;

        State.Blink += Context->DeltaTime;
    }

    Canvas->Rectangle( Box, Style->Control, Style->ControlRounding );
    if ( Style->Borders )
        Canvas->Border( Box, Focused ? Style->Accent : Style->Outline, Style->ControlRounding, Style->Thickness );

    float TextTop = Box.Top + ( Box.Height - Font->LineSpan ) * 0.5f;
    int Length = ( int )strlen( Buffer );

    float View = Box.Width - Inset * 2.0f;
    float CaretX = Font->Span( Buffer, State.Caret );

    float Reach = Font->Measure( Buffer ).Horizontal;
    float Target = State.Scroll;

    if ( Focused ) {
        if ( CaretX - Target > View )
            Target = CaretX - View;

        if ( CaretX - Target < 0.0f )
            Target = CaretX;
    } else {
        Target = 0.0f;
    }

    float Ceiling = Reach - View;
    if ( Ceiling < 0.0f )
        Ceiling = 0.0f;

    if ( Target > Ceiling )
        Target = Ceiling;

    if ( Target < 0.0f )
        Target = 0.0f;

    float Chase = 1.0f - expf( -34.0f * Context->DeltaTime );

    State.Scroll += ( Target - State.Scroll ) * Chase;
    if ( fabsf( Target - State.Scroll ) < 0.5f )
        State.Scroll = Target;

    State.Pen += ( CaretX - State.Pen ) * Chase;
    if ( fabsf( CaretX - State.Pen ) < 0.5f )
        State.Pen = CaretX;

    Canvas->PushClip( CRectangle( Box.Left + Inset, Box.Top, View, Box.Height ) );
    float BaseX = Box.Left + Inset - State.Scroll;

    if ( Focused && State.Caret != State.Mark ) {
        int First = State.Caret < State.Mark ? State.Caret : State.Mark;
        int Last = State.Caret < State.Mark ? State.Mark : State.Caret;

        float Start = BaseX + Font->Span( Buffer, First );
        float Finish = BaseX + Font->Span( Buffer, Last );

        Canvas->Rectangle( CRectangle( Start, Box.Top + 3.0f * Style->Scale, Finish - Start, Box.Height - 6.0f * Style->Scale ), Style->Accent.Fade( 0.32f ), 2.0f );
    }

    if ( Length > 0 )
        Canvas->Text( CVector( BaseX, TextTop ), Style->Text, Buffer );
    else if ( Hint )
        Canvas->Text( CVector( Box.Left + Inset, TextTop ), Style->Faint.Fade( 0.7f ), Hint );

    if ( Focused ) {
        float Pulse = 0.4f + 0.6f * ( 0.5f + 0.5f * cosf( State.Blink * 6.2831853f ) );
        Canvas->Rectangle( CRectangle( BaseX + State.Pen, Box.Top + 4.0f * Style->Scale, 1.5f * Style->Scale, Box.Height - 8.0f * Style->Scale ), Style->Text.Fade( Pulse ), 0.75f * Style->Scale );
    }

    Canvas->PopClip( );

    if ( Shown[ 0 ] )
        Canvas->Text( CVector( Box.Right( ) + Style->Spacing, TextTop ), Style->Text, Shown );

    return Changed;
}

static int LineAt( const char* Buffer, int Index ) {
    int Position = 0;
    int Line = 0;

    while ( Buffer[ Position ] && Line < Index ) {
        if ( Buffer[ Position ] == '\n' )
            Line++;

        Position++;
    }

    return Position;
}

bool CWidgets::Area( const char* Title, char* Buffer, int Capacity, float Height ) {
    unsigned int Identifier = Context->Hash( Title );

    float Tall = Height > 0.0f ? Height : Font->LineSpan * 5.0f + Style->PaddingTall;
    CRectangle Box = Layout->Place( CVector( Layout->Width( ), Tall ) );

    float Inset = Style->PaddingWide * 0.5f;
    CVector Point = Input->MousePosition;

    bool Hovered = Box.Contains( Point ) && Canvas->Visible( Point ) && Frames->Beneath == Frames->Current && !Context->Covered( Point );
    if ( Hovered )
        Input->Pointer = PointerText;

    CEditor& State = Editor( Identifier );

    if ( Hovered && Input->MousePressed( 0 ) ) {
        Context->FocusedItem = Identifier;
        Context->ActiveItem = Identifier;

        Context->FocusClaimed = true;

        int Line = ( int )( ( Point.Vertical - ( Box.Top + Inset ) + State.Scroll ) / Font->LineSpan );
        if ( Line < 0 )
            Line = 0;

        int Head = LineAt( Buffer, Line );
        int Column = Font->Hit( Buffer + Head, Point.Horizontal - ( Box.Left + Inset ) );

        State.Caret = Head + Column;
        State.Mark = State.Caret;

        State.Blink = 0.0f;
    }

    bool Focused = Context->FocusedItem == Identifier;
    bool Changed = false;

    if ( Context->ActiveItem == Identifier && !Input->MouseDown( 0 ) )
        Context->ActiveItem = 0;

    if ( Focused ) {
        Context->WantKeyboard = true;

        Changed = Edit( State, Buffer, Capacity, true );

        if ( Input->KeyPressed( KeyEscape ) )
            Context->FocusedItem = 0;

        State.Blink += Context->DeltaTime;
    }

    Canvas->Rectangle( Box, Style->Control, Style->ControlRounding );
    if ( Style->Borders )
        Canvas->Border( Box, Focused ? Style->Accent : Style->Outline, Style->ControlRounding, Style->Thickness );

    int CaretLine = 0;

    for ( int Position = 0; Position < State.Caret; Position++ ) {
        if ( Buffer[ Position ] == '\n' )
            CaretLine++;
    }

    float View = Box.Height - Inset * 2.0f;
    float CaretY = CaretLine * Font->LineSpan;

    if ( CaretY - State.Scroll > View - Font->LineSpan )
        State.Scroll = CaretY - View + Font->LineSpan;

    if ( CaretY - State.Scroll < 0.0f )
        State.Scroll = CaretY;

    if ( State.Scroll < 0.0f )
        State.Scroll = 0.0f;

    Canvas->PushClip( Box.Pad( Inset, Inset ) );

    int First = State.Caret < State.Mark ? State.Caret : State.Mark;
    int Last = State.Caret < State.Mark ? State.Mark : State.Caret;

    const char* Cursor = Buffer;
    int Offset = 0;

    int Line = 0;

    while ( true ) {
        const char* Stop = Cursor;

        while ( *Stop && *Stop != '\n' )
            Stop++;

        int Bytes = ( int )( Stop - Cursor );
        float LineTop = Box.Top + Inset + Line * Font->LineSpan - State.Scroll;

        if ( LineTop > Box.Top - Font->LineSpan && LineTop < Box.Bottom( ) ) {
            if ( Focused && Last > Offset && First < Offset + Bytes + 1 ) {
                int SelectFrom = First > Offset ? First - Offset : 0;
                int SelectTo = Last < Offset + Bytes ? Last - Offset : Bytes;

                float Start = Box.Left + Inset + Font->Span( Cursor, SelectFrom );
                float Finish = Box.Left + Inset + Font->Span( Cursor, SelectTo );

                if ( Last > Offset + Bytes )
                    Finish += 4.0f * Style->Scale;

                Canvas->Rectangle( CRectangle( Start, LineTop, Finish - Start, Font->LineSpan ), Style->Accent.Fade( 0.32f ), 1.0f );
            }

            if ( Bytes > 0 ) {
                char* Slice = ( char* )Arena->Allocate( ( size_t )Bytes + 1 );

                memcpy( Slice, Cursor, ( size_t )Bytes );
                Slice[ Bytes ] = 0;

                Canvas->Text( CVector( Box.Left + Inset, LineTop ), Style->Text, Slice );
            }
        }

        if ( !*Stop )
            break;

        Cursor = Stop + 1;
        Offset += Bytes + 1;

        Line++;
    }

    if ( Focused && State.Blink - ( float )( int )State.Blink < 0.5f ) {
        int Head = LineHead( Buffer, State.Caret );
        float CaretX = Box.Left + Inset + Font->Span( Buffer + Head, State.Caret - Head );

        float Top = Box.Top + Inset + CaretLine * Font->LineSpan - State.Scroll;
        Canvas->Rectangle( CRectangle( CaretX, Top, 1.0f * Style->Scale, Font->LineSpan ), Style->Text, 0.0f );
    }

    Canvas->PopClip( );
    return Changed;
}

bool CWidgets::SliderWhole( const char* Title, int& Current, int Minimum, int Maximum ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    float Depth = Font->LineSpan + Style->KnobSize * 2.0f + 8.0f * Style->Scale;
    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Depth ) );

    bool Hovered = false;
    bool Held = false;

    Behavior( Identifier, Bounds, Hovered, Held );
    int Before = Current;

    if ( Held && Maximum > Minimum && Bounds.Width > 0.0f ) {
        int Span = Maximum - Minimum;
        float Local = Input->MousePosition.Horizontal - Bounds.Left;
        if ( Local < 0.0f )
            Local = 0.0f;
        if ( Local > Bounds.Width )
            Local = Bounds.Width;

        if ( Span <= 16 ) {
            float Slot = Bounds.Width / ( float )( Span + 1 );
            int Step = ( int )( Local / Slot );
            if ( Step > Span )
                Step = Span;
            Current = Minimum + Step;
        } else {
            float Ratio = Local / Bounds.Width;
            Current = Minimum + ( int )( ( float )Span * Ratio + 0.5f );
        }
    }

    if ( Current < Minimum )
        Current = Minimum;

    if ( Current > Maximum )
        Current = Maximum;

    float Glow = Ease( Identifier, Hovered || Held );

    char* Reading = Format->Print( "%d", Current );
    CVector Extent = Font->Measure( Reading );
    CVector Titled = Font->Measure( Shown );

    Canvas->Text( Bounds.Origin( ), Style->Text, Shown );
    Canvas->Text( CVector( Bounds.Left + Titled.Horizontal + 8.0f * Style->Scale, Bounds.Top ), Style->Faint.Blend( Style->Text, Glow ), Reading );

    float Line = Style->GrooveWidth;
    float Middle = Bounds.Bottom( ) - Style->KnobSize - 1.0f;

    CRectangle Groove( Bounds.Left, Middle - Line * 0.5f, Bounds.Width, Line );
    Canvas->Rectangle( Groove, Style->Groove, Line * 0.5f );

    float Portion = Maximum > Minimum ? ( float )( Current - Minimum ) / ( float )( Maximum - Minimum ) : 0.0f;
    Canvas->Rectangle( CRectangle( Groove.Left, Groove.Top, Groove.Width * Portion, Line ), Style->Accent, Line * 0.5f );

    if ( Maximum - Minimum <= 16 && Maximum > Minimum ) {
        int Span = Maximum - Minimum;
        for ( int Tick = 0; Tick <= Span; Tick++ ) {
            float Across = Groove.Left + Groove.Width * ( ( float )Tick / ( float )Span );
            float Mark = 3.0f * Style->Scale;
            Canvas->Rectangle( CRectangle( Across - 0.5f * Style->Scale, Middle - Mark, 1.0f * Style->Scale, Mark * 2.0f ), Style->Faint.Fade( 0.55f ), 0.0f );
        }
    }

    float Radius = Style->KnobSize + ( Held ? 1.5f : Glow * 0.75f ) * Style->Scale;
    float Center = Groove.Left + Groove.Width * Portion;

    if ( Center < Groove.Left + Radius )
        Center = Groove.Left + Radius;

    if ( Center > Groove.Right( ) - Radius )
        Center = Groove.Right( ) - Radius;

    Canvas->Circle( CVector( Center, Middle ), Radius, Style->Knob );

    if ( Style->Borders ) {
        CRectangle Ring( Center - Radius, Middle - Radius, Radius * 2.0f, Radius * 2.0f );
        Canvas->Border( Ring, Style->Outline.Blend( Style->Accent, Glow ), Radius, Style->Thickness );
    }

    return Current != Before;
}

bool CWidgets::DragBox( unsigned int Identifier, CRectangle Box, float& Value, float Speed, float Minimum, float Maximum, const char* Pattern ) {
    bool Hovered = false;
    bool Held = false;

    Behavior( Identifier, Box, Hovered, Held );
    float Before = Value;

    if ( Hovered || Held )
        Input->Pointer = PointerAcross;

    CMotion& Movement = Motion( Identifier );

    if ( Held && Input->MousePressed( 0 ) )
        Movement.Slide = Value;

    if ( Held ) {
        float Delta = Input->MousePosition.Horizontal - Input->PressPosition.Horizontal;
        Value = Movement.Slide + Delta * ( Speed > 0.0f ? Speed : 1.0f );
    }

    if ( Maximum > Minimum ) {
        if ( Value < Minimum )
            Value = Minimum;

        if ( Value > Maximum )
            Value = Maximum;
    }

    float Glow = Ease( Identifier, Hovered || Held );

    CColor Fill = Style->Control.Blend( Style->Hovered, Glow );
    if ( Held )
        Fill = Style->Pressed;

    Canvas->Rectangle( Box, Fill, Style->ControlRounding );
    if ( Style->Borders )
        Canvas->Border( Box, Held ? Style->Accent : Style->Outline, Style->ControlRounding, Style->Thickness );

    char* Reading = Format->Print( Pattern && Pattern[ 0 ] ? Pattern : "%.3f", ( double )Value );
    CVector Extent = Font->Measure( Reading );

    Canvas->PushClip( Box );
    Canvas->Text( CVector( Box.Center( ).Horizontal - Extent.Horizontal * 0.5f, Box.Top + ( Box.Height - Font->LineSpan ) * 0.5f ), Style->Text, Reading );

    Canvas->PopClip( );
    return Value != Before;
}

bool CWidgets::Drag( const char* Title, float& Current, float Speed, float Minimum, float Maximum, const char* Format ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    CRectangle Row = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );
    float Label = Shown[ 0 ] ? Font->Measure( Shown ).Horizontal + Style->Spacing : 0.0f;

    CRectangle Box( Row.Left, Row.Top, Row.Width - Label, Row.Height );
    bool Changed = DragBox( Identifier, Box, Current, Speed, Minimum, Maximum, Format );

    if ( Shown[ 0 ] )
        Canvas->Text( CVector( Box.Right( ) + Style->Spacing, Box.Top + ( Box.Height - Font->LineSpan ) * 0.5f ), Style->Text, Shown );

    return Changed;
}

bool CWidgets::DragWhole( const char* Title, int& Current, float Speed, int Minimum, int Maximum ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    CRectangle Row = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );
    float Label = Shown[ 0 ] ? Font->Measure( Shown ).Horizontal + Style->Spacing : 0.0f;

    CRectangle Box( Row.Left, Row.Top, Row.Width - Label, Row.Height );
    float Temp = ( float )Current;

    bool Changed = DragBox( Identifier, Box, Temp, Speed, ( float )Minimum, ( float )Maximum, "%.0f" );
    if ( Changed )
        Current = ( int )( Temp + ( Temp < 0.0f ? -0.5f : 0.5f ) );

    if ( Shown[ 0 ] )
        Canvas->Text( CVector( Box.Right( ) + Style->Spacing, Box.Top + ( Box.Height - Font->LineSpan ) * 0.5f ), Style->Text, Shown );

    return Changed;
}

bool CWidgets::Vector( const char* Title, float* Values, int Count, float Minimum, float Maximum ) {
    if ( !Values || Count <= 0 )
        return false;

    if ( Count > 4 )
        Count = 4;

    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    CRectangle Row = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );
    float Label = Shown[ 0 ] ? Font->Measure( Shown ).Horizontal + Style->Spacing : 0.0f;

    float Gap = Style->Spacing * 0.5f;
    float Wide = ( Row.Width - Label - Gap * ( float )( Count - 1 ) ) / ( float )Count;

    bool Changed = false;

    for ( int Component = 0; Component < Count; Component++ ) {
        CRectangle Box( Row.Left + ( Wide + Gap ) * ( float )Component, Row.Top, Wide, Row.Height );
        unsigned int Piece = Identifier ^ ( 0x9E3779B9u * ( unsigned int )( Component + 1 ) );

        if ( DragBox( Piece, Box, Values[ Component ], ( Maximum - Minimum ) / 400.0f, Minimum, Maximum, "%.2f" ) )
            Changed = true;
    }

    if ( Shown[ 0 ] )
        Canvas->Text( CVector( Row.Left + Count * Wide + ( Count - 1 ) * Gap + Style->Spacing, Row.Top + ( Row.Height - Font->LineSpan ) * 0.5f ), Style->Text, Shown );

    return Changed;
}

bool CWidgets::Spinner( const char* Title, CRectangle Row, double& Value, double Step, bool Whole ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    float Label = Shown[ 0 ] ? Font->Measure( Shown ).Horizontal + Style->Spacing : 0.0f;
    CRectangle Box( Row.Left, Row.Top, Row.Width - Label, Row.Height );

    float Wide = Box.Height;

    CRectangle Minus( Box.Left, Box.Top, Wide, Box.Height );
    CRectangle Plus( Box.Right( ) - Wide, Box.Top, Wide, Box.Height );

    CRectangle Middle( Box.Left + Wide, Box.Top, Box.Width - Wide * 2.0f, Box.Height );

    CEditor& State = Editor( Identifier );
    bool Changed = false;

    bool Hovered = false;
    bool Held = false;

    if ( Behavior( Identifier ^ 0x11111111u, Minus, Hovered, Held ) ) {
        Value -= Step;
        Changed = true;
    }

    if ( Behavior( Identifier ^ 0x22222222u, Plus, Hovered, Held ) ) {
        Value += Step;
        Changed = true;
    }

    CVector Point = Input->MousePosition;
    bool OverMiddle = Middle.Contains( Point ) && Canvas->Visible( Point ) && Frames->Beneath == Frames->Current && !Context->Covered( Point );

    if ( OverMiddle )
        Input->Pointer = PointerText;

    bool Focused = Context->FocusedItem == Identifier;

    if ( OverMiddle && Input->MousePressed( 0 ) ) {
        Context->FocusedItem = Identifier;
        Context->FocusClaimed = true;

        Focused = true;

        State.Mark = 0;
        State.Caret = ( int )strlen( State.Local );

        State.History.clear( );
        State.Carets.clear( );

        State.At = 0;
        State.Blink = 0.0f;
    }

    if ( !Focused ) {
        if ( Whole )
            snprintf( State.Local, sizeof( State.Local ), "%lld", ( long long )( Value < 0.0 ? Value - 0.5 : Value + 0.5 ) );
        else
            snprintf( State.Local, sizeof( State.Local ), "%.4g", Value );
    } else {
        Context->WantKeyboard = true;

        if ( Edit( State, State.Local, ( int )sizeof( State.Local ), false ) ) {
            Value = atof( State.Local );
            Changed = true;
        }

        if ( Input->KeyPressed( KeyEnter ) || Input->KeyPressed( KeyEscape ) ) {
            Value = atof( State.Local );
            Context->FocusedItem = 0;
        }

        State.Blink += Context->DeltaTime;
    }

    Canvas->Rectangle( Box, Style->Control, Style->ControlRounding );
    if ( Style->Borders )
        Canvas->Border( Box, Focused ? Style->Accent : Style->Outline, Style->ControlRounding, Style->Thickness );

    float Stroke = Style->IconStroke;
    CColor Ink = Style->Faint;

    Canvas->Stroke( Minus.Shrink( Minus.Width * 0.34f ), CVector( 0.0f, 0.5f ), CVector( 1.0f, 0.5f ), Ink, Stroke );

    Canvas->Stroke( Plus.Shrink( Plus.Width * 0.34f ), CVector( 0.0f, 0.5f ), CVector( 1.0f, 0.5f ), Ink, Stroke );
    Canvas->Stroke( Plus.Shrink( Plus.Width * 0.34f ), CVector( 0.5f, 0.0f ), CVector( 0.5f, 1.0f ), Ink, Stroke );

    float TextTop = Box.Top + ( Box.Height - Font->LineSpan ) * 0.5f;
    CVector Extent = Font->Measure( State.Local );

    Canvas->PushClip( Middle );
    float Across = Focused ? Middle.Left + 4.0f * Style->Scale : Middle.Center( ).Horizontal - Extent.Horizontal * 0.5f;

    if ( Focused && State.Caret != State.Mark ) {
        int First = State.Caret < State.Mark ? State.Caret : State.Mark;
        int Last = State.Caret < State.Mark ? State.Mark : State.Caret;

        float Start = Across + Font->Span( State.Local, First );
        float Finish = Across + Font->Span( State.Local, Last );

        Canvas->Rectangle( CRectangle( Start, Box.Top + 3.0f * Style->Scale, Finish - Start, Box.Height - 6.0f * Style->Scale ), Style->Accent.Fade( 0.32f ), 2.0f );
    }

    Canvas->Text( CVector( Across, TextTop ), Style->Text, State.Local );

    if ( Focused && State.Blink - ( float )( int )State.Blink < 0.5f ) {
        float CaretX = Across + Font->Span( State.Local, State.Caret );
        Canvas->Rectangle( CRectangle( CaretX, Box.Top + 4.0f * Style->Scale, 1.0f * Style->Scale, Box.Height - 8.0f * Style->Scale ), Style->Text, 0.0f );
    }

    Canvas->PopClip( );

    if ( Shown[ 0 ] )
        Canvas->Text( CVector( Box.Right( ) + Style->Spacing, TextTop ), Style->Text, Shown );

    return Changed;
}

bool CWidgets::Number( const char* Title, int& Value, int Step ) {
    CRectangle Box = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );
    double Temp = ( double )Value;

    bool Changed = Spinner( Title, Box, Temp, ( double )Step, true );
    if ( Changed )
        Value = ( int )( Temp < 0.0 ? Temp - 0.5 : Temp + 0.5 );

    return Changed;
}

static void Chevron( CRectangle Mark, float Turn, CColor Ink, float Line ) {
    CVector First( 0.3f + ( 0.15f - 0.3f ) * Turn, 0.18f + ( 0.34f - 0.18f ) * Turn );
    CVector Corner( 0.68f + ( 0.5f - 0.68f ) * Turn, 0.5f + ( 0.68f - 0.5f ) * Turn );

    CVector Last( 0.3f + ( 0.85f - 0.3f ) * Turn, 0.82f + ( 0.34f - 0.82f ) * Turn );

    Canvas->Stroke( Mark, First, Corner, Ink, Line );
    Canvas->Stroke( Mark, Corner, Last, Ink, Line );
}

bool CWidgets::Tree( const char* Title, unsigned long long Image, bool Start ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight * 0.92f ) );

    bool Hovered = false;
    bool Held = false;

    bool Clicked = Behavior( Identifier, Bounds, Hovered, Held );
    bool Open = Opens.count( Identifier ) == 0 ? Start : Opens[ Identifier ] != 0;

    if ( Clicked )
        Open = !Open;

    Opens[ Identifier ] = Open ? 1 : 0;
    Connect( Bounds );

    CMotion& Movement = Motion( Identifier );

    float Speed = Style->FadeSpeed * Context->DeltaTime;
    if ( Speed > 1.0f )
        Speed = 1.0f;

    Movement.Slide += ( ( Open ? 1.0f : 0.0f ) - Movement.Slide ) * Speed;

    if ( Movement.Slide < 0.002f )
        Movement.Slide = 0.0f;

    if ( Movement.Slide > 0.998f )
        Movement.Slide = 1.0f;

    float Glow = Ease( Identifier, Hovered );

    if ( Hovered || Held )
        Canvas->Rectangle( Bounds, Style->Hovered.Fade( 0.35f + 0.4f * Glow ), Style->ControlRounding * 0.6f );

    float Size = Font->LineSpan * 0.46f;
    CRectangle Arrow( Bounds.Left + 5.0f * Style->Scale, Bounds.Top + ( Bounds.Height - Size ) * 0.5f, Size, Size );

    Chevron( Arrow, Movement.Slide, Hovered ? Style->Text : Style->Faint, Style->IconStroke );
    float TextLeft = Arrow.Right( ) + Style->Spacing * 0.7f;

    if ( Image != 0 ) {
        float Icon = Font->LineSpan;
        CRectangle Box( TextLeft, Bounds.Top + ( Bounds.Height - Icon ) * 0.5f, Icon, Icon );

        Canvas->Image( Box, Image, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), CColor( 255, 255, 255 ), Style->ControlRounding * 0.3f );
        TextLeft = Box.Right( ) + Style->Spacing * 0.6f;
    }

    Canvas->Text( CVector( TextLeft, Bounds.Top + ( Bounds.Height - Font->LineSpan ) * 0.5f ), Style->Text, Shown );

    if ( Movement.Slide <= 0.0f )
        return false;

    CTwig Twig;

    Twig.Rail = Bounds.Left + Font->LineSpan * 0.5f;
    Twig.Head = Bounds.Bottom( );

    Twig.Foot = Bounds.Bottom( );
    Twigs.push_back( Twig );

    Layout->BeginReveal( Title, Movement.Slide );
    Layout->Indent( Font->LineSpan );

    return true;
}

void CWidgets::Connect( CRectangle Bounds ) {
    if ( Twigs.empty( ) )
        return;

    CTwig& Twig = Twigs.back( );
    float Middle = Bounds.Top + Bounds.Height * 0.5f;

    float Thick = Style->Thickness;
    if ( Thick < 1.0f )
        Thick = 1.0f;

    Canvas->Rectangle( CRectangle( Twig.Rail, Middle - Thick * 0.5f, Bounds.Left - Twig.Rail, Thick ), Style->Outline, 0.0f );
    Twig.Foot = Middle;
}

bool CWidgets::TreeLeaf( const char* Title, bool Selected ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight * 0.82f ) );

    bool Hovered = false;
    bool Held = false;

    bool Clicked = Behavior( Identifier, Bounds, Hovered, Held );
    Connect( Bounds );

    CMotion& Movement = Motion( Identifier );

    float Speed = Style->FadeSpeed * Context->DeltaTime;
    if ( Speed > 1.0f )
        Speed = 1.0f;

    Movement.Slide += ( ( Selected ? 1.0f : 0.0f ) - Movement.Slide ) * Speed;
    float Glow = Ease( Identifier, Hovered );

    if ( Movement.Slide > 0.01f )
        Canvas->Rectangle( Bounds, Style->Accent.Fade( 0.18f * Movement.Slide ), Style->ControlRounding * 0.6f );
    else if ( Glow > 0.01f )
        Canvas->Rectangle( Bounds, Style->Hovered.Fade( Glow ), Style->ControlRounding * 0.6f );

    CColor Ink = Style->Text.Blend( Style->Accent, Movement.Slide );
    Canvas->Text( CVector( Bounds.Left + Style->PaddingWide * 0.5f, Bounds.Top + ( Bounds.Height - Font->LineSpan ) * 0.5f ), Ink, Shown );

    return Clicked;
}

void CWidgets::TreePop( ) {
    if ( !Twigs.empty( ) ) {
        CTwig Twig = Twigs.back( );
        Twigs.pop_back( );

        float Thick = Style->Thickness;
        if ( Thick < 1.0f )
            Thick = 1.0f;

        if ( Twig.Foot > Twig.Head )
            Canvas->Rectangle( CRectangle( Twig.Rail, Twig.Head, Thick, Twig.Foot - Twig.Head ), Style->Outline, 0.0f );
    }

    Layout->EndReveal( );
}

bool CWidgets::Collapsing( const char* Title, bool Open, unsigned long long Image ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );

    auto Entry = Opens.find( Identifier );
    bool State = Entry == Opens.end( ) ? Open : Entry->second != 0;

    bool Hovered = false;
    bool Held = false;

    bool Clicked = Behavior( Identifier, Bounds, Hovered, Held );
    if ( Clicked )
        State = !State;

    Opens[ Identifier ] = State ? 1 : 0;

    CMotion& Movement = Motion( Identifier );

    float Speed = Style->FadeSpeed * Context->DeltaTime;
    if ( Speed > 1.0f )
        Speed = 1.0f;

    Movement.Slide += ( ( State ? 1.0f : 0.0f ) - Movement.Slide ) * Speed;

    CColor Fill = Style->Control.Blend( Style->Hovered, Ease( Identifier, Hovered ) );
    Canvas->Rectangle( Bounds, Fill, Style->ControlRounding );

    if ( Style->Borders )
        Canvas->Border( Bounds, State ? Style->Outline.Blend( Style->Accent, 0.3f ) : Style->Outline, Style->ControlRounding, Style->Thickness );

    float Size = Font->LineSpan * 0.5f;
    CRectangle Arrow( Bounds.Left + Style->PaddingWide * 0.6f, Bounds.Top + ( Bounds.Height - Size ) * 0.5f, Size, Size );

    Chevron( Arrow, Movement.Slide, Style->Text, Style->IconStroke );
    float TextLeft = Arrow.Right( ) + Style->Spacing;

    if ( Image != 0 ) {
        float Icon = Font->LineSpan;
        CRectangle Box( TextLeft, Bounds.Top + ( Bounds.Height - Icon ) * 0.5f, Icon, Icon );

        Canvas->Image( Box, Image, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), CColor( 255, 255, 255 ), Style->ControlRounding * 0.3f );
        TextLeft = Box.Right( ) + Style->Spacing * 0.7f;
    }

    Canvas->Text( CVector( TextLeft, Bounds.Top + ( Bounds.Height - Font->LineSpan ) * 0.5f ), Style->Text, Shown );

    return State;
}

bool CWidgets::BeginCollapse( const char* Title, bool Open ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );

    auto Entry = Opens.find( Identifier );
    bool State = Entry == Opens.end( ) ? Open : Entry->second != 0;

    bool Hovered = false;
    bool Held = false;

    bool Clicked = Behavior( Identifier, Bounds, Hovered, Held );
    if ( Clicked )
        State = !State;

    Opens[ Identifier ] = State ? 1 : 0;
    CMotion& Movement = Motion( Identifier );

    float Speed = Style->FadeSpeed * Context->DeltaTime;
    if ( Speed > 1.0f )
        Speed = 1.0f;

    Movement.Slide += ( ( State ? 1.0f : 0.0f ) - Movement.Slide ) * Speed;

    if ( Movement.Slide < 0.002f )
        Movement.Slide = 0.0f;

    if ( Movement.Slide > 0.998f )
        Movement.Slide = 1.0f;

    CColor Fill = Style->Control.Blend( Style->Hovered, Ease( Identifier, Hovered ) );
    Canvas->Rectangle( Bounds, Fill, Style->ControlRounding );

    if ( Style->Borders )
        Canvas->Border( Bounds, State ? Style->Outline.Blend( Style->Accent, 0.3f ) : Style->Outline, Style->ControlRounding, Style->Thickness );

    float Size = Font->LineSpan * 0.5f;
    CRectangle Arrow( Bounds.Left + Style->PaddingWide * 0.6f, Bounds.Top + ( Bounds.Height - Size ) * 0.5f, Size, Size );

    Chevron( Arrow, Movement.Slide, Style->Text, Style->IconStroke );
    Canvas->Text( CVector( Arrow.Right( ) + Style->Spacing, Bounds.Top + ( Bounds.Height - Font->LineSpan ) * 0.5f ), Style->Text, Shown );

    if ( Movement.Slide <= 0.0f )
        return false;

    return Layout->BeginReveal( Title, Movement.Slide );
}

void CWidgets::EndCollapse( ) {
    Layout->EndReveal( );
}

bool CWidgets::Selectable( const char* Title, bool Selected, float Height ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    float Tall = Height > 0.0f ? Height : Style->ControlHeight * 0.85f;
    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Tall ) );

    bool Hovered = false;
    bool Held = false;

    bool Clicked = Behavior( Identifier, Bounds, Hovered, Held );
    CMotion& Movement = Motion( Identifier );

    float Speed = Style->FadeSpeed * Context->DeltaTime;
    if ( Speed > 1.0f )
        Speed = 1.0f;

    Movement.Slide += ( ( Selected ? 1.0f : 0.0f ) - Movement.Slide ) * Speed;
    float Glow = Ease( Identifier, Hovered );

    if ( Movement.Slide > 0.01f )
        Canvas->Rectangle( Bounds, Style->Accent.Fade( 0.18f * Movement.Slide ), Style->ControlRounding * 0.6f );
    else if ( Glow > 0.01f )
        Canvas->Rectangle( Bounds, Style->Hovered.Fade( Glow ), Style->ControlRounding * 0.6f );

    CColor Ink = Style->Text.Blend( Style->Accent, Movement.Slide );
    Canvas->Text( CVector( Bounds.Left + Style->PaddingWide * 0.5f, Bounds.Top + ( Tall - Font->LineSpan ) * 0.5f ), Ink, Shown );

    return Clicked;
}

bool CWidgets::List( const char* Title, int& Selected, const char* const* Names, int Count, int Rows ) {
    if ( !Names || Count <= 0 )
        return false;

    float ItemHeight = Style->ControlHeight * 0.85f;
    float Tall = ItemHeight * ( float )Rows + Style->PaddingTall * 0.5f;

    bool Changed = false;

    if ( Layout->BeginChild( Title, CVector( Layout->Width( ), Tall ), true ) ) {
        Context->PushIdentifier( Title );

        for ( int Position = 0; Position < Count; Position++ ) {
            Context->PushIdentifier( Position );

            if ( Selectable( Names[ Position ], Selected == Position, ItemHeight ) ) {
                Selected = Position;
                Changed = true;
            }

            Context->PopIdentifier( );
        }

        Context->PopIdentifier( );
    }

    Layout->EndChild( );
    return Changed;
}

static void ToHueSat( CColor Value, float& Hue, float& Saturation, float& Bright ) {
    float Red = Value.Red / 255.0f;
    float Green = Value.Green / 255.0f;

    float Blue = Value.Blue / 255.0f;

    float Top = Red > Green ? ( Red > Blue ? Red : Blue ) : ( Green > Blue ? Green : Blue );
    float Low = Red < Green ? ( Red < Blue ? Red : Blue ) : ( Green < Blue ? Green : Blue );

    Bright = Top;
    float Spread = Top - Low;

    Saturation = Top > 0.0f ? Spread / Top : 0.0f;

    if ( Spread <= 0.0f ) {
        Hue = 0.0f;
        return;
    }

    if ( Top == Red )
        Hue = fmodf( ( Green - Blue ) / Spread + 6.0f, 6.0f );
    else if ( Top == Green )
        Hue = ( Blue - Red ) / Spread + 2.0f;
    else
        Hue = ( Red - Green ) / Spread + 4.0f;

    Hue /= 6.0f;
}

static CColor FromHueSat( float Hue, float Saturation, float Bright, int Alpha ) {
    float Sixth = floorf( Hue * 6.0f );

    float Fraction = Hue * 6.0f - Sixth;
    int Segment = ( ( int )Sixth ) % 6;

    if ( Segment < 0 )
        Segment += 6;

    float Dim = Bright * ( 1.0f - Saturation );
    float Fall = Bright * ( 1.0f - Fraction * Saturation );

    float Rise = Bright * ( 1.0f - ( 1.0f - Fraction ) * Saturation );

    float Red = Bright;
    float Green = Bright;

    float Blue = Bright;

    if ( Segment == 0 ) {
        Green = Rise;
        Blue = Dim;
    } else if ( Segment == 1 ) {
        Red = Fall;
        Blue = Dim;
    } else if ( Segment == 2 ) {
        Red = Dim;
        Blue = Rise;
    } else if ( Segment == 3 ) {
        Red = Dim;
        Green = Fall;
    } else if ( Segment == 4 ) {
        Red = Rise;
        Green = Dim;
    } else {
        Green = Dim;
        Blue = Fall;
    }

    return CColor( ( int )( Red * 255.0f + 0.5f ), ( int )( Green * 255.0f + 0.5f ), ( int )( Blue * 255.0f + 0.5f ), Alpha );
}

bool CWidgets::Color( const char* Title, CColor& Value, bool Alpha ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    CRectangle Row = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );
    CRectangle Swatch( Row.Left, Row.Top, Style->ControlHeight * 1.9f, Row.Height );

    bool Hovered = false;
    bool Held = false;

    bool Clicked = Behavior( Identifier, Swatch, Hovered, Held );

    if ( Clicked )
        Reveal = Reveal == Identifier ? 0 : Identifier;

    Canvas->Rectangle( Swatch, Value.Fade( 1.0f ), Style->ControlRounding );
    if ( Style->Borders )
        Canvas->Border( Swatch, Reveal == Identifier ? Style->Accent : Style->Outline, Style->ControlRounding, Style->Thickness );

    if ( Shown[ 0 ] )
        Canvas->Text( CVector( Swatch.Right( ) + Style->Spacing, Row.Top + ( Row.Height - Font->LineSpan ) * 0.5f ), Style->Text, Shown );

    bool Showing = Reveal == Identifier;
    CMotion& Movement = Motion( Identifier );

    float Speed = Style->FadeSpeed * 1.4f * Context->DeltaTime;
    if ( Speed > 1.0f )
        Speed = 1.0f;

    Movement.Slide += ( ( Showing ? 1.0f : 0.0f ) - Movement.Slide ) * Speed;

    if ( !Showing && Movement.Slide < 0.01f )
        return false;

    float Grown = Movement.Slide;
    float Wide = 232.0f * Style->Scale;

    float Tall = 200.0f * Style->Scale;
    CRectangle Zone( Swatch.Left, Swatch.Bottom( ) + 6.0f * Style->Scale, Wide, Tall );

    if ( Context->Display.Vertical > 0.0f && Zone.Bottom( ) > Context->Display.Vertical )
        Zone.Top = Swatch.Top - Zone.Height - 6.0f * Style->Scale;

    if ( Context->Display.Horizontal > 0.0f && Zone.Right( ) > Context->Display.Horizontal )
        Zone.Left = Context->Display.Horizontal - Zone.Width - 4.0f;

    if ( Showing || Grown > 0.15f ) {
        Context->Shielding = true;
        Context->Shield = Zone;

        Context->ShieldStamp = Context->FrameIndex;
    }

    float Hue = 0.0f;
    float Saturation = 0.0f;

    float Bright = 0.0f;
    ToHueSat( Value, Hue, Saturation, Bright );

    int Former = Canvas->Route( OverlayRoute );
    float FormerOpacity = Canvas->Opacity;

    Canvas->Opacity = Grown;
    Canvas->PushClip( Zone.Inflate( Style->Softness + 8.0f ), true );

    if ( Style->Shadows )
        Canvas->Shadow( Zone, Style->Shade, Style->ControlRounding + 2.0f, Style->Softness * 0.7f );

    Canvas->Rectangle( Zone, Style->Popup, Style->ControlRounding + 2.0f );
    if ( Style->Borders )
        Canvas->Border( Zone, Style->Outline, Style->ControlRounding + 2.0f, Style->Thickness );

    float Lip = 12.0f * Style->Scale;
    float BarWide = 16.0f * Style->Scale;

    CRectangle Square( Zone.Left + Lip, Zone.Top + Lip, Zone.Width - Lip * 3.0f - BarWide * ( Alpha ? 2.0f : 1.0f ), Zone.Height - Lip * 2.0f );
    CRectangle HueBar( Square.Right( ) + Lip * 0.5f, Square.Top, BarWide, Square.Height );

    CColor Pure = FromHueSat( Hue, 1.0f, 1.0f, 255 );

    Canvas->Gradient( Square, CColor( 255, 255, 255 ), Pure, Style->ControlRounding * 0.5f, true );
    Canvas->Gradient( Square, CColor( 0, 0, 0, 0 ), CColor( 0, 0, 0, 255 ), Style->ControlRounding * 0.5f, false );

    for ( int Step = 0; Step < 6; Step++ ) {
        CColor Above = FromHueSat( ( float )Step / 6.0f, 1.0f, 1.0f, 255 );
        CColor Below = FromHueSat( ( float )( Step + 1 ) / 6.0f, 1.0f, 1.0f, 255 );

        CRectangle Slice( HueBar.Left, HueBar.Top + HueBar.Height * ( float )Step / 6.0f, HueBar.Width, HueBar.Height / 6.0f + 1.0f );
        Canvas->Gradient( Slice, Above, Below, 0.0f, false );
    }

    CVector Point = Input->MousePosition;
    bool Live = Showing && Grown > 0.5f;

    bool Changed = false;

    if ( Live && Input->MouseDown( 0 ) && Square.Inflate( 4.0f ).Contains( Point ) ) {
        Saturation = ( Point.Horizontal - Square.Left ) / Square.Width;
        Bright = 1.0f - ( Point.Vertical - Square.Top ) / Square.Height;

        if ( Saturation < 0.0f )
            Saturation = 0.0f;

        if ( Saturation > 1.0f )
            Saturation = 1.0f;

        if ( Bright < 0.0f )
            Bright = 0.0f;

        if ( Bright > 1.0f )
            Bright = 1.0f;

        Value = FromHueSat( Hue, Saturation, Bright, Value.Alpha );
        Changed = true;
    }

    if ( Live && Input->MouseDown( 0 ) && HueBar.Inflate( 4.0f ).Contains( Point ) && !Square.Contains( Point ) ) {
        Hue = ( Point.Vertical - HueBar.Top ) / HueBar.Height;

        if ( Hue < 0.0f )
            Hue = 0.0f;

        if ( Hue > 0.999f )
            Hue = 0.999f;

        Value = FromHueSat( Hue, Saturation, Bright, Value.Alpha );
        Changed = true;
    }

    if ( Alpha ) {
        CRectangle AlphaBar( HueBar.Right( ) + Lip * 0.5f, Square.Top, BarWide, Square.Height );

        Canvas->Gradient( AlphaBar, Value.Fade( 1.0f ), Value.Fade( 0.0f ), 0.0f, false );
        if ( Style->Borders )
            Canvas->Border( AlphaBar, Style->Outline, 0.0f, Style->Thickness );

        if ( Live && Input->MouseDown( 0 ) && AlphaBar.Inflate( 4.0f ).Contains( Point ) ) {
            float Amount = 1.0f - ( Point.Vertical - AlphaBar.Top ) / AlphaBar.Height;
            if ( Amount < 0.0f )
                Amount = 0.0f;

            if ( Amount > 1.0f )
                Amount = 1.0f;

            Value.Alpha = ( unsigned char )( Amount * 255.0f + 0.5f );
            Changed = true;
        }

        float Marker = AlphaBar.Top + ( 1.0f - Value.Alpha / 255.0f ) * AlphaBar.Height;
        Canvas->Rectangle( CRectangle( AlphaBar.Left - 2.0f, Marker - 1.5f, AlphaBar.Width + 4.0f, 3.0f ), CColor( 255, 255, 255 ), 1.0f );
    }

    if ( Style->Borders )
        Canvas->Border( Square, Style->Outline, Style->ControlRounding * 0.5f, Style->Thickness );

    CVector Dot( Square.Left + Saturation * Square.Width, Square.Top + ( 1.0f - Bright ) * Square.Height );
    Canvas->Border( CRectangle( Dot.Horizontal - 5.0f, Dot.Vertical - 5.0f, 10.0f, 10.0f ), CColor( 255, 255, 255 ), 5.0f, 2.0f );

    float HueMark = HueBar.Top + Hue * HueBar.Height;
    Canvas->Rectangle( CRectangle( HueBar.Left - 2.0f, HueMark - 1.5f, HueBar.Width + 4.0f, 3.0f ), CColor( 255, 255, 255 ), 1.0f );

    Canvas->PopClip( );
    Canvas->Opacity = FormerOpacity;

    Canvas->Route( Former );

    if ( Showing && Input->MousePressed( 0 ) && !Zone.Contains( Point ) && !Swatch.Contains( Point ) )
        Reveal = 0;

    return Changed;
}

bool CWidgets::BeginTabs( const char* Identifier ) {
    unsigned int Key = Context->Hash( Identifier );
    CRectangle Bar = Layout->Place( CVector( Layout->Width( ), Style->TabHeight ) );

    CPanel Panel;

    Panel.Identifier = Key;
    Panel.Zone = Bar;

    Panel.Cursor = CVector( Bar.Left, Bar.Top );
    Panel.Bar = true;

    Panels.push_back( Panel );

    Canvas->Rectangle( CRectangle( Bar.Left, Bar.Bottom( ) - Style->Thickness, Bar.Width, Style->Thickness ), Style->Outline.Fade( 0.6f ), 0.0f );
    return true;
}

bool CWidgets::Tab( const char* Title, bool* Open ) {
    if ( Panels.empty( ) )
        return false;

    CPanel& Panel = Panels.back( );

    if ( Open && !*Open )
        return false;

    unsigned int Identifier = Panel.Identifier ^ Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    CVector Naming = Font->Measure( Shown );

    float CloseRoom = Open ? Style->PaddingWide * 1.1f : 0.0f;
    float TextLeft = Panel.Cursor.Horizontal;

    CRectangle Tab( TextLeft, Panel.Zone.Top, Naming.Horizontal + CloseRoom, Style->TabHeight );
    Panel.Cursor.Horizontal = Tab.Right( ) + Style->PaddingWide * 1.3f;

    unsigned int& Current = Picks[ Panel.Identifier ];
    if ( Current == 0 )
        Current = Identifier;

    float HitLeft = Tab.Left - Style->PaddingWide * 0.5f;
    if ( HitLeft < Panel.Zone.Left )
        HitLeft = Panel.Zone.Left;

    CRectangle Hit( HitLeft, Tab.Top, Tab.Right( ) + Style->PaddingWide * 0.5f - HitLeft, Tab.Height );

    bool Hovered = false;
    bool Held = false;

    bool Clicked = Behavior( Identifier, Hit, Hovered, Held );

    if ( Clicked )
        Current = Identifier;

    bool Active = Current == Identifier;
    if ( Hovered && !Active )
        Canvas->Rectangle( Hit.Pad( 0.0f, 5.0f * Style->Scale ), Style->Hovered, Style->ControlRounding * 0.6f );

    CColor Ink = Active ? Style->Text : Style->Faint;
    Canvas->Text( CVector( TextLeft, Tab.Top + ( Tab.Height - Font->LineSpan ) * 0.5f ), Ink, Shown );

    if ( Open ) {
        float Slot = 14.0f * Style->Scale;
        CRectangle Shut( Tab.Right( ) - Slot, Tab.Top + ( Tab.Height - Slot ) * 0.5f, Slot, Slot );

        bool ShutHovered = false;
        bool ShutHeld = false;

        if ( Behavior( Identifier ^ 0x5C5C5C5Cu, Shut, ShutHovered, ShutHeld ) )
            *Open = false;

        CColor Cross = ShutHovered ? Style->Text : Style->Faint;
        CRectangle Mark = Shut.Shrink( Slot * 0.3f );

        Canvas->Stroke( Mark, CVector( 0.05f, 0.05f ), CVector( 0.95f, 0.95f ), Cross, Style->IconStroke );
        Canvas->Stroke( Mark, CVector( 0.95f, 0.05f ), CVector( 0.05f, 0.95f ), Cross, Style->IconStroke );
    }

    if ( Active ) {
        CMotion& Movement = Motion( Panel.Identifier );

        float Speed = Style->FadeSpeed * Context->DeltaTime;
        if ( Speed > 1.0f )
            Speed = 1.0f;

        float Relative = TextLeft - Panel.Zone.Left;

        if ( Movement.Glow <= 0.0f )
            Movement.Glow = Relative;

        if ( Movement.Slide <= 0.0f )
            Movement.Slide = Naming.Horizontal;

        Movement.Glow += ( Relative - Movement.Glow ) * Speed;
        Movement.Slide += ( Naming.Horizontal - Movement.Slide ) * Speed;

        CRectangle Line( Panel.Zone.Left + Movement.Glow, Panel.Zone.Bottom( ) - Style->Underline, Movement.Slide, Style->Underline );
        Canvas->Rectangle( Line, Style->Accent, Style->Underline * 0.5f );
    }

    return Active;
}

void CWidgets::EndTabs( ) {
    if ( !Panels.empty( ) )
        Panels.pop_back( );
}

static void PlotFrame( CRectangle& Box, const float* Values, int Count, float& Minimum, float& Maximum ) {
    if ( Minimum >= Maximum ) {
        Minimum = Values[ 0 ];
        Maximum = Values[ 0 ];

        for ( int Position = 1; Position < Count; Position++ ) {
            if ( Values[ Position ] < Minimum )
                Minimum = Values[ Position ];

            if ( Values[ Position ] > Maximum )
                Maximum = Values[ Position ];
        }
    }

    if ( Maximum <= Minimum )
        Maximum = Minimum + 1.0f;
}

CRectangle CWidgets::Chart( const char* Title, float Height ) {
    bool Titled = Title && Title[ 0 ];
    float Lead = Titled ? Font->LineSpan + Style->Spacing * 0.4f : 0.0f;

    CRectangle Whole = Layout->Place( CVector( Layout->Width( ), Lead + Height ) );

    if ( Titled )
        Canvas->Text( Whole.Origin( ), Style->Faint, Context->Caption( Title ) );

    CRectangle Box( Whole.Left, Whole.Top + Lead, Whole.Width, Height );

    Canvas->Rectangle( Box, Style->Groove.Fade( 0.3f ), Style->ControlRounding );
    if ( Style->Borders )
        Canvas->Border( Box, Style->Outline, Style->ControlRounding, Style->Thickness );

    return Box;
}

void CWidgets::Plot( const char* Title, const float* Values, int Count, float Minimum, float Maximum ) {
    if ( !Values || Count <= 1 )
        return;

    CRectangle Box = Chart( Title, Style->ControlHeight * 2.4f );
    PlotFrame( Box, Values, Count, Minimum, Maximum );

    CRectangle Inner = Box.Pad( Style->PaddingWide * 0.4f, Style->PaddingTall * 0.4f );
    Canvas->PushClip( Inner );

    float Step = Inner.Width / ( float )( Count - 1 );

    for ( int Position = 0; Position + 1 < Count; Position++ ) {
        float FirstAmount = ( Values[ Position ] - Minimum ) / ( Maximum - Minimum );
        float NextAmount = ( Values[ Position + 1 ] - Minimum ) / ( Maximum - Minimum );

        CVector From( Inner.Left + Step * ( float )Position, Inner.Bottom( ) - FirstAmount * Inner.Height );
        CVector Till( Inner.Left + Step * ( float )( Position + 1 ), Inner.Bottom( ) - NextAmount * Inner.Height );

        Canvas->Line( From, Till, Style->Accent, Style->Thickness * 2.0f );
    }

    Canvas->PopClip( );
}

void CWidgets::Area( const char* Title, const float* Values, int Count, float Minimum, float Maximum ) {
    if ( !Values || Count <= 1 )
        return;

    CRectangle Box = Chart( Title, Style->ControlHeight * 2.4f );
    PlotFrame( Box, Values, Count, Minimum, Maximum );

    CRectangle Inner = Box.Pad( Style->PaddingWide * 0.4f, Style->PaddingTall * 0.4f );
    Canvas->PushClip( Inner );

    float Step = Inner.Width / ( float )( Count - 1 );

    for ( int Column = 0; ( float )Column < Inner.Width; Column += 2 ) {
        float Along = ( float )Column / Step;
        int Index = ( int )Along;

        if ( Index >= Count - 1 )
            Index = Count - 2;

        float Fraction = Along - ( float )Index;
        float Value = Values[ Index ] + ( Values[ Index + 1 ] - Values[ Index ] ) * Fraction;

        float Amount = ( Value - Minimum ) / ( Maximum - Minimum );
        float Tall = Amount * Inner.Height;

        Canvas->Rectangle( CRectangle( Inner.Left + ( float )Column, Inner.Bottom( ) - Tall, 2.5f, Tall ), Style->Accent.Fade( 0.22f ), 0.0f );
    }

    for ( int Position = 0; Position + 1 < Count; Position++ ) {
        float FirstAmount = ( Values[ Position ] - Minimum ) / ( Maximum - Minimum );
        float NextAmount = ( Values[ Position + 1 ] - Minimum ) / ( Maximum - Minimum );

        CVector From( Inner.Left + Step * ( float )Position, Inner.Bottom( ) - FirstAmount * Inner.Height );
        CVector Till( Inner.Left + Step * ( float )( Position + 1 ), Inner.Bottom( ) - NextAmount * Inner.Height );

        Canvas->Line( From, Till, Style->Accent, Style->Thickness * 2.0f );
    }

    Canvas->PopClip( );
}

void CWidgets::Histogram( const char* Title, const float* Values, int Count, float Minimum, float Maximum ) {
    if ( !Values || Count <= 0 )
        return;

    CRectangle Box = Chart( Title, Style->ControlHeight * 2.4f );
    PlotFrame( Box, Values, Count, Minimum, Maximum );

    CRectangle Inner = Box.Pad( Style->PaddingWide * 0.4f, Style->PaddingTall * 0.4f );
    Canvas->PushClip( Inner );

    float Slot = Inner.Width / ( float )Count;

    for ( int Position = 0; Position < Count; Position++ ) {
        float Amount = ( Values[ Position ] - Minimum ) / ( Maximum - Minimum );
        float Tall = Amount * Inner.Height;

        CRectangle Bar( Inner.Left + Slot * ( float )Position + 1.5f, Inner.Bottom( ) - Tall, Slot - 3.0f, Tall );
        Canvas->Rectangle( Bar, Style->Accent.Fade( 0.85f ), Style->ControlRounding * 0.4f );
    }

    Canvas->PopClip( );
}

void CWidgets::Pie( const char* Title, const float* Values, int Count, bool Donut ) {
    if ( !Values || Count <= 0 )
        return;

    bool Titled = Title && Title[ 0 ];
    float Lead = Titled ? Font->LineSpan + Style->Spacing * 0.4f : 0.0f;

    CRectangle Whole = Layout->Place( CVector( Layout->Width( ), Lead + 180.0f * Style->Scale ) );

    if ( Titled )
        Canvas->Text( Whole.Origin( ), Style->Faint, Context->Caption( Title ) );

    CRectangle Box( Whole.Left, Whole.Top + Lead, Whole.Width, 180.0f * Style->Scale );
    float Total = 0.0f;

    for ( int Position = 0; Position < Count; Position++ ) {
        if ( Values[ Position ] > 0.0f )
            Total += Values[ Position ];
    }

    if ( Total <= 0.0f )
        return;

    CVector Middle = Box.Center( );
    float Radius = ( Box.Height < Box.Width ? Box.Height : Box.Width ) * 0.5f - 6.0f * Style->Scale;

    float Start = -1.5707963f;

    for ( int Position = 0; Position < Count; Position++ ) {
        if ( Values[ Position ] <= 0.0f )
            continue;

        float Sweep = Values[ Position ] / Total * 6.2831853f;
        CColor Slice = FromHueSat( ( float )Position / ( float )Count, 0.5f, 0.95f, 255 );

        Canvas->Sector( Middle, Radius, Start, Sweep + 0.004f, Donut ? 0.52f : 0.0f, Slice );
        Start += Sweep;
    }
}

void CWidgets::PanelOpen( CRectangle Zone ) {
    CPanel Panel;

    Panel.Zone = Zone;
    Panel.Cursor = CVector( Zone.Left + 6.0f * Style->Scale, Zone.Top + 6.0f * Style->Scale );

    Panel.Former = Canvas->Route( OverlayRoute );
    Panel.Opacity = Canvas->Opacity;

    Panels.push_back( Panel );

    Context->Shielding = true;
    Context->Shield = Zone;

    Context->ShieldStamp = Context->FrameIndex;

    Canvas->PushClip( Zone.Inflate( Style->Softness + 8.0f ), true );

    if ( Style->Shadows )
        Canvas->Shadow( Zone, Style->Shade, Style->ControlRounding, Style->Softness * 0.6f );

    Canvas->Rectangle( Zone, Style->Popup, Style->ControlRounding );
    if ( Style->Borders )
        Canvas->Border( Zone, Style->Outline, Style->ControlRounding, Style->Thickness );

    Canvas->PushClip( Zone );

    if ( Zone.Contains( Input->MousePosition ) )
        MenuTouched = true;
}

void CWidgets::PanelClose( ) {
    if ( Panels.empty( ) )
        return;

    CPanel Panel = Panels.back( );
    Panels.pop_back( );

    Extents[ Panel.Identifier ] = CVector( Panel.Widest, Panel.Cursor.Vertical - Panel.Zone.Top + 6.0f * Style->Scale );

    Canvas->PopClip( );
    Canvas->PopClip( );

    Canvas->Opacity = Panel.Opacity;
    Canvas->Route( Panel.Former );
}

bool CWidgets::BeginMenuBar( ) {
    CRectangle Bar = Layout->Stretch( Style->TabHeight );

    Canvas->Rectangle( Bar, Style->Header, 0.0f );
    Canvas->Rectangle( CRectangle( Bar.Left, Bar.Bottom( ) - Style->Thickness, Bar.Width, Style->Thickness ), Style->Outline.Fade( 0.6f ), 0.0f );

    CPanel Panel;

    Panel.Zone = Bar;
    Panel.Cursor = CVector( Bar.Left + Style->PaddingWide * 0.3f, Bar.Top );

    Panel.Bar = true;
    Panels.push_back( Panel );

    return true;
}

void CWidgets::EndMenuBar( ) {
    if ( !Panels.empty( ) )
        Panels.pop_back( );

    if ( !MenuChain.empty( ) && Input->MousePressed( 0 ) && !MenuTouched )
        MenuChain.clear( );
}

bool CWidgets::BeginMenu( const char* Title ) {
    if ( Panels.empty( ) )
        return false;

    CPanel& Host = Panels.back( );

    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );

    CVector Point = Input->MousePosition;
    CVector Naming = Font->Measure( Shown );

    int Depth = 0;

    for ( const CPanel& Layer : Panels ) {
        if ( !Layer.Bar )
            Depth++;
    }

    bool Open = ( int )MenuChain.size( ) > Depth && MenuChain[ Depth ] == Identifier;
    CRectangle Zone;

    if ( Host.Bar ) {
        CRectangle Row( Host.Cursor.Horizontal, Host.Zone.Top, Naming.Horizontal + Style->PaddingWide, Host.Zone.Height );
        Host.Cursor.Horizontal = Row.Right( ) + 2.0f * Style->Scale;

        bool Hovered = Row.Contains( Point ) && Frames->Beneath == Frames->Current && !Context->Covered( Point );
        if ( Hovered )
            MenuTouched = true;

        if ( Hovered && Input->MousePressed( 0 ) ) {
            if ( Open ) {
                MenuChain.clear( );
            } else {
                MenuChain.clear( );
                MenuChain.push_back( Identifier );
            }
        } else if ( !MenuChain.empty( ) && Hovered && MenuChain[ 0 ] != Identifier ) {
            MenuChain.clear( );
            MenuChain.push_back( Identifier );
        }

        Open = !MenuChain.empty( ) && MenuChain[ 0 ] == Identifier;

        if ( Open || Hovered )
            Canvas->Rectangle( Row.Pad( 2.0f * Style->Scale, 5.0f * Style->Scale ), Open ? Style->Hovered : Style->Hovered.Fade( 0.6f ), Style->ControlRounding * 0.5f );

        Canvas->Text( CVector( Row.Left + Style->PaddingWide * 0.5f, Row.Top + ( Row.Height - Font->LineSpan ) * 0.5f ), Style->Text, Shown );

        if ( !Open )
            return false;

        auto Found = Extents.find( Identifier );
        CVector Size = Found != Extents.end( ) ? Found->second : CVector( 190.0f * Style->Scale, Style->ControlHeight );

        Zone = CRectangle( Row.Left, Row.Bottom( ) + 2.0f * Style->Scale, Size.Horizontal, Size.Vertical );
    } else {
        float Margin = Host.Cursor.Horizontal - Host.Zone.Left;
        float Tall = Style->ControlHeight * 0.88f;

        CRectangle Row( Host.Cursor.Horizontal, Host.Cursor.Vertical, Host.Zone.Width - Margin * 2.0f, Tall );
        Host.Cursor.Vertical += Tall;

        float Pad = Style->PaddingWide * 0.55f;

        float Need = Margin * 2.0f + Pad * 2.0f + Font->LineSpan * 2.0f + Naming.Horizontal;
        if ( Need > Host.Widest )
            Host.Widest = Need;

        bool Hovered = Row.Contains( Point );
        if ( Hovered )
            MenuTouched = true;

        if ( Hovered ) {
            MenuChain.resize( ( size_t )Depth );
            MenuChain.push_back( Identifier );

            Open = true;
        }

        if ( Hovered || Open )
            Canvas->Rectangle( Row, Style->Hovered, Style->ControlRounding * 0.6f );

        Canvas->Text( CVector( Row.Left + Pad + Font->LineSpan, Row.Top + ( Tall - Font->LineSpan ) * 0.5f ), Style->Text, Shown );

        float Size = Font->LineSpan * 0.45f;
        Chevron( CRectangle( Row.Right( ) - Pad - Size, Row.Center( ).Vertical - Size * 0.5f, Size, Size ), 0.0f, Style->Faint, Style->IconStroke );

        if ( !Open )
            return false;

        auto Found = Extents.find( Identifier );
        CVector Reach = Found != Extents.end( ) ? Found->second : CVector( 190.0f * Style->Scale, Style->ControlHeight );

        Zone = CRectangle( Host.Zone.Right( ) - 3.0f * Style->Scale, Row.Top - Style->PaddingTall * 0.35f, Reach.Horizontal, Reach.Vertical );
    }

    if ( Context->Display.Vertical > 0.0f && Zone.Bottom( ) > Context->Display.Vertical )
        Zone.Top = Context->Display.Vertical - Zone.Height - 4.0f;

    if ( Context->Display.Horizontal > 0.0f && Zone.Right( ) > Context->Display.Horizontal )
        Zone.Left = Context->Display.Horizontal - Zone.Width - 4.0f;

    PanelOpen( Zone );
    Panels.back( ).Identifier = Identifier;

    return true;
}

void CWidgets::EndMenu( ) {
    PanelClose( );
}

bool CWidgets::MenuItem( const char* Title, const char* Shortcut, bool Selected ) {
    if ( Panels.empty( ) )
        return false;

    CPanel& Host = Panels.back( );
    const char* Shown = Context->Caption( Title );

    float Margin = Host.Cursor.Horizontal - Host.Zone.Left;
    float Tall = Style->ControlHeight * 0.88f;

    CRectangle Row( Host.Cursor.Horizontal, Host.Cursor.Vertical, Host.Zone.Width - Margin * 2.0f, Tall );
    Host.Cursor.Vertical += Tall;

    CVector Point = Input->MousePosition;

    float Pad = Style->PaddingWide * 0.55f;
    float Gutter = Font->LineSpan;

    CVector Naming = Font->Measure( Shown );
    float Extra = Shortcut ? Font->Measure( Shortcut ).Horizontal + Style->PaddingWide : 0.0f;

    float Need = Margin * 2.0f + Pad * 2.0f + Gutter + Naming.Horizontal + Extra;
    if ( Need > Host.Widest )
        Host.Widest = Need;

    bool Hovered = Row.Contains( Point );
    if ( Hovered )
        MenuTouched = true;

    if ( Hovered )
        Canvas->Rectangle( Row, Style->Accent.Fade( 0.22f ), Style->ControlRounding * 0.6f );

    Canvas->Text( CVector( Row.Left + Pad + Gutter, Row.Top + ( Tall - Font->LineSpan ) * 0.5f ), Style->Text, Shown );

    if ( Selected ) {
        CRectangle Tick( Row.Left + Pad, Row.Center( ).Vertical - Gutter * 0.3f, Gutter * 0.6f, Gutter * 0.6f );

        Canvas->Stroke( Tick, CVector( 0.08f, 0.52f ), CVector( 0.4f, 0.84f ), Style->Accent, Style->IconStroke );
        Canvas->Stroke( Tick, CVector( 0.4f, 0.84f ), CVector( 0.92f, 0.16f ), Style->Accent, Style->IconStroke );
    }

    if ( Shortcut ) {
        CVector Span = Font->Measure( Shortcut );
        Canvas->Text( CVector( Row.Right( ) - Pad, Row.Top + ( Tall - Font->LineSpan ) * 0.5f ) - CVector( Span.Horizontal, 0.0f ), Style->Faint, Shortcut );
    }

    bool Clicked = Hovered && Input->MousePressed( 0 );

    if ( Clicked ) {
        MenuChain.clear( );
        Popup = 0;
    }

    return Clicked;
}

void CWidgets::OpenPopup( const char* Identifier ) {
    Popup = Context->Hash( Identifier );
    PopupAt = Input->MousePosition;
}

bool CWidgets::BeginPopup( const char* Identifier ) {
    unsigned int Key = Context->Hash( Identifier );

    if ( Popup != Key )
        return false;

    auto Found = Extents.find( Key );
    CVector Size = Found != Extents.end( ) ? Found->second : CVector( 190.0f * Style->Scale, Style->ControlHeight );

    CRectangle Zone( PopupAt.Horizontal, PopupAt.Vertical, Size.Horizontal, Size.Vertical );

    if ( Context->Display.Vertical > 0.0f && Zone.Bottom( ) > Context->Display.Vertical )
        Zone.Top = Context->Display.Vertical - Zone.Height - 4.0f;

    if ( Context->Display.Horizontal > 0.0f && Zone.Right( ) > Context->Display.Horizontal )
        Zone.Left = Context->Display.Horizontal - Zone.Width - 4.0f;

    if ( Input->MousePressed( 0 ) && !Zone.Contains( Input->MousePosition ) ) {
        Popup = 0;
        return false;
    }

    PanelOpen( Zone );
    Panels.back( ).Identifier = Key;

    return true;
}

void CWidgets::EndPopup( ) {
    PanelClose( );
}

bool CWidgets::Decimal( const char* Title, float& Value, float Step, const char* Format ) {
    ( void )Format;

    CRectangle Box = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );
    double Temp = ( double )Value;

    bool Changed = Spinner( Title, Box, Temp, ( double )( Step > 0.0f ? Step : 0.1f ), false );

    if ( Changed )
        Value = ( float )Temp;

    return Changed;
}

bool CWidgets::Field( const char* Title, std::string& Value, const char* Hint ) {
    char Scratch[ 1024 ];
    size_t Count = Value.size( ) < 1023 ? Value.size( ) : 1023;
    memcpy( Scratch, Value.c_str( ), Count );
    Scratch[ Count ] = 0;

    bool Changed = Field( Title, Scratch, 1024, Hint );
    if ( Changed )
        Value = Scratch;

    return Changed;
}

bool CWidgets::Area( const char* Title, std::string& Value, float Height ) {
    char Scratch[ 4096 ];
    size_t Count = Value.size( ) < 4095 ? Value.size( ) : 4095;
    memcpy( Scratch, Value.c_str( ), Count );
    Scratch[ Count ] = 0;

    bool Changed = Area( Title, Scratch, 4096, Height );
    if ( Changed )
        Value = Scratch;

    return Changed;
}

static const char* KeyLabel( int Key ) {
    switch ( Key ) {
    case 0: return "None";
    case KeyEscape: return "Esc";
    case KeyTab: return "Tab";
    case KeyEnter: return "Enter";
    case KeySpace: return "Space";
    case KeyBackspace: return "Backspace";
    case KeyShift: return "Shift";
    case KeyControl: return "Ctrl";
    case KeyAlt: return "Alt";
    case KeyDelete: return "Delete";
    case KeyHome: return "Home";
    case KeyEnd: return "End";
    case KeyLeft: return "Left";
    case KeyRight: return "Right";
    case KeyUp: return "Up";
    case KeyDown: return "Down";
    case KeyF1: return "F1";
    case KeyF2: return "F2";
    case KeyF3: return "F3";
    case KeyF4: return "F4";
    case KeyF5: return "F5";
    case KeyF6: return "F6";
    case KeyF7: return "F7";
    case KeyF8: return "F8";
    case KeyF9: return "F9";
    case KeyF10: return "F10";
    case KeyF11: return "F11";
    case KeyF12: return "F12";
    default:
        break;
    }

    if ( Key >= 65 && Key <= 90 )
        return Format->Print( "%c", Key );

    if ( Key >= 48 && Key <= 57 )
        return Format->Print( "%c", Key );

    return Format->Print( "Key %d", Key );
}

bool CWidgets::Knob( const char* Title, float& Current, float Minimum, float Maximum ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );
    LatestName = Shown;

    if ( Maximum <= Minimum )
        Maximum = Minimum + 1.0f;

    float Size = Style->ControlHeight * 2.4f;
    CRectangle Row = Layout->Place( CVector( Layout->Width( ), Size + Font->LineSpan + Style->Spacing * 0.4f ) );
    CRectangle Dial( Row.Left, Row.Top, Size, Size );
    CVector Mid = Dial.Center( );

    bool Hovered = false;
    bool Held = false;
    Behavior( Identifier, Dial, Hovered, Held );

    bool Changed = false;
    CMotion& Movement = Motion( Identifier );
    if ( Held ) {
        if ( Input->MousePressed( 0 ) )
            Movement.Slide = Current;
        float Range = Maximum - Minimum;
        float Next = Movement.Slide - ( Input->MousePosition.Vertical - Input->PressPosition.Vertical ) * Range * 0.005f;
        if ( Next < Minimum )
            Next = Minimum;
        if ( Next > Maximum )
            Next = Maximum;
        if ( Next != Current ) {
            Current = Next;
            Changed = true;
        }
    }
    if ( Hovered && fabsf( Input->WheelDelta ) > 0.0f ) {
        Current += Input->WheelDelta * ( Maximum - Minimum ) * 0.04f;
        if ( Current < Minimum )
            Current = Minimum;
        if ( Current > Maximum )
            Current = Maximum;
        Input->WheelDelta = 0.0f;
        Changed = true;
    }

    float Unit = ( Current - Minimum ) / ( Maximum - Minimum );
    if ( Unit < 0.0f )
        Unit = 0.0f;
    if ( Unit > 1.0f )
        Unit = 1.0f;

    float Glow = Ease( Identifier, Hovered || Held );
    Canvas->Circle( Mid, Size * 0.48f, Style->Control.Blend( Style->Hovered, Glow ) );
    Canvas->Sector( Mid, Size * 0.48f, -2.0943951f, 4.1887902f * Unit, Size * 0.36f, Style->Accent );

    float Needle = -2.3561945f + 4.7123890f * Unit;
    CVector Tip( Mid.Horizontal + cosf( Needle ) * Size * 0.34f, Mid.Vertical + sinf( Needle ) * Size * 0.34f );
    Canvas->Line( Mid, Tip, Style->Text, 2.0f * Style->Scale );

    Canvas->Text( CVector( Dial.Right( ) + Style->Spacing, Row.Top + ( Size - Font->LineSpan ) * 0.35f ), Style->Text, Shown );
    Canvas->Text( CVector( Dial.Right( ) + Style->Spacing, Row.Top + ( Size - Font->LineSpan ) * 0.35f + Font->LineSpan ), Style->Faint, Format->Print( "%.2f", ( double )Current ) );

    return Changed;
}

bool CWidgets::Keybind( const char* Title, int& Key ) {
    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );
    LatestName = Shown;

    CRectangle Row = Layout->Place( CVector( Layout->Width( ), Style->ControlHeight ) );
    float Label = Shown[ 0 ] ? Font->Measure( Shown ).Horizontal + Style->Spacing : 0.0f;
    CRectangle Box( Row.Left + Label, Row.Top, Row.Width - Label, Row.Height );

    bool Hovered = false;
    bool Held = false;
    bool Clicked = Behavior( Identifier, Box, Hovered, Held );

    static unsigned int Listening = 0;
    if ( Clicked )
        Listening = Identifier;

    if ( Listening == Identifier && !Disabled( ) ) {
        for ( int Scan = 8; Scan < 256; Scan++ ) {
            if ( !Input->KeyPressed( Scan ) )
                continue;

            Listening = 0;
            if ( Scan == KeyEscape ) {
                Key = 0;
                return true;
            }

            Key = Scan;
            return true;
        }
    }

    bool Live = Listening == Identifier;
    CColor Fill = Live ? Style->Selected : Style->Control.Blend( Style->Hovered, Ease( Identifier, Hovered ) );
    Canvas->Rectangle( Box, Fill, Style->ControlRounding );
    if ( Style->Borders )
        Canvas->Border( Box, Live ? Style->Accent : Style->Outline, Style->ControlRounding, Style->Thickness );

    if ( Shown[ 0 ] )
        Canvas->Text( Row.Origin( ), Style->Faint, Shown );

    const char* LabelKey = Live ? "Press a key" : KeyLabel( Key );
    CVector Size = Font->Measure( LabelKey );
    Canvas->Text( CVector( Box.Left + ( Box.Width - Size.Horizontal ) * 0.5f, Box.Top + ( Box.Height - Font->LineSpan ) * 0.5f ), Style->Text, LabelKey );
    return false;
}

bool CWidgets::Splitter( float& Ratio ) {
    if ( Ratio < 0.08f )
        Ratio = 0.08f;
    if ( Ratio > 0.92f )
        Ratio = 0.92f;

    unsigned int Identifier = Context->Hash( "##splitter" );
    CRectangle Bar = Layout->Place( CVector( Layout->Width( ), Style->SplitterSize ) );

    bool Hovered = false;
    bool Held = false;
    Behavior( Identifier, Bar, Hovered, Held );

    bool Changed = false;
    if ( Held && Bar.Width > 1.0f ) {
        float Next = ( Input->MousePosition.Horizontal - Bar.Left ) / Bar.Width;
        if ( Next < 0.08f )
            Next = 0.08f;
        if ( Next > 0.92f )
            Next = 0.92f;
        if ( Next != Ratio ) {
            Ratio = Next;
            Changed = true;
        }
        Input->Pointer = PointerAcross;
    } else if ( Hovered ) {
        Input->Pointer = PointerAcross;
    }

    CColor Fill = Style->Outline.Blend( Style->Accent, Ease( Identifier, Hovered || Held ) );
    Canvas->Rectangle( Bar, Fill, Bar.Height * 0.5f );
    return Changed;
}

struct CVirtual {
    int Count = 0;
    float Row = 0.0f;
    int Last = 0;
};

static std::vector< CVirtual > Virtuals;

bool CWidgets::BeginVirtual( const char* Title, int Count, float RowHeight, int& First, int& Last, float Height ) {
    if ( RowHeight < 1.0f )
        RowHeight = Style->ControlHeight;

    float View = Height > 0.0f ? Height : Layout->Remaining( );
    if ( View < RowHeight * 4.0f )
        View = RowHeight * 8.0f;

    First = 0;
    Last = 0;

    if ( !Layout->BeginChild( Title, CVector( Layout->Width( ), View ), true ) )
        return false;

    float Scroll = Layout->ChildScroll( );
    First = ( int )( Scroll / RowHeight );
    if ( First < 0 )
        First = 0;
    if ( First > Count )
        First = Count;

    int Visible = ( int )( Layout->ChildView( ) / RowHeight ) + 2;
    Last = First + Visible;
    if ( Last > Count )
        Last = Count;

    if ( First > 0 )
        Layout->Dummy( CVector( Layout->Width( ), ( float )First * RowHeight ) );

    CVirtual Frame;
    Frame.Count = Count;
    Frame.Row = RowHeight;
    Frame.Last = Last;
    Virtuals.push_back( Frame );

    Context->PushIdentifier( Title );
    return true;
}

void CWidgets::EndVirtual( ) {
    if ( !Virtuals.empty( ) ) {
        CVirtual Frame = Virtuals.back( );
        Virtuals.pop_back( );
        int Tail = Frame.Count - Frame.Last;
        if ( Tail > 0 )
            Layout->Dummy( CVector( Layout->Width( ), ( float )Tail * Frame.Row ) );
    }

    Context->PopIdentifier( );
    Layout->EndChild( );
}

bool CWidgets::FilterList( const char* Title, int& Selected, const char* const* Names, int Count, char* Query, int QueryCapacity, int Rows ) {
    if ( !Names || Count <= 0 || !Query || QueryCapacity <= 0 )
        return false;

    Field( Title, Query, QueryCapacity, "filter" );

    std::vector< int > Hits;
    Hits.reserve( ( size_t )Count );
    for ( int Index = 0; Index < Count; Index++ ) {
        if ( Query[ 0 ] == 0 || ( Names[ Index ] && strstr( Names[ Index ], Query ) ) )
            Hits.push_back( Index );
    }

    bool Changed = false;
    int First = 0;
    int Last = 0;
    float RowH = Style->ControlHeight * 0.85f;

    if ( BeginVirtual( Format->Print( "%s##filter", Title ), ( int )Hits.size( ), RowH, First, Last, RowH * ( float )Rows ) ) {
        for ( int Slot = First; Slot < Last; Slot++ ) {
            int Index = Hits[ ( size_t )Slot ];
            Context->PushIdentifier( Index );
            if ( Selectable( Names[ Index ], Selected == Index, RowH ) ) {
                Selected = Index;
                Changed = true;
            }
            Context->PopIdentifier( );
        }
        EndVirtual( );
    }

    return Changed;
}

void CWidgets::Meter( float Fraction ) {
    if ( Fraction < 0.0f )
        Fraction = 0.0f;
    if ( Fraction > 1.0f )
        Fraction = 1.0f;

    CRectangle Box = Layout->Place( CVector( Layout->Width( ), 10.0f * Style->Scale ) );
    Canvas->Rectangle( Box, Style->Groove, Box.Height * 0.5f );

    CColor Fill = Style->Accent;
    if ( Fraction > 0.85f )
        Fill = Style->Danger;
    else if ( Fraction > 0.65f )
        Fill = Style->Warning;

    if ( Fraction > 0.0f )
        Canvas->Rectangle( CRectangle( Box.Left, Box.Top, Box.Width * Fraction, Box.Height ), Fill, Box.Height * 0.5f );
}

void CWidgets::Waveform( const float* Values, int Count ) {
    CRectangle Box = Layout->Place( CVector( Layout->Width( ), 48.0f * Style->Scale ) );
    Canvas->Rectangle( Box, Style->Backdrop, 8.0f * Style->Scale );
    if ( !Values || Count < 2 )
        return;

    CVector Prev;
    bool Have = false;
    float Mid = Box.Top + Box.Height * 0.5f;
    float Amp = Box.Height * 0.42f;
    for ( int Index = 0; Index < Count; Index++ ) {
        float Across = Box.Left + 4.0f * Style->Scale + ( Box.Width - 8.0f * Style->Scale ) * ( ( float )Index / ( float )( Count - 1 ) );
        CVector Now( Across, Mid - Values[ Index ] * Amp );
        if ( Have )
            Canvas->Line( Prev, Now, Style->Accent, 1.4f * Style->Scale );
        Prev = Now;
        Have = true;
    }
}

void CWidgets::Spectrum( const float* Values, int Count ) {
    CRectangle Box = Layout->Place( CVector( Layout->Width( ), 84.0f * Style->Scale ) );
    Canvas->Rectangle( Box, Style->Backdrop, 8.0f * Style->Scale );
    if ( !Values || Count <= 0 )
        return;

    float Gap = 1.4f * Style->Scale;
    float Bar = ( Box.Width - 10.0f * Style->Scale - Gap * ( float )( Count - 1 ) ) / ( float )Count;
    float Floor = 3.0f * Style->Scale;
    for ( int Index = 0; Index < Count; Index++ ) {
        float Rest = Values[ Index ];
        if ( Rest < 0.0f )
            Rest = 0.0f;
        if ( Rest > 1.0f )
            Rest = 1.0f;
        float High = Floor + ( Box.Height - 10.0f * Style->Scale - Floor ) * Rest;
        float Left = Box.Left + 5.0f * Style->Scale + ( float )Index * ( Bar + Gap );
        float Top = Box.Bottom( ) - 5.0f * Style->Scale - High;
        float Mix = Count > 1 ? ( float )Index / ( float )( Count - 1 ) : 0.0f;
        Canvas->Rectangle( CRectangle( Left, Top, Bar, High ), Style->Accent.Blend( Style->AccentSoft, Mix ), Bar * 0.35f );
    }
}

bool CWidgets::BeginModal( const char* Title, bool* Open, CVector Extent ) {
    if ( Open && !*Open )
        return false;

    CRectangle Screen( 0.0f, 0.0f, Context->Display.Horizontal, Context->Display.Vertical );
    int Former = Canvas->Route( OverlayRoute );
    Canvas->Rectangle( Screen, CColor( 0, 0, 0, 148 ), 0.0f );
    Canvas->Route( Former );

    CVector Size( Extent.Horizontal * Style->Scale, Extent.Vertical * Style->Scale );
    CVector Anchor( ( Context->Display.Horizontal - Size.Horizontal ) * 0.5f, ( Context->Display.Vertical - Size.Vertical ) * 0.5f );

    bool Opened = Frames->Begin( Title, Open, FrameClose, Anchor, Size );
    if ( !Opened )
        return false;

    Frames->Raise( Frames->Current );
    CFrameState* State = Frames->Find( Frames->Current );
    if ( State && Input->MousePressed( 0 ) && !State->Bounds.Contains( Input->MousePosition ) ) {
        if ( Open )
            *Open = false;
    }

    return Open && *Open ? true : Opened;
}

void CWidgets::EndModal( ) {
    Frames->End( );
}

bool CWidgets::Stars( const char* Title, int& Current, int Maximum ) {
    if ( Maximum < 1 )
        Maximum = 1;
    if ( Maximum > 10 )
        Maximum = 10;
    if ( Current < 0 )
        Current = 0;
    if ( Current > Maximum )
        Current = Maximum;

    unsigned int Identifier = Context->Hash( Title );
    const char* Shown = Context->Caption( Title );
    ( void )Identifier;

    float Mark = 20.0f * Style->Scale;
    float Gap = 5.0f * Style->Scale;
    float Row = Mark + 6.0f * Style->Scale;
    if ( Row < Style->ControlHeight )
        Row = Style->ControlHeight;

    CRectangle Bounds = Layout->Place( CVector( Layout->Width( ), Row ) );
    CVector Titled = Font->Measure( Shown );
    Canvas->Text( CVector( Bounds.Left, Bounds.Top + ( Row - Font->LineSpan ) * 0.5f ), Style->Text, Shown );

    float Left = Bounds.Left + Titled.Horizontal + Style->Spacing;
    int Before = Current;
    int Preview = Current;

    for ( int Step = 1; Step <= Maximum; Step++ ) {
        CRectangle Cell( Left + ( float )( Step - 1 ) * ( Mark + Gap ), Bounds.Top + ( Row - Mark ) * 0.5f, Mark, Mark );
        Context->PushIdentifier( Step );
        unsigned int Star = Context->Hash( "star" );
        bool Hovered = false;
        bool Held = false;
        Behavior( Star, Cell, Hovered, Held );
        Context->PopIdentifier( );

        if ( Hovered )
            Preview = Step;
        if ( Held && Hovered )
            Current = Step;
    }

    if ( Current < 0 )
        Current = 0;
    if ( Current > Maximum )
        Current = Maximum;

    for ( int Step = 1; Step <= Maximum; Step++ ) {
        CRectangle Cell( Left + ( float )( Step - 1 ) * ( Mark + Gap ), Bounds.Top + ( Row - Mark ) * 0.5f, Mark, Mark );
        CVector Mid = Cell.Center( );
        float Radius = Mark * 0.46f;
        CVector Points[ 10 ];
        for ( int Arm = 0; Arm < 5; Arm++ ) {
            float Outer = -1.5707963f + ( float )Arm * 1.2566371f;
            float Inner = Outer + 0.62831853f;
            Points[ Arm * 2 ] = CVector( Mid.Horizontal + cosf( Outer ) * Radius, Mid.Vertical + sinf( Outer ) * Radius );
            Points[ Arm * 2 + 1 ] = CVector( Mid.Horizontal + cosf( Inner ) * Radius * 0.40f, Mid.Vertical + sinf( Inner ) * Radius * 0.40f );
        }

        bool Lit = Step <= Preview;
        CColor Fill = Lit ? Style->Accent : Style->Groove;
        if ( Preview > Current && Step > Current && Step <= Preview )
            Fill = Style->Accent.Fade( 0.45f );
        Canvas->Polygon( Points, 10, Fill );
        if ( Style->Borders )
            Canvas->PolygonBorder( Points, 10, Lit ? Style->AccentSoft : Style->Outline, 1.0f * Style->Scale );
    }

    char* Reading = Format->Print( "%d / %d", Current, Maximum );
    CVector Extent = Font->Measure( Reading );
    Canvas->Text( CVector( Left + ( float )Maximum * ( Mark + Gap ) + 4.0f * Style->Scale, Bounds.Top + ( Row - Font->LineSpan ) * 0.5f ), Style->Faint, Reading );

    return Current != Before;
}