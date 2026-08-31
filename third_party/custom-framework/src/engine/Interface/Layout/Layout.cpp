#include <cmath>

#include "Font.h"
#include "Input.h"
#include "Frames.h"
#include "Canvas.h"
#include "Layout.h"

#include "Style.h"
#include "Context.h"

static constexpr unsigned int SaltGrip = 0x51EDA9C3u;

void CLayout::Begin( CRectangle Region ) {
    Parcel = CParcel( );

    Parcel.Area = Region;
    Parcel.Cursor = Region.Origin( );

    Saved.clear( );
    Clusters.clear( );

    ChildKeys.clear( );
    ChildBoxes.clear( );

    Widths.clear( );
    Tables.clear( );
}

float CLayout::End( ) {
    Parcel.Adjacent = false;
    return Parcel.Depth;
}

void CLayout::Sweep( ) {
    auto Entry = Nooks.begin( );

    while ( Entry != Nooks.end( ) ) {
        if ( Context->FrameIndex - Entry->second.Stamp > 2 )
            Entry = Nooks.erase( Entry );
        else
            Entry++;
    }
}

void CLayout::Destroy( ) {
    Parcel = CParcel( );

    Saved.clear( );
    Clusters.clear( );

    ChildKeys.clear( );
    ChildBoxes.clear( );

    Widths.clear( );
    Tables.clear( );

    Nooks.clear( );
    Layouts.clear( );
}

void CLayout::Fold( CRectangle Bounds ) {
    CCluster& Cluster = Clusters.back( );
    CVector Corner( Bounds.Right( ), Bounds.Bottom( ) );

    if ( !Cluster.Any ) {
        Cluster.Least = Bounds.Origin( );

        Cluster.Most = Corner;
        Cluster.Any = true;

        return;
    }

    if ( Bounds.Left < Cluster.Least.Horizontal )
        Cluster.Least.Horizontal = Bounds.Left;

    if ( Bounds.Top < Cluster.Least.Vertical )
        Cluster.Least.Vertical = Bounds.Top;

    if ( Corner.Horizontal > Cluster.Most.Horizontal )
        Cluster.Most.Horizontal = Corner.Horizontal;

    if ( Corner.Vertical > Cluster.Most.Vertical )
        Cluster.Most.Vertical = Corner.Vertical;
}

CRectangle CLayout::Place( CVector Extent ) {
    if ( Parcel.Adjacent ) {
        Parcel.Adjacent = false;
    } else {
        Parcel.Cursor.Horizontal = Clusters.empty( ) ? Parcel.Area.Left + Parcel.Margin : Clusters.back( ).Start.Horizontal;
        Parcel.Cursor.Vertical += Parcel.LinePeak > 0.0f ? Parcel.LinePeak + Style->Spacing : 0.0f;

        Parcel.LinePeak = 0.0f;
    }

    CRectangle Bounds( Parcel.Cursor, Extent );

    if ( Extent.Vertical > Parcel.LinePeak )
        Parcel.LinePeak = Extent.Vertical;

    if ( Bounds.Bottom( ) - Parcel.Area.Top > Parcel.Depth )
        Parcel.Depth = Bounds.Bottom( ) - Parcel.Area.Top;

    Parcel.Cursor.Horizontal += Extent.Horizontal + Style->Spacing;

    if ( !Clusters.empty( ) )
        Fold( Bounds );

    return Bounds;
}

CRectangle CLayout::Stretch( float Height ) {
    Parcel.Adjacent = false;
    return Place( CVector( Parcel.Area.Width - Parcel.Margin, Height ) );
}

void CLayout::Beside( ) {
    SameLine( );
}

void CLayout::SameLine( float Offset, float Gap ) {
    Parcel.Adjacent = true;
    float Start = Clusters.empty( ) ? Parcel.Area.Left + Parcel.Margin : Clusters.back( ).Start.Horizontal;

    if ( Offset > 0.0f )
        Parcel.Cursor.Horizontal = Start + Offset;
    else if ( Gap >= 0.0f )
        Parcel.Cursor.Horizontal += Gap - Style->Spacing;
}

void CLayout::Skip( float Amount ) {
    Parcel.Adjacent = false;
    Parcel.Cursor.Horizontal = Clusters.empty( ) ? Parcel.Area.Left + Parcel.Margin : Clusters.back( ).Start.Horizontal;

    if ( Parcel.LinePeak > 0.0f ) {
        Parcel.Cursor.Vertical += Parcel.LinePeak + Style->Spacing;
        Parcel.LinePeak = 0.0f;
    }

    Parcel.Cursor.Vertical += Amount;
}

void CLayout::Spacing( ) {
    Skip( Style->Spacing );
}

void CLayout::Dummy( CVector Extent ) {
    Place( Extent );
}

void CLayout::Indent( float Amount ) {
    Parcel.Margin += Amount > 0.0f ? Amount : Style->PaddingWide;
}

void CLayout::Unindent( float Amount ) {
    Parcel.Margin -= Amount > 0.0f ? Amount : Style->PaddingWide;

    if ( Parcel.Margin < 0.0f )
        Parcel.Margin = 0.0f;
}

void CLayout::PushWidth( float Width ) {
    Widths.push_back( Parcel.ItemWidth );
    Parcel.ItemWidth = Width;
}

void CLayout::PopWidth( ) {
    if ( Widths.empty( ) ) {
        Parcel.ItemWidth = -1.0f;
        return;
    }

    Parcel.ItemWidth = Widths.back( );
    Widths.pop_back( );
}

void CLayout::BeginGroup( ) {
    CCluster Cluster;
    Cluster.Start = Parcel.Cursor;

    Clusters.push_back( Cluster );
}

CRectangle CLayout::EndGroup( ) {
    if ( Clusters.empty( ) )
        return CRectangle( );

    CCluster Cluster = Clusters.back( );
    Clusters.pop_back( );

    CRectangle Bounds;

    if ( Cluster.Any )
        Bounds = CRectangle( Cluster.Least, Cluster.Most - Cluster.Least );
    else
        Bounds = CRectangle( Cluster.Start, CVector( 0.0f, 0.0f ) );

    Parcel.Cursor = Cluster.Start;
    Parcel.LinePeak = 0.0f;

    Parcel.Adjacent = true;
    Place( Bounds.Extent( ) );

    return Bounds;
}

bool CLayout::BeginChild( const char* Identifier, CVector Extent, bool Framed ) {
    unsigned int Key = Context->Hash( Identifier );

    float Wide = Extent.Horizontal > 0.0f ? Extent.Horizontal : Width( );
    float Tall = Extent.Vertical > 0.0f ? Extent.Vertical : 120.0f * Style->Scale;

    CRectangle Box = Place( CVector( Wide, Tall ) );

    CNook& Nook = Nooks[ Key ];
    Nook.Stamp = Context->FrameIndex;

    if ( Framed ) {
        Canvas->Rectangle( Box, Style->Groove.Fade( 0.3f ), Style->ControlRounding );

        if ( Style->Borders )
            Canvas->Border( Box, Style->Outline, Style->ControlRounding, Style->Thickness );
    }

    float Pad = Style->PaddingWide * 0.5f;
    float View = Box.Height - Pad * 2.0f;

    bool Overflow = Nook.Reach > View && View > 0.0f;
    float Most = Overflow ? Nook.Reach - View : 0.0f;

    CVector Point = Input->MousePosition;
    bool Over = Box.Contains( Point ) && Frames->Beneath == Frames->Current && !Context->Covered( Point );

    if ( Overflow && Over && Input->WheelDelta != 0.0f ) {
        Nook.Aim -= Input->WheelDelta * 48.0f * Style->Scale;
        Input->WheelDelta = 0.0f;
    }

    if ( Nook.Aim > Most )
        Nook.Aim = Most;

    if ( Nook.Aim < 0.0f )
        Nook.Aim = 0.0f;

    if ( Style->ScrollSmooth ) {
        Nook.Scroll += ( Nook.Aim - Nook.Scroll ) * ( 1.0f - expf( -Style->ScrollSpeed * Context->DeltaTime ) );

        if ( fabsf( Nook.Aim - Nook.Scroll ) < 0.4f )
            Nook.Scroll = Nook.Aim;
    } else {
        Nook.Scroll = Nook.Aim;
    }

    Saved.push_back( Parcel );

    ChildKeys.push_back( Key );
    ChildBoxes.push_back( Box );

    CRectangle Inner = Box.Pad( Pad, Pad );
    Canvas->PushClip( Box );

    Parcel = CParcel( );
    Parcel.Area = CRectangle( Inner.Left, Inner.Top - Nook.Scroll, Inner.Width, Inner.Height + Nook.Scroll );

    Parcel.Cursor = Parcel.Area.Origin( );
    return true;
}

void CLayout::EndChild( ) {
    if ( Saved.empty( ) || ChildKeys.empty( ) )
        return;

    float Content = Parcel.Depth;

    Canvas->PopClip( );

    Parcel = Saved.back( );
    Saved.pop_back( );

    unsigned int Key = ChildKeys.back( );
    CRectangle Box = ChildBoxes.back( );

    ChildKeys.pop_back( );
    ChildBoxes.pop_back( );

    CNook& Nook = Nooks[ Key ];
    Nook.Reach = Content;

    float View = Box.Height - Style->PaddingWide;

    if ( Nook.Reach <= View || View <= 0.0f )
        return;

    float Lane = Style->GrooveWidth + 1.0f * Style->Scale;
    CRectangle Track( Box.Right( ) - Lane - 2.0f * Style->Scale, Box.Top + 3.0f * Style->Scale, Lane, Box.Height - 6.0f * Style->Scale );

    float Span = Track.Height * ( View / Nook.Reach );
    if ( Span < 24.0f * Style->Scale )
        Span = 24.0f * Style->Scale;

    float Most = Nook.Reach - View;
    float Range = Track.Height - Span;

    CRectangle Thumb( Track.Left, Track.Top + ( Most > 0.0f ? Nook.Scroll / Most : 0.0f ) * Range, Lane, Span );

    Canvas->Rectangle( Track, Style->ScrollTrack, Lane * 0.5f );
    Canvas->Rectangle( Thumb, Style->ScrollThumb, Lane * 0.5f );
}

bool CLayout::BeginReveal( const char* Identifier, float Amount ) {
    unsigned int Key = Context->Hash( Identifier );

    CNook& Nook = Nooks[ Key ];
    Nook.Stamp = Context->FrameIndex;

    float Gap = Style->Spacing;
    float Shown = ( Nook.Reach + Gap ) * Amount;

    if ( Shown < 0.0f )
        Shown = 0.0f;

    if ( Parcel.LinePeak > 0.0f ) {
        Parcel.Cursor.Vertical += Parcel.LinePeak;
        Parcel.LinePeak = 0.0f;
    }

    Parcel.Cursor.Horizontal = Clusters.empty( ) ? Parcel.Area.Left + Parcel.Margin : Clusters.back( ).Start.Horizontal;
    Parcel.Adjacent = true;

    CRectangle Box = Place( CVector( Width( ), Shown ) );

    Saved.push_back( Parcel );

    ChildKeys.push_back( Key );
    ChildBoxes.push_back( Box );

    Canvas->PushClip( Box );

    Parcel = CParcel( );
    Parcel.Area = CRectangle( Box.Left, Box.Top + Gap * Amount, Box.Width, 100000.0f );

    Parcel.Cursor = Parcel.Area.Origin( );
    return true;
}

void CLayout::EndReveal( ) {
    if ( Saved.empty( ) || ChildKeys.empty( ) )
        return;

    float Content = Parcel.Depth;
    Canvas->PopClip( );

    Parcel = Saved.back( );
    Saved.pop_back( );

    unsigned int Key = ChildKeys.back( );

    ChildKeys.pop_back( );
    ChildBoxes.pop_back( );

    Nooks[ Key ].Reach = Content;
}

void CLayout::Furnish( ) {
    CTable& Table = Tables.back( );

    if ( Table.Ready )
        return;

    while ( ( int )Table.Titles.size( ) < Table.Columns )
        Table.Titles.push_back( "" );

    while ( ( int )Table.Widths.size( ) < Table.Columns )
        Table.Widths.push_back( 0.0f );

    auto Found = Layouts.find( Table.Identifier );

    if ( Found != Layouts.end( ) && ( int )Found->second.size( ) == Table.Columns ) {
        Table.Widths = Found->second;
        Table.Ready = true;

        return;
    }

    float Fixed = 0.0f;
    int Weighted = 0;

    for ( int Column = 0; Column < Table.Columns; Column++ ) {
        if ( Table.Widths[ Column ] > 0.0f )
            Fixed += Table.Widths[ Column ];
        else
            Weighted++;
    }

    float Spare = Table.Area.Width - Fixed;
    if ( Spare < 0.0f )
        Spare = 0.0f;

    float Each = Weighted > 0 ? Spare / ( float )Weighted : 0.0f;

    for ( int Column = 0; Column < Table.Columns; Column++ ) {
        if ( Table.Widths[ Column ] <= 0.0f )
            Table.Widths[ Column ] = Each;
    }

    Layouts[ Table.Identifier ] = Table.Widths;
    Table.Ready = true;
}

bool CLayout::BeginTable( const char* Identifier, int Columns, bool Border, bool Striped ) {
    if ( Columns < 1 )
        return false;

    if ( Columns > 64 )
        Columns = 64;

    CTable Table;

    Table.Identifier = Context->Hash( Identifier );
    Table.Columns = Columns;

    Table.Border = Border;
    Table.Striped = Striped;

    float Top = Parcel.Cursor.Vertical + ( Parcel.LinePeak > 0.0f ? Parcel.LinePeak + Style->Spacing : 0.0f );
    Table.Area = CRectangle( Parcel.Area.Left + Parcel.Margin, Top, Width( ), 0.0f );

    Table.RowTop = Top;

    Saved.push_back( Parcel );
    Tables.push_back( Table );

    return true;
}

void CLayout::TableSetup( const char* Title, float Width ) {
    if ( Tables.empty( ) )
        return;

    CTable& Table = Tables.back( );

    if ( ( int )Table.Titles.size( ) >= Table.Columns )
        return;

    Table.Titles.push_back( Title ? Title : "" );
    Table.Widths.push_back( Width );
}

void CLayout::TableHeaders( ) {
    if ( Tables.empty( ) )
        return;

    Furnish( );
    CTable& Table = Tables.back( );

    float Height = Style->ControlHeight;
    CRectangle Bar( Table.Area.Left, Table.RowTop, Table.Area.Width, Height );

    Canvas->Rectangle( Bar, Style->Header, 0.0f );

    CVector Point = Input->MousePosition;
    float Across = Table.Area.Left;

    for ( int Column = 0; Column < Table.Columns; Column++ ) {
        CRectangle Cell( Across, Table.RowTop, Table.Widths[ Column ], Height );

        Canvas->PushClip( Cell.Pad( Style->PaddingWide * 0.4f, 0.0f ) );
        Canvas->Text( CVector( Cell.Left + Style->PaddingWide * 0.4f, Cell.Top + ( Height - Font->LineSpan ) * 0.5f ), Style->Faint, Table.Titles[ Column ].c_str( ) );

        Canvas->PopClip( );
        Across += Table.Widths[ Column ];

        if ( !Table.Border || Column + 1 >= Table.Columns )
            continue;

        unsigned int Zone = Table.Identifier ^ ( SaltGrip * ( unsigned int )( Column + 1 ) );
        CRectangle Grip( Across - 3.0f * Style->Scale, Table.RowTop, 6.0f * Style->Scale, Height );

        bool Hovered = Grip.Contains( Point ) && Frames->Beneath == Frames->Current && !Context->Covered( Point );
        bool Held = Context->ActiveItem == Zone;

        if ( Hovered || Held )
            Input->Pointer = PointerAcross;

        if ( Hovered && Input->MousePressed( 0 ) && Context->ActiveItem == 0 ) {
            Context->ActiveItem = Zone;
            Held = true;
        }

        if ( !Held )
            continue;

        if ( !Input->MouseDown( 0 ) ) {
            Context->ActiveItem = 0;
            continue;
        }

        float Least = 32.0f * Style->Scale;
        float Shift = Point.Horizontal - Across;

        float Grown = Table.Widths[ Column ] + Shift;
        float Shrunk = Table.Widths[ Column + 1 ] - Shift;

        if ( Grown >= Least && Shrunk >= Least ) {
            Table.Widths[ Column ] = Grown;
            Table.Widths[ Column + 1 ] = Shrunk;

            Layouts[ Table.Identifier ] = Table.Widths;
        }
    }

    Table.RowTop += Height;

    if ( Table.RowTop - Parcel.Area.Top > Parcel.Depth )
        Parcel.Depth = Table.RowTop - Parcel.Area.Top;
}

void CLayout::CellFinish( ) {
    CTable& Table = Tables.back( );

    if ( Parcel.Depth > Table.RowPeak )
        Table.RowPeak = Parcel.Depth;

    Canvas->PopClip( );
    Table.CellOpen = false;
}

void CLayout::RowFinish( ) {
    CTable& Table = Tables.back( );
    float Pad = Style->PaddingTall * 0.4f;

    Table.RowTop += Table.RowPeak + Pad * 2.0f;

    if ( Table.Border )
        Canvas->Rectangle( CRectangle( Table.Area.Left, Table.RowTop, Table.Area.Width, Style->Thickness ), Style->Outline.Fade( 0.5f ), 0.0f );

    Table.RowOpen = false;
}

void CLayout::TableRow( ) {
    if ( Tables.empty( ) )
        return;

    CTable& Table = Tables.back( );

    if ( Table.CellOpen )
        CellFinish( );

    if ( Table.RowOpen )
        RowFinish( );

    Table.Row++;

    Table.RowOpen = true;
    Table.Column = -1;

    Table.RowPeak = 0.0f;
}

bool CLayout::TableColumn( ) {
    if ( Tables.empty( ) )
        return false;

    CTable& Table = Tables.back( );

    if ( !Table.RowOpen )
        return false;

    if ( Table.CellOpen )
        CellFinish( );

    Table.Column++;
    if ( Table.Column >= Table.Columns )
        return false;

    float Across = Table.Area.Left;

    for ( int Column = 0; Column < Table.Column; Column++ )
        Across += Table.Widths[ Column ];

    float Pad = Style->PaddingWide * 0.4f;
    float Tall = Style->PaddingTall * 0.4f;

    CRectangle Cell( Across, Table.RowTop, Table.Widths[ Table.Column ], 100000.0f );
    Canvas->PushClip( Cell );

    Parcel.Area = CRectangle( Across + Pad, Table.RowTop + Tall, Table.Widths[ Table.Column ] - Pad * 2.0f, 100000.0f );
    Parcel.Cursor = Parcel.Area.Origin( );

    Parcel.LinePeak = 0.0f;
    Parcel.Depth = 0.0f;

    Parcel.Margin = 0.0f;
    Parcel.ItemWidth = -1.0f;

    Parcel.Adjacent = false;
    Table.CellOpen = true;

    return true;
}

void CLayout::EndTable( ) {
    if ( Tables.empty( ) )
        return;

    if ( Tables.back( ).CellOpen )
        CellFinish( );

    if ( Tables.back( ).RowOpen )
        RowFinish( );

    CTable Table = Tables.back( );
    Tables.pop_back( );

    if ( !Saved.empty( ) ) {
        Parcel = Saved.back( );
        Saved.pop_back( );
    }

    float Bottom = Table.RowTop;
    float Height = Bottom - Table.Area.Top;

    if ( Table.Border && Height > 0.0f ) {
        Canvas->Border( CRectangle( Table.Area.Left, Table.Area.Top, Table.Area.Width, Height ), Style->Outline, 0.0f, Style->Thickness );

        float Across = Table.Area.Left;

        for ( int Column = 0; Column + 1 < Table.Columns; Column++ ) {
            Across += Table.Widths[ Column ];
            Canvas->Rectangle( CRectangle( Across, Table.Area.Top, Style->Thickness, Height ), Style->Outline.Fade( 0.5f ), 0.0f );
        }
    }

    Parcel.Adjacent = false;
    Parcel.Cursor.Horizontal = Parcel.Area.Left + Parcel.Margin;

    Parcel.Cursor.Vertical = Bottom;
    Parcel.LinePeak = 0.0f;

    if ( Bottom - Parcel.Area.Top > Parcel.Depth )
        Parcel.Depth = Bottom - Parcel.Area.Top;
}

float CLayout::Width( ) const {
    if ( Parcel.ItemWidth > 0.0f )
        return Parcel.ItemWidth;

    return Parcel.Area.Width - Parcel.Margin;
}

float CLayout::Left( ) const {
    return Parcel.Area.Left + Parcel.Margin;
}

CVector CLayout::Where( ) const {
    return Parcel.Cursor;
}

float CLayout::Remaining( ) const {
    return Parcel.Area.Top + Parcel.Area.Height - Parcel.Cursor.Vertical;
}

float CLayout::ChildScroll( ) const {
    if ( ChildKeys.empty( ) )
        return 0.0f;

    auto Found = Nooks.find( ChildKeys.back( ) );
    if ( Found == Nooks.end( ) )
        return 0.0f;

    return Found->second.Scroll;
}

float CLayout::ChildView( ) const {
    if ( ChildBoxes.empty( ) )
        return 0.0f;

    return ChildBoxes.back( ).Height;
}