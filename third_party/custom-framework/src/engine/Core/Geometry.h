#pragma once

class CVector {
public:
    CVector( ) = default;
    CVector( float Across, float Down ) : Horizontal( Across ), Vertical( Down ) { }

    CVector operator+( CVector Other ) const {
        return CVector( Horizontal + Other.Horizontal, Vertical + Other.Vertical );
    }

    CVector operator-( CVector Other ) const {
        return CVector( Horizontal - Other.Horizontal, Vertical - Other.Vertical );
    }

    CVector operator*( float Amount ) const {
        return CVector( Horizontal * Amount, Vertical * Amount );
    }

    float Horizontal = 0.0f;
    float Vertical = 0.0f;
};

class CColor {
public:
    CColor( ) = default;
    CColor( int Red, int Green, int Blue, int Alpha = 255 ) : Red( ( unsigned char )Red ), Green( ( unsigned char )Green ), Blue( ( unsigned char )Blue ), Alpha( ( unsigned char )Alpha ) { }

    unsigned int Pack( ) const {
        return ( unsigned int )Red | ( ( unsigned int )Green << 8 ) | ( ( unsigned int )Blue << 16 ) | ( ( unsigned int )Alpha << 24 );
    }

    CColor Blend( CColor Other, float Amount ) const {
        if ( Amount < 0.0f )
            Amount = 0.0f;

        if ( Amount > 1.0f )
            Amount = 1.0f;

        float Inverse = 1.0f - Amount;
        return CColor( ( int )( Red * Inverse + Other.Red * Amount ), ( int )( Green * Inverse + Other.Green * Amount ), ( int )( Blue * Inverse + Other.Blue * Amount ), ( int )( Alpha * Inverse + Other.Alpha * Amount ) );
    }

    CColor Fade( float Amount ) const {
        if ( Amount < 0.0f )
            Amount = 0.0f;

        if ( Amount > 1.0f )
            Amount = 1.0f;

        return CColor( Red, Green, Blue, ( int )( Alpha * Amount ) );
    }

    unsigned char Red = 0;
    unsigned char Green = 0;
    unsigned char Blue = 0;
    unsigned char Alpha = 255;
};

class CRectangle {
public:
    CRectangle( ) = default;
    CRectangle( float Left, float Top, float Width, float Height ) : Left( Left ), Top( Top ), Width( Width ), Height( Height ) { }
    CRectangle( CVector Origin, CVector Extent ) : Left( Origin.Horizontal ), Top( Origin.Vertical ), Width( Extent.Horizontal ), Height( Extent.Vertical ) { }

    float Right( ) const {
        return Left + Width;
    }

    float Bottom( ) const {
        return Top + Height;
    }

    CVector Origin( ) const {
        return CVector( Left, Top );
    }

    CVector Extent( ) const {
        return CVector( Width, Height );
    }

    CVector Center( ) const {
        return CVector( Left + Width * 0.5f, Top + Height * 0.5f );
    }

    bool Contains( CVector Point ) const {
        return Point.Horizontal >= Left && Point.Horizontal < Right( ) && Point.Vertical >= Top && Point.Vertical < Bottom( );
    }

    CRectangle Shrink( float Amount ) const {
        return CRectangle( Left + Amount, Top + Amount, Width - Amount * 2.0f, Height - Amount * 2.0f );
    }

    CRectangle Pad( float Across, float Down ) const {
        return CRectangle( Left + Across, Top + Down, Width - Across * 2.0f, Height - Down * 2.0f );
    }

    CRectangle Inflate( float Amount ) const {
        return Shrink( -Amount );
    }

    CRectangle Clip( const CRectangle& Other ) const {
        float EdgeLeft = Left > Other.Left ? Left : Other.Left;
        float EdgeTop = Top > Other.Top ? Top : Other.Top;

        float EdgeRight = Right( ) < Other.Right( ) ? Right( ) : Other.Right( );
        float EdgeBottom = Bottom( ) < Other.Bottom( ) ? Bottom( ) : Other.Bottom( );

        float Across = EdgeRight - EdgeLeft;
        float Down = EdgeBottom - EdgeTop;

        return CRectangle( EdgeLeft, EdgeTop, Across > 0.0f ? Across : 0.0f, Down > 0.0f ? Down : 0.0f );
    }

    float Left = 0.0f;
    float Top = 0.0f;

    float Width = 0.0f;
    float Height = 0.0f;
};