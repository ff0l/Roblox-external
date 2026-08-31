#include <cmath>
#include <string>
#include <istream>
#include <ostream>

#include "Font.h"
#include "Input.h"
#include "Canvas.h"
#include "Frames.h"
#include "Docking.h"

#include "Style.h"
#include "Context.h"

static constexpr unsigned int SaltTab = 0x7FEB352Du;
static constexpr unsigned int SaltSplit = 0x846CA68Bu;
static constexpr unsigned int SaltIsle = 0x94D049BBu;
static constexpr unsigned int SaltDrag = 0x2545F491u;

static CRectangle Glide( CRectangle From, CRectangle Wanted, float Amount ) {
    return CRectangle( From.Left + ( Wanted.Left - From.Left ) * Amount, From.Top + ( Wanted.Top - From.Top ) * Amount, From.Width + ( Wanted.Width - From.Width ) * Amount, From.Height + ( Wanted.Height - From.Height ) * Amount );
}

static bool Raised( const std::vector< CDockNode >& Nodes, unsigned int Anchor ) {
    unsigned int Below = Frames->Beneath;

    if ( Below == 0 )
        return true;

    CFrameState* State = Frames->Find( Below );
    if ( !State )
        return true;

    if ( State->Home != 0 && State->Home < Nodes.size( ) && Nodes[ State->Home ].Anchor == Anchor )
        return true;

    const std::vector< unsigned int >& Order = Frames->Order( );

    int Member = -1;
    int Blocker = -1;

    for ( size_t Position = 0; Position < Order.size( ); Position++ ) {
        CFrameState* Owner = Frames->Find( Order[ Position ] );

        if ( Owner && Owner->Group == Anchor && Member < 0 )
            Member = ( int )Position;

        if ( Order[ Position ] == Below )
            Blocker = ( int )Position;
    }

    return Member > Blocker;
}

void CDocking::Destroy( ) {
    Nodes.clear( );
    Isles.clear( );

    Root = 0;
    Flow = 0.0f;
}

unsigned int CDocking::Allocate( ) {
    if ( Nodes.empty( ) )
        Nodes.resize( 1 );

    for ( size_t Slot = 1; Slot < Nodes.size( ); Slot++ ) {
        if ( !Nodes[ Slot ].Used ) {
            Nodes[ Slot ] = CDockNode( );
            Nodes[ Slot ].Used = true;

            return ( unsigned int )Slot;
        }
    }

    CDockNode Fresh;
    Fresh.Used = true;

    Nodes.push_back( Fresh );
    return ( unsigned int )Nodes.size( ) - 1;
}

bool CDocking::HasTabs( unsigned int Node ) const {
    if ( Node == 0 || Node >= Nodes.size( ) || !Nodes[ Node ].Used )
        return false;

    if ( Nodes[ Node ].Leaf( ) )
        return !Nodes[ Node ].Tabs.empty( );

    return HasTabs( Nodes[ Node ].First ) || HasTabs( Nodes[ Node ].Second );
}

void CDocking::Prune( ) {
    for ( size_t Slot = 1; Slot < Nodes.size( ); Slot++ ) {
        CDockNode& Node = Nodes[ Slot ];

        if ( !Node.Used || !Node.Leaf( ) )
            continue;

        for ( size_t Position = Node.Tabs.size( ); Position > 0; Position-- ) {
            unsigned int Window = Node.Tabs[ Position - 1 ];
            CFrameState* State = Frames->Find( Window );

            bool Stale = !State || State->Home != ( unsigned int )Slot || Context->FrameIndex - State->Stamp > 2;

            if ( Stale ) {
                if ( State && State->Home == ( unsigned int )Slot ) {
                    State->Home = 0;
                    State->Group = 0;

                    State->DockFront = false;
                }

                Node.Tabs.erase( Node.Tabs.begin( ) + ( Position - 1 ) );
            }
        }

        bool Listed = false;

        for ( unsigned int Window : Node.Tabs ) {
            if ( Window == Node.Front )
                Listed = true;
        }

        if ( !Listed )
            Node.Front = Node.Tabs.empty( ) ? 0 : Node.Tabs[ 0 ];
    }

    for ( size_t Position = Isles.size( ); Position > 0; Position-- ) {
        unsigned int Isle = Isles[ Position - 1 ];

        if ( Isle >= Nodes.size( ) || !Nodes[ Isle ].Used ) {
            Isles.erase( Isles.begin( ) + ( Position - 1 ) );
            continue;
        }

        CDockNode& Node = Nodes[ Isle ];
        if ( !Node.Leaf( ) )
            continue;

        if ( Node.Tabs.empty( ) ) {
            Node.Used = false;
            Node.Tabs.clear( );

            Isles.erase( Isles.begin( ) + ( Position - 1 ) );
            continue;
        }

        if ( Node.Tabs.size( ) == 1 ) {
            CFrameState* State = Frames->Find( Node.Tabs[ 0 ] );

            if ( State ) {
                State->Home = 0;
                State->Group = 0;

                State->DockFront = false;

                State->Position = Node.Bounds.Origin( );
                State->Size = Node.Bounds.Extent( );

                State->Reveal = 0.7f;
                Frames->Raise( Node.Tabs[ 0 ] );
            }

            Node.Used = false;
            Node.Tabs.clear( );

            Isles.erase( Isles.begin( ) + ( Position - 1 ) );
        }
    }

    bool Folded = true;

    while ( Folded ) {
        Folded = false;

        for ( size_t Slot = 1; Slot < Nodes.size( ); Slot++ ) {
            CDockNode& Node = Nodes[ Slot ];

            if ( !Node.Used || !Node.Leaf( ) || !Node.Tabs.empty( ) )
                continue;

            if ( Node.Parent == 0 )
                continue;

            if ( Node.Keep ) {
                unsigned int Sibling = Nodes[ Node.Parent ].First == ( unsigned int )Slot ? Nodes[ Node.Parent ].Second : Nodes[ Node.Parent ].First;

                if ( HasTabs( Sibling ) )
                    continue;
            }

            Fold( ( unsigned int )Slot );
            Folded = true;

            break;
        }
    }
}

void CDocking::Fold( unsigned int Node ) {
    unsigned int Above = Nodes[ Node ].Parent;
    if ( Above == 0 )
        return;

    unsigned int Sibling = Nodes[ Above ].First == Node ? Nodes[ Above ].Second : Nodes[ Above ].First;
    CDockNode Kept = Nodes[ Sibling ];

    Nodes[ Above ].First = Kept.First;
    Nodes[ Above ].Second = Kept.Second;

    Nodes[ Above ].Front = Kept.Front;
    Nodes[ Above ].Ratio = Kept.Ratio;

    Nodes[ Above ].Vertical = Kept.Vertical;
    Nodes[ Above ].Keep = Kept.Keep;

    Nodes[ Above ].Tabs = Kept.Tabs;

    if ( Kept.First != 0 ) {
        Nodes[ Kept.First ].Parent = Above;
        Nodes[ Kept.Second ].Parent = Above;
    }

    for ( unsigned int Window : Nodes[ Above ].Tabs ) {
        CFrameState* State = Frames->Find( Window );

        if ( State ) {
            State->Home = Above;
            State->HomeStamp = Context->FrameIndex;
        }
    }

    Nodes[ Sibling ].Used = false;
    Nodes[ Sibling ].Tabs.clear( );

    Nodes[ Node ].Used = false;
    Nodes[ Node ].Tabs.clear( );
}

void CDocking::Arrange( unsigned int Node, CRectangle Bounds, unsigned int Anchor ) {
    CDockNode& Item = Nodes[ Node ];

    Item.Bounds = Bounds;
    Item.Anchor = Anchor;

    if ( Item.Leaf( ) )
        return;

    float Gap = Style->SplitterSize;

    if ( Item.Vertical ) {
        float Share = ( Bounds.Height - Gap ) * Item.Ratio;

        Arrange( Item.First, CRectangle( Bounds.Left, Bounds.Top, Bounds.Width, Share ), Anchor );
        Arrange( Item.Second, CRectangle( Bounds.Left, Bounds.Top + Share + Gap, Bounds.Width, Bounds.Height - Share - Gap ), Anchor );

        return;
    }

    float Share = ( Bounds.Width - Gap ) * Item.Ratio;

    Arrange( Item.First, CRectangle( Bounds.Left, Bounds.Top, Share, Bounds.Height ), Anchor );
    Arrange( Item.Second, CRectangle( Bounds.Left + Share + Gap, Bounds.Top, Bounds.Width - Share - Gap, Bounds.Height ), Anchor );
}

void CDocking::Steer( ) {
    CVector Point = Input->MousePosition;

    for ( unsigned int Isle : Isles ) {
        CDockNode& Node = Nodes[ Isle ];

        unsigned int Zone = Isle ^ SaltIsle;
        float Grip = Style->GripSize;

        CRectangle Corner( Node.Bounds.Right( ) - Grip, Node.Bounds.Bottom( ) - Grip, Grip, Grip );

        bool Hovered = Corner.Contains( Point ) && Raised( Nodes, Isle ) && !Context->Covered( Point );
        bool Held = Context->ActiveItem == Zone;

        if ( Hovered || Held )
            Input->Pointer = PointerSlant;

        if ( Hovered && Input->MousePressed( 0 ) && Context->ActiveItem == 0 ) {
            Context->ActiveItem = Zone;
            Hold = CVector( Node.Bounds.Right( ) - Point.Horizontal, Node.Bounds.Bottom( ) - Point.Vertical );

            Held = true;
        }

        if ( Held ) {
            if ( Input->MouseDown( 0 ) ) {
                float Reach = Point.Horizontal + Hold.Horizontal - Node.Bounds.Left;
                float Drop = Point.Vertical + Hold.Vertical - Node.Bounds.Top;

                if ( Reach >= 180.0f )
                    Node.Bounds.Width = Reach;

                if ( Drop >= Style->TabHeight + 60.0f )
                    Node.Bounds.Height = Drop;
            } else {
                Context->ActiveItem = 0;
            }
        }

        unsigned int Moving = Isle ^ SaltDrag;

        bool Barred = Node.Handle.Width > 0.0f && Node.Handle.Contains( Point ) && Raised( Nodes, Isle ) && !Context->Covered( Point );
        bool Carried = Context->ActiveItem == Moving;

        if ( Barred && Input->MousePressed( 0 ) && Context->ActiveItem == 0 ) {
            Context->ActiveItem = Moving;
            Hold = Point - Node.Bounds.Origin( );

            unsigned int Leaf = Isle;

            while ( !Nodes[ Leaf ].Leaf( ) )
                Leaf = Nodes[ Leaf ].First;

            if ( Nodes[ Leaf ].Front != 0 )
                Frames->Raise( Nodes[ Leaf ].Front );

            Carried = true;
        }

        if ( Carried ) {
            if ( Input->MouseDown( 0 ) ) {
                Node.Bounds.Left = Point.Horizontal - Hold.Horizontal;
                Node.Bounds.Top = Point.Vertical - Hold.Vertical;
            } else {
                Context->ActiveItem = 0;
            }
        }

        if ( Context->Display.Horizontal > 0.0f && Context->Display.Vertical > 0.0f ) {
            if ( Node.Bounds.Left < 60.0f - Node.Bounds.Width )
                Node.Bounds.Left = 60.0f - Node.Bounds.Width;

            if ( Node.Bounds.Left > Context->Display.Horizontal - 60.0f )
                Node.Bounds.Left = Context->Display.Horizontal - 60.0f;

            if ( Node.Bounds.Top < 0.0f )
                Node.Bounds.Top = 0.0f;

            if ( Node.Bounds.Top > Context->Display.Vertical - Style->TabHeight )
                Node.Bounds.Top = Context->Display.Vertical - Style->TabHeight;
        }
    }
}

void CDocking::Islands( ) {
    for ( unsigned int Isle : Isles ) {
        CDockNode& Node = Nodes[ Isle ];

        Arrange( Isle, Node.Bounds, Isle );

        int Lane = 0;

        for ( unsigned int Member : Frames->Order( ) ) {
            CFrameState* Owner = Frames->Find( Member );

            if ( Owner && Owner->Group == Isle ) {
                Lane = Owner->Channel;
                break;
            }
        }

        Node.Channel = Lane;
        int Former = Canvas->Route( Lane );

        if ( Style->Shadows )
            Canvas->Shadow( Node.Bounds, Style->Shade, Style->Rounding, Style->Softness * Style->Elevation );

        if ( Style->Glass )
            Canvas->Gradient( Node.Bounds, Style->Surface.Blend( CColor( 255, 255, 255 ), 0.05f ), Style->Surface, Style->Rounding, false );
        else
            Canvas->Rectangle( Node.Bounds, Style->Surface, Style->Rounding );

        if ( Style->Borders )
            Canvas->Border( Node.Bounds, Style->Outline, Style->Rounding, Style->Thickness );

        Canvas->Route( Former );
    }
}

void CDocking::Splitters( ) {
    CVector Point = Input->MousePosition;

    for ( size_t Slot = 1; Slot < Nodes.size( ); Slot++ ) {
        CDockNode& Node = Nodes[ Slot ];

        if ( !Node.Used || Node.Leaf( ) )
            continue;

        CRectangle Grip;

        if ( Node.Vertical )
            Grip = CRectangle( Node.Bounds.Left, Nodes[ Node.First ].Bounds.Bottom( ), Node.Bounds.Width, Style->SplitterSize );
        else
            Grip = CRectangle( Nodes[ Node.First ].Bounds.Right( ), Node.Bounds.Top, Style->SplitterSize, Node.Bounds.Height );

        unsigned int Zone = SaltSplit ^ ( unsigned int )Slot;

        bool Hovered = Grip.Inflate( 3.0f ).Contains( Point ) && Raised( Nodes, Node.Anchor ) && !Context->Covered( Point );
        bool Held = Context->ActiveItem == Zone;

        if ( Hovered || Held )
            Input->Pointer = Node.Vertical ? PointerDown : PointerAcross;

        if ( Hovered && Input->MousePressed( 0 ) && Context->ActiveItem == 0 ) {
            Context->ActiveItem = Zone;
            Held = true;
        }

        if ( Held ) {
            if ( Input->MouseDown( 0 ) ) {
                float Span = Node.Vertical ? Node.Bounds.Height - Style->SplitterSize : Node.Bounds.Width - Style->SplitterSize;
                float Along = Node.Vertical ? Point.Vertical - Node.Bounds.Top : Point.Horizontal - Node.Bounds.Left;

                if ( Span > 0.0f )
                    Node.Ratio = Along / Span;

                if ( Node.Ratio < 0.08f )
                    Node.Ratio = 0.08f;

                if ( Node.Ratio > 0.92f )
                    Node.Ratio = 0.92f;
            } else {
                Context->ActiveItem = 0;
            }
        }

        if ( Hovered || Held ) {
            int Former = Canvas->Route( Nodes[ Node.Anchor ].Channel );

            Canvas->Rectangle( Grip, Style->Outline.Blend( Style->Accent, Held ? 0.85f : 0.45f ), Style->SplitterSize * 0.5f );
            Canvas->Route( Former );
        }
    }
}

void CDocking::Leaves( ) {
    CVector Point = Input->MousePosition;

    for ( size_t Slot = 1; Slot < Nodes.size( ); Slot++ ) {
        CDockNode& Node = Nodes[ Slot ];

        if ( !Node.Used || !Node.Leaf( ) )
            continue;

        bool Roaming = Node.Anchor != Root;
        int Former = Canvas->Route( Roaming ? Nodes[ Node.Anchor ].Channel : 0 );

        if ( Node.Tabs.empty( ) ) {
            Canvas->Rectangle( Node.Bounds, Style->Groove.Fade( 0.3f ), Style->Rounding );

            if ( Style->Borders )
                Canvas->Border( Node.Bounds, Style->Outline.Fade( 0.5f ), Style->Rounding, Style->Thickness );

            const char* Hint = "Drag a window here";
            CVector Mark = Font->Measure( Hint );

            if ( Node.Bounds.Width > Mark.Horizontal + 20.0f && Node.Bounds.Height > Mark.Vertical + 12.0f )
                Canvas->Text( CVector( Node.Bounds.Center( ).Horizontal - Mark.Horizontal * 0.5f, Node.Bounds.Center( ).Vertical - Mark.Vertical * 0.5f ), Style->Faint.Fade( 0.7f ), Hint );

            Canvas->Route( Former );
            continue;
        }

        CRectangle Bar( Node.Bounds.Left, Node.Bounds.Top, Node.Bounds.Width, Style->TabHeight );
        CRectangle Content( Node.Bounds.Left, Node.Bounds.Top + Style->TabHeight, Node.Bounds.Width, Node.Bounds.Height - Style->TabHeight );

        bool Crown = Roaming && Node.Bounds.Top <= Nodes[ Node.Anchor ].Bounds.Top + 0.5f;
        Canvas->Rectangle( Bar, Style->Tab, Crown ? Style->Rounding : 0.0f );

        if ( Crown ) {
            CRectangle Patch( Bar.Left, Bar.Top + Bar.Height * 0.5f, Bar.Width, Bar.Height * 0.5f );
            Canvas->Rectangle( Patch, Style->Tab, 0.0f );
        }

        Canvas->PushClip( Bar );
        float Across = Bar.Left;

        bool Broken = false;

        for ( size_t Position = 0; Position < Node.Tabs.size( ) && !Broken; Position++ ) {
            unsigned int Window = Node.Tabs[ Position ];
            CFrameState* State = Frames->Find( Window );

            if ( !State )
                continue;

            CVector Mark = Font->Measure( State->Title );
            CRectangle Tab( Across, Bar.Top, Mark.Horizontal + Style->PaddingWide * 1.6f, Style->TabHeight );

            Across = Tab.Right( );
            unsigned int Zone = SaltTab ^ Window;

            bool Hovered = Tab.Contains( Point ) && Raised( Nodes, Node.Anchor ) && !Context->Covered( Point );
            bool Held = Context->ActiveItem == Zone;

            if ( Hovered && Input->MousePressed( 0 ) && Context->ActiveItem == 0 ) {
                Context->ActiveItem = Zone;
                Node.Front = Window;

                Frames->Raise( Window );
                Held = true;
            }

            if ( Hovered && Input->MouseDouble( 0 ) ) {
                Detach( Window );

                State->Position = Content.Origin( ) + CVector( 32.0f, 32.0f );
                State->Reveal = 0.5f;

                Frames->Raise( Window );
                Context->ActiveItem = 0;

                break;
            }

            if ( Held ) {
                if ( !Input->MouseDown( 0 ) ) {
                    Context->ActiveItem = 0;
                } else {
                    bool Above = Point.Vertical < Bar.Top - 6.0f;
                    bool Below = Point.Vertical > Bar.Bottom( ) + 6.0f;

                    bool Before = Point.Horizontal < Bar.Left - 24.0f;
                    bool Beyond = Point.Horizontal > Bar.Right( ) + 24.0f;

                    if ( Above || Below || Before || Beyond ) {
                        Detach( Window );

                        float Grasp = Point.Horizontal - Tab.Left + Style->PaddingWide;
                        if ( Grasp > State->Size.Horizontal - 40.0f )
                            Grasp = State->Size.Horizontal - 40.0f;

                        State->Grab = CVector( Grasp, Style->TitleHeight * 0.5f );
                        State->Position = Point - State->Grab;

                        State->Dragging = ( State->Options & FrameDock ) != 0;
                        State->Reveal = 0.6f;

                        Context->ActiveItem = Window;
                        Frames->Raise( Window );

                        break;
                    }

                    if ( Position > 0 && Point.Horizontal < Tab.Left - 6.0f ) {
                        Node.Tabs[ Position ] = Node.Tabs[ Position - 1 ];
                        Node.Tabs[ Position - 1 ] = Window;

                        Broken = true;
                    } else if ( Position + 1 < Node.Tabs.size( ) && Point.Horizontal > Tab.Right( ) + 6.0f ) {
                        Node.Tabs[ Position ] = Node.Tabs[ Position + 1 ];
                        Node.Tabs[ Position + 1 ] = Window;

                        Broken = true;
                    }
                }
            }

            bool Front = Node.Front == Window;

            if ( Front ) {
                Canvas->Rectangle( Tab, Style->TabActive, Crown ? Style->Rounding * 0.6f : 0.0f );

                CRectangle Seam( Tab.Left + 6.0f * Style->Scale, Tab.Bottom( ) - Style->Underline, Tab.Width - 12.0f * Style->Scale, Style->Underline );
                Canvas->Rectangle( Seam, Style->Accent, Style->Underline * 0.5f );
            } else if ( Hovered ) {
                Canvas->Rectangle( Tab, Style->Hovered, Crown ? Style->Rounding * 0.6f : 0.0f );
            }

            Canvas->Text( CVector( Tab.Left + Style->PaddingWide * 0.8f, Tab.Top + ( Tab.Height - Font->LineSpan ) * 0.5f ), Front ? Style->Text : Style->Faint, State->Title );
        }

        Canvas->PopClip( );

        if ( Roaming )
            Nodes[ Node.Anchor ].Handle = CRectangle( Across, Bar.Top, Bar.Right( ) - Across, Bar.Height );

        if ( Style->Borders && !Roaming )
            Canvas->Border( Node.Bounds, Style->Outline, 0.0f, Style->Thickness );

        Canvas->Route( Former );

        for ( unsigned int Window : Node.Tabs ) {
            CFrameState* State = Frames->Find( Window );

            if ( !State )
                continue;

            State->Home = ( unsigned int )Slot;
            State->Group = Roaming ? Node.Anchor : 0;

            State->DockFront = Node.Front == Window;

            State->Bounds = State->DockFront ? Content : CRectangle( );
            State->Stamp = Context->FrameIndex;

            State->HomeStamp = Context->FrameIndex;
        }
    }
}

void CDocking::Preview( ) {
    unsigned int Dragged = Context->ActiveItem;
    CFrameState* State = Dragged ? Frames->Find( Dragged ) : nullptr;

    bool Carrying = Enabled && State && State->Dragging && ( State->Options & FrameDock ) != 0;
    CVector Point = Input->MousePosition;

    unsigned int Target = 0;
    unsigned int Fellow = 0;

    int Zone = -1;
    CRectangle Wanted;

    CRectangle Anchors[ 9 ];
    bool Alive[ 9 ] = { };

    if ( Carrying ) {
        const std::vector< unsigned int >& Order = Frames->Order( );

        for ( size_t Position = Order.size( ); Position > 0; Position-- ) {
            unsigned int Other = Order[ Position - 1 ];

            if ( Other == Dragged )
                continue;

            CFrameState* Candidate = Frames->Find( Other );
            if ( !Candidate || Candidate->Home != 0 || Context->FrameIndex - Candidate->Stamp > 1 )
                continue;

            if ( Candidate->Bounds.Contains( Point ) ) {
                Fellow = Other;
                break;
            }
        }

        if ( Fellow == 0 ) {
            for ( size_t Slot = 1; Slot < Nodes.size( ); Slot++ ) {
                if ( Nodes[ Slot ].Used && Nodes[ Slot ].Leaf( ) && Nodes[ Slot ].Anchor == Root && Nodes[ Slot ].Bounds.Contains( Point ) )
                    Target = ( unsigned int )Slot;
            }

            for ( unsigned int Isle : Isles ) {
                for ( size_t Slot = 1; Slot < Nodes.size( ); Slot++ ) {
                    if ( Nodes[ Slot ].Used && Nodes[ Slot ].Leaf( ) && Nodes[ Slot ].Anchor == Isle && Nodes[ Slot ].Bounds.Contains( Point ) )
                        Target = ( unsigned int )Slot;
                }
            }
        }
    }

    if ( Target != 0 ) {
        CDockNode& Node = Nodes[ Target ];

        CRectangle Bar( Node.Bounds.Left, Node.Bounds.Top, Node.Bounds.Width, Style->TabHeight );
        CRectangle Content( Node.Bounds.Left, Node.Bounds.Top + Style->TabHeight, Node.Bounds.Width, Node.Bounds.Height - Style->TabHeight );

        bool Filled = !Node.Tabs.empty( );

        float Large = 38.0f * Style->Scale;
        float Small = 26.0f * Style->Scale;

        float Gap = 8.0f * Style->Scale;
        float Half = Large * 0.5f;

        CVector Middle = Node.Bounds.Center( );
        bool Roomy = Node.Bounds.Width > Large * 3.0f + Gap * 4.0f && Node.Bounds.Height > Large * 3.0f + Gap * 4.0f + Style->TabHeight;

        Alive[ 0 ] = true;
        Anchors[ 0 ] = CRectangle( Middle.Horizontal - Half, Middle.Vertical - Half, Large, Large );

        if ( Filled && Roomy ) {
            Anchors[ 1 ] = CRectangle( Middle.Horizontal - Half - Gap - Large, Middle.Vertical - Half, Large, Large );
            Anchors[ 2 ] = CRectangle( Middle.Horizontal + Half + Gap, Middle.Vertical - Half, Large, Large );

            Anchors[ 3 ] = CRectangle( Middle.Horizontal - Half, Middle.Vertical - Half - Gap - Large, Large, Large );
            Anchors[ 4 ] = CRectangle( Middle.Horizontal - Half, Middle.Vertical + Half + Gap, Large, Large );

            Anchors[ 5 ] = CRectangle( Middle.Horizontal - Half - Gap - Small, Middle.Vertical - Half - Gap - Small, Small, Small );
            Anchors[ 6 ] = CRectangle( Middle.Horizontal + Half + Gap, Middle.Vertical - Half - Gap - Small, Small, Small );

            Anchors[ 7 ] = CRectangle( Middle.Horizontal - Half - Gap - Small, Middle.Vertical + Half + Gap, Small, Small );
            Anchors[ 8 ] = CRectangle( Middle.Horizontal + Half + Gap, Middle.Vertical + Half + Gap, Small, Small );

            for ( int Spot = 1; Spot < 9; Spot++ )
                Alive[ Spot ] = true;
        }

        if ( Filled && Bar.Contains( Point ) ) {
            Zone = 0;
        } else {
            for ( int Spot = 0; Spot < 9; Spot++ ) {
                if ( Alive[ Spot ] && Anchors[ Spot ].Contains( Point ) )
                    Zone = Spot;
            }
        }

        float Wide = Node.Bounds.Width * 0.5f;
        float Tall = Node.Bounds.Height * 0.5f;

        if ( Zone == 0 )
            Wanted = Filled ? Content : Node.Bounds;

        if ( Zone == 1 )
            Wanted = CRectangle( Node.Bounds.Left, Node.Bounds.Top, Wide, Node.Bounds.Height );

        if ( Zone == 2 )
            Wanted = CRectangle( Node.Bounds.Left + Wide, Node.Bounds.Top, Wide, Node.Bounds.Height );

        if ( Zone == 3 )
            Wanted = CRectangle( Node.Bounds.Left, Node.Bounds.Top, Node.Bounds.Width, Tall );

        if ( Zone == 4 )
            Wanted = CRectangle( Node.Bounds.Left, Node.Bounds.Top + Tall, Node.Bounds.Width, Tall );

        if ( Zone == 5 )
            Wanted = CRectangle( Node.Bounds.Left, Node.Bounds.Top, Wide, Tall );

        if ( Zone == 6 )
            Wanted = CRectangle( Node.Bounds.Left + Wide, Node.Bounds.Top, Wide, Tall );

        if ( Zone == 7 )
            Wanted = CRectangle( Node.Bounds.Left, Node.Bounds.Top + Tall, Wide, Tall );

        if ( Zone == 8 )
            Wanted = CRectangle( Node.Bounds.Left + Wide, Node.Bounds.Top + Tall, Wide, Tall );
    }

    if ( Fellow != 0 ) {
        CFrameState* Candidate = Frames->Find( Fellow );

        if ( Candidate ) {
            float Large = 38.0f * Style->Scale;
            CVector Middle = Candidate->Bounds.Center( );

            Alive[ 0 ] = true;
            Anchors[ 0 ] = CRectangle( Middle.Horizontal - Large * 0.5f, Middle.Vertical - Large * 0.5f, Large, Large );

            CRectangle Strip( Candidate->Bounds.Left, Candidate->Bounds.Top, Candidate->Bounds.Width, Style->TitleHeight );

            if ( Anchors[ 0 ].Contains( Point ) || Strip.Contains( Point ) ) {
                Zone = 9;
                Wanted = Candidate->Bounds;
            }
        }
    }

    float Pace = Style->FadeSpeed * Context->DeltaTime;
    if ( Pace > 1.0f )
        Pace = 1.0f;

    if ( Zone < 0 ) {
        Flow -= Pace;

        if ( Flow < 0.0f )
            Flow = 0.0f;
    } else {
        if ( Flow <= 0.01f )
            Ghost = Wanted;

        Ghost = Glide( Ghost, Wanted, Pace );
        Flow += Pace;

        if ( Flow > 1.0f )
            Flow = 1.0f;
    }

    if ( Carrying && Flow > 0.01f ) {
        int Former = Canvas->Route( OverlayRoute );
        Canvas->PushClip( Area, true );

        CRectangle Shape = Ghost.Shrink( 4.0f );

        Canvas->Rectangle( Shape, Style->Accent.Fade( 0.16f * Flow ), Style->Rounding );
        Canvas->Border( Shape, Style->Accent.Fade( Flow ), Style->Rounding, Style->Thickness * 1.5f );

        if ( Zone == 0 && Target != 0 && !Nodes[ Target ].Tabs.empty( ) ) {
            CRectangle Bar( Nodes[ Target ].Bounds.Left, Nodes[ Target ].Bounds.Top, Nodes[ Target ].Bounds.Width, Style->TabHeight );
            Canvas->Rectangle( Bar, Style->Accent.Fade( 0.28f * Flow ), 0.0f );
        }

        if ( Zone == 9 && Fellow != 0 ) {
            CFrameState* Candidate = Frames->Find( Fellow );

            if ( Candidate ) {
                CRectangle Strip( Candidate->Bounds.Left, Candidate->Bounds.Top, Candidate->Bounds.Width, Style->TitleHeight );
                Canvas->Rectangle( Strip, Style->Accent.Fade( 0.28f * Flow ), Style->Rounding );
            }
        }

        Canvas->PopClip( );
        Canvas->Route( Former );
    }

    if ( Carrying && ( Target != 0 || Fellow != 0 ) ) {
        int Former = Canvas->Route( OverlayRoute );
        Canvas->PushClip( Area, true );

        float Corner = Style->ControlRounding * 0.7f;

        for ( int Spot = 0; Spot < 9; Spot++ ) {
            if ( !Alive[ Spot ] )
                continue;

            CRectangle Pill = Anchors[ Spot ];
            bool Chosen = Zone == Spot || ( Spot == 0 && Zone == 9 );

            if ( Style->Shadows )
                Canvas->Shadow( Pill, Style->Shade, Corner, Style->Softness * 0.4f );

            Canvas->Rectangle( Pill, Style->Popup, Corner );
            Canvas->Border( Pill, Chosen ? Style->Accent : Style->Outline, Corner, Style->Thickness );

            CRectangle Inner = Pill.Shrink( 6.0f * Style->Scale );
            CRectangle Mark = Inner;

            if ( Spot == 1 )
                Mark = CRectangle( Inner.Left, Inner.Top, Inner.Width * 0.5f, Inner.Height );

            if ( Spot == 2 )
                Mark = CRectangle( Inner.Left + Inner.Width * 0.5f, Inner.Top, Inner.Width * 0.5f, Inner.Height );

            if ( Spot == 3 )
                Mark = CRectangle( Inner.Left, Inner.Top, Inner.Width, Inner.Height * 0.5f );

            if ( Spot == 4 )
                Mark = CRectangle( Inner.Left, Inner.Top + Inner.Height * 0.5f, Inner.Width, Inner.Height * 0.5f );

            if ( Spot == 5 )
                Mark = CRectangle( Inner.Left, Inner.Top, Inner.Width * 0.5f, Inner.Height * 0.5f );

            if ( Spot == 6 )
                Mark = CRectangle( Inner.Left + Inner.Width * 0.5f, Inner.Top, Inner.Width * 0.5f, Inner.Height * 0.5f );

            if ( Spot == 7 )
                Mark = CRectangle( Inner.Left, Inner.Top + Inner.Height * 0.5f, Inner.Width * 0.5f, Inner.Height * 0.5f );

            if ( Spot == 8 )
                Mark = CRectangle( Inner.Left + Inner.Width * 0.5f, Inner.Top + Inner.Height * 0.5f, Inner.Width * 0.5f, Inner.Height * 0.5f );

            Canvas->Rectangle( Mark, Style->Accent.Fade( Chosen ? 0.85f : 0.4f ), 3.0f * Style->Scale );
        }

        Canvas->PopClip( );
        Canvas->Route( Former );
    }

    if ( Carrying && Input->MouseReleased( 0 ) ) {
        if ( Zone == 9 && Fellow != 0 ) {
            CFrameState* Candidate = Frames->Find( Fellow );

            if ( Candidate ) {
                unsigned int Isle = Allocate( );

                Nodes[ Isle ].Bounds = CRectangle( Candidate->Position, Candidate->Size );
                Nodes[ Isle ].Anchor = Isle;

                Isles.push_back( Isle );

                Insert( Fellow, Isle, 0 );
                Insert( Dragged, Isle, 0 );

                Candidate->Group = Isle;
                State->Group = Isle;

                Frames->Raise( Dragged );
                Context->ActiveItem = 0;
            }
        } else if ( Zone >= 0 && Target != 0 ) {
            if ( Zone >= 5 ) {
                Insert( Dragged, Target, Zone == 5 || Zone == 6 ? 3 : 4 );
                CFrameState* Landed = Frames->Find( Dragged );

                if ( Landed && Landed->Home != 0 )
                    Quarter( Landed->Home, Zone == 5 || Zone == 7 );
            } else {
                Insert( Dragged, Target, Zone );
            }

            Context->ActiveItem = 0;
        }

        State->Dragging = false;
        Flow = 0.0f;
    }

    if ( !Carrying )
        Flow = 0.0f;
}

void CDocking::Insert( unsigned int Window, unsigned int Node, int Zone ) {
    CFrameState* State = Frames->Find( Window );
    if ( !State )
        return;

    if ( Zone == 0 || Nodes[ Node ].Tabs.empty( ) ) {
        Nodes[ Node ].Tabs.push_back( Window );
        Nodes[ Node ].Front = Window;

        Nodes[ Node ].Keep = false;

        State->Home = Node;
        State->DockFront = true;

        State->HomeStamp = Context->FrameIndex;
        return;
    }

    unsigned int Keep = Allocate( );
    unsigned int Fresh = Allocate( );

    CDockNode& Split = Nodes[ Node ];

    Nodes[ Keep ].Parent = Node;
    Nodes[ Keep ].Tabs = Split.Tabs;

    Nodes[ Keep ].Front = Split.Front;

    Nodes[ Fresh ].Parent = Node;
    Nodes[ Fresh ].Tabs.push_back( Window );

    Nodes[ Fresh ].Front = Window;

    Split.Tabs.clear( );
    Split.Front = 0;

    Split.Ratio = 0.5f;
    Split.Vertical = Zone == 3 || Zone == 4;

    bool Leading = Zone == 1 || Zone == 3;

    Split.First = Leading ? Fresh : Keep;
    Split.Second = Leading ? Keep : Fresh;

    for ( unsigned int Moved : Nodes[ Keep ].Tabs ) {
        CFrameState* Kept = Frames->Find( Moved );

        if ( Kept ) {
            Kept->Home = Keep;
            Kept->HomeStamp = Context->FrameIndex;
        }
    }

    State->Home = Fresh;
    State->DockFront = true;

    State->HomeStamp = Context->FrameIndex;
}

void CDocking::Quarter( unsigned int Node, bool WestSide ) {
    unsigned int Full = Allocate( );
    unsigned int Empty = Allocate( );

    CDockNode& Strip = Nodes[ Node ];

    Nodes[ Full ].Parent = Node;
    Nodes[ Full ].Tabs = Strip.Tabs;

    Nodes[ Full ].Front = Strip.Front;

    Nodes[ Empty ].Parent = Node;
    Nodes[ Empty ].Keep = true;

    Strip.Tabs.clear( );
    Strip.Front = 0;

    Strip.Ratio = 0.5f;
    Strip.Vertical = false;

    Strip.First = WestSide ? Full : Empty;
    Strip.Second = WestSide ? Empty : Full;

    for ( unsigned int Moved : Nodes[ Full ].Tabs ) {
        CFrameState* Kept = Frames->Find( Moved );

        if ( Kept ) {
            Kept->Home = Full;
            Kept->HomeStamp = Context->FrameIndex;
        }
    }
}

bool CDocking::Adopt( const char* Title, int Zone ) {
    unsigned int Identifier = Context->Hash( Title );
    CFrameState* State = Frames->Find( Identifier );

    if ( !State || State->Home != 0 )
        return false;

    if ( Nodes.empty( ) )
        Nodes.resize( 1 );

    if ( Root == 0 || !Nodes[ Root ].Used )
        Root = Allocate( );

    unsigned int Leaf = Root;

    while ( !Nodes[ Leaf ].Leaf( ) )
        Leaf = Nodes[ Leaf ].First;

    Insert( Identifier, Leaf, Zone );
    return true;
}

void CDocking::Detach( unsigned int Window ) {
    CFrameState* State = Frames->Find( Window );
    if ( !State || State->Home == 0 || State->Home >= Nodes.size( ) )
        return;

    CDockNode& Node = Nodes[ State->Home ];

    for ( size_t Position = 0; Position < Node.Tabs.size( ); Position++ ) {
        if ( Node.Tabs[ Position ] == Window ) {
            Node.Tabs.erase( Node.Tabs.begin( ) + Position );
            break;
        }
    }

    if ( Node.Front == Window )
        Node.Front = Node.Tabs.empty( ) ? 0 : Node.Tabs[ 0 ];

    State->Home = 0;
    State->Group = 0;

    State->DockFront = false;
}

void CDocking::Save( std::ostream& Stream ) const {
    Stream << "docking " << Nodes.size( ) << " " << Root << "\n";

    for ( size_t Slot = 0; Slot < Nodes.size( ); Slot++ ) {
        const CDockNode& Node = Nodes[ Slot ];

        Stream << Slot << " " << ( Node.Used ? 1 : 0 ) << " " << Node.Parent << " " << Node.First << " " << Node.Second << " " << Node.Front << " " << ( Node.Vertical ? 1 : 0 ) << " " << ( Node.Keep ? 1 : 0 ) << " " << Node.Ratio << " " << Node.Bounds.Left << " " << Node.Bounds.Top << " " << Node.Bounds.Width << " " << Node.Bounds.Height << " " << Node.Tabs.size( );

        for ( unsigned int Tab : Node.Tabs )
            Stream << " " << Tab;

        Stream << "\n";
    }

    Stream << "isles " << Isles.size( );

    for ( unsigned int Isle : Isles )
        Stream << " " << Isle;

    Stream << "\n";
}

bool CDocking::Repair( ) {
    if ( Root >= Nodes.size( ) )
        return false;

    for ( size_t Slot = 1; Slot < Nodes.size( ); Slot++ ) {
        CDockNode& Node = Nodes[ Slot ];

        if ( !Node.Used )
            continue;

        if ( Node.Parent >= Nodes.size( ) || Node.First >= Nodes.size( ) || Node.Second >= Nodes.size( ) )
            return false;

        if ( !std::isfinite( Node.Ratio ) )
            Node.Ratio = 0.5f;

        if ( Node.Ratio < 0.08f )
            Node.Ratio = 0.08f;

        if ( Node.Ratio > 0.92f )
            Node.Ratio = 0.92f;

        if ( !std::isfinite( Node.Bounds.Left ) || !std::isfinite( Node.Bounds.Top ) || !std::isfinite( Node.Bounds.Width ) || !std::isfinite( Node.Bounds.Height ) )
            Node.Bounds = CRectangle( 60.0f, 60.0f, 380.0f, 320.0f );

        if ( Node.Leaf( ) ) {
            if ( Node.Second != 0 )
                return false;

            continue;
        }

        if ( Node.Second == 0 || Node.First == Slot || Node.Second == Slot || Node.First == Node.Second )
            return false;

        if ( !Nodes[ Node.First ].Used || !Nodes[ Node.Second ].Used )
            return false;

        if ( Nodes[ Node.First ].Parent != Slot || Nodes[ Node.Second ].Parent != Slot )
            return false;
    }

    return true;
}

void CDocking::Load( std::istream& Stream ) {
    std::string Tag;
    size_t Count = 0;

    Stream >> Tag >> Count >> Root;
    if ( Tag != "docking" || !Stream || Count > 4096 ) {
        Root = 0;
        return;
    }

    Nodes.assign( Count, CDockNode( ) );

    for ( size_t Index = 0; Index < Count; Index++ ) {
        size_t Slot = 0;
        int Used = 0;

        unsigned int Parent = 0;
        unsigned int First = 0;

        unsigned int Second = 0;
        unsigned int Front = 0;

        int Vertical = 0;
        int Keep = 0;

        float Ratio = 0.5f;
        float Left = 0.0f;

        float Top = 0.0f;
        float Wide = 0.0f;

        float Tall = 0.0f;
        size_t Tabs = 0;

        Stream >> Slot >> Used >> Parent >> First >> Second >> Front >> Vertical >> Keep >> Ratio >> Left >> Top >> Wide >> Tall >> Tabs;

        if ( !Stream || Tabs > 256 ) {
            Nodes.clear( );
            Isles.clear( );

            Root = 0;
            return;
        }

        if ( Slot >= Nodes.size( ) ) {
            for ( size_t Skip = 0; Skip < Tabs; Skip++ ) {
                unsigned int Waste = 0;
                Stream >> Waste;
            }

            continue;
        }

        CDockNode& Node = Nodes[ Slot ];

        Node.Used = Used != 0;
        Node.Parent = Parent;

        Node.First = First;
        Node.Second = Second;

        Node.Front = Front;
        Node.Vertical = Vertical != 0;

        Node.Keep = Keep != 0;
        Node.Ratio = Ratio;

        Node.Bounds = CRectangle( Left, Top, Wide, Tall );
        Node.Tabs.clear( );

        for ( size_t Position = 0; Position < Tabs; Position++ ) {
            unsigned int Window = 0;

            Stream >> Window;
            Node.Tabs.push_back( Window );
        }
    }

    std::string IsleTag;
    size_t IsleCount = 0;

    Stream >> IsleTag >> IsleCount;
    Isles.clear( );

    if ( !Stream || IsleCount > 4096 )
        IsleCount = 0;

    for ( size_t Index = 0; Index < IsleCount; Index++ ) {
        unsigned int Isle = 0;

        Stream >> Isle;
        Isles.push_back( Isle );
    }

    if ( !Repair( ) ) {
        Nodes.clear( );
        Isles.clear( );

        Root = 0;
    }
}

void CDocking::Space( CRectangle Bounds ) {
    Area = Bounds;

    if ( Nodes.empty( ) )
        Nodes.resize( 1 );

    if ( Root == 0 || !Nodes[ Root ].Used )
        Root = Allocate( );

    Prune( );
    Steer( );

    Arrange( Root, Area, Root );
    Islands( );

    Leaves( );
    Splitters( );

    Preview( );
}