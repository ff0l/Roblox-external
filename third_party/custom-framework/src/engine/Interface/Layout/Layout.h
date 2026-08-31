#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "Geometry.h"

class CParcel {
public:
    CRectangle Area;
    CVector Cursor;

    float LinePeak = 0.0f;
    float Depth = 0.0f;

    float Margin = 0.0f;
    float ItemWidth = -1.0f;

    bool Adjacent = false;
};

class CCluster {
public:
    CVector Start;

    CVector Least;
    CVector Most;

    bool Any = false;
};

class CNook {
public:
    float Scroll = 0.0f;
    float Aim = 0.0f;

    float Reach = 0.0f;
    unsigned int Stamp = 0;
};

class CTable {
public:
    unsigned int Identifier = 0;
    CRectangle Area;

    int Columns = 0;
    int Column = 0;

    int Row = 0;

    float RowTop = 0.0f;
    float RowPeak = 0.0f;

    bool RowOpen = false;
    bool CellOpen = false;

    bool Border = true;
    bool Striped = true;

    bool Ready = false;

    std::vector< float > Widths;
    std::vector< std::string > Titles;
};

class CLayout {
public:
    void Begin( CRectangle Region );
    float End( );

    CRectangle Place( CVector Extent );
    CRectangle Stretch( float Height );

    void Beside( );
    void SameLine( float Offset = 0.0f, float Gap = -1.0f );

    void Skip( float Amount );
    void Spacing( );

    void Dummy( CVector Extent );

    void Indent( float Amount = 0.0f );
    void Unindent( float Amount = 0.0f );

    void PushWidth( float Width );
    void PopWidth( );

    void BeginGroup( );
    CRectangle EndGroup( );

    bool BeginChild( const char* Identifier, CVector Extent, bool Framed = true );
    void EndChild( );

    bool BeginReveal( const char* Identifier, float Amount );
    void EndReveal( );

    bool BeginTable( const char* Identifier, int Columns, bool Border = true, bool Striped = true );
    void TableSetup( const char* Title, float Width = 0.0f );

    void TableHeaders( );
    void TableRow( );

    bool TableColumn( );
    void EndTable( );

    void Sweep( );
    void Destroy( );

    float Width( ) const;
    float Left( ) const;

    CVector Where( ) const;
    float Remaining( ) const;

    float ChildScroll( ) const;
    float ChildView( ) const;

private:
    void Fold( CRectangle Bounds );
    void Furnish( );

    void RowFinish( );
    void CellFinish( );

    CParcel Parcel;

    std::vector< CParcel > Saved;
    std::vector< CCluster > Clusters;

    std::vector< unsigned int > ChildKeys;
    std::vector< CRectangle > ChildBoxes;

    std::vector< float > Widths;
    std::vector< CTable > Tables;

    std::unordered_map< unsigned int, CNook > Nooks;
    std::unordered_map< unsigned int, std::vector< float > > Layouts;
};

inline auto Layout = std::make_unique< CLayout >( );