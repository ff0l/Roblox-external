#include <cmath>
#include <string>
#include <istream>
#include <ostream>
#include <cstring>
#include <algorithm>

#include "Font.h"
#include "Input.h"
#include "Canvas.h"
#include "Frames.h"
#include "Layout.h"

#include "Style.h"
#include "Context.h"

static constexpr unsigned int SaltAcross = 0x1B873593u;
static constexpr unsigned int SaltDown = 0x85EBCA6Bu;
static constexpr unsigned int SaltSlant = 0xC2B2AE35u;
static constexpr unsigned int SaltFold = 0x27D4EB2Fu;
static constexpr unsigned int SaltClose = 0x9E3779B9u;
static constexpr unsigned int SaltScroll = 0x165667B1u;

void CFrames::Update( ) {
    Current = 0;
    Producing = false;

    std::stable_partition( Stack.begin( ), Stack.end( ), [ this ]( unsigned int Identifier ) {
        auto Entry = States.find( Identifier );
        return Entry != States.end( ) && Entry->second.Home != 0 && Entry->second.Group == 0;
    } );

    for ( auto Entry = States.begin( ); Entry != States.end( ); ) {
        CFrameState& State = Entry->second;
        bool Gone = Context->FrameIndex - State.Touch > 300 && State.Home == 0 && State.Group == 0;

        if ( Gone ) {
            unsigned int Identifier = Entry->first;
            Stack.erase( std::remove( Stack.begin( ), Stack.end( ), Identifier ), Stack.end( ) );

            if ( State.Channel > 0 )
                Spares.push_back( State.Channel );

            Entry = States.erase( Entry );
        } else {
            Entry++;
        }
    }

    Beneath = Topmost( Input->MousePosition );
}

void CFrames::Destroy( ) {
    States.clear( );
    Stack.clear( );

    Spares.clear( );

    NextChannel = 1;
    Current = 0;

    Beneath = 0;
    Producing = false;
}

void CFrames::Raise( unsigned int Identifier ) {
    auto Entry = States.find( Identifier );
    unsigned int Group = Entry != States.end( ) ? Entry->second.Group : 0;

    if ( Group != 0 ) {
        std::vector< unsigned int > Members;

        for ( size_t Position = 0; Position < Stack.size( ); ) {
            auto Other = States.find( Stack[ Position ] );

            if ( Other != States.end( ) && Other->second.Group == Group ) {
                Members.push_back( Stack[ Position ] );
                Stack.erase( Stack.begin( ) + Position );
            } else {
                Position++;
            }
        }

        Stack.insert( Stack.end( ), Members.begin( ), Members.end( ) );
        return;
    }

    auto Spot = std::find( Stack.begin( ), Stack.end( ), Identifier );
    if ( Spot == Stack.end( ) ) {
        Stack.push_back( Identifier );
        return;
    }

    Stack.erase( Spot );
    Stack.push_back( Identifier );
}

unsigned int CFrames::Topmost( CVector Point ) const {
    unsigned int Result = 0;

    for ( unsigned int Identifier : Stack ) {
        auto Entry = States.find( Identifier );
        if ( Entry == States.end( ) )
            continue;

        if ( Context->FrameIndex - Entry->second.Stamp > 1 )
            continue;

        if ( Entry->second.Bounds.Contains( Point ) )
            Result = Identifier;
    }

    return Result;
}

CFrameState* CFrames::Find( unsigned int Identifier ) {
    auto Entry = States.find( Identifier );
    if ( Entry == States.end( ) )
        return nullptr;

    return &Entry->second;
}

const std::vector< unsigned int >& CFrames::Order( ) const {
    return Stack;
}

void CFrames::Save( std::ostream& Stream ) const {
    Stream << "frames " << States.size( ) << "\n";

    for ( const auto& Pair : States ) {
        const CFrameState& State = Pair.second;
        Stream << Pair.first << " " << State.Position.Horizontal << " " << State.Position.Vertical << " " << State.Size.Horizontal << " " << State.Size.Vertical << " " << ( State.Collapsed ? 1 : 0 ) << " " << State.Home << " " << State.Group << " " << State.Channel << "\n";
    }
}

void CFrames::Load( std::istream& Stream ) {
    std::string Tag;
    size_t Count = 0;

    Stream >> Tag >> Count;
    if ( Tag != "frames" || !Stream || Count > 512 )
        return;

    for ( size_t Index = 0; Index < Count; Index++ ) {
        unsigned int Identifier = 0;

        float Left = 0.0f;
        float Top = 0.0f;

        float Wide = 0.0f;
        float Tall = 0.0f;

        int Folded = 0;
        unsigned int Home = 0;

        unsigned int Group = 0;
        int Channel = 0;

        Stream >> Identifier >> Left >> Top >> Wide >> Tall >> Folded >> Home >> Group >> Channel;
        if ( !Stream )
            return;

        if ( !std::isfinite( Left ) || !std::isfinite( Top ) ) {
            Left = 60.0f;
            Top = 60.0f;
        }

        if ( !std::isfinite( Wide ) || Wide < 160.0f || Wide > 16384.0f )
            Wide = 380.0f;

        if ( !std::isfinite( Tall ) || Tall < Style->TitleHeight || Tall > 16384.0f )
            Tall = 320.0f;

        if ( Channel < 1 || Channel > 4096 ) {
            Channel = NextChannel;
            NextChannel++;
        }

        CFrameState& State = States[ Identifier ];

        State.Position = CVector( Left, Top );
        State.Size = CVector( Wide, Tall );

        State.Collapsed = Folded != 0;
        State.Collapse = Folded ? 1.0f : 0.0f;

        State.Home = Home;
        State.Group = Group;

        State.Channel = Channel;
        State.Ready = true;

        State.Reveal = 1.0f;
        State.Touch = Context->FrameIndex;

        State.Stamp = Context->FrameIndex;
        State.HomeStamp = Context->FrameIndex;
        State.Loose = true;

        Spares.erase( std::remove( Spares.begin( ), Spares.end( ), Channel ), Spares.end( ) );

        if ( Channel >= NextChannel )
            NextChannel = Channel + 1;

        if ( std::find( Stack.begin( ), Stack.end( ), Identifier ) == Stack.end( ) )
            Stack.push_back( Identifier );
    }
}

bool CFrames::Tap( unsigned int Identifier, CRectangle Bounds, bool& Hovered ) {
    CVector Point = Input->MousePosition;

    Hovered = Bounds.Contains( Point ) && Beneath == Current && !Context->Covered( Point );
    if ( Hovered && Input->MousePressed( 0 ) && Context->ActiveItem == 0 )
        Context->ActiveItem = Identifier;

    bool Held = Context->ActiveItem == Identifier;
    bool Clicked = Held && Hovered && Input->MouseReleased( 0 );

    if ( Held && Input->MouseReleased( 0 ) )
        Context->ActiveItem = 0;

    return Clicked;
}

void CFrames::Resize( unsigned int Identifier, CFrameState& State ) {
    CVector Point = Input->MousePosition;
    CRectangle Bounds( State.Position, State.Size );

    float Grip = Style->GripSize;

    CRectangle Corner( Bounds.Right( ) - Grip, Bounds.Bottom( ) - Grip, Grip, Grip );
    CRectangle Side( Bounds.Right( ) - 5.0f, Bounds.Top + Style->TitleHeight, 5.0f, Bounds.Height - Style->TitleHeight - Grip );

    CRectangle Floor( Bounds.Left, Bounds.Bottom( ) - 5.0f, Bounds.Width - Grip, 5.0f );

    unsigned int Zones[ 3 ] = { Identifier ^ SaltSlant, Identifier ^ SaltAcross, Identifier ^ SaltDown };
    CRectangle Regions[ 3 ] = { Corner, Side, Floor };

    int Pointers[ 3 ] = { PointerSlant, PointerAcross, PointerDown };

    for ( int Zone = 0; Zone < 3; Zone++ ) {
        bool Hovered = Regions[ Zone ].Contains( Point ) && Beneath == Identifier && !Context->Covered( Point );
        bool Held = Context->ActiveItem == Zones[ Zone ];

        if ( Hovered || Held )
            Input->Pointer = Pointers[ Zone ];

        if ( Hovered && Input->MousePressed( 0 ) && Context->ActiveItem == 0 ) {
            Context->ActiveItem = Zones[ Zone ];
            State.Grab = CVector( Bounds.Right( ) - Point.Horizontal, Bounds.Bottom( ) - Point.Vertical );

            Held = true;
        }

        if ( !Held )
            continue;

        if ( !Input->MouseDown( 0 ) ) {
            Context->ActiveItem = 0;
            continue;
        }

        float Reach = Point.Horizontal + State.Grab.Horizontal - State.Position.Horizontal;
        float Drop = Point.Vertical + State.Grab.Vertical - State.Position.Vertical;

        if ( Zone != 2 && Reach >= 160.0f ) {
            State.Size.Horizontal = Reach;
            State.Loose = true;
        }

        if ( Zone != 1 && Drop >= Style->TitleHeight + 48.0f ) {
            State.Size.Vertical = Drop;
            State.Loose = true;
        }
    }
}

void CFrames::Content( unsigned int Identifier, CFrameState& State, CRectangle Region, float Tall ) {
    CVector Point = Input->MousePosition;
    CRectangle Padded = Region.Pad( Style->PaddingWide, Tall );

    State.Inner = Region;
    State.View = Padded.Height;

    bool Overflow = State.Reach > State.View + 1.0f && State.View > 0.0f;
    float Most = Overflow ? State.Reach - State.View : 0.0f;

    if ( State.Aim > Most )
        State.Aim = Most;

    if ( State.Aim < 0.0f )
        State.Aim = 0.0f;

    if ( Overflow ) {
        float Lane = Style->GrooveWidth + 1.0f * Style->Scale;

        CRectangle Track( Region.Right( ) - Lane - 3.0f * Style->Scale, Region.Top + 4.0f * Style->Scale, Lane, Region.Height - 8.0f * Style->Scale );
        float Span = Track.Height * ( State.View / State.Reach );

        if ( Span < 26.0f * Style->Scale )
            Span = 26.0f * Style->Scale;

        float Range = Track.Height - Span;
        CRectangle Thumb( Track.Left, Track.Top + ( Most > 0.0f ? State.Scroll / Most : 0.0f ) * Range, Lane, Span );

        unsigned int Zone = Identifier ^ SaltScroll;

        bool Hovered = Thumb.Inflate( 5.0f ).Contains( Point ) && Beneath == Identifier && !Context->Covered( Point );
        bool Held = Context->ActiveItem == Zone;

        if ( Hovered && Input->MousePressed( 0 ) && Context->ActiveItem == 0 ) {
            Context->ActiveItem = Zone;
            State.Grab.Vertical = Point.Vertical - Thumb.Top;

            Held = true;
        }

        if ( Held ) {
            if ( Input->MouseDown( 0 ) ) {
                float Along = Point.Vertical - Track.Top - State.Grab.Vertical;

                if ( Range > 0.0f )
                    State.Aim = Along / Range * Most;

                if ( State.Aim > Most )
                    State.Aim = Most;

                if ( State.Aim < 0.0f )
                    State.Aim = 0.0f;

                State.Scroll = State.Aim;
            } else {
                Context->ActiveItem = 0;
            }
        }
    } else {
        State.Aim = 0.0f;
    }

    if ( Style->ScrollSmooth ) {
        State.Scroll += ( State.Aim - State.Scroll ) * ( 1.0f - expf( -Style->ScrollSpeed * Context->DeltaTime ) );

        if ( fabsf( State.Aim - State.Scroll ) < 0.4f )
            State.Scroll = State.Aim;
    } else {
        State.Scroll = State.Aim;
    }

    if ( State.Scroll > Most )
        State.Scroll = Most;

    if ( State.Scroll < 0.0f )
        State.Scroll = 0.0f;

    Layout->Begin( CRectangle( Padded.Left, Padded.Top - State.Scroll, Padded.Width, Padded.Height + State.Scroll ) );
}

bool CFrames::Begin( const char* Title, bool* Open, unsigned int Options, CVector Anchor, CVector Extent ) {
    unsigned int Identifier = Context->Hash( Title );
    CFrameState& State = States[ Identifier ];

    State.Touch = Context->FrameIndex;

    if ( !State.Ready ) {
        State.Position = Anchor;
        State.Size = Extent;

        if ( !Spares.empty( ) ) {
            State.Channel = Spares.back( );
            Spares.pop_back( );
        } else {
            State.Channel = NextChannel;
            NextChannel++;
        }

        State.Ready = true;
    }

    const char* Shown = Context->Caption( Title );
    strncpy_s( State.Title, Shown ? Shown : "", _TRUNCATE );
    State.Options = Options;

    if ( Options & FramePin ) {
        State.Position = Anchor;
        State.Size.Horizontal = Extent.Horizontal;
        State.Ceiling = Extent.Vertical;
        if ( !( Options & FrameFit ) )
            State.Size.Vertical = Extent.Vertical;
        else if ( State.Size.Vertical < 8.0f )
            State.Size.Vertical = 160.0f * Style->Scale;
        if ( State.Ceiling > 8.0f && State.Size.Vertical > State.Ceiling )
            State.Size.Vertical = State.Ceiling;
    }

    Current = Identifier;
    Producing = false;

    if ( Open && !*Open ) {
        State.Dragging = false;
        State.Reveal = 0.0f;

        return false;
    }

    State.Stamp = Context->FrameIndex;

    if ( std::find( Stack.begin( ), Stack.end( ), Identifier ) == Stack.end( ) )
        Stack.push_back( Identifier );

    CVector Point = Input->MousePosition;

    if ( State.Home != 0 && Context->FrameIndex - State.HomeStamp > 8 ) {
        State.Home = 0;
        State.Group = 0;

        State.DockFront = false;
    }

    if ( State.Home != 0 ) {
        State.Dragging = false;
        State.Reveal = 1.0f;

        State.Collapse = 0.0f;
        State.Collapsed = false;

        if ( State.Group != 0 && Input->MousePressed( 0 ) && Context->ActiveItem == 0 && Beneath == Identifier && !Context->Covered( Point ) )
            Raise( Identifier );

        if ( !State.DockFront )
            return false;

        CRectangle Region = State.Bounds;

        Canvas->Route( State.Channel );
        Canvas->Rectangle( Region, Style->Surface, 0.0f );

        Canvas->PushClip( Region );
        Context->PushOwner( Identifier );

        Content( Identifier, State, Region, Style->PaddingTall );

        Producing = true;
        return true;
    }

    float Shortened = Style->TitleHeight + ( State.Size.Vertical - Style->TitleHeight ) * ( 1.0f - State.Collapse );

    CRectangle Grip( State.Position.Horizontal, State.Position.Vertical, State.Size.Horizontal, Style->TitleHeight );
    CRectangle Whole( State.Position, CVector( State.Size.Horizontal, Shortened ) );

    if ( Input->MousePressed( 0 ) && Context->ActiveItem == 0 && Beneath == Identifier && Whole.Contains( Point ) && !Context->Covered( Point ) )
        Raise( Identifier );

    Canvas->Route( State.Channel );

    float Slot = 26.0f * Style->Scale;
    float Inset = ( Style->TitleHeight - Slot ) * 0.5f;

    CRectangle Shut( Grip.Right( ) - Slot - Inset, Grip.Top + Inset, Slot, Slot );
    CRectangle Fold = Shut;

    if ( ( Options & FrameClose ) && Open )
        Fold = CRectangle( Shut.Left - Slot - 6.0f * Style->Scale, Shut.Top, Slot, Slot );

    bool FoldHovered = false;
    bool ShutHovered = false;

    bool FoldClicked = false;
    bool ShutClicked = false;

    if ( Options & FrameCollapse )
        FoldClicked = Tap( Identifier ^ SaltFold, Fold, FoldHovered );

    if ( ( Options & FrameClose ) && Open )
        ShutClicked = Tap( Identifier ^ SaltClose, Shut, ShutHovered );

    if ( FoldClicked )
        State.Collapsed = !State.Collapsed;

    if ( ShutClicked )
        *Open = false;

    if ( ( Options & FrameCollapse ) && Input->MouseDouble( 0 ) && Grip.Contains( Point ) && Beneath == Identifier && !FoldHovered && !ShutHovered )
        State.Collapsed = !State.Collapsed;

    if ( ( Options & FrameMove ) && Input->MousePressed( 0 ) && Beneath == Identifier && Grip.Contains( Point ) && !Context->Covered( Point ) && Context->ActiveItem == 0 ) {
        Context->ActiveItem = Identifier;
        State.Grab = Point - State.Position;
    }

    if ( Context->ActiveItem == Identifier ) {
        if ( Input->MouseDown( 0 ) ) {
            State.Position = Point - State.Grab;
            State.Dragging = ( Options & FrameDock ) != 0;
        } else {
            Context->ActiveItem = 0;
            State.Dragging = false;
        }
    } else {
        State.Dragging = false;
    }

    if ( ( Options & FrameResize ) && !State.Collapsed )
        Resize( Identifier, State );

    if ( Context->Display.Horizontal > 0.0f && Context->Display.Vertical > 0.0f ) {
        float LeastLeft = 60.0f - State.Size.Horizontal;
        float MostLeft = Context->Display.Horizontal - 60.0f;

        float MostTop = Context->Display.Vertical - Style->TitleHeight;

        if ( State.Position.Horizontal < LeastLeft )
            State.Position.Horizontal = LeastLeft;

        if ( State.Position.Horizontal > MostLeft )
            State.Position.Horizontal = MostLeft;

        if ( State.Position.Vertical < 0.0f )
            State.Position.Vertical = 0.0f;

        if ( State.Position.Vertical > MostTop )
            State.Position.Vertical = MostTop;
    }

    float Pace = Style->FadeSpeed * Context->DeltaTime;
    if ( Pace > 1.0f )
        Pace = 1.0f;

    State.Reveal += ( 1.0f - State.Reveal ) * Pace;
    State.Collapse += ( ( State.Collapsed ? 1.0f : 0.0f ) - State.Collapse ) * Pace;

    if ( State.Collapse < 0.002f )
        State.Collapse = 0.0f;

    if ( State.Collapse > 0.998f )
        State.Collapse = 1.0f;

    Shortened = Style->TitleHeight + ( State.Size.Vertical - Style->TitleHeight ) * ( 1.0f - State.Collapse );
    float Rise = ( 1.0f - State.Reveal ) * 12.0f * Style->Scale;

    CRectangle Bounds( State.Position, CVector( State.Size.Horizontal, Shortened ) );
    Bounds.Top += Rise;

    State.Bounds = Bounds;
    Grip = CRectangle( Bounds.Left, Bounds.Top, Bounds.Width, Style->TitleHeight );

    Fold.Top += Rise;
    Shut.Top += Rise;

    bool Front = !Stack.empty( ) && Stack.back( ) == Identifier;
    Canvas->Opacity = State.Reveal;

    if ( Style->Shadows )
        Canvas->Shadow( Bounds, Style->Shade, Style->Rounding, ( Front ? 1.5f : 1.0f ) * Style->Softness * Style->Elevation );

    if ( Style->Glass ) {
        Canvas->Gradient( Bounds, Style->Surface.Blend( CColor( 255, 255, 255 ), 0.05f ), Style->Surface, Style->Rounding, false );
        Canvas->Rectangle( CRectangle( Bounds.Left + Style->Rounding, Bounds.Top + Style->Thickness, Bounds.Width - Style->Rounding * 2.0f, Style->Thickness ), Style->Highlight, 0.0f );
    } else {
        Canvas->Rectangle( Bounds, Style->Surface, Style->Rounding );
    }

    if ( Style->Borders )
        Canvas->Border( Bounds, Front ? Style->Outline.Blend( Style->Accent, 0.4f ) : Style->Outline, Style->Rounding, Style->Thickness );

    float Line = Style->IconStroke;

    if ( Options & FrameCollapse ) {
        if ( FoldHovered )
            Canvas->Rectangle( Fold, Style->Hovered, Slot * 0.5f );

        CRectangle Mark = Fold.Shrink( Slot * 0.3f );
        CColor Ink = FoldHovered ? Style->Text : Style->Faint;

        float Turn = State.Collapse;

        CVector First( 0.05f + 0.25f * Turn, 0.3f - 0.25f * Turn );
        CVector Corner( 0.5f + 0.22f * Turn, 0.72f - 0.22f * Turn );

        CVector Last( 0.95f - 0.65f * Turn, 0.3f + 0.65f * Turn );

        Canvas->Stroke( Mark, First, Corner, Ink, Line );
        Canvas->Stroke( Mark, Corner, Last, Ink, Line );
    }

    if ( ( Options & FrameClose ) && Open ) {
        if ( ShutHovered )
            Canvas->Rectangle( Shut, Style->Danger.Fade( 0.9f ), Slot * 0.5f );

        CRectangle Mark = Shut.Shrink( Slot * 0.32f );
        CColor Ink = ShutHovered ? CColor( 255, 255, 255 ) : Style->Faint;

        Canvas->Stroke( Mark, CVector( 0.05f, 0.05f ), CVector( 0.95f, 0.95f ), Ink, Line );
        Canvas->Stroke( Mark, CVector( 0.95f, 0.05f ), CVector( 0.05f, 0.95f ), Ink, Line );
    }

    float TitleLeft = Bounds.Left + Style->PaddingWide;
    float TitleRight = Bounds.Right( ) - Style->PaddingWide;

    if ( Options & FrameCollapse )
        TitleRight = Fold.Left - 8.0f * Style->Scale;
    else if ( ( Options & FrameClose ) && Open )
        TitleRight = Shut.Left - 8.0f * Style->Scale;

    CVector Naming = ::Heading->Measure( State.Title );
    float TitleSpot = TitleLeft + ( TitleRight - TitleLeft - Naming.Horizontal ) * Style->TitleAlign;

    Canvas->Write( ::Heading.get( ), CVector( TitleSpot, Bounds.Top + ( Style->TitleHeight - ::Heading->LineSpan - ::Heading->Leading ) * 0.5f + 1.0f * Style->Scale ), Style->Text, State.Title );

    if ( State.Collapse >= 1.0f )
        return false;

    CRectangle Region( Bounds.Left, Bounds.Top + Style->TitleHeight, Bounds.Width, Bounds.Height - Style->TitleHeight );

    Canvas->PushClip( Region );
    Context->PushOwner( Identifier );

    Canvas->Opacity = State.Reveal * ( 1.0f - State.Collapse );

    Content( Identifier, State, Region, Style->PaddingTall * 0.5f );

    Producing = true;
    return true;
}

void CFrames::End( ) {
    if ( Producing ) {
        float Used = Layout->End( );
        CFrameState* State = Find( Current );

        if ( State ) {
            State->Reach = Used;

            if ( ( State->Options & FrameFit ) && Used > 0.0f && !State->Loose ) {
                float Need = Style->TitleHeight + Used + Style->PaddingTall;
                if ( Need < Style->TitleHeight + 52.0f * Style->Scale )
                    Need = Style->TitleHeight + 52.0f * Style->Scale;
                if ( State->Ceiling > Style->TitleHeight && Need > State->Ceiling )
                    Need = State->Ceiling;
                float Room = Context->Display.Vertical - State->Position.Vertical - 8.0f * Style->Scale;
                if ( Room > 0.0f && Need > Room )
                    Need = Room;
                State->Size.Vertical = Need;
            }

            bool Overflow = State->Reach > State->View + 1.0f && State->View > 0.0f;

            if ( Overflow && Input->WheelDelta != 0.0f && Beneath == Current && !Context->Covered( Input->MousePosition ) ) {
                State->Aim -= Input->WheelDelta * 64.0f * Style->Scale;
                float Most = State->Reach - State->View;

                if ( State->Aim > Most )
                    State->Aim = Most;

                if ( State->Aim < 0.0f )
                    State->Aim = 0.0f;
            }

            if ( State->Reach > State->View + 1.0f && State->View > 0.0f ) {
                float Lane = Style->GrooveWidth + 1.0f * Style->Scale;

                CRectangle Track( State->Inner.Right( ) - Lane - 3.0f * Style->Scale, State->Inner.Top + 4.0f * Style->Scale, Lane, State->Inner.Height - 8.0f * Style->Scale );
                float Span = Track.Height * ( State->View / State->Reach );

                if ( Span < 26.0f * Style->Scale )
                    Span = 26.0f * Style->Scale;

                float Most = State->Reach - State->View;
                float Range = Track.Height - Span;

                CRectangle Thumb( Track.Left, Track.Top + ( Most > 0.0f ? State->Scroll / Most : 0.0f ) * Range, Lane, Span );

                bool Busy = Context->ActiveItem == ( Current ^ SaltScroll );
                bool Near = Thumb.Inflate( 5.0f ).Contains( Input->MousePosition ) && Beneath == Current;

                float Tone = 0.0f;

                if ( Near )
                    Tone = 0.3f;

                if ( Busy )
                    Tone = 0.6f;

                Canvas->Rectangle( Track, Style->ScrollTrack, Lane * 0.5f );
                Canvas->Rectangle( Thumb, Style->ScrollThumb.Blend( Style->Accent, Tone ), Lane * 0.5f );
            }
        }

        Context->PopOwner( );
        Canvas->PopClip( );
    }

    Canvas->Opacity = 1.0f;
    Canvas->Route( 0 );

    Producing = false;
    Current = 0;
}