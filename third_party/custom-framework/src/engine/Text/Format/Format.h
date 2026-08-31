#pragma once

#include <memory>

inline unsigned int DecodeCharacter( const char*& Cursor ) {
    unsigned int First = ( unsigned int )( unsigned char )*Cursor;
    Cursor++;

    if ( First < 0x80 )
        return First;

    if ( ( First & 0xE0 ) == 0xC0 && ( *Cursor & 0xC0 ) == 0x80 ) {
        unsigned int Second = ( unsigned int )( unsigned char )*Cursor;
        Cursor++;

        return ( ( First & 0x1F ) << 6 ) | ( Second & 0x3F );
    }

    if ( ( First & 0xF0 ) == 0xE0 && ( *Cursor & 0xC0 ) == 0x80 ) {
        unsigned int Second = ( unsigned int )( unsigned char )*Cursor;
        Cursor++;

        if ( ( *Cursor & 0xC0 ) != 0x80 )
            return '?';

        unsigned int Third = ( unsigned int )( unsigned char )*Cursor;
        Cursor++;

        return ( ( First & 0x0F ) << 12 ) | ( ( Second & 0x3F ) << 6 ) | ( Third & 0x3F );
    }

    if ( ( First & 0xF8 ) == 0xF0 ) {
        while ( ( *Cursor & 0xC0 ) == 0x80 )
            Cursor++;

        return '?';
    }

    return '?';
}

class CFormat {
public:
    char* Print( const char* Pattern, ... );
};

inline auto Format = std::make_unique< CFormat >( );