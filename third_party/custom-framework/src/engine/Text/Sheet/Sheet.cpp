#include "Sheet.h"

static constexpr int SheetWidth = 1024;
static constexpr int SheetLimit = 8192;

bool CSheet::Create( ) {
    Destroy( );

    Height = 512;
    Pixels.assign( ( size_t )SheetWidth * Height, ( unsigned char )0 );

    Version = 1;
    return true;
}

void CSheet::Destroy( ) {
    Pixels.clear( );
    Height = 0;

    PenAcross = 1;
    PenDown = 1;

    RowPeak = 0;
    Version = 0;
}

bool CSheet::Grow( int Needed ) {
    int Goal = Height;

    while ( Goal < Needed && Goal < SheetLimit )
        Goal *= 2;

    if ( Goal < Needed )
        return false;

    Pixels.resize( ( size_t )SheetWidth * Goal, ( unsigned char )0 );
    Height = Goal;

    return true;
}

bool CSheet::Place( int Across, int Down, int& Left, int& Top ) {
    if ( Pixels.empty( ) || Across <= 0 || Down <= 0 || Across > SheetWidth - 2 )
        return false;

    if ( PenAcross + Across + 1 > SheetWidth ) {
        PenAcross = 1;

        PenDown += RowPeak + 1;
        RowPeak = 0;
    }

    if ( PenDown + Down + 1 > Height ) {
        if ( !Grow( PenDown + Down + 1 ) )
            return false;
    }

    Left = PenAcross;
    Top = PenDown;

    PenAcross += Across + 1;
    if ( Down > RowPeak )
        RowPeak = Down;

    Version++;
    return true;
}

unsigned char* CSheet::Pixel( int Left, int Top ) {
    return Pixels.data( ) + ( size_t )Top * SheetWidth + Left;
}

const unsigned char* CSheet::Data( ) const {
    return Pixels.data( );
}

int CSheet::Extent( ) const {
    return SheetWidth;
}

int CSheet::Depth( ) const {
    return Height;
}