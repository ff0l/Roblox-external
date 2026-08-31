#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "Geometry.h"

class CMotion {
public:
    float Glow = 0.0f;
    float Slide = 0.0f;

    unsigned int Stamp = 0;
    bool Seen = false;
};

class CEditor {
public:
    int Caret = 0;
    int Mark = 0;

    float Blink = 0.0f;
    float Scroll = 0.0f;

    float Pen = 0.0f;
    unsigned int Stamp = 0;

    char Local[ 64 ] = { };

    std::vector< std::string > History;
    std::vector< int > Carets;

    int At = 0;
};

class CTwig {
public:
    float Rail = 0.0f;

    float Head = 0.0f;
    float Foot = 0.0f;
};

class CPanel {
public:
    unsigned int Identifier = 0;
    CRectangle Zone;

    CVector Cursor;
    float Widest = 0.0f;

    float Grown = 1.0f;
    int Former = 0;

    float Opacity = 1.0f;
    bool Bar = false;

    bool Opened = false;
};

class CWidgets {
public:
    void Destroy( );
    void Sweep( );

    void Label( const char* Message );
    void Faint( const char* Message );

    void Heading( const char* Message );
    void Section( const char* Message );

    void Wrapped( const char* Message );
    void Bullet( const char* Message );

    void Colored( CColor Tint, const char* Message );

    bool Button( const char* Title, float Width = 0.0f );
    bool IconButton( const char* Title, unsigned long long Image, float Size, bool Active = false );
    bool Small( const char* Title );

    bool Check( const char* Title, bool& State );
    bool Toggle( const char* Title, bool& State );

    bool Radio( const char* Title, int& Selected, int Option );
    bool Slider( const char* Title, float& Current, float Minimum, float Maximum );

    bool SliderWhole( const char* Title, int& Current, int Minimum, int Maximum );
    bool Stars( const char* Title, int& Current, int Maximum = 5 );
    bool Drag( const char* Title, float& Current, float Speed, float Minimum, float Maximum, const char* Format = "%.3f" );

    bool DragWhole( const char* Title, int& Current, float Speed, int Minimum, int Maximum );
    bool Vector( const char* Title, float* Values, int Count, float Minimum, float Maximum );

    bool Number( const char* Title, int& Value, int Step = 1 );
    bool Decimal( const char* Title, float& Value, float Step = 0.0f, const char* Format = "%.3f" );

    bool Field( const char* Title, char* Buffer, int Capacity, const char* Hint = nullptr );
    bool Area( const char* Title, char* Buffer, int Capacity, float Height = 0.0f );

    bool Choice( const char* Title, int& Selected, const char* const* Names, int Count );
    bool Segments( const char* Title, int& Selected, const char* const* Names, int Count );

    bool Tree( const char* Title, unsigned long long Image = 0, bool Start = false );
    bool TreeLeaf( const char* Title, bool Selected = false );

    void TreePop( );

    bool Collapsing( const char* Title, bool Open = true, unsigned long long Image = 0 );

    bool BeginCollapse( const char* Title, bool Open = true );
    void EndCollapse( );

    bool Selectable( const char* Title, bool Selected, float Height = 0.0f );

    bool List( const char* Title, int& Selected, const char* const* Names, int Count, int Rows = 5 );
    bool Color( const char* Title, CColor& Value, bool Alpha = true );

    bool BeginTabs( const char* Identifier );
    bool Tab( const char* Title, bool* Open = nullptr );

    void EndTabs( );

    void Plot( const char* Title, const float* Values, int Count, float Minimum = 0.0f, float Maximum = 0.0f );
    void Histogram( const char* Title, const float* Values, int Count, float Minimum = 0.0f, float Maximum = 0.0f );

    void Area( const char* Title, const float* Values, int Count, float Minimum = 0.0f, float Maximum = 0.0f );
    void Pie( const char* Title, const float* Values, int Count, bool Donut = false );

    bool BeginMenuBar( );
    void EndMenuBar( );

    bool BeginMenu( const char* Title );
    void EndMenu( );

    bool MenuItem( const char* Title, const char* Shortcut = nullptr, bool Selected = false );

    void OpenPopup( const char* Identifier );
    bool BeginPopup( const char* Identifier );

    void EndPopup( );

    CRectangle Picture( unsigned long long Image, CVector Extent );
    void Progress( float Fraction );

    void Separator( );
    void Tooltip( const char* Message );

    void BeginDisabled( bool Off = true );
    void EndDisabled( );
    bool Disabled( ) const;

    void PushAlpha( float Amount );
    void PopAlpha( );

    bool Field( const char* Title, std::string& Value, const char* Hint = nullptr );
    bool Area( const char* Title, std::string& Value, float Height = 0.0f );

    bool Knob( const char* Title, float& Current, float Minimum, float Maximum );
    bool Keybind( const char* Title, int& Key );
    bool Splitter( float& Ratio );
    bool FilterList( const char* Title, int& Selected, const char* const* Names, int Count, char* Query, int QueryCapacity, int Rows = 8 );
    bool BeginVirtual( const char* Title, int Count, float RowHeight, int& First, int& Last, float Height = 0.0f );
    void EndVirtual( );

    void Meter( float Fraction );
    void Waveform( const float* Values, int Count );
    void Spectrum( const float* Values, int Count );

    bool BeginModal( const char* Title, bool* Open, CVector Extent = CVector( 420.0f, 240.0f ) );
    void EndModal( );

    CRectangle LastItem( ) const;
    const char* LastName( ) const;
    bool Hit( const char* Title, CRectangle Bounds, bool& Hovered, bool& Held );

private:
    bool Behavior( unsigned int Identifier, CRectangle Bounds, bool& Hovered, bool& Held );

    CMotion& Motion( unsigned int Identifier );
    float Ease( unsigned int Identifier, bool Hovered );

    CEditor& Editor( unsigned int Identifier );
    bool Edit( CEditor& State, char* Buffer, int Capacity, bool Multiline );

    void Snapshot( CEditor& State, const char* Buffer );
    bool Spinner( const char* Title, CRectangle Bounds, double& Value, double Step, bool Whole );

    CRectangle Chart( const char* Title, float Height );

    bool DragBox( unsigned int Identifier, CRectangle Box, float& Value, float Speed, float Minimum, float Maximum, const char* Pattern );

    void PanelOpen( CRectangle Zone );
    void PanelClose( );

    void Connect( CRectangle Bounds );

    std::unordered_map< unsigned int, CMotion > Motions;
    std::unordered_map< unsigned int, CEditor > Editors;

    std::unordered_map< unsigned int, unsigned char > Opens;
    std::unordered_map< unsigned int, unsigned int > Picks;

    std::vector< CPanel > Panels;
    std::vector< CTwig > Twigs;

    std::vector< unsigned int > MenuChain;

    std::unordered_map< unsigned int, CVector > Extents;
    CVector PopupAt;

    unsigned int Reveal = 0;
    unsigned int Popup = 0;

    unsigned int Latest = 0;
    unsigned int Aimed = 0;

    float Dwell = 0.0f;
    bool MenuTouched = false;

    std::vector< unsigned char > DisableStack;
    std::vector< float > AlphaStack;
    int VirtualCount = 0;

    CRectangle LatestBounds;
    const char* LatestName = "";
};

inline auto Widgets = std::make_unique< CWidgets >( );