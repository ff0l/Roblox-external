#pragma once

#include <iosfwd>
#include <memory>
#include <vector>

#include "Geometry.h"

class CDockNode {
public:
    unsigned int Parent = 0;

    unsigned int First = 0;
    unsigned int Second = 0;

    unsigned int Front = 0;
    unsigned int Anchor = 0;

    int Channel = 0;
    float Ratio = 0.5f;

    bool Vertical = false;

    bool Used = false;
    bool Keep = false;

    CRectangle Bounds;
    CRectangle Handle;

    std::vector< unsigned int > Tabs;

    bool Leaf( ) const {
        return First == 0;
    }
};

class CDocking {
public:
    void Space( CRectangle Bounds );
    bool Adopt( const char* Title, int Zone );

    void Detach( unsigned int Window );
    void Destroy( );

    void Save( std::ostream& Stream ) const;
    void Load( std::istream& Stream );

    bool Enabled = true;

private:
    unsigned int Allocate( );
    bool Repair( );

    bool HasTabs( unsigned int Node ) const;
    void Prune( );

    void Fold( unsigned int Node );
    void Arrange( unsigned int Node, CRectangle Bounds, unsigned int Anchor );

    void Steer( );
    void Islands( );

    void Splitters( );

    void Leaves( );
    void Preview( );

    void Insert( unsigned int Window, unsigned int Node, int Zone );
    void Quarter( unsigned int Node, bool WestSide );

    std::vector< CDockNode > Nodes;
    std::vector< unsigned int > Isles;

    unsigned int Root = 0;

    CRectangle Area;
    CRectangle Ghost;

    CVector Hold;
    float Flow = 0.0f;
};

inline auto Docking = std::make_unique< CDocking >( );