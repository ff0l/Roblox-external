#include "Font.h"
#include "Sheet.h"
#include "Canvas.h"
#include "Format.h"
#include "Context.h"
#include "Input.h"

static float Restrain( float Rounding, CRectangle Bounds ) {
    float Limit = ( Bounds.Width < Bounds.Height ? Bounds.Width : Bounds.Height ) * 0.5f;

    if ( Rounding > Limit )
        return Limit;

    if ( Rounding < 0.0f )
        return 0.0f;

    return Rounding;
}

void CCanvas::Begin( CVector Display ) {
    Screen = Display;

    for ( CChannel& Channel : Channels ) {
        Channel.Items.clear( );
        Channel.Marks.clear( );
    }

    if ( Channels.empty( ) )
        Channels.resize( 8 );

    Clips.clear( );
    Clips.push_back( CRectangle( 0.0f, 0.0f, Display.Horizontal, Display.Vertical ) );

    Active = 0;
    Shader = 0;

    Opacity = 1.0f;
}

int CCanvas::Route( int Channel ) {
    if ( Channel < 0 )
        Channel = 0;

    if ( Channel >= ( int )Channels.size( ) )
        Channels.resize( ( size_t )Channel + 1 );

    int Former = Active;
    Active = Channel;

    return Former;
}

unsigned int CCanvas::Effect( unsigned int Wanted ) {
    unsigned int Former = Shader;
    Shader = Wanted;

    return Former;
}

void CCanvas::PushClip( CRectangle Bounds, bool Absolute ) {
    if ( Clips.empty( ) ) {
        Clips.push_back( Bounds );
        return;
    }

    if ( Absolute ) {
        Clips.push_back( Bounds.Clip( Clips[ 0 ] ) );
        return;
    }

    Clips.push_back( Bounds.Clip( Clips.back( ) ) );
}

void CCanvas::PopClip( ) {
    if ( Clips.size( ) > 1 )
        Clips.pop_back( );
}

bool CCanvas::Visible( CVector Point ) const {
    if ( Clips.empty( ) )
        return false;

    return Clips.back( ).Contains( Point );
}

CInstance* CCanvas::Emit( CRectangle Bounds, unsigned long long Picture ) {
    if ( Clips.empty( ) )
        return nullptr;

    if ( Bounds.Width <= 0.0f || Bounds.Height <= 0.0f )
        return nullptr;

    const CRectangle& Scope = Clips.back( );
    if ( Scope.Width <= 0.0f || Scope.Height <= 0.0f )
        return nullptr;

    if ( Bounds.Left >= Scope.Right( ) || Bounds.Right( ) <= Scope.Left || Bounds.Top >= Scope.Bottom( ) || Bounds.Bottom( ) <= Scope.Top )
        return nullptr;

    CChannel& Channel = Channels[ Active ];
    CInstance& Item = Channel.Items.emplace_back( );

    Item.Bounds = Bounds;

    Item.ClipLeft = Scope.Left;
    Item.ClipTop = Scope.Top;

    Item.ClipRight = Scope.Right( );
    Item.ClipBottom = Scope.Bottom( );

    bool Fresh = Channel.Marks.empty( ) || Channel.Marks.back( ).Image != Picture || Channel.Marks.back( ).Effect != Shader || Channel.Marks.back( ).Callback;
    if ( Fresh ) {
        CBatch& Mark = Channel.Marks.emplace_back( );

        Mark.Offset = ( int )Channel.Items.size( ) - 1;
        Mark.Count = 1;

        Mark.Image = Picture;
        Mark.Effect = Shader;

        return &Item;
    }

    Channel.Marks.back( ).Count++;
    return &Item;
}

void CCanvas::Rectangle( CRectangle Bounds, CColor Fill, float Rounding ) {
    CInstance* Item = Emit( Bounds.Inflate( 1.0f ), 0 );
    if ( !Item )
        return;

    if ( Opacity < 1.0f )
        Fill = Fill.Fade( Opacity );

    Item->ColorStart = Fill.Pack( );
    Item->ColorFinish = Item->ColorStart;

    Item->Rounding = Restrain( Rounding, Bounds );
    Item->Inflate = 1.0f;
}

void CCanvas::Gradient( CRectangle Bounds, CColor First, CColor Second, float Rounding, bool Horizontal ) {
    CInstance* Item = Emit( Bounds.Inflate( 1.0f ), 0 );
    if ( !Item )
        return;

    if ( Opacity < 1.0f ) {
        First = First.Fade( Opacity );
        Second = Second.Fade( Opacity );
    }

    Item->ColorStart = First.Pack( );
    Item->ColorFinish = Second.Pack( );

    Item->Rounding = Restrain( Rounding, Bounds );
    Item->Inflate = 1.0f;

    if ( Horizontal )
        Item->Flags |= 4;
}

void CCanvas::Border( CRectangle Bounds, CColor Stroke, float Rounding, float Thickness ) {
    if ( Thickness < 1.0f )
        Thickness = 1.0f;

    float Extra = Thickness * 0.5f + 2.0f;

    CInstance* Item = Emit( Bounds.Inflate( Extra ), 0 );
    if ( !Item )
        return;

    if ( Opacity < 1.0f )
        Stroke = Stroke.Fade( Opacity );

    Item->ColorStart = Stroke.Pack( );
    Item->ColorFinish = Item->ColorStart;

    Item->Rounding = Restrain( Rounding, Bounds );
    Item->Thickness = Thickness;

    Item->Inflate = Extra;
    Item->Flags |= 2;
}

void CCanvas::Shadow( CRectangle Bounds, CColor Tint, float Rounding, float Softness ) {
    if ( Softness < 1.0f )
        Softness = 1.0f;

    CInstance* Item = Emit( Bounds.Inflate( Softness ), 0 );
    if ( !Item )
        return;

    if ( Opacity < 1.0f )
        Tint = Tint.Fade( Opacity );

    Item->ColorStart = Tint.Pack( );
    Item->ColorFinish = Item->ColorStart;

    Item->Rounding = Restrain( Rounding, Bounds );
    Item->Softness = Softness;

    Item->Inflate = Softness;
}

void CCanvas::Circle( CVector Middle, float Radius, CColor Fill ) {
    if ( Radius <= 0.0f )
        return;

    CRectangle Bounds( Middle.Horizontal - Radius, Middle.Vertical - Radius, Radius * 2.0f, Radius * 2.0f );
    Rectangle( Bounds, Fill, Radius );
}

void CCanvas::Wedge( CRectangle Bounds, CColor Fill, int Turn ) {
    CInstance* Item = Emit( Bounds.Inflate( 1.0f ), 0 );
    if ( !Item )
        return;

    if ( Turn < 0 || Turn > 3 )
        Turn = 0;

    if ( Opacity < 1.0f )
        Fill = Fill.Fade( Opacity );

    Item->ColorStart = Fill.Pack( );
    Item->ColorFinish = Item->ColorStart;

    Item->Source = CRectangle( ( float )Turn, 0.0f, 0.0f, 0.0f );

    Item->Inflate = 1.0f;
    Item->Flags |= 16;
}

void CCanvas::Sector( CVector Middle, float Radius, float Start, float Sweep, float Inner, CColor Fill ) {
    if ( Radius <= 0.0f )
        return;

    CRectangle Bounds( Middle.Horizontal - Radius, Middle.Vertical - Radius, Radius * 2.0f, Radius * 2.0f );

    CInstance* Item = Emit( Bounds.Inflate( 1.0f ), 0 );
    if ( !Item )
        return;

    if ( Opacity < 1.0f )
        Fill = Fill.Fade( Opacity );

    Item->ColorStart = Fill.Pack( );
    Item->ColorFinish = Item->ColorStart;

    Item->Source = CRectangle( Start, Sweep, Inner, 0.0f );

    Item->Inflate = 1.0f;
    Item->Flags |= 64;
}

void CCanvas::Polygon( const CVector* Points, int Count, CColor Fill ) {
    if ( !Points || Count < 3 || Count > 4 )
        return;

    CVector Corners[ 4 ] = { Points[ 0 ], Points[ 1 ], Points[ 2 ], Count > 3 ? Points[ 3 ] : Points[ 0 ] };

    float Left = Corners[ 0 ].Horizontal;
    float Top = Corners[ 0 ].Vertical;

    float Right = Left;
    float Bottom = Top;

    for ( int Index = 1; Index < 4; Index++ ) {
        if ( Corners[ Index ].Horizontal < Left )
            Left = Corners[ Index ].Horizontal;

        if ( Corners[ Index ].Vertical < Top )
            Top = Corners[ Index ].Vertical;

        if ( Corners[ Index ].Horizontal > Right )
            Right = Corners[ Index ].Horizontal;

        if ( Corners[ Index ].Vertical > Bottom )
            Bottom = Corners[ Index ].Vertical;
    }

    CRectangle Box( Left, Top, Right - Left, Bottom - Top );

    if ( Box.Width <= 0.0f || Box.Height <= 0.0f )
        return;

    CInstance* Item = Emit( Box.Inflate( 1.0f ), 0 );
    if ( !Item )
        return;

    if ( Opacity < 1.0f )
        Fill = Fill.Fade( Opacity );

    Item->ColorStart = Fill.Pack( );
    Item->ColorFinish = Item->ColorStart;

    float Across = 1.0f / Box.Width;
    float Down = 1.0f / Box.Height;

    Item->Source = CRectangle( ( Corners[ 0 ].Horizontal - Box.Left ) * Across, ( Corners[ 0 ].Vertical - Box.Top ) * Down, ( Corners[ 1 ].Horizontal - Box.Left ) * Across, ( Corners[ 1 ].Vertical - Box.Top ) * Down );

    Item->Rounding = ( Corners[ 2 ].Horizontal - Box.Left ) * Across;
    Item->Thickness = ( Corners[ 2 ].Vertical - Box.Top ) * Down;

    Item->Softness = ( Corners[ 3 ].Horizontal - Box.Left ) * Across;
    Item->Spare = ( Corners[ 3 ].Vertical - Box.Top ) * Down;

    Item->Inflate = 1.0f;
    Item->Flags |= 128;
}

void CCanvas::PolygonBorder( const CVector* Points, int Count, CColor Tint, float Thickness ) {
    if ( !Points || Count < 3 || Count > 4 )
        return;

    for ( int Index = 0; Index < Count; Index++ )
        Line( Points[ Index ], Points[ ( Index + 1 ) % Count ], Tint, Thickness );
}

void CCanvas::Stroke( CRectangle Bounds, CVector From, CVector Till, CColor Tint, float Thickness ) {
    CInstance* Item = Emit( Bounds.Inflate( Thickness ), 0 );
    if ( !Item )
        return;

    if ( Thickness < 1.0f )
        Thickness = 1.0f;

    if ( Opacity < 1.0f )
        Tint = Tint.Fade( Opacity );

    Item->ColorStart = Tint.Pack( );
    Item->ColorFinish = Item->ColorStart;

    Item->Source = CRectangle( From.Horizontal, From.Vertical, Till.Horizontal, Till.Vertical );
    Item->Thickness = Thickness;

    Item->Inflate = Thickness;
    Item->Flags |= 32;
}

void CCanvas::Line( CVector From, CVector Till, CColor Tint, float Thickness ) {
    float Left = From.Horizontal < Till.Horizontal ? From.Horizontal : Till.Horizontal;
    float Top = From.Vertical < Till.Vertical ? From.Vertical : Till.Vertical;

    float Wide = From.Horizontal < Till.Horizontal ? Till.Horizontal - From.Horizontal : From.Horizontal - Till.Horizontal;
    float Tall = From.Vertical < Till.Vertical ? Till.Vertical - From.Vertical : From.Vertical - Till.Vertical;

    CVector Head( Wide > 0.0f ? ( From.Horizontal - Left ) / Wide : 0.5f, Tall > 0.0f ? ( From.Vertical - Top ) / Tall : 0.5f );
    CVector Tail( Wide > 0.0f ? ( Till.Horizontal - Left ) / Wide : 0.5f, Tall > 0.0f ? ( Till.Vertical - Top ) / Tall : 0.5f );

    Stroke( CRectangle( Left, Top, Wide, Tall ), Head, Tail, Tint, Thickness );
}

void CCanvas::Text( CVector Anchor, CColor Tint, const char* Message ) {
    Write( Font.get( ), Anchor, Tint, Message );
}

void CCanvas::Write( CFont* Face, CVector Anchor, CColor Tint, const char* Message ) {
    if ( !Face || !Message )
        return;

    float PenAcross = ( float )( int )( Anchor.Horizontal + 0.5f );
    float PenDown = ( float )( int )( Anchor.Vertical + 0.5f );

    if ( Opacity < 1.0f )
        Tint = Tint.Fade( Opacity );

    unsigned int Packed = Tint.Pack( );

    while ( *Message ) {
        const CGlyph* Glyph = Face->Fetch( DecodeCharacter( Message ) );

        if ( Glyph->Span.Horizontal > 0.0f ) {
            CRectangle Bounds( PenAcross + Glyph->Offset.Horizontal, PenDown + Glyph->Offset.Vertical, Glyph->Span.Horizontal, Glyph->Span.Vertical );

            CInstance* Item = Emit( Bounds, 0 );
            if ( Item ) {
                Item->Source = Glyph->Source;

                Item->ColorStart = Packed;
                Item->ColorFinish = Packed;

                Item->Flags |= 1;
            }
        }

        PenAcross += Glyph->Advance;
    }
}

void CCanvas::Outlined( CVector Anchor, CColor Tint, CColor Border, float Thickness, const char* Message ) {
    WriteOutlined( Font.get( ), Anchor, Tint, Border, Thickness, Message );
}

void CCanvas::WriteOutlined( CFont* Face, CVector Anchor, CColor Tint, CColor Border, float Thickness, const char* Message ) {
    if ( !Face || !Message )
        return;

    if ( Thickness > 0.0f && Border.Alpha > 0 ) {
        float Straight = Thickness;
        float Slanted = Thickness * 0.70710678f;

        CVector Offsets[ 8 ] = { CVector( -Straight, 0.0f ), CVector( Straight, 0.0f ), CVector( 0.0f, -Straight ), CVector( 0.0f, Straight ), CVector( -Slanted, -Slanted ), CVector( Slanted, -Slanted ), CVector( -Slanted, Slanted ), CVector( Slanted, Slanted ) };

        for ( int Index = 0; Index < 8; Index++ )
            Write( Face, Anchor + Offsets[ Index ], Border, Message );
    }

    Write( Face, Anchor, Tint, Message );
}

void CCanvas::Image( CRectangle Bounds, unsigned long long Picture, CRectangle Source, CColor Tint, float Rounding ) {
    CInstance* Item = Emit( Bounds, Picture );
    if ( !Item )
        return;

    if ( Opacity < 1.0f )
        Tint = Tint.Fade( Opacity );

    Item->Source = Source;

    Item->ColorStart = Tint.Pack( );
    Item->ColorFinish = Item->ColorStart;

    Item->Rounding = Restrain( Rounding, Bounds );
    Item->Flags |= 8;
}

void CCanvas::Custom( CRectangle Bounds, void ( *Callback )( void* Stream, void* Detail ), void* Detail ) {
    ( void )Bounds;

    if ( !Callback )
        return;

    CChannel& Channel = Channels[ Active ];
    CBatch& Mark = Channel.Marks.emplace_back( );

    Mark.Offset = ( int )Channel.Items.size( );
    Mark.Count = 0;

    Mark.Callback = Callback;
    Mark.Detail = Detail;
}

void CCanvas::Arrange( const int* Order, int Count ) {
    Combined.clear( );
    Joined.clear( );

    float Across = 1.0f / ( float )Sheet->Extent( );
    float Down = 1.0f / ( float )Sheet->Depth( );

    for ( int Position = 0; Position < Count; Position++ ) {
        int Route = Order[ Position ];

        if ( Route < 0 || Route >= ( int )Channels.size( ) )
            continue;

        const CChannel& Channel = Channels[ Route ];
        if ( Channel.Items.empty( ) && Channel.Marks.empty( ) )
            continue;

        int Base = ( int )Combined.size( );
        Combined.insert( Combined.end( ), Channel.Items.begin( ), Channel.Items.end( ) );

        for ( size_t Item = ( size_t )Base; Item < Combined.size( ); Item++ ) {
            if ( Combined[ Item ].Flags & 1 ) {
                Combined[ Item ].Source.Left *= Across;
                Combined[ Item ].Source.Top *= Down;

                Combined[ Item ].Source.Width *= Across;
                Combined[ Item ].Source.Height *= Down;
            }
        }

        for ( const CBatch& Mark : Channel.Marks ) {
            bool Merge = !Joined.empty( ) && !Mark.Callback && !Joined.back( ).Callback && Joined.back( ).Image == Mark.Image && Joined.back( ).Effect == Mark.Effect;
            if ( Merge ) {
                Joined.back( ).Count += Mark.Count;
                continue;
            }

            CBatch& Batch = Joined.emplace_back( );

            Batch.Offset = Base + Mark.Offset;
            Batch.Count = Mark.Count;

            Batch.Image = Mark.Image;
            Batch.Effect = Mark.Effect;

            Batch.Callback = Mark.Callback;
            Batch.Detail = Mark.Detail;
        }
    }

    Product.Items = Combined.data( );
    Product.Count = ( int )Combined.size( );

    Product.Batches = Joined.data( );
    Product.BatchCount = ( int )Joined.size( );

    Product.Extent = Screen;
    Product.Moment = ( float )Context->Elapsed;
    Product.Mouse = Input->MousePosition;
    Product.PressLeft = Input->MouseDown( 0 ) ? 1.0f : 0.0f;
    Product.PressRight = Input->MouseDown( 1 ) ? 1.0f : 0.0f;

    LastCount = Product.Count;
    LastBatches = Product.BatchCount;
}

const CDrawData& CCanvas::Data( ) const {
    return Product;
}