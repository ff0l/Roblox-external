#pragma once

#include <memory>
#include <string>
#include <vector>
#include <fstream>

#include "Font.h"
#include "Arena.h"
#include "Input.h"
#include "Sheet.h"
#include "Style.h"
#include "Canvas.h"
#include "Format.h"
#include "Frames.h"
#include "Layout.h"
#include "Native.h"
#include "Context.h"
#include "Docking.h"
#include "Shaders.h"
#include "Widgets.h"
#include "Geometry.h"
#include "Pictures.h"

class CEngine {
public:
    bool Create( const char* const* Families = nullptr, int FamilyCount = 0, float Size = 16.0f ) {
        Style->Dark( );

        if ( !Sheet->Create( ) )
            return false;

        const char* Default[ 5 ] = { "Segoe UI Variable Display", "Segoe UI", "Segoe UI Symbol", "Segoe UI Emoji", "Arial" };
        const char* const* Faces = Families && FamilyCount > 0 ? Families : Default;
        int Count = Families && FamilyCount > 0 ? FamilyCount : 5;
        float Body = Size > 0.0f ? Size : 16.0f;

        if ( !Font->Create( Faces, Count, Body, 400 ) )
            return false;

        if ( !Heading->Create( Faces, Count, Body + 4.0f, 600 ) )
            return false;

        if ( !Arena->Create( 1048576 ) )
            return false;

        return Context->Create( );
    }

    void Destroy( ) {
        Widgets->Destroy( );
        Docking->Destroy( );

        Frames->Destroy( );
        Layout->Destroy( );

        Heading->Destroy( );
        Font->Destroy( );

        Sheet->Destroy( );
        Arena->Destroy( );

        Context->Destroy( );
    }

    void Rescale( float Factor ) {
        Style->Rescale( Factor );

        Font->Rescale( Factor );
        Heading->Rescale( Factor );
    }

    void Begin( CVector Display ) {
        Context->Begin( );
        Context->Display = Display;

        Input->Pointer = PointerArrow;
        Arena->Reset( );

        Widgets->Sweep( );
        Layout->Sweep( );

        Frames->Update( );
        Canvas->Begin( Display );
    }

    void End( ) {
        Sequence.clear( );
        Sequence.push_back( 0 );

        for ( unsigned int Identifier : Frames->Order( ) ) {
            CFrameState* State = Frames->Find( Identifier );

            if ( !State )
                continue;

            bool Listed = false;

            for ( int Channel : Sequence ) {
                if ( Channel == State->Channel )
                    Listed = true;
            }

            if ( !Listed )
                Sequence.push_back( State->Channel );
        }

        Sequence.push_back( OverlayRoute );
        Canvas->Arrange( Sequence.data( ), ( int )Sequence.size( ) );

        if ( !Input->MouseDown( 0 ) )
            Context->ActiveItem = 0;

        if ( Input->MousePressed( 0 ) && !Context->FocusClaimed )
            Context->FocusedItem = 0;

        Input->Settle( );
    }

    const CDrawData& Data( ) const {
        return Canvas->Data( );
    }

    void Diagnostics( bool* Open = nullptr ) {
        if ( Frames->Begin( "Profiler", Open, FrameDefault, CVector( 798.0f, 76.0f ), CVector( 372.0f, 356.0f ) ) ) {
            Widgets->Heading( Format->Print( "%.0f FPS", ( double )Context->Framerate ) );
            Widgets->Faint( Format->Print( "%.2f ms per frame", ( double )( Context->DeltaTime * 1000.0f ) ) );

            Widgets->Section( "Draw" );

            Widgets->Label( Format->Print( "%d instances", Canvas->LastCount ) );
            Widgets->Label( Format->Print( "%d draw calls", Canvas->LastBatches ) );

            Widgets->Progress( ( float )Canvas->LastBatches / 32.0f );

            Widgets->Section( "Interaction" );

            Widgets->Label( Format->Print( "hovered  %08X", Context->HoveredItem ) );
            Widgets->Label( Format->Print( "active   %08X", Context->ActiveItem ) );

            Widgets->Label( Format->Print( "focused  %08X", Context->FocusedItem ) );

            Widgets->Section( "Faults" );
            Widgets->Label( Format->Print( "%d recorded", ( int )Context->Faults.size( ) ) );

            if ( !Context->Faults.empty( ) )
                Widgets->Faint( Context->Faults.back( ).Message.c_str( ) );
        }

        Frames->End( );
    }

    bool Save( const char* Path ) {
        std::ofstream Stream( Path );

        if ( !Stream )
            return false;

        Stream << "UR 5\n";

        Frames->Save( Stream );
        Docking->Save( Stream );

        return true;
    }

    bool Load( const char* Path ) {
        std::ifstream Stream( Path );

        if ( !Stream )
            return false;

        std::string Tag;
        int Version = 0;

        Stream >> Tag >> Version;
        if ( Tag != "UR" || Version < 5 )
            return false;

        Frames->Load( Stream );
        Docking->Load( Stream );

        return true;
    }

private:
    std::vector< int > Sequence;
};

inline auto Engine = std::make_unique< CEngine >( );