#if defined( _WIN32 )

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#elif defined( __APPLE__ )

#include <cmath>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

#endif

#include "Font.h"
#include "Sheet.h"
#include "Format.h"
#include "Context.h"

#if defined( _WIN32 )
#pragma comment( lib, "gdi32.lib" )
#endif

CFont::~CFont( ) {
    Destroy( );
}

bool CFont::Create( const char* const* Names, int Count, float Height, int Boldness ) {
    Destroy( );

    if ( !Names || Count <= 0 || Height <= 0.0f )
        return false;

    for ( int Position = 0; Position < Count; Position++ )
        Families.push_back( Names[ Position ] );

    BaseHeight = Height;
    Scale = 1.0f;

    Weight = Boldness;
    return Prepare( );
}

void CFont::Destroy( ) {
    Release( );

    Families.clear( );
    Glyphs.clear( );

    for ( int Position = 0; Position < 128; Position++ )
        Quick[ Position ] = nullptr;

    BaseHeight = 0.0f;
    Scale = 1.0f;

    Weight = 400;

    LineSpan = 0.0f;
    Ascent = 0.0f;

    Leading = 0.0f;
}

void CFont::Rescale( float Factor ) {
    if ( Factor <= 0.0f )
        Factor = 1.0f;

    Scale = Factor;
    Glyphs.clear( );

    for ( int Position = 0; Position < 128; Position++ )
        Quick[ Position ] = nullptr;

    Release( );
    Prepare( );
}

#if defined( _WIN32 )

void CFont::Release( ) {
    if ( Surface ) {
        DeleteDC( ( HDC )Surface );
        Surface = nullptr;
    }

    for ( void* Handle : Handles )
        DeleteObject( ( HFONT )Handle );

    Handles.clear( );
}

bool CFont::Prepare( ) {
    Release( );

    Surface = CreateCompatibleDC( nullptr );
    if ( !Surface ) {
        Context->Report( "Could not create a font surface" );
        return false;
    }

    int Pixels = ( int )( BaseHeight * Scale + 0.5f );
    if ( Pixels < 4 )
        Pixels = 4;

    for ( const std::string& Family : Families ) {
        HFONT Handle = CreateFontA( -Pixels, 0, 0, 0, Weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, Family.c_str( ) );
        if ( Handle )
            Handles.push_back( Handle );
    }

    if ( Handles.empty( ) ) {
        Context->Report( "Could not open any requested font family" );
        return false;
    }

    SelectObject( ( HDC )Surface, ( HFONT )Handles[ 0 ] );

    TEXTMETRICA Metrics = { };
    GetTextMetricsA( ( HDC )Surface, &Metrics );

    Ascent = ( float )Metrics.tmAscent;
    LineSpan = ( float )( Metrics.tmHeight + Metrics.tmExternalLeading );

    Leading = ( float )Metrics.tmInternalLeading;

    return true;
}

bool CFont::Rasterize( unsigned int Codepoint, CGlyph& Glyph ) {
    if ( !Surface || Handles.empty( ) || Codepoint > 0xFFFF )
        return false;

    HDC Surface2 = ( HDC )Surface;
    wchar_t Wide = ( wchar_t )Codepoint;

    HFONT Chosen = nullptr;

    for ( void* Handle : Handles ) {
        SelectObject( Surface2, ( HFONT )Handle );
        unsigned short Index = 0;

        if ( GetGlyphIndicesW( Surface2, &Wide, 1, &Index, GGI_MARK_NONEXISTING_GLYPHS ) == 1 && Index != 0xFFFF ) {
            Chosen = ( HFONT )Handle;
            break;
        }
    }

    if ( !Chosen )
        return false;

    SelectObject( Surface2, Chosen );

    MAT2 Identity = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
    GLYPHMETRICS Outline = { };

    DWORD Needed = GetGlyphOutlineW( Surface2, Codepoint, GGO_GRAY8_BITMAP, &Outline, 0, nullptr, &Identity );
    if ( Needed == GDI_ERROR )
        return false;

    Glyph.Advance = ( float )Outline.gmCellIncX;
    Glyph.Offset = CVector( ( float )Outline.gmptGlyphOrigin.x, Ascent - ( float )Outline.gmptGlyphOrigin.y );

    int Across = ( int )Outline.gmBlackBoxX;
    int Down = ( int )Outline.gmBlackBoxY;

    if ( Needed == 0 || Across <= 0 || Down <= 0 )
        return true;

    std::vector< unsigned char > Scratch( Needed );

    DWORD Copied = GetGlyphOutlineW( Surface2, Codepoint, GGO_GRAY8_BITMAP, &Outline, Needed, Scratch.data( ), &Identity );
    if ( Copied == GDI_ERROR )
        return false;

    int Pitch = ( Across + 3 ) & ~3;
    if ( ( size_t )Pitch * Down > Needed )
        return false;

    int Left = 0;
    int Top = 0;

    if ( !Sheet->Place( Across, Down, Left, Top ) ) {
        Context->Report( "Glyph sheet is full" );
        return false;
    }

    for ( int Row = 0; Row < Down; Row++ ) {
        unsigned char* Line = Sheet->Pixel( Left, Top + Row );

        for ( int Column = 0; Column < Across; Column++ ) {
            int Shade = ( int )Scratch[ ( size_t )Row * Pitch + Column ] * 255 / 64;
            if ( Shade > 255 )
                Shade = 255;

            Line[ Column ] = ( unsigned char )Shade;
        }
    }

    Glyph.Source = CRectangle( ( float )Left, ( float )Top, ( float )Across, ( float )Down );
    Glyph.Span = CVector( ( float )Across, ( float )Down );

    return true;
}

#elif defined( __APPLE__ )

void CFont::Release( ) {
    for ( void* Handle : Handles )
        CFRelease( ( CTFontRef )Handle );

    Handles.clear( );
    Surface = nullptr;
}

bool CFont::Prepare( ) {
    Release( );

    int Pixels = ( int )( BaseHeight * Scale + 0.5f );
    if ( Pixels < 4 )
        Pixels = 4;

    for ( const std::string& Family : Families ) {
        CFStringRef Name = CFStringCreateWithCString( kCFAllocatorDefault, Family.c_str( ), kCFStringEncodingUTF8 );
        if ( !Name )
            continue;

        CTFontRef Handle = CTFontCreateWithName( Name, ( CGFloat )Pixels, nullptr );
        CFRelease( Name );

        if ( !Handle )
            continue;

        if ( Weight >= 600 ) {
            CTFontRef Heavy = CTFontCreateCopyWithSymbolicTraits( Handle, ( CGFloat )Pixels, nullptr, kCTFontTraitBold, kCTFontTraitBold );

            if ( Heavy ) {
                CFRelease( Handle );
                Handle = Heavy;
            }
        }

        Handles.push_back( ( void* )Handle );
    }

    if ( Handles.empty( ) ) {
        Context->Report( "Could not open any requested font family" );
        return false;
    }

    CTFontRef Primary = ( CTFontRef )Handles[ 0 ];

    float Rise = ( float )std::ceil( CTFontGetAscent( Primary ) );
    float Drop = ( float )std::ceil( CTFontGetDescent( Primary ) );

    float Gap = ( float )std::ceil( CTFontGetLeading( Primary ) );

    Ascent = Rise;
    LineSpan = Rise + Drop + Gap;

    Leading = Rise + Drop - ( float )Pixels;
    if ( Leading < 0.0f )
        Leading = 0.0f;

    return true;
}

bool CFont::Rasterize( unsigned int Codepoint, CGlyph& Glyph ) {
    if ( Handles.empty( ) || Codepoint > 0xFFFF )
        return false;

    UniChar Wide = ( UniChar )Codepoint;

    CTFontRef Chosen = nullptr;
    CGGlyph Index = 0;

    for ( void* Handle : Handles ) {
        CGGlyph Found = 0;

        if ( CTFontGetGlyphsForCharacters( ( CTFontRef )Handle, &Wide, &Found, 1 ) && Found != 0 ) {
            Chosen = ( CTFontRef )Handle;
            Index = Found;

            break;
        }
    }

    if ( !Chosen )
        return false;

    CGSize Advance = { };
    CTFontGetAdvancesForGlyphs( Chosen, kCTFontOrientationHorizontal, &Index, &Advance, 1 );

    Glyph.Advance = ( float )( int )( Advance.width + 0.5 );

    CGRect Box = CTFontGetBoundingRectsForGlyphs( Chosen, kCTFontOrientationHorizontal, &Index, nullptr, 1 );

    if ( CGRectIsNull( Box ) || CGRectIsEmpty( Box ) )
        return true;

    int BoxLeft = ( int )std::floor( Box.origin.x ) - 1;
    int BoxBottom = ( int )std::floor( Box.origin.y ) - 1;

    int BoxRight = ( int )std::ceil( Box.origin.x + Box.size.width ) + 1;
    int BoxTop = ( int )std::ceil( Box.origin.y + Box.size.height ) + 1;

    Glyph.Offset = CVector( ( float )BoxLeft, Ascent - ( float )BoxTop );

    int Across = BoxRight - BoxLeft;
    int Down = BoxTop - BoxBottom;

    if ( Across <= 0 || Down <= 0 || Across > 4096 || Down > 4096 )
        return true;

    std::vector< unsigned char > Scratch( ( size_t )Across * Down, ( unsigned char )0 );

    CGContextRef Painter = CGBitmapContextCreate( Scratch.data( ), ( size_t )Across, ( size_t )Down, 8, ( size_t )Across, nullptr, kCGImageAlphaOnly );
    if ( !Painter )
        return false;

    CGContextSetShouldAntialias( Painter, true );
    CGContextSetShouldSmoothFonts( Painter, false );

    CGContextSetAllowsFontSubpixelPositioning( Painter, false );
    CGContextSetShouldSubpixelPositionFonts( Painter, false );

    CGContextSetAllowsFontSubpixelQuantization( Painter, false );
    CGContextSetShouldSubpixelQuantizeFonts( Painter, false );

    CGContextSetGrayFillColor( Painter, 1.0, 1.0 );

    CGPoint Spot = CGPointMake( ( CGFloat )-BoxLeft, ( CGFloat )-BoxBottom );
    CTFontDrawGlyphs( Chosen, &Index, &Spot, 1, Painter );

    CGContextFlush( Painter );
    CGContextRelease( Painter );

    int Left = 0;
    int Top = 0;

    if ( !Sheet->Place( Across, Down, Left, Top ) ) {
        Context->Report( "Glyph sheet is full" );
        return false;
    }

    for ( int Row = 0; Row < Down; Row++ ) {
        unsigned char* Line = Sheet->Pixel( Left, Top + Row );

        for ( int Column = 0; Column < Across; Column++ )
            Line[ Column ] = Scratch[ ( size_t )Row * Across + Column ];
    }

    Glyph.Source = CRectangle( ( float )Left, ( float )Top, ( float )Across, ( float )Down );
    Glyph.Span = CVector( ( float )Across, ( float )Down );

    return true;
}

#endif

const CGlyph* CFont::Fetch( unsigned int Codepoint ) {
    if ( Codepoint < 128 && Quick[ Codepoint ] )
        return Quick[ Codepoint ];

    auto Existing = Glyphs.find( Codepoint );

    if ( Existing != Glyphs.end( ) ) {
        if ( Codepoint < 128 )
            Quick[ Codepoint ] = &Existing->second;

        return &Existing->second;
    }

    CGlyph Glyph;

    if ( !Rasterize( Codepoint, Glyph ) && Codepoint != '?' ) {
        const CGlyph* Fallback = Fetch( '?' );
        Glyphs[ Codepoint ] = *Fallback;
    } else {
        Glyphs[ Codepoint ] = Glyph;
    }

    const CGlyph* Result = &Glyphs[ Codepoint ];

    if ( Codepoint < 128 )
        Quick[ Codepoint ] = Result;

    return Result;
}

CVector CFont::Measure( const char* Message ) {
    float Reach = 0.0f;

    if ( !Message )
        return CVector( Reach, LineSpan );

    while ( *Message )
        Reach += Fetch( DecodeCharacter( Message ) )->Advance;

    return CVector( Reach, LineSpan );
}

CRectangle CFont::Bounds( const char* Message ) {
    if ( !Message )
        return CRectangle( );

    float Pen = 0.0f;
    bool Any = false;

    float Left = 0.0f;
    float Top = 0.0f;

    float Right = 0.0f;
    float Bottom = 0.0f;

    while ( *Message ) {
        const CGlyph* Glyph = Fetch( DecodeCharacter( Message ) );

        if ( Glyph->Span.Horizontal > 0.0f && Glyph->Span.Vertical > 0.0f ) {
            float NearLeft = Pen + Glyph->Offset.Horizontal;
            float NearTop = Glyph->Offset.Vertical;

            float FarRight = NearLeft + Glyph->Span.Horizontal;
            float FarBottom = NearTop + Glyph->Span.Vertical;

            if ( !Any ) {
                Left = NearLeft;
                Top = NearTop;

                Right = FarRight;
                Bottom = FarBottom;

                Any = true;
            } else {
                if ( NearLeft < Left )
                    Left = NearLeft;

                if ( NearTop < Top )
                    Top = NearTop;

                if ( FarRight > Right )
                    Right = FarRight;

                if ( FarBottom > Bottom )
                    Bottom = FarBottom;
            }
        }

        Pen += Glyph->Advance;
    }

    if ( !Any )
        return CRectangle( );

    return CRectangle( Left, Top, Right - Left, Bottom - Top );
}

float CFont::Span( const char* Message, int Bytes ) {
    if ( !Message || Bytes <= 0 )
        return 0.0f;

    float Reach = 0.0f;

    const char* Cursor = Message;
    const char* Limit = Message + Bytes;

    while ( *Cursor && Cursor < Limit )
        Reach += Fetch( DecodeCharacter( Cursor ) )->Advance;

    return Reach;
}

int CFont::Hit( const char* Message, float Across ) {
    if ( !Message )
        return 0;

    float Reach = 0.0f;
    const char* Cursor = Message;

    while ( *Cursor ) {
        const char* Before = Cursor;
        float Advance = Fetch( DecodeCharacter( Cursor ) )->Advance;

        if ( Across < Reach + Advance * 0.5f )
            return ( int )( Before - Message );

        Reach += Advance;
    }

    return ( int )( Cursor - Message );
}
