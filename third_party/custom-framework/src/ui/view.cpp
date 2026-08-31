#include "ur/view.hpp"

#include "Context.h"
#include "Frames.h"
#include "Style.h"

#include <unordered_map>

namespace ur {
namespace view {

static std::unordered_map< std::string, float > Numbers;
static std::unordered_map< std::string, int > Indexes;
static std::unordered_map< std::string, bool > Flags;
static std::unordered_map< std::string, std::string > Texts;

float& number( const char* Id, float Fallback ) {
    std::string Key = Id ? Id : "";
    auto Found = Numbers.find( Key );
    if ( Found == Numbers.end( ) )
        Found = Numbers.emplace( Key, Fallback ).first;
    return Found->second;
}

int& index( const char* Id, int Fallback ) {
    std::string Key = Id ? Id : "";
    auto Found = Indexes.find( Key );
    if ( Found == Indexes.end( ) )
        Found = Indexes.emplace( Key, Fallback ).first;
    return Found->second;
}

bool& flag( const char* Id, bool Fallback ) {
    std::string Key = Id ? Id : "";
    auto Found = Flags.find( Key );
    if ( Found == Flags.end( ) )
        Found = Flags.emplace( Key, Fallback ).first;
    return Found->second;
}

std::string& text( const char* Id, const char* Fallback ) {
    std::string Key = Id ? Id : "";
    auto Found = Texts.find( Key );
    if ( Found == Texts.end( ) )
        Found = Texts.emplace( Key, Fallback ? Fallback : "" ).first;
    return Found->second;
}

void clear( ) {
    Numbers.clear( );
    Indexes.clear( );
    Flags.clear( );
    Texts.clear( );
}

unsigned int card( ) {
    return FrameMove | FrameResize | FrameCollapse | FrameFit;
}

void Board::begin( int Wanted ) {
    Pad = 12.0f * Style->Scale;
    Gap = 10.0f * Style->Scale;
    float Wide = Context->Display.Horizontal;
    Floor = Context->Display.Vertical - Pad;
    Columns = Wanted;
    if ( Columns <= 0 ) {
        if ( Wide >= 1680.0f )
            Columns = 4;
        else if ( Wide >= 1180.0f )
            Columns = 3;
        else
            Columns = 2;
    }
    if ( Columns < 1 )
        Columns = 1;
    if ( Columns > 6 )
        Columns = 6;

    Width = ( Wide - Pad * 2.0f - Gap * ( float )( Columns - 1 ) ) / ( float )Columns;
    if ( Width < 160.0f * Style->Scale )
        Width = 160.0f * Style->Scale;

    for ( int Column = 0; Column < Columns; Column++ ) {
        ColumnX[ Column ] = Pad + ( float )Column * ( Width + Gap );
        ColumnY[ Column ] = Pad;
    }
}

void Board::cell( const char* Title, float Guess, CVector& Origin, CVector& Extent, int Span ) {
    if ( Span < 1 )
        Span = 1;
    if ( Span > Columns )
        Span = Columns;

    float High = Guess * Style->Scale;
    if ( Title && Title[ 0 ] ) {
        unsigned int Identifier = Context->Hash( Title );
        CFrameState* State = Frames->Find( Identifier );
        if ( State && State->Ready && State->Size.Vertical > 8.0f )
            High = State->Size.Vertical;
    }

    int Pick = 0;
    float Best = 1.0e9f;
    for ( int Start = 0; Start <= Columns - Span; Start++ ) {
        float Top = ColumnY[ Start ];
        for ( int Extra = 1; Extra < Span; Extra++ ) {
            if ( ColumnY[ Start + Extra ] > Top )
                Top = ColumnY[ Start + Extra ];
        }
        if ( Top < Best ) {
            Best = Top;
            Pick = Start;
        }
    }

    float Top = ColumnY[ Pick ];
    for ( int Extra = 1; Extra < Span; Extra++ ) {
        if ( ColumnY[ Pick + Extra ] > Top )
            Top = ColumnY[ Pick + Extra ];
    }

    float Remain = Floor - Top;
    float Lowest = 72.0f * Style->Scale;
    if ( High > Remain && Remain > Lowest )
        High = Remain;
    if ( High < Lowest )
        High = Lowest;
    if ( Remain < Lowest )
        Remain = Lowest;

    Origin = CVector( ColumnX[ Pick ], Top );
    Extent = CVector( Width * ( float )Span + Gap * ( float )( Span - 1 ), High );

    float Next = Top + High + Gap;
    for ( int Extra = 0; Extra < Span; Extra++ )
        ColumnY[ Pick + Extra ] = Next;
}

}
}
