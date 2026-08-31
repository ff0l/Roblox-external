#include "ur/desk.hpp"
#include "ur/hear.hpp"

#include "Canvas.h"
#include "Font.h"
#include "Layout.h"
#include "Style.h"
#include "Widgets.h"

#include <Windows.h>
#include <cmath>

namespace ur {
namespace desk {

static CFont TimeFace;
static CFont DateFace;
static float TimeScale = -1.0f;

static void EnsureFaces( ) {
    if ( fabsf( TimeScale - Style->Scale ) < 0.001f && TimeFace.LineSpan > 1.0f )
        return;

    TimeFace.Destroy( );
    DateFace.Destroy( );

    const char* Families[ 5 ] = { "Segoe UI Variable Display", "Segoe UI", "Segoe UI Symbol", "Segoe UI Emoji", "Arial" };
    TimeFace.Create( Families, 5, 38.0f * Style->Scale, 500 );
    DateFace.Create( Families, 5, 13.0f * Style->Scale, 500 );
    TimeScale = Style->Scale;
}

static void Hand( CVector Mid, float Angle, float Length, float Thick, CColor Ink ) {
    CVector Tip( Mid.Horizontal + cosf( Angle ) * Length, Mid.Vertical + sinf( Angle ) * Length );
    Canvas->Line( Mid, Tip, Ink, Thick );
}

static void Analog( CRectangle Box, const SYSTEMTIME& Local ) {
    CVector Mid = Box.Center( );
    float Radius = ( Box.Width < Box.Height ? Box.Width : Box.Height ) * 0.5f - 2.0f * Style->Scale;
    if ( Radius < 12.0f * Style->Scale )
        return;

    float Second = ( float )Local.wSecond + ( float )Local.wMilliseconds * 0.001f;
    float Minute = ( float )Local.wMinute + Second / 60.0f;
    float Hour = ( float )( Local.wHour % 12 ) + Minute / 60.0f;

    Canvas->Circle( Mid, Radius, Style->Control.Blend( Style->Elevated, 0.55f ) );
    Canvas->Circle( Mid, Radius * 0.92f, Style->Surface );
    Canvas->Sector( Mid, Radius * 0.92f, -1.5707963f, 6.2831853f * ( Second / 60.0f ), Radius * 0.84f, Style->Accent );

    for ( int Tick = 0; Tick < 12; Tick++ ) {
        float Angle = ( float )Tick * 0.5235988f - 1.5707963f;
        float Inner = Tick % 3 == 0 ? Radius * 0.70f : Radius * 0.76f;
        float Outer = Radius * 0.82f;
        CVector From( Mid.Horizontal + cosf( Angle ) * Inner, Mid.Vertical + sinf( Angle ) * Inner );
        CVector Till( Mid.Horizontal + cosf( Angle ) * Outer, Mid.Vertical + sinf( Angle ) * Outer );
        Canvas->Line( From, Till, Tick % 3 == 0 ? Style->Text : Style->Faint, Tick % 3 == 0 ? 2.0f * Style->Scale : 1.0f * Style->Scale );
    }

    float HourA = Hour * 0.5235988f - 1.5707963f;
    float MinuteA = Minute * 0.1047198f - 1.5707963f;
    float SecondA = Second * 0.1047198f - 1.5707963f;

    Hand( Mid, HourA, Radius * 0.48f, 2.6f * Style->Scale, Style->Text );
    Hand( Mid, MinuteA, Radius * 0.68f, 1.8f * Style->Scale, Style->Text.Fade( 0.92f ) );
    Hand( Mid, SecondA, Radius * 0.74f, 1.1f * Style->Scale, Style->Accent );
    Canvas->Circle( Mid, 3.2f * Style->Scale, Style->Accent );
}

void clock( ) {
    EnsureFaces( );

    SYSTEMTIME Local = { };
    GetLocalTime( &Local );

    float Wide = Layout->Width( );
    float Face = Wide * 0.40f;
    if ( Face > 108.0f * Style->Scale )
        Face = 108.0f * Style->Scale;
    if ( Face < 72.0f * Style->Scale )
        Face = 72.0f * Style->Scale;
    float High = Face + 6.0f * Style->Scale;
    CRectangle Area = Layout->Place( CVector( Wide, High ) );

    bool Beside = Wide >= Face + 96.0f * Style->Scale;
    CRectangle Dial;
    if ( Beside )
        Dial = CRectangle( Area.Left + 4.0f * Style->Scale, Area.Top + ( Area.Height - Face ) * 0.5f, Face, Face );
    else {
        Face = ( Wide < High ? Wide : High ) - 10.0f * Style->Scale;
        Dial = CRectangle( Area.Left + ( Wide - Face ) * 0.5f, Area.Top + 2.0f * Style->Scale, Face, Face );
    }

    Analog( Dial, Local );

    char Stamp[ 16 ];
    wsprintfA( Stamp, "%02u:%02u", Local.wHour, Local.wMinute );
    char Secs[ 8 ];
    wsprintfA( Secs, "%02u", Local.wSecond );

    const char* Days[ 7 ] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    const char* Months[ 12 ] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
    char Date[ 64 ];
    wsprintfA( Date, "%s  %u %s", Days[ Local.wDayOfWeek ], Local.wDay, Months[ Local.wMonth - 1 ] );

    CFont* FaceFont = TimeFace.LineSpan > 1.0f ? &TimeFace : Heading.get( );
    CFont* Small = DateFace.LineSpan > 1.0f ? &DateFace : Font.get( );

    if ( Beside ) {
        float TextLeft = Dial.Right( ) + 14.0f * Style->Scale;
        float TextWide = Area.Right( ) - TextLeft - 6.0f * Style->Scale;
        CVector TimeSize = FaceFont->Measure( Stamp );
        CVector SecSize = Font->Measure( Secs );
        CVector DateSize = Small->Measure( Date );
        float Block = TimeSize.Vertical + Font->LineSpan + DateSize.Vertical + 8.0f * Style->Scale;
        float Top = Area.Top + ( Area.Height - Block ) * 0.5f;

        Canvas->PushClip( CRectangle( TextLeft, Area.Top, TextWide, Area.Height ) );
        Canvas->Write( FaceFont, CVector( TextLeft, Top ), Style->Text, Stamp );
        Canvas->Text( CVector( TextLeft + TimeSize.Horizontal + 6.0f * Style->Scale, Top + TimeSize.Vertical - Font->LineSpan - 1.0f * Style->Scale ), Style->Accent, Secs );
        Canvas->Write( Small, CVector( TextLeft, Top + TimeSize.Vertical + 6.0f * Style->Scale ), Style->Faint, Date );
        Canvas->PopClip( );
        ( void )SecSize;
    } else {
        CVector DateSize = Small->Measure( Date );
        float Top = Dial.Bottom( ) + 6.0f * Style->Scale;
        if ( Top + DateSize.Vertical <= Area.Bottom( ) )
            Canvas->Write( Small, CVector( Area.Left + ( Wide - DateSize.Horizontal ) * 0.5f, Top ), Style->Faint, Date );
    }
}

void mix( ) {
    hear::tick( );

    float Wide = Layout->Width( );
    int Kind = ( int )hear::source( );
    int Sources = 0;
    const char* const* Names = hear::source_names( Sources );
    if ( Widgets->Segments( "Source", Kind, Names, Sources ) )
        hear::set_source( ( hear::Source )Kind );

    float Drive = hear::gain( );
    if ( Widgets->Slider( "Sensitivity", Drive, 0.4f, 12.0f ) )
        hear::set_gain( Drive );

    CColor Wash = Style->Accent.Blend( hear::tint( ), 0.55f + hear::level( ) * 0.45f );

    Widgets->Meter( hear::level( ) );

    float WaveH = 48.0f * Style->Scale;
    CRectangle WaveBox = Layout->Place( CVector( Wide, WaveH ) );
    Canvas->Rectangle( WaveBox, Style->Control, 8.0f * Style->Scale );

    const int WaveN = 96;
    float Wave[ WaveN ];
    hear::wave( Wave, WaveN );
    CVector Prev;
    bool Have = false;
    float MidY = WaveBox.Top + WaveBox.Height * 0.5f;
    float Amp = WaveBox.Height * 0.40f;
    for ( int i = 0; i < WaveN; i++ ) {
        float X = WaveBox.Left + 6.0f * Style->Scale + ( WaveBox.Width - 12.0f * Style->Scale ) * ( ( float )i / ( float )( WaveN - 1 ) );
        float Y = MidY - Wave[ i ] * Amp * ( 1.15f + hear::level( ) * 0.55f );
        CVector Now( X, Y );
        if ( Have )
            Canvas->Line( Prev, Now, Wash, 1.8f * Style->Scale );
        Prev = Now;
        Have = true;
    }

    float BarH = 86.0f * Style->Scale;
    CRectangle Box = Layout->Place( CVector( Wide, BarH ) );
    Canvas->Rectangle( Box, Style->Control, 8.0f * Style->Scale );

    const int Count = 32;
    float Bands[ Count ];
    hear::bands( Bands, Count );
    float Gap = 1.8f * Style->Scale;
    float BarW = ( Box.Width - 12.0f * Style->Scale - Gap * ( float )( Count - 1 ) ) / ( float )Count;
    float Floor = 4.0f * Style->Scale;
    for ( int i = 0; i < Count; i++ ) {
        float T = ( float )i / ( float )( Count - 1 );
        float Rest = hear::live( ) ? Bands[ i ] : 0.08f + 0.04f * fabsf( sinf( ( float )i * 0.4f ) );
        float Height = Floor + ( Box.Height - 12.0f * Style->Scale - Floor ) * Rest;
        float X = Box.Left + 6.0f * Style->Scale + ( float )i * ( BarW + Gap );
        float Y = Box.Bottom( ) - 6.0f * Style->Scale - Height;
        CColor Fill = Style->Accent.Blend( hear::tint( ), T * 0.65f );
        if ( !hear::live( ) )
            Fill = Style->Faint.Fade( 0.35f );
        Canvas->Rectangle( CRectangle( X, Y, BarW, Height ), Fill, BarW * 0.4f );
    }

    const char* Note = hear::status( );
    CVector NoteSize = Font->Measure( Note );
    CRectangle Foot = Layout->Place( CVector( Wide, Font->LineSpan + 2.0f * Style->Scale ) );
    Canvas->Text( CVector( Foot.Left + ( Wide - NoteSize.Horizontal ) * 0.5f, Foot.Top ), Style->Faint, Note );
}

void wave( const float* Values, int Count ) {
    Widgets->Waveform( Values, Count );
}

void spectrum( const float* Values, int Count ) {
    Widgets->Spectrum( Values, Count );
}

void meter( float Level ) {
    Widgets->Meter( Level );
}

}
}
