#pragma once

#include <memory>
#include <vector>

class CSheet {
public:
    bool Create( );
    void Destroy( );

    bool Place( int Across, int Down, int& Left, int& Top );
    unsigned char* Pixel( int Left, int Top );

    const unsigned char* Data( ) const;

    int Extent( ) const;
    int Depth( ) const;

    unsigned int Version = 0;

private:
    bool Grow( int Needed );

    std::vector< unsigned char > Pixels;
    int Height = 0;

    int PenAcross = 1;
    int PenDown = 1;

    int RowPeak = 0;
};

inline auto Sheet = std::make_unique< CSheet >( );