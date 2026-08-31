#include "ur/player.hpp"
#include "ur/glyphs.hpp"
#include "ur/hear.hpp"

#include "Canvas.h"
#include "Context.h"
#include "Font.h"
#include "Format.h"
#include "Input.h"
#include "Layout.h"
#include "Style.h"
#include "Widgets.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace ur {
namespace player {

static CGraphics* Gfx = nullptr;
static unsigned long long Art = 0;
static unsigned long long LastArt = 0;
static std::string LastId;

static void SyncArt( const media::Track& Track ) {
    if ( !Gfx )
        return;
    if ( Track.id == LastId )
        return;
    LastId = Track.id;
    if ( LastArt && LastArt != Art )
        Gfx->DestroyImage( LastArt );
    LastArt = Art;
    Art = 0;
    if ( !Track.art.empty( ) && Track.art_width > 0 && Track.art_height > 0 )
        Art = Gfx->CreateImage( Track.art.data( ), Track.art_width, Track.art_height );
}

static bool Hit( const char* Id, CRectangle Bounds ) {
    unsigned int Identifier = Context->Hash( Id );
    CVector Point = Input->MousePosition;
    bool Hovered = Bounds.Contains( Point ) && Canvas->Visible( Point );
    if ( Hovered && Input->MousePressed( 0 ) && Context->ActiveItem == 0 )
        Context->ActiveItem = Identifier;
    bool Held = Context->ActiveItem == Identifier;
    bool Clicked = Held && Hovered && Input->MouseReleased( 0 );
    if ( Held && Input->MouseReleased( 0 ) )
        Context->ActiveItem = 0;
    return Clicked;
}

static void Mark( CRectangle Bounds, int Kind, bool Playing, CColor Ink ) {
    CVector Mid = Bounds.Center( );
    float S = Bounds.Width * 0.18f;
    if ( Kind == 0 || Kind == 2 ) {
        float Dir = Kind == 0 ? -1.0f : 1.0f;
        CVector Tip( Mid.Horizontal + Dir * S * 1.1f, Mid.Vertical );
        CVector Top( Mid.Horizontal - Dir * S * 0.8f, Mid.Vertical - S * 1.1f );
        CVector Bot( Mid.Horizontal - Dir * S * 0.8f, Mid.Vertical + S * 1.1f );
        CVector Tri[ 3 ] = { Tip, Top, Bot };
        Canvas->Polygon( Tri, 3, Ink );
        float BarW = S * 0.36f;
        float BarH = S * 2.2f;
        float BarX = Kind == 0 ? Mid.Horizontal - S * 1.5f : Mid.Horizontal + S * 1.14f;
        Canvas->Rectangle( CRectangle( BarX, Mid.Vertical - BarH * 0.5f, BarW, BarH ), Ink, 0.8f );
    } else if ( Playing ) {
        float BarW = S * 0.4f;
        float BarH = S * 2.1f;
        float Gap = S * 0.5f;
        Canvas->Rectangle( CRectangle( Mid.Horizontal - Gap - BarW, Mid.Vertical - BarH * 0.5f, BarW, BarH ), Ink, 1.0f );
        Canvas->Rectangle( CRectangle( Mid.Horizontal + Gap, Mid.Vertical - BarH * 0.5f, BarW, BarH ), Ink, 1.0f );
    } else {
        CVector Tip( Mid.Horizontal + S * 1.2f, Mid.Vertical );
        CVector Top( Mid.Horizontal - S, Mid.Vertical - S * 1.2f );
        CVector Bot( Mid.Horizontal - S, Mid.Vertical + S * 1.2f );
        CVector Play[ 3 ] = { Tip, Top, Bot };
        Canvas->Polygon( Play, 3, Ink );
    }
}

static bool Control( const char* Id, CRectangle Bounds, int Kind, bool Playing ) {
    bool Clicked = Hit( Id, Bounds );
    bool Hovered = Bounds.Contains( Input->MousePosition );
    float Round = 8.0f * Style->Scale;
    CColor Fill = ( Kind == 1 && Playing ) ? Style->Accent : Style->Control;
    if ( Hovered )
        Fill = Fill.Blend( Style->Hovered, 0.55f );
    Canvas->Rectangle( Bounds, Fill, Round );
    CColor Ink = ( Kind == 1 && Playing ) ? CColor( 255, 255, 255 ) : Style->Text;
    Mark( Bounds, Kind, Playing, Ink );
    return Clicked;
}

static bool SeekBar( float& Position, float Duration, CRectangle Slot ) {
    float Line = 3.0f * Style->Scale;
    CRectangle Groove( Slot.Left, Slot.Top + ( Slot.Height - Line ) * 0.5f, Slot.Width, Line );

    unsigned int Identifier = Context->Hash( "##seek" );
    CVector Point = Input->MousePosition;
    bool Hovered = Slot.Contains( Point ) && Canvas->Visible( Point );
    if ( Hovered && Input->MousePressed( 0 ) && Context->ActiveItem == 0 )
        Context->ActiveItem = Identifier;
    bool Held = Context->ActiveItem == Identifier;
    if ( Held && Input->MouseReleased( 0 ) )
        Context->ActiveItem = 0;

    bool Changed = false;
    if ( Held && Duration > 0.1f && Groove.Width > 1.0f ) {
        float Ratio = ( Point.Horizontal - Groove.Left ) / Groove.Width;
        if ( Ratio < 0.0f )
            Ratio = 0.0f;
        if ( Ratio > 1.0f )
            Ratio = 1.0f;
        Position = Duration * Ratio;
        Changed = true;
    }

    float Portion = Duration > 0.1f ? Position / Duration : 0.0f;
    if ( Portion < 0.0f )
        Portion = 0.0f;
    if ( Portion > 1.0f )
        Portion = 1.0f;

    Canvas->Rectangle( Groove, Style->Groove, Line * 0.5f );
    if ( Portion > 0.0f )
        Canvas->Rectangle( CRectangle( Groove.Left, Groove.Top, Groove.Width * Portion, Line ), Style->Accent, Line * 0.5f );

    float Knob = ( 4.5f + ( Held ? 1.2f : ( Hovered ? 0.6f : 0.0f ) ) ) * Style->Scale;
    Canvas->Rectangle( CRectangle( Groove.Left + Groove.Width * Portion - Knob, Groove.Top + Line * 0.5f - Knob, Knob * 2.0f, Knob * 2.0f ), Style->Knob, Knob );
    return Changed;
}

void bind( CGraphics* Graphics ) {
    Gfx = Graphics;
    Art = 0;
    LastArt = 0;
    LastId.clear( );
}

void draw_expanded( const Options& Options ) {
    const media::Track& Track = media::current( );
    SyncArt( Track );

    float Wide = Layout->Width( );
    float WellH = Wide * 9.0f / 16.0f;
    CRectangle Slot = Layout->Place( CVector( Wide, WellH ) );
    float Round = 6.0f * Style->Scale;
    Canvas->Rectangle( Slot, Style->Elevated, Round );

    CRectangle Frame = Slot;
    if ( Track.art_width > 0 && Track.art_height > 0 ) {
        float Aspect = ( float )Track.art_width / ( float )Track.art_height;
        if ( Aspect < 0.2f )
            Aspect = 0.2f;
        if ( Aspect > 2.8f )
            Aspect = 2.8f;
        float WellAspect = Wide / WellH;
        float DrawW = Wide;
        float DrawH = WellH;
        if ( Aspect > WellAspect ) {
            DrawH = WellH;
            DrawW = WellH * Aspect;
        } else {
            DrawW = Wide;
            DrawH = Wide / Aspect;
        }
        Frame = CRectangle( Slot.Left + ( Wide - DrawW ) * 0.5f, Slot.Top + ( WellH - DrawH ) * 0.5f, DrawW, DrawH );
    }

    if ( Options.art && Art ) {
        Canvas->PushClip( Slot );
        Canvas->Image( Frame, Art, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), CColor( 255, 255, 255 ), Round );
        Canvas->PopClip( );
    } else {
        const char* Empty = Options.empty ? Options.empty : "Nothing playing";
        CVector Quiet = Font->Measure( Empty );
        Canvas->Text( CVector( Slot.Left + ( Wide - Quiet.Horizontal ) * 0.5f, Slot.Top + ( WellH - Font->LineSpan ) * 0.5f ), Style->Faint, Empty );
    }

    float Play = 28.0f * Style->Scale;
    float Side = 24.0f * Style->Scale;
    float Gap = 6.0f * Style->Scale;
    float Total = Side * 2.0f + Play + Gap * 2.0f;
    float Meta = Heading->LineSpan + Font->LineSpan + 4.0f * Style->Scale;
    float SeekH = 14.0f * Style->Scale;
    float BlockH = Meta + SeekH + Play + 4.0f * Style->Scale;
    CRectangle Block = Layout->Place( CVector( Wide, BlockH ) );

    const char* Title = Track.title.empty( ) ? "Not playing" : Track.title.c_str( );
    const char* Artist = Track.artist.empty( ) ? ( Track.app.empty( ) ? " " : Track.app.c_str( ) ) : Track.artist.c_str( );
    CRectangle TitleBox( Block.Left, Block.Top, Wide, Heading->LineSpan );
    Canvas->PushClip( TitleBox );
    Canvas->Write( Heading.get( ), TitleBox.Origin( ), Style->Text, Title );
    Canvas->PopClip( );
    CRectangle ArtistBox( Block.Left, TitleBox.Bottom( ) + 1.0f * Style->Scale, Wide, Font->LineSpan );
    Canvas->PushClip( ArtistBox );
    Canvas->Text( ArtistBox.Origin( ), Style->Faint, Artist );
    Canvas->PopClip( );

    float Duration = Track.duration > 0.1f ? ( float )Track.duration : 1.0f;
    float Position = ( float )Track.position;
    if ( Position < 0.0f )
        Position = 0.0f;
    if ( Position > Duration )
        Position = Duration;

    CRectangle Seek( Block.Left, ArtistBox.Bottom( ) + 4.0f * Style->Scale, Wide, SeekH );
    if ( Options.seek && SeekBar( Position, Duration, Seek ) )
        media::seek( Position );

    CRectangle Row( Block.Left, Seek.Bottom( ) + 2.0f * Style->Scale, Wide, Play );

    int NowM = ( int )Position / 60;
    int NowS = ( int )Position % 60;
    int EndM = ( int )Duration / 60;
    int EndS = ( int )Duration % 60;
    const char* Now = Format->Print( "%d:%02d", NowM, NowS );
    Canvas->Text( CVector( Row.Left, Row.Top + ( Play - Font->LineSpan ) * 0.5f ), Style->Faint, Now );
    const char* End = Format->Print( "%d:%02d", EndM, EndS );
    CVector EndSize = Font->Measure( End );
    Canvas->Text( CVector( Row.Right( ) - EndSize.Horizontal, Row.Top + ( Play - Font->LineSpan ) * 0.5f ), Style->Faint, End );

    float Left = Row.Left + ( Wide - Total ) * 0.5f;
    CRectangle Prev( Left, Row.Top + ( Play - Side ) * 0.5f, Side, Side );
    CRectangle Toggle( Left + Side + Gap, Row.Top, Play, Play );
    CRectangle Next( Left + Side + Gap + Play + Gap, Row.Top + ( Play - Side ) * 0.5f, Side, Side );

    if ( Options.transport ) {
        if ( Control( "##prev", Prev, 0, false ) )
            media::prev( );
        if ( Control( "##play", Toggle, 1, Track.playing ) )
            media::toggle( );
        if ( Control( "##next", Next, 2, false ) )
            media::next( );
    }
}

void draw_compact( const Options& Options ) {
    const media::Track& Track = media::current( );
    SyncArt( Track );

    float High = 40.0f * Style->Scale;
    float Wide = Layout->Width( );
    CRectangle Row = Layout->Place( CVector( Wide, High ) );
    float Round = 6.0f * Style->Scale;

    if ( Options.art && Art )
        Canvas->Image( CRectangle( Row.Left, Row.Top, High, High ), Art, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), CColor( 255, 255, 255 ), Round );

    float TextLeft = Row.Left + ( Options.art ? High + 8.0f * Style->Scale : 0.0f );
    Canvas->PushClip( CRectangle( TextLeft, Row.Top, Row.Right( ) - TextLeft, High ) );
    Canvas->Text( CVector( TextLeft, Row.Top + 3.0f * Style->Scale ), Style->Text, Track.title.empty( ) ? Options.empty : Track.title.c_str( ) );
    Canvas->Text( CVector( TextLeft, Row.Top + 20.0f * Style->Scale ), Style->Faint, Track.artist.empty( ) ? Track.app.c_str( ) : Track.artist.c_str( ) );
    Canvas->PopClip( );
}

void draw_chip( const Options& Options ) {
    const media::Track& Track = media::current( );
    SyncArt( Track );

    float High = 58.0f * Style->Scale;
    float Wide = Layout->Width( );
    CRectangle Row = Layout->Place( CVector( Wide, High ) );
    float Round = 8.0f * Style->Scale;
    float ArtS = High - 8.0f * Style->Scale;
    float Pad = 4.0f * Style->Scale;

    Canvas->Rectangle( Row, Style->Elevated.Blend( hear::tint( ), hear::level( ) * 0.28f ), Round );
    if ( Options.transport && Hit( "##chip", Row ) )
        media::toggle( );

    CRectangle Cover( Row.Left + Pad, Row.Top + Pad, ArtS, ArtS );
    if ( Options.art && Art )
        Canvas->Image( Cover, Art, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), CColor( 255, 255, 255 ), 6.0f * Style->Scale );
    else
        Canvas->Rectangle( Cover, Style->Control, 6.0f * Style->Scale );

    float TextLeft = Cover.Right( ) + 8.0f * Style->Scale;
    float TextWide = Row.Right( ) - TextLeft - 8.0f * Style->Scale;
    Canvas->PushClip( CRectangle( TextLeft, Row.Top + 4.0f * Style->Scale, TextWide, 36.0f * Style->Scale ) );
    Canvas->Text( CVector( TextLeft, Row.Top + 6.0f * Style->Scale ), Style->Text, Track.title.empty( ) ? Options.empty : Track.title.c_str( ) );
    const char* Who = Track.artist.empty( ) ? ( Track.app.empty( ) ? "Spotify · YouTube · any player" : Track.app.c_str( ) ) : Track.artist.c_str( );
    Canvas->Text( CVector( TextLeft, Row.Top + 22.0f * Style->Scale ), Style->Faint, Who );
    Canvas->PopClip( );

    float Line = 3.0f * Style->Scale;
    CRectangle Groove( TextLeft, Row.Bottom( ) - 10.0f * Style->Scale, TextWide, Line );
    float Portion = Track.duration > 0.1 ? ( float )( Track.position / Track.duration ) : 0.0f;
    if ( Portion < 0.0f )
        Portion = 0.0f;
    if ( Portion > 1.0f )
        Portion = 1.0f;
    Canvas->Rectangle( Groove, Style->Groove, Line * 0.5f );
    if ( Portion > 0.0f )
        Canvas->Rectangle( CRectangle( Groove.Left, Groove.Top, Groove.Width * Portion, Line ), Style->Accent, Line * 0.5f );

    if ( Track.playing ) {
        float Dot = 5.0f * Style->Scale;
        Canvas->Rectangle( CRectangle( Row.Right( ) - 12.0f * Style->Scale, Row.Top + 8.0f * Style->Scale, Dot, Dot ), Style->Accent, Dot * 0.5f );
    }
}

}
}
