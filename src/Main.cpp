#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <Shellapi.h>
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_NONE 0x00000000
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "catalog.hpp"
#include "store.hpp"
#include "offsets.hpp"
#include "world.hpp"
#include "move.hpp"
#include "silent.hpp"
#include "weather.hpp"
#include "gameplay.hpp"
#include "ur/ur.hpp"
#include "explorer.hpp"
#include "browse.hpp"

namespace {

constexpr float MenuWidth = 700.0f;
constexpr float MenuHeight = 610.0f;
constexpr float HeaderHeight = 52.0f;
constexpr float RailWidth = 80.0f;
constexpr float TabHeight = 70.0f;
constexpr float TabGap = 0.0f;
constexpr float ExploreWidth = 500.0f;
constexpr float ExploreHeight = 600.0f;
constexpr int TabCount = 5;
constexpr int TabAimbot = 0;
constexpr int TabEsp = 1;
constexpr int TabRage = 2;
constexpr int TabConfigs = 3;
constexpr int TabSettings = 4;

static const char* Fonts[ ] = {
    "Poppins",
    "Segoe UI",
    "Segoe UI Symbol"
};

struct TabSpec {
    const char* name;
    const char* id;
    ur::icons::Icon icon;
};

static const TabSpec Tabs[ TabCount ] = {
    { "Aimbot", "tab.aimbot", ur::icons::Icon::Crosshairs },
    { "ESP", "tab.esp", ur::icons::Icon::Eye },
    { "Rage", "tab.rage", ur::icons::Icon::FireFlame },
    { "Configs", "tab.configs", ur::icons::Icon::Folder },
    { "Settings", "tab.settings", ur::icons::Icon::Gear }
};

struct Shell {
    CVector origin;
    CVector grab;
    bool ready = false;
    bool painted = false;
    bool visible = true;
    bool held = false;
    bool mouse = false;
    bool escape = false;
    bool insert = false;
    int tab = 0;
    float tabAt = 0.0f;
    float pageIn = 1.0f;
    float pageDir = 1.0f;
    int menuKey = VK_INSERT;
    bool listen = false;
    bool limit = false;
    float fps = 240.0f;
    bool capped = false;
    bool misc = false;
    bool game = false;
    bool overlay = false;
    bool theme = false;
    bool slide = false;
    const char* knob = nullptr;
    bool afk = false;
    bool uncap = false;
    bool vsync = false;
    bool watermark = true;
    bool showFps = true;
    bool stream = false;
    bool streamed = false;
    float fade = 100.0f;
};

struct Combat {
    bool on = false;
    bool team = true;
    bool vis = true;
    bool sticky = false;
    bool pred = false;
    bool drawFov = true;
    float fov = 72.0f;
    float smooth = 40.0f;
    int key = VK_RBUTTON;
    int bones = 1;
    int sort = 0;
    bool listen = false;
    bool general = false;
    bool targeting = false;
};

struct Fury {
    bool jump = false;
    bool noclip = false;
};

struct Quiet {
    bool on = false;
    bool team = true;
    bool vis = true;
    bool pred = false;
    int key = 'M';
    int bones = 1;
    int sort = 0;
    bool listen = false;
    bool fold = false;
};

struct Vision {
    bool on = false;
    bool box = true;
    bool name = true;
    bool health = true;
    bool dist = true;
    bool skeleton = false;
    bool snap = false;
    bool team = true;
    bool overlay = false;
    bool visual = false;
    bool custom = false;
    float range = 500.0f;
};

enum EspFeat {
    FeatBox = 0,
    FeatName,
    FeatHealth,
    FeatDist,
    FeatSkel,
    FeatSnap,
    FeatCount
};

struct Coat {
    int feat = 0;
    int vis[ FeatCount ] = { 3, 9, 0, 9, 3, 3 };
    int hid[ FeatCount ] = { 12, 12, 11, 12, 12, 12 };
    int globVis = 3;
    int globHid = 12;
};

struct Vault {
    char names[ store::SlotMax ][ store::NameCap ] = { };
    int count = 0;
    int pick = 0;
    char live[ store::NameCap ] = "Default";
    char draft[ store::NameCap ] = { };
    bool type = false;
    bool confirm = false;
    bool ready = false;
    float scroll = 0.0f;
    float noteAge = 0.0f;
    char note[ 64 ] = { };
};

struct Browse {
    CVector origin;
    CVector grab;
    bool ready = false;
    bool open = false;
    bool held = false;
    bool docked = true;
    CVector dock;
    uintptr_t pick = 0;
    float scroll = 0.0f;
    float propScroll = 0.0f;
    char find[ 48 ] = { };
    bool type = false;
    bool confirm = false;
    bool mark = false;
    char note[ 64 ] = { };
    float noteAge = 0.0f;
};

static Shell Menu;
static Browse Tree;
static Combat Aim;
static Quiet Mute;
static Fury Rage;
static Vision Esp;
static Coat Dye;
static const CColor EspTints[ ] = {
    CColor( 72, 220, 118 ),
    CColor( 232, 72, 72 ),
    CColor( 64, 220, 230 ),
    CColor( 64, 132, 255 ),
    CColor( 168, 88, 255 ),
    CColor( 255, 96, 180 ),
    CColor( 255, 148, 48 ),
    CColor( 255, 220, 64 ),
    CColor( 160, 255, 64 ),
    CColor( 244, 244, 248 ),
    CColor( 232, 188, 72 ),
    CColor( 176, 24, 48 ),
    CColor( 18, 18, 22 )
};

static CColor FeatColor( int Feat, bool Seen ) {
    int Pick = Seen ? Dye.vis[ Feat ] : Dye.hid[ Feat ];
    if ( Pick < 0 || Pick >= 13 )
        Pick = 3;
    return EspTints[ Pick ];
}

static Vault Packs;

struct Channel {
    bool open = false;
    bool dismissed = false;
    bool mismatch = false;
    unsigned nextScan = 0;
    char client[ 48 ] = { };
    char dump[ 48 ] = { };
};

static Channel LiveCh;
static bool ChanMouse = false;

static bool KeyWas[ 256 ] = { };
static HANDLE FaceHandle = nullptr;
static char FacePath[ MAX_PATH ] = { };
static CFont TitleFace;
static float TitleScale = 0.0f;

struct Tone {
    CColor surface;
    CColor elevated;
    CColor header;
    CColor outline;
    CColor highlight;
    CColor text;
    CColor faint;
    CColor accent;
    CColor accentSoft;
    CColor shade;
    CColor card;
    CColor rail;
    CColor groove;
    CColor trackOff;
    CColor trackOn;
    CColor foldLine;
    CColor ink;
    CColor inkHot;
};

static Tone Dress;
static const char* DropId = nullptr;
static CRectangle DropField;
static const char* const* DropOpts = nullptr;
static int DropCount = 0;
static int* DropPick = nullptr;
static int* DropBits = nullptr;
static bool DropMany = false;
static bool DropFresh = false;

static bool Held( int Key ) {
    return ( GetAsyncKeyState( Key ) & 0x8000 ) != 0;
}

struct Stamp {
    CVector origin;
    CVector grab;
    bool ready = false;
    bool held = false;
};

static Stamp Badge;

static bool Moving( ) {
    return Menu.held || Tree.held || Badge.held;
}

static bool Edge( int Key, bool& Prior ) {
    bool Now = Held( Key );
    bool Hit = Now && !Prior;
    Prior = Now;
    return Hit;
}

static CVector Cursor( ) {
    HWND Handle = ( HWND )ur::app::window( );
    POINT Point = { };
    GetCursorPos( &Point );
    if ( Handle )
        ScreenToClient( Handle, &Point );
    return CVector( ( float )Point.x, ( float )Point.y );
}

static bool CursorVisible( ) {
    if ( Menu.visible )
        return true;
    CURSORINFO Info = { };
    Info.cbSize = sizeof( Info );
    if ( !GetCursorInfo( &Info ) )
        return false;
    return ( Info.flags & CURSOR_SHOWING ) != 0;
}

static CVector ScreenMid( ) {
    const world::Snap& Live = world::View( );
    if ( Live.clientW > 64 && Live.clientH > 64 ) {
        POINT Mid{ Live.clientX + Live.clientW / 2, Live.clientY + Live.clientH / 2 };
        HWND Overlay = ( HWND )ur::app::window( );
        if ( Overlay )
            ScreenToClient( Overlay, &Mid );
        return CVector( ( float )Mid.x, ( float )Mid.y );
    }
    return CVector( ( float )ur::app::width( ) * 0.5f, ( float )ur::app::height( ) * 0.5f );
}

static void Tokens( ) {
    if ( !Menu.painted ) {
        ur::theme::apply( 4 );
        Style->Rounding = 14.0f;
        Style->FadeSpeed = 28.0f;
        Style->Glass = true;
        Style->Shadows = false;
        Menu.painted = true;
    }

    static const Tone Set[ 3 ] = {
        {
            CColor( 14, 16, 20, 208 ), CColor( 16, 18, 22, 168 ), CColor( 16, 18, 24, 230 ),
            CColor( 80, 110, 150, 70 ), CColor( 160, 196, 255, 22 ), CColor( 228, 234, 244 ),
            CColor( 122, 134, 154 ), CColor( 88, 168, 255 ), CColor( 140, 190, 255 ),
            CColor( 8, 12, 22, 120 ), CColor( 36, 40, 52, 242 ), CColor( 10, 12, 16, 140 ),
            CColor( 24, 28, 36, 230 ), CColor( 28, 32, 40, 230 ), CColor( 48, 92, 150, 230 ),
            CColor( 78, 90, 112, 110 ), CColor( 214, 220, 232 ), CColor( 236, 240, 248 )
        },
        {
            CColor( 22, 16, 12, 208 ), CColor( 28, 20, 14, 168 ), CColor( 24, 18, 14, 230 ),
            CColor( 140, 100, 60, 70 ), CColor( 220, 170, 110, 22 ), CColor( 240, 226, 208 ),
            CColor( 150, 122, 96 ), CColor( 196, 132, 72 ), CColor( 220, 176, 120 ),
            CColor( 16, 10, 6, 120 ), CColor( 48, 34, 24, 242 ), CColor( 16, 10, 8, 140 ),
            CColor( 32, 22, 16, 230 ), CColor( 36, 26, 20, 230 ), CColor( 140, 88, 48, 230 ),
            CColor( 130, 96, 64, 110 ), CColor( 226, 208, 186 ), CColor( 244, 232, 214 )
        },
        {
            CColor( 14, 18, 14, 208 ), CColor( 16, 22, 16, 168 ), CColor( 16, 22, 16, 230 ),
            CColor( 90, 130, 80, 70 ), CColor( 160, 210, 140, 22 ), CColor( 228, 236, 220 ),
            CColor( 118, 140, 112 ), CColor( 112, 168, 86 ), CColor( 160, 200, 130 ),
            CColor( 8, 14, 8, 120 ), CColor( 32, 42, 32, 242 ), CColor( 10, 14, 10, 140 ),
            CColor( 22, 30, 22, 230 ), CColor( 26, 34, 26, 230 ), CColor( 72, 120, 64, 230 ),
            CColor( 80, 112, 78, 110 ), CColor( 210, 224, 206 ), CColor( 232, 240, 226 )
        }
    };

    int Index = skin::tone( );
    if ( Index < 0 || Index >= 3 )
        Index = 0;
    Dress = Set[ Index ];
    Style->Backdrop = CColor( 0, 0, 0, 0 );
    Style->Surface = Dress.surface;
    Style->Elevated = Dress.elevated;
    Style->Header = Dress.header;
    Style->Outline = Dress.outline;
    Style->Highlight = Dress.highlight;
    Style->Text = Dress.text;
    Style->Faint = Dress.faint;
    Style->Accent = Dress.accent;
    Style->AccentSoft = Dress.accentSoft;
    Style->Shade = Dress.shade;
}

static void Center( float Across, float Vertical, float Scale ) {
    Menu.origin = CVector( ( Across - MenuWidth * Scale ) * 0.5f, ( Vertical - MenuHeight * Scale ) * 0.5f );
}

static void OpenLiveFolds( ) {
    Aim.general = Aim.on;
    Aim.targeting = Aim.on;
    Rage.jump = move::Live( ).jump || move::Live( ).infJump;
    Rage.noclip = move::Live( ).noclip;
    Mute.fold = Mute.on;
    Esp.overlay = Esp.on;
    Esp.visual = Esp.skeleton || Esp.snap;
    Esp.custom = false;
    Menu.misc = false;
    Menu.game = false;
    Menu.overlay = false;
    Menu.theme = false;
}

static void ClampBox( CVector& Origin, float Across, float Vertical, float Wide, float Tall ) {
    if ( Origin.Horizontal < 8.0f )
        Origin.Horizontal = 8.0f;
    if ( Origin.Vertical < 8.0f )
        Origin.Vertical = 8.0f;
    if ( Origin.Horizontal + Wide > Across - 8.0f )
        Origin.Horizontal = Across - Wide - 8.0f;
    if ( Origin.Vertical + Tall > Vertical - 8.0f )
        Origin.Vertical = Vertical - Tall - 8.0f;
}

static void Clamp( float Across, float Vertical, float Wide, float Tall ) {
    ClampBox( Menu.origin, Across, Vertical, Wide, Tall );
}

static void Gate( bool Over, const CVector& Point ) {
    ur::overlay::Options& Overlay = ur::app::overlay_options( );
    bool Through = !Over && !Moving( ) && !Menu.slide;
    if ( Overlay.click_through != Through )
        Overlay.click_through = Through;

    Input->ApplyPosition( Point.Horizontal, Point.Vertical );
    if ( Menu.held )
        Input->Pointer = PointerMove;
    else if ( Over )
        Input->Pointer = PointerHand;
}

static void Drag( const CRectangle& Bounds, const CVector& Point, float Across, float Vertical, float Wide, float Tall, bool AllowStart ) {
    bool Press = Held( VK_LBUTTON );

    if ( Press && !Menu.mouse && AllowStart && Bounds.Contains( Point ) && !Menu.slide ) {
        Menu.held = true;
        Menu.grab = Point - Menu.origin;
    }

    if ( Menu.held ) {
        if ( Press )
            Menu.origin = Point - Menu.grab;
        else
            Menu.held = false;
    }

    if ( !Press ) {
        Menu.slide = false;
        Menu.knob = nullptr;
    }

    Menu.mouse = Press;
    Clamp( Across, Vertical, Wide, Tall );
}

static const char* KeyLabel( int Code ) {
    switch ( Code ) {
    case VK_LBUTTON: return "Mouse 1";
    case VK_RBUTTON: return "Mouse 2";
    case VK_MBUTTON: return "Mouse 3";
    case VK_XBUTTON1: return "Mouse 4";
    case VK_XBUTTON2: return "Mouse 5";
    case VK_INSERT: return "Insert";
    case VK_DELETE: return "Delete";
    case VK_HOME: return "Home";
    case VK_END: return "End";
    case VK_PRIOR: return "Page Up";
    case VK_NEXT: return "Page Down";
    case VK_SPACE: return "Space";
    case VK_TAB: return "Tab";
    case VK_RETURN: return "Enter";
    case VK_BACK: return "Backspace";
    case VK_PAUSE: return "Pause";
    case VK_SNAPSHOT: return "Print Screen";
    case VK_LEFT: return "Left";
    case VK_RIGHT: return "Right";
    case VK_UP: return "Up";
    case VK_DOWN: return "Down";
    case VK_SHIFT:
    case VK_LSHIFT: return "Shift";
    case VK_RSHIFT: return "Right Shift";
    case VK_CONTROL:
    case VK_LCONTROL: return "Ctrl";
    case VK_RCONTROL: return "Right Ctrl";
    case VK_MENU:
    case VK_LMENU: return "Alt";
    case VK_RMENU: return "Right Alt";
    case VK_CAPITAL: return "Caps Lock";
    case VK_NUMLOCK: return "Num Lock";
    case VK_SCROLL: return "Scroll Lock";
    case VK_OEM_1: return ";";
    case VK_OEM_PLUS: return "=";
    case VK_OEM_COMMA: return ",";
    case VK_OEM_MINUS: return "-";
    case VK_OEM_PERIOD: return ".";
    case VK_OEM_2: return "/";
    case VK_OEM_3: return "`";
    case VK_OEM_4: return "[";
    case VK_OEM_5: return "\\";
    case VK_OEM_6: return "]";
    case VK_OEM_7: return "'";
    case VK_MULTIPLY: return "Num *";
    case VK_ADD: return "Num +";
    case VK_SUBTRACT: return "Num -";
    case VK_DECIMAL: return "Num .";
    case VK_DIVIDE: return "Num /";
    default:
        break;
    }

    if ( Code >= VK_NUMPAD0 && Code <= VK_NUMPAD9 ) {
        static char Num[ 8 ];
        snprintf( Num, sizeof( Num ), "Num %d", Code - VK_NUMPAD0 );
        return Num;
    }

    if ( Code >= VK_F1 && Code <= VK_F24 ) {
        static char Fn[ 8 ];
        snprintf( Fn, sizeof( Fn ), "F%d", Code - VK_F1 + 1 );
        return Fn;
    }

    if ( Code >= '0' && Code <= '9' ) {
        static char Digit[ 2 ];
        Digit[ 0 ] = ( char )Code;
        Digit[ 1 ] = 0;
        return Digit;
    }

    if ( Code >= 'A' && Code <= 'Z' ) {
        static char Letter[ 2 ];
        Letter[ 0 ] = ( char )Code;
        Letter[ 1 ] = 0;
        return Letter;
    }

    static char Fallback[ 16 ];
    snprintf( Fallback, sizeof( Fallback ), "Key %d", Code );
    return Fallback;
}

static int PollBind( bool Mouse1 ) {
    int Hit = 0;
    for ( int Code = 1; Code < 256; Code++ ) {
        if ( Code == VK_ESCAPE )
            continue;
        if ( Code == VK_LBUTTON && !Mouse1 )
            continue;
        bool Now = Held( Code );
        if ( Now && !KeyWas[ Code ] )
            Hit = Code;
        KeyWas[ Code ] = Now;
    }
    return Hit;
}

static void SyncBindKeys( ) {
    for ( int Code = 1; Code < 256; Code++ )
        KeyWas[ Code ] = Held( Code );
}

static void Pace( ) {
    static LARGE_INTEGER Freq = { };
    static LARGE_INTEGER Last = { };
    if ( Freq.QuadPart == 0 )
        QueryPerformanceFrequency( &Freq );

    bool WantSync = Menu.vsync && !Menu.limit;
    if ( ur::app::vsync( ) != WantSync ) {
        ur::app::set_vsync( WantSync );
        Last.QuadPart = 0;
    }
    Menu.capped = Menu.limit;

    if ( !Menu.limit )
        return;

    float Cap = Menu.fps;
    if ( Cap < 60.0f )
        Cap = 60.0f;
    if ( Cap > 1000.0f )
        Cap = 1000.0f;

    LARGE_INTEGER Now = { };
    QueryPerformanceCounter( &Now );
    if ( Last.QuadPart != 0 ) {
        double Goal = 1.0 / ( double )Cap;
        for ( ;; ) {
            QueryPerformanceCounter( &Now );
            double Spent = ( double )( Now.QuadPart - Last.QuadPart ) / ( double )Freq.QuadPart;
            if ( Spent >= Goal )
                break;
            if ( Goal - Spent > 0.002 )
                Sleep( 1 );
        }
    }
    QueryPerformanceCounter( &Last );
}

static CColor Mix( CColor From, CColor Till, float Amount ) {
    return From.Blend( Till, Amount );
}

static void EnsureTitle( float Scale ) {
    if ( TitleFace.LineSpan < 1.0f )
        TitleFace.Create( Fonts, 3, 22.0f, 600 );
    if ( TitleScale != Scale ) {
        TitleFace.Rescale( Scale );
        TitleScale = Scale;
    }
}

static void DrawIce( const CRectangle& Clip, const CRectangle& Fill, float Round, float Amount ) {
    if ( Amount < 0.01f )
        return;

    float Keep = Canvas->Opacity;
    Canvas->Opacity = Keep * Amount;
    Canvas->PushClip( Clip );
    unsigned int Former = Canvas->Effect( skin::effect( ) );
    Canvas->Rectangle( Fill, CColor( 255, 255, 255 ), Round );
    Canvas->Effect( Former );
    Canvas->PopClip( );
    Canvas->Opacity = Keep;
}

static void DrawTitle( const CRectangle& Header, float Scale, const char* Title ) {
    EnsureTitle( Scale );
    CVector Size = TitleFace.Measure( Title );
    float Left = Header.Left + ( Header.Width - Size.Horizontal ) * 0.5f;
    float Top = Header.Top + ( Header.Height - TitleFace.LineSpan ) * 0.5f;
    Canvas->Write( &TitleFace, CVector( Left, Top ), Dress.inkHot.Fade( 0.93f ), Title );
}

static CRectangle TabBounds( const CRectangle& Rail, float Scale, int Index ) {
    float Step = TabHeight * Scale + TabGap * Scale;
    return CRectangle( Rail.Left, Rail.Top + Step * ( float )Index, Rail.Width, TabHeight * Scale );
}

static CRectangle TabPlate( const CRectangle& Tab, bool Top, bool Bot, float Round ) {
    if ( Top && !Bot )
        return CRectangle( Tab.Left, Tab.Top, Tab.Width, Tab.Height + Round );
    if ( Bot && !Top )
        return CRectangle( Tab.Left, Tab.Top - Round, Tab.Width, Tab.Height + Round );
    return Tab;
}

static void DrawTabPlate( const CRectangle& Tab, int Index, float Round, CColor Fill, unsigned int Effect, float Amount ) {
    if ( Amount < 0.01f )
        return;

    bool Top = Index == 0;
    bool Bot = Index == TabCount - 1;
    bool Cap = Top || Bot;
    float Use = Cap ? Round : 0.0f;
    float Keep = Canvas->Opacity;
    Canvas->Opacity = Keep * Amount;
    if ( Cap )
        Canvas->PushClip( Tab );
    unsigned int Former = Effect ? Canvas->Effect( Effect ) : 0;
    Canvas->Rectangle( TabPlate( Tab, Top, Bot, Round ), Fill, Use );
    if ( Effect )
        Canvas->Effect( Former );
    if ( Cap )
        Canvas->PopClip( );
    Canvas->Opacity = Keep;
}

static void DrawTabSwipe( const CRectangle& Rail, float Scale ) {
    float Want = ( float )Menu.tab;
    float Step = 20.0f * Context->DeltaTime;
    if ( Step > 1.0f )
        Step = 1.0f;
    Menu.tabAt += ( Want - Menu.tabAt ) * Step;

    float Stride = TabHeight * Scale + TabGap * Scale;
    float Tall = TabHeight * Scale;
    float Round = 12.0f * Scale;
    CRectangle Stack( Rail.Left, Rail.Top, Rail.Width, Stride * ( float )TabCount );
    CRectangle Fill( Rail.Left, Rail.Top + Stride * Menu.tabAt, Rail.Width, Tall );
    bool Top = Fill.Top <= Stack.Top + 0.75f;
    bool Bot = Fill.Bottom( ) >= Stack.Bottom( ) - 0.75f;
    float Use = ( Top || Bot ) ? Round : 0.0f;
    Canvas->PushClip( Stack );
    DrawIce( Fill, TabPlate( Fill, Top, Bot, Round ), Use, 1.0f );
    Canvas->PopClip( );
}

static void DrawTab( const CRectangle& Tab, const TabSpec& Spec, int Index, float Scale, bool Hovered ) {
    char HoverId[ 48 ];
    snprintf( HoverId, sizeof( HoverId ), "%s.hover", Spec.id );

    bool Selected = Menu.tab == Index;
    float Dist = Menu.tabAt - ( float )Index;
    if ( Dist < 0.0f )
        Dist = -Dist;
    float Active = Dist < 1.0f ? 1.0f - Dist : 0.0f;
    float Hover = ur::motion::toward( HoverId, ( Hovered && !Selected ) ? 1.0f : 0.0f, 26.0f );
    float Round = 12.0f * Scale;
    float Mark = 24.0f * Scale;

    DrawTabPlate( Tab, Index, Round, CColor( 255, 255, 255, 18 ), 0, Hover * ( 1.0f - Active ) );

    unsigned long long Icon = ur::glyphs::image( Spec.icon, ( int )( 26.0f * Scale + 0.5f ), ur::glyphs::Weight::Solid );
    CRectangle Glyph( Tab.Left + ( Tab.Width - Mark ) * 0.5f, Tab.Top + 13.0f * Scale, Mark, Mark );
    CColor Ink = Mix( Mix( Style->Faint, Dress.ink, Hover ), Dress.inkHot, Active );
    if ( Icon )
        Canvas->Image( Glyph, Icon, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), Ink, 0.0f );

    CVector Size = Font->Measure( Spec.name );
    float LabelTop = Glyph.Bottom( ) + 7.0f * Scale;
    Canvas->Text( CVector( Tab.Left + ( Tab.Width - Size.Horizontal ) * 0.5f, LabelTop ), Ink, Spec.name );
}

static CRectangle CloseBounds( const CRectangle& Header, float Scale ) {
    float Size = 32.0f * Scale;
    return CRectangle( Header.Right( ) - Size - 8.0f * Scale, Header.Top + ( Header.Height - Size ) * 0.5f, Size, Size );
}

static bool DrawClose( const CRectangle& Header, const CVector& Point, bool Click, float Scale, const char* Motion, bool Exit ) {
    CRectangle Close = CloseBounds( Header, Scale );
    bool Over = Close.Contains( Point ) && !Moving( );
    float Hover = ur::motion::toward( Motion, Over ? 1.0f : 0.0f, 26.0f );
    float Round = 9.0f * Scale;

    if ( Hover > 0.02f )
        Canvas->Rectangle( Close, CColor( 210, 64, 72, ( int )( 200.0f * Hover ) ), Round );

    unsigned long long Icon = ur::glyphs::image( ur::icons::Icon::Xmark, ( int )( 14.0f * Scale + 0.5f ), ur::glyphs::Weight::Solid );
    CColor Ink = Mix( CColor( 220, 226, 236 ), CColor( 255, 246, 246 ), Hover );
    float Mark = 14.0f * Scale;
    if ( Icon )
        Canvas->Image( CRectangle( Close.Left + ( Close.Width - Mark ) * 0.5f, Close.Top + ( Close.Height - Mark ) * 0.5f, Mark, Mark ), Icon, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), Ink, 0.0f );

    if ( Over && Click && Exit )
        ur::app::quit( );

    return Over;
}

static bool Listening( ) {
    return Menu.listen || Aim.listen || Mute.listen || Packs.type || Tree.type;
}

static bool DrawSlider( float Left, float Top, float Wide, const char* Label, const char* Id, float& Value, float Lo, float Hi, const CVector& Point, bool Click, bool Press, float Scale ) {
    if ( Value < Lo )
        Value = Lo;
    if ( Value > Hi )
        Value = Hi;
    Value = ( float )( int )( Value + 0.5f );

    float Row = 26.0f * Scale;
    float TextW = 0.0f;
    if ( Label ) {
        Canvas->Text( CVector( Left, Top + ( Row - Font->LineSpan ) * 0.5f ), Style->Text, Label );
        TextW = Font->Measure( Label ).Horizontal + 10.0f * Scale;
    }

    char Stamp[ 24 ];
    snprintf( Stamp, sizeof( Stamp ), "%d", ( int )Value );
    CVector Size = Font->Measure( Stamp );
    float Thumb = 14.0f * Scale;
    float GrooveW = Wide - TextW - Size.Horizontal - Thumb - 18.0f * Scale;
    if ( GrooveW < 48.0f * Scale )
        GrooveW = 48.0f * Scale;
    CRectangle Groove( Left + TextW, Top + 10.0f * Scale, GrooveW, 6.0f * Scale );
    CRectangle Hit( Left, Top, Wide, Row );
    bool Mine = Menu.slide && Menu.knob == Id;
    bool Over = Hit.Contains( Point ) && !Moving( ) && !Listening( );
    if ( Over && Click ) {
        Menu.slide = true;
        Menu.knob = Id;
        Mine = true;
    }
    if ( !Press ) {
        Menu.slide = false;
        Menu.knob = nullptr;
        Mine = false;
    }
    if ( Mine && Groove.Width > 1.0f ) {
        float Ratio = ( Point.Horizontal - Groove.Left ) / Groove.Width;
        if ( Ratio < 0.0f )
            Ratio = 0.0f;
        if ( Ratio > 1.0f )
            Ratio = 1.0f;
        Value = ( float )( int )( Lo + Ratio * ( Hi - Lo ) + 0.5f );
    }

    float Portion = ( Value - Lo ) / ( Hi - Lo );
    if ( Portion < 0.0f )
        Portion = 0.0f;
    if ( Portion > 1.0f )
        Portion = 1.0f;
    Canvas->Text( CVector( Groove.Right( ) + Thumb * 0.5f + 10.0f * Scale, Top + ( Row - Font->LineSpan ) * 0.5f ), Style->Text, Stamp );
    Canvas->Rectangle( Groove, Dress.groove, 2.5f * Scale );
    if ( Portion > 0.0f )
        Canvas->Rectangle( CRectangle( Groove.Left, Groove.Top, Groove.Width * Portion, Groove.Height ), Mix( Style->Accent, Style->AccentSoft, 0.35f ), 2.5f * Scale );
    Canvas->Rectangle( CRectangle( Groove.Left + Groove.Width * Portion - Thumb * 0.5f, Groove.Top + ( Groove.Height - Thumb ) * 0.5f, Thumb, Thumb ), CColor( 236, 242, 252 ), Thumb * 0.5f );
    return Over || Mine;
}

static bool DrawBind( float Left, float Top, const char* Label, const char* Motion, int& Code, bool& Listen, const CVector& Point, bool Click, float Scale ) {
    Canvas->Text( CVector( Left, Top ), Style->Faint, Label );
    CRectangle Field( Left, Top + Font->LineSpan + 6.0f * Scale, 156.0f * Scale, 30.0f * Scale );
    bool Over = Field.Contains( Point ) && !Moving( ) && !Menu.slide;
    if ( Over && Click ) {
        Listen = true;
        SyncBindKeys( );
    }

    float Wait = ur::motion::toward( Motion, Listen ? 1.0f : 0.0f, 26.0f );
    char HoverId[ 48 ];
    snprintf( HoverId, sizeof( HoverId ), "%s.hover", Motion );
    float Hover = ur::motion::toward( HoverId, ( Over && !Listen ) ? 1.0f : 0.0f, 26.0f );
    DrawIce( Field, Field, 6.0f * Scale, 1.0f );
    Canvas->Border( Field, Mix( CColor( 90, 110, 140, 160 ), Style->AccentSoft, Wait * 0.75f + Hover * 0.4f ), 6.0f * Scale, 1.0f );
    Canvas->Text( CVector( Field.Left + 10.0f * Scale, Field.Top + ( Field.Height - Font->LineSpan ) * 0.5f ), Mix( Style->Text, Style->AccentSoft, Wait ), Listen ? "Press a key..." : KeyLabel( Code ) );
    return Over;
}

static bool DrawSwitch( const CRectangle& Row, const char* Label, const char* Id, bool& Value, const CVector& Point, bool Click, float Scale );

static bool DrawMiscBody( const CRectangle& Body, const CVector& Point, bool Click, bool Press, float Scale ) {
    float Left = Body.Left + 12.0f * Scale;
    float Wide = Body.Width - 24.0f * Scale;
    float Top = Body.Top + 8.0f * Scale;
    float Row = 28.0f * Scale;
    bool Busy = false;

    CRectangle LimitRow( Left, Top, Wide, Row );
    float TrackW = 44.0f * Scale;
    float TrackH = 22.0f * Scale;
    CRectangle Track( LimitRow.Right( ) - TrackW, LimitRow.Top + ( Row - TrackH ) * 0.5f, TrackW, TrackH );
    bool OverLimit = LimitRow.Contains( Point ) && !Moving( );
    if ( OverLimit && Click )
        Menu.limit = !Menu.limit;

    float On = ur::motion::toward( "set.limit", Menu.limit ? 1.0f : 0.0f, 28.0f );
    Canvas->Text( CVector( Left, LimitRow.Top + ( Row - Font->LineSpan ) * 0.5f ), Style->Text, "Limit FPS" );
    if ( Menu.limit ) {
        char Live[ 24 ];
        snprintf( Live, sizeof( Live ), "%.0f", ( double )Context->Framerate );
        CVector LiveSize = Font->Measure( Live );
        Canvas->Text( CVector( Track.Left - 8.0f * Scale - LiveSize.Horizontal, LimitRow.Top + ( Row - Font->LineSpan ) * 0.5f ), Style->Faint, Live );
    }
    Canvas->Rectangle( Track, Mix( Dress.trackOff, Mix( Dress.trackOn, Style->Accent, 0.4f ), On ), TrackH * 0.5f );
    float Knob = 18.0f * Scale;
    CRectangle Dot( Track.Left + 2.0f * Scale + ( TrackW - Knob - 4.0f * Scale ) * On, Track.Top + ( TrackH - Knob ) * 0.5f, Knob, Knob );
    Canvas->Rectangle( Dot, Dress.inkHot, Knob * 0.5f );
    Busy = Busy || OverLimit;

    float Slide = ur::motion::toward( "set.slider", Menu.limit ? 1.0f : 0.0f, 32.0f );
    if ( Slide > 0.02f ) {
        CRectangle SlideRow( Left, LimitRow.Bottom( ) + 4.0f * Scale, Wide, 26.0f * Scale );
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * Slide;
        if ( Menu.fps < 60.0f )
            Menu.fps = 60.0f;
        if ( Menu.fps > 1000.0f )
            Menu.fps = 1000.0f;
        Menu.fps = ( float )( int )( Menu.fps + 0.5f );

        Busy = DrawSlider( Left, SlideRow.Top, Wide, nullptr, "set.fps", Menu.fps, 60.0f, 1000.0f, Point, Click, Press, Scale ) || Busy;
        Canvas->Opacity = Amount;
        Top = SlideRow.Bottom( ) + 6.0f * Scale;
    } else {
        Top = LimitRow.Bottom( ) + 6.0f * Scale;
    }

    CRectangle SyncRow( Left, Top, Wide, Row );
    Busy = DrawSwitch( SyncRow, "VSync", "set.vsync", Menu.vsync, Point, Click, Scale ) || Busy;
    Top = SyncRow.Bottom( ) + 6.0f * Scale;

    Canvas->Text( CVector( Left, Top ), Style->Faint, "Toggle menu" );
    CRectangle Field( Left, Top + Font->LineSpan + 4.0f * Scale, 156.0f * Scale, 28.0f * Scale );
    bool OverBind = Field.Contains( Point ) && !Moving( );
    if ( OverBind && Click ) {
        Menu.listen = true;
        SyncBindKeys( );
    }

    float Wait = ur::motion::toward( "set.listen", Menu.listen ? 1.0f : 0.0f, 26.0f );
    float BindHover = ur::motion::toward( "set.bind.hover", ( OverBind && !Menu.listen ) ? 1.0f : 0.0f, 26.0f );
    DrawIce( Field, Field, 6.0f * Scale, 1.0f );
    Canvas->Border( Field, Mix( CColor( 90, 110, 140, 160 ), Style->AccentSoft, Wait * 0.75f + BindHover * 0.4f ), 6.0f * Scale, 1.0f );
    Canvas->Text( CVector( Field.Left + 10.0f * Scale, Field.Top + ( Field.Height - Font->LineSpan ) * 0.5f ), Mix( Style->Text, Style->AccentSoft, Wait ), Menu.listen ? "Press a key..." : KeyLabel( Menu.menuKey ) );
    return Busy || OverBind;
}

static bool DrawSwitch( const CRectangle& Row, const char* Label, const char* Id, bool& Value, const CVector& Point, bool Click, float Scale ) {
    float TrackW = 44.0f * Scale;
    float TrackH = 22.0f * Scale;
    CRectangle Track( Row.Right( ) - TrackW, Row.Top + ( Row.Height - TrackH ) * 0.5f, TrackW, TrackH );
    bool Over = Row.Contains( Point ) && !Moving( ) && !Menu.slide;
    if ( Over && Click )
        Value = !Value;

    float On = ur::motion::toward( Id, Value ? 1.0f : 0.0f, 28.0f );
    Canvas->Text( CVector( Row.Left, Row.Top + ( Row.Height - Font->LineSpan ) * 0.5f ), Style->Text, Label );
    Canvas->Rectangle( Track, Mix( Dress.trackOff, Mix( Dress.trackOn, Style->Accent, 0.4f ), On ), TrackH * 0.5f );
    float Knob = 18.0f * Scale;
    Canvas->Rectangle( CRectangle( Track.Left + 2.0f * Scale + ( TrackW - Knob - 4.0f * Scale ) * On, Track.Top + ( TrackH - Knob ) * 0.5f, Knob, Knob ), Dress.inkHot, Knob * 0.5f );
    return Over;
}

static bool DrawGameBody( const CRectangle& Body, const CVector& Point, bool Click, float Scale ) {
    float Left = Body.Left + 14.0f * Scale;
    float Wide = Body.Width - 28.0f * Scale;
    float Top = Body.Top + 10.0f * Scale;
    float Row = 32.0f * Scale;
    bool Busy = false;

    CRectangle First( Left, Top, Wide, Row );
    Busy = DrawSwitch( First, "Anti-AFK", "game.afk", Menu.afk, Point, Click, Scale ) || Busy;
    CRectangle Second( Left, First.Bottom( ) + 2.0f * Scale, Wide, Row );
    Busy = DrawSwitch( Second, "Uncapped FPS", "game.uncap", Menu.uncap, Point, Click, Scale ) || Busy;
    CRectangle Third( Left, Second.Bottom( ) + 2.0f * Scale, Wide, Row );
    Canvas->Text( CVector( Third.Left, Third.Top + ( Row - Font->LineSpan ) * 0.5f ), Style->Text, "Game Explorer" );
    const char* Action = Tree.open ? "Close" : "Open";
    CVector OpenSize = Font->Measure( "Close" );
    CVector ActionSize = Font->Measure( Action );
    float ChipW = OpenSize.Horizontal + 16.0f * Scale;
    float ChipH = 22.0f * Scale;
    CRectangle Chip( Third.Right( ) - ChipW, Third.Top + ( Row - ChipH ) * 0.5f, ChipW, ChipH );
    bool OverOpen = Chip.Contains( Point ) && !Moving( ) && !Menu.slide;
    float Hover = ur::motion::toward( "game.explorer.open", OverOpen ? 1.0f : 0.0f, 26.0f );
    CColor Ink = Tree.open
        ? Mix( CColor( 214, 220, 232 ), CColor( 232, 64, 72 ), Hover )
        : Mix( Style->AccentSoft, CColor( 220, 236, 255 ), Hover );
    Canvas->Text( CVector( Chip.Left + ( ChipW - ActionSize.Horizontal ) * 0.5f, Chip.Top + ( ChipH - Font->LineSpan ) * 0.5f ), Ink, Action );
    if ( OverOpen && Click ) {
        if ( Tree.open ) {
            Tree.open = false;
            Tree.type = false;
            Tree.confirm = false;
            browse::Close( );
        } else {
            Tree.open = true;
            Tree.docked = true;
            Tree.ready = false;
            Tree.confirm = false;
            browse::Open( );
            Tree.pick = browse::Pick( );
        }
    }
    Busy = Busy || OverOpen;

    CRectangle Fourth( Left, Third.Bottom( ) + 2.0f * Scale, Wide, Row );
    Canvas->Text( CVector( Fourth.Left, Fourth.Top + ( Row - Font->LineSpan ) * 0.5f ), Style->Text, "Offsets" );
    const char* Refresh = "Refresh";
    CVector RefreshSize = Font->Measure( Refresh );
    float RefreshW = RefreshSize.Horizontal + 16.0f * Scale;
    CRectangle RefreshChip( Fourth.Right( ) - RefreshW, Fourth.Top + ( Row - ChipH ) * 0.5f, RefreshW, ChipH );
    CRectangle RefreshHit( RefreshChip.Left - 6.0f * Scale, Fourth.Top, RefreshW + 6.0f * Scale, Row );
    bool Work = offsets::Busy( );
    bool OverRefresh = RefreshHit.Contains( Point ) && !Moving( ) && !Menu.slide && !Work;
    float Spin = ur::motion::toward( "game.offsets.spin", Work ? 1.0f : 0.0f, 22.0f );
    float RefreshHover = ur::motion::toward( "game.offsets.refresh", OverRefresh ? 1.0f : 0.0f, 26.0f );
    float Pulse = 0.55f + 0.45f * ( 0.5f + 0.5f * sinf( ( float )Context->Elapsed * 7.0f ) );
    if ( Spin > 0.02f )
        DrawIce( RefreshChip, RefreshChip, ChipH * 0.5f, Spin * ( 0.4f + Pulse * 0.6f ) );
    Canvas->Text(
        CVector( RefreshChip.Left + ( RefreshW - RefreshSize.Horizontal ) * 0.5f, RefreshChip.Top + ( ChipH - Font->LineSpan ) * 0.5f ),
        Mix( Mix( Style->AccentSoft, CColor( 220, 236, 255 ), RefreshHover ), Dress.inkHot, Spin ),
        Refresh
    );

    char Status[ 48 ] = { };
    CColor Note = Mix( Style->Faint, Style->AccentSoft, 0.45f );
    if ( Work || Spin > 0.08f ) {
        const char* Step = offsets::StageText( );
        if ( !Step[ 0 ] )
            Step = "updating";
        int Dots = ( ( int )( Context->Elapsed * 4.0 ) % 3 ) + 1;
        snprintf( Status, sizeof( Status ), "%s%.*s", Step, Dots, "..." );
        Note = Mix( Style->AccentSoft, Dress.inkHot, Pulse );
    } else if ( offsets::Fresh( ) ) {
        lstrcpynA( Status, offsets::Ready( ) ? "updated" : "failed", ( int )sizeof( Status ) );
        Note = offsets::Ready( ) ? Mix( Style->AccentSoft, CColor( 160, 220, 180 ), 0.55f ) : Mix( Style->Faint, CColor( 232, 96, 96 ), 0.8f );
    } else if ( offsets::Ready( ) ) {
        char Live[ 40 ] = { };
        offsets::CopyVersion( Live, ( int )sizeof( Live ) );
        const char* Hash = Live;
        if ( strncmp( Hash, "version-", 8 ) == 0 )
            Hash += 8;
        snprintf( Status, sizeof( Status ), "%s%s", offsets::Stale( ) ? "cached · " : "", Hash[ 0 ] ? Hash : "ready" );
        if ( offsets::Stale( ) )
            Note = Mix( Style->Faint, CColor( 232, 168, 96 ), 0.55f );
    } else {
        offsets::CopyError( Status, ( int )sizeof( Status ) );
        if ( !Status[ 0 ] )
            lstrcpynA( Status, "offline", ( int )sizeof( Status ) );
        Note = Mix( Style->Faint, CColor( 232, 168, 96 ), 0.75f );
    }

    float StatusLeft = Fourth.Left + Font->Measure( "Offsets" ).Horizontal + 10.0f * Scale;
    float StatusMax = RefreshChip.Left - 8.0f * Scale - StatusLeft;
    if ( StatusMax > 12.0f * Scale ) {
        CVector StatusSize = Font->Measure( Status );
        while ( Status[ 0 ] && StatusSize.Horizontal > StatusMax ) {
            size_t Len = strlen( Status );
            if ( Len < 2 )
                break;
            Status[ Len - 1 ] = 0;
            StatusSize = Font->Measure( Status );
        }
        Canvas->Text( CVector( StatusLeft, Fourth.Top + ( Row - Font->LineSpan ) * 0.5f ), Note, Status );
        if ( Spin > 0.02f && StatusMax > 20.0f * Scale ) {
            float Sweep = ( float )fmod( Context->Elapsed * 1.35f, 1.0 );
            float BarW = 28.0f * Scale;
            float Travel = StatusMax - BarW;
            if ( Travel < 1.0f )
                Travel = 1.0f;
            CRectangle Shine( StatusLeft + Travel * Sweep, Fourth.Top + Row - 3.0f * Scale, BarW, 2.0f * Scale );
            DrawIce( CRectangle( StatusLeft, Shine.Top, StatusMax, Shine.Height ), Shine, Shine.Height * 0.5f, Spin * Pulse );
        }
    }
    if ( OverRefresh && Click )
        offsets::Request( true );
    Busy = Busy || OverRefresh || Work;
    return Busy;
}

static void TickAfk( ) {
    play::TickAfk( Menu.afk, Context->DeltaTime );
    play::TickUncap( Menu.uncap );
}

static bool DrawFold( float Left, float Top, float Wide, float Head, float BodyNeed, float Round, float Scale, const char* Name, const char* Motion, bool& OpenFlag, const CVector& Point, bool Click, CRectangle& Body, float& Open ) {
    Open = ur::motion::toward( Motion, OpenFlag ? 1.0f : 0.0f, 32.0f );
    CRectangle Card( Left, Top, Wide, Head + BodyNeed * Open );
    CRectangle Bar( Left, Top, Wide, Head );
    bool OverBar = Bar.Contains( Point ) && !Moving( );
    if ( OverBar && Click )
        OpenFlag = !OpenFlag;

    Canvas->Rectangle( Card, Dress.card, Round );
    if ( Open > 0.02f )
        DrawIce( Bar, CRectangle( Left, Top, Wide, Head + Round ), Round, Open );
    Canvas->Border( Card, Dress.foldLine, Round, 1.0f );

    CColor Title = Mix( Dress.ink, Dress.inkHot, Open );
    Canvas->Write( Heading.get( ), CVector( Left + 14.0f * Scale, Bar.Top + ( Head - Heading->LineSpan ) * 0.5f ), Title, Name );

    ur::icons::Icon Arrow = Open > 0.5f ? ur::icons::Icon::ChevronUp : ur::icons::Icon::ChevronDown;
    unsigned long long Icon = ur::glyphs::image( Arrow, ( int )( 15.0f * Scale + 0.5f ), ur::glyphs::Weight::Solid );
    float Mark = 15.0f * Scale;
    if ( Icon )
        Canvas->Image( CRectangle( Bar.Right( ) - Mark - 14.0f * Scale, Bar.Top + ( Head - Mark ) * 0.5f, Mark, Mark ), Icon, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), Mix( Style->Faint, Dress.inkHot, Open ), 0.0f );

    Body = CRectangle( Left, Bar.Bottom( ), Wide, BodyNeed );
    return OverBar;
}

static CRectangle DropListBox( float Scale ) {
    if ( !DropId || DropCount <= 0 )
        return CRectangle( );
    float Item = 26.0f * Scale;
    float Tall = Item * ( float )DropCount + 6.0f * Scale;
    float Top = DropField.Bottom( ) + 4.0f * Scale;
    float Limit = ( float )ur::app::height( ) - 8.0f * Scale;
    if ( Top + Tall > Limit )
        Top = DropField.Top - 4.0f * Scale - Tall;
    return CRectangle( DropField.Left, Top, DropField.Width, Tall );
}

static bool DropHit( const CVector& Point, float Scale ) {
    if ( !DropId )
        return false;
    return DropListBox( Scale ).Contains( Point );
}

static bool DrawDrop( float Left, float Top, float Wide, const char* Label, const char* Id, const char* const* Options, int Count, int& Pick, const CVector& Point, bool Click, float Scale ) {
    if ( Pick < 0 || Pick >= Count )
        Pick = 0;
    if ( Label ) {
        Canvas->Text( CVector( Left, Top ), Style->Faint, Label );
        Top += Font->LineSpan + 4.0f * Scale;
    }

    CRectangle Field( Left, Top, Wide, 28.0f * Scale );
    bool Over = Field.Contains( Point ) && !Moving( ) && !Menu.slide;
    bool Open = DropId && strcmp( DropId, Id ) == 0;
    if ( Over && Click ) {
        DropId = Open ? nullptr : Id;
        DropFresh = DropId != nullptr;
    }
    Open = DropId && strcmp( DropId, Id ) == 0;

    float Tone = ur::motion::toward( Id, Open ? 1.0f : ( Over ? 0.45f : 0.0f ), 26.0f );
    DrawIce( Field, Field, 6.0f * Scale, 0.55f + Tone * 0.45f );
    Canvas->Border( Field, Mix( Dress.foldLine, Style->AccentSoft, Tone ), 6.0f * Scale, 1.0f );
    Canvas->Text( CVector( Field.Left + 10.0f * Scale, Field.Top + ( Field.Height - Font->LineSpan ) * 0.5f ), Style->Text, Options[ Pick ] );

    unsigned long long Icon = ur::glyphs::image( Open ? ur::icons::Icon::ChevronUp : ur::icons::Icon::ChevronDown, ( int )( 11.0f * Scale + 0.5f ), ur::glyphs::Weight::Solid );
    float Mark = 11.0f * Scale;
    if ( Icon )
        Canvas->Image( CRectangle( Field.Right( ) - Mark - 10.0f * Scale, Field.Top + ( Field.Height - Mark ) * 0.5f, Mark, Mark ), Icon, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), Style->Faint, 0.0f );

    if ( Open ) {
        DropField = Field;
        DropOpts = Options;
        DropCount = Count;
        DropPick = &Pick;
        DropBits = nullptr;
        DropMany = false;
    }
    return Over;
}

static const char* BitLabel( const char* const* Options, int Count, int Bits ) {
    static char Line[ 96 ];
    Line[ 0 ] = 0;
    int Used = 0;
    for ( int Index = 0; Index < Count; Index++ ) {
        if ( ( Bits & ( 1 << Index ) ) == 0 )
            continue;
        char Piece[ 96 ];
        snprintf( Piece, sizeof( Piece ), "%s%s", Used ? ", " : "", Options[ Index ] );
        size_t Have = strlen( Line );
        snprintf( Line + Have, sizeof( Line ) - Have, "%s", Piece );
        Used += 1;
    }
    if ( !Used )
        snprintf( Line, sizeof( Line ), "None" );
    return Line;
}

static bool DrawDropBits( float Left, float Top, float Wide, const char* Label, const char* Id, const char* const* Options, int Count, int& Bits, const CVector& Point, bool Click, float Scale ) {
    if ( ( Bits & ( ( 1 << Count ) - 1 ) ) == 0 )
        Bits = 1;
    if ( Label ) {
        Canvas->Text( CVector( Left, Top ), Style->Faint, Label );
        Top += Font->LineSpan + 4.0f * Scale;
    }

    CRectangle Field( Left, Top, Wide, 28.0f * Scale );
    bool Over = Field.Contains( Point ) && !Moving( ) && !Menu.slide;
    bool Open = DropId && strcmp( DropId, Id ) == 0;
    if ( Over && Click ) {
        DropId = Open ? nullptr : Id;
        DropFresh = DropId != nullptr;
    }
    Open = DropId && strcmp( DropId, Id ) == 0;

    float Tone = ur::motion::toward( Id, Open ? 1.0f : ( Over ? 0.45f : 0.0f ), 26.0f );
    DrawIce( Field, Field, 6.0f * Scale, 0.55f + Tone * 0.45f );
    Canvas->Border( Field, Mix( Dress.foldLine, Style->AccentSoft, Tone ), 6.0f * Scale, 1.0f );
    Canvas->Text( CVector( Field.Left + 10.0f * Scale, Field.Top + ( Field.Height - Font->LineSpan ) * 0.5f ), Style->Text, BitLabel( Options, Count, Bits ) );

    unsigned long long Icon = ur::glyphs::image( Open ? ur::icons::Icon::ChevronUp : ur::icons::Icon::ChevronDown, ( int )( 11.0f * Scale + 0.5f ), ur::glyphs::Weight::Solid );
    float Mark = 11.0f * Scale;
    if ( Icon )
        Canvas->Image( CRectangle( Field.Right( ) - Mark - 10.0f * Scale, Field.Top + ( Field.Height - Mark ) * 0.5f, Mark, Mark ), Icon, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), Style->Faint, 0.0f );

    if ( Open ) {
        DropField = Field;
        DropOpts = Options;
        DropCount = Count;
        DropPick = nullptr;
        DropBits = &Bits;
        DropMany = true;
    }
    return Over;
}

static bool DrawDropList( const CVector& Point, bool Click, float Scale ) {
    if ( !DropId || !DropOpts || DropCount <= 0 )
        return false;
    if ( !DropMany && !DropPick )
        return false;
    if ( DropMany && !DropBits )
        return false;

    CRectangle List = DropListBox( Scale );
    float Item = 26.0f * Scale;
    bool Over = List.Contains( Point );
    Canvas->Rectangle( List, Dress.card, 6.0f * Scale );
    Canvas->Border( List, Dress.foldLine, 6.0f * Scale, 1.0f );
    for ( int Index = 0; Index < DropCount; Index++ ) {
        CRectangle Row( List.Left + 3.0f * Scale, List.Top + 3.0f * Scale + Item * ( float )Index, List.Width - 6.0f * Scale, Item );
        bool Hit = Row.Contains( Point );
        bool On = DropMany ? ( ( *DropBits & ( 1 << Index ) ) != 0 ) : ( *DropPick == Index );
        if ( On || Hit )
            Canvas->Rectangle( Row, On ? Style->Accent.Fade( 0.28f ) : CColor( 255, 255, 255, 16 ), 4.0f * Scale );
        Canvas->Text( CVector( Row.Left + 8.0f * Scale, Row.Top + ( Row.Height - Font->LineSpan ) * 0.5f ), On ? Dress.inkHot : Style->Text, DropOpts[ Index ] );
        if ( Hit && Click && !DropFresh ) {
            if ( DropMany ) {
                *DropBits ^= ( 1 << Index );
                if ( ( *DropBits & ( ( 1 << DropCount ) - 1 ) ) == 0 )
                    *DropBits = 1 << Index;
            } else {
                *DropPick = Index;
                DropId = nullptr;
            }
        }
    }
    if ( Click && !DropFresh && !Over && !DropField.Contains( Point ) )
        DropId = nullptr;
    DropFresh = false;
    return Over;
}

static bool DrawAimGeneral( const CRectangle& Body, const CVector& Point, bool Click, float Scale ) {
    float Left = Body.Left + 14.0f * Scale;
    float Wide = Body.Width - 28.0f * Scale;
    float Top = Body.Top + 12.0f * Scale;
    float Row = 32.0f * Scale;
    bool Busy = false;
    CRectangle First( Left, Top, Wide, Row );
    Busy = DrawSwitch( First, "Enabled", "aim.on", Aim.on, Point, Click, Scale ) || Busy;
    CRectangle Second( Left, First.Bottom( ) + 2.0f * Scale, Wide, Row );
    Busy = DrawSwitch( Second, "Team check", "aim.team", Aim.team, Point, Click, Scale ) || Busy;
    CRectangle Third( Left, Second.Bottom( ) + 2.0f * Scale, Wide, Row );
    Busy = DrawSwitch( Third, "Visible only", "aim.vis", Aim.vis, Point, Click, Scale ) || Busy;
    CRectangle Fourth( Left, Third.Bottom( ) + 2.0f * Scale, Wide, Row );
    Busy = DrawSwitch( Fourth, "Sticky aim", "aim.sticky", Aim.sticky, Point, Click, Scale ) || Busy;
    CRectangle Fifth( Left, Fourth.Bottom( ) + 2.0f * Scale, Wide, Row );
    Busy = DrawSwitch( Fifth, "Prediction", "aim.pred", Aim.pred, Point, Click, Scale ) || Busy;
    return Busy;
}

static bool DrawAimSilent( const CRectangle& Body, const CVector& Point, bool Click, bool Press, float Scale ) {
    ( void )Press;
    static const char* Bones[ ] = { "Head", "Neck", "Chest", "Stomach", "Body", "Legs" };
    float Pad = 14.0f * Scale;
    float Gap = 16.0f * Scale;
    float Inner = Body.Width - Pad * 2.0f;
    float Col = ( Inner - Gap ) * 0.5f;
    float Left = Body.Left + Pad;
    float Right = Left + Col + Gap;
    float Top = Body.Top + 12.0f * Scale;
    float Row = 30.0f * Scale;
    bool Busy = false;

    CRectangle First( Left, Top, Col, Row );
    Busy = DrawSwitch( First, "Enabled", "silent.on", Mute.on, Point, Click, Scale ) || Busy;
    CRectangle Second( Left, First.Bottom( ) + 2.0f * Scale, Col, Row );
    Busy = DrawSwitch( Second, "Team check", "silent.team", Mute.team, Point, Click, Scale ) || Busy;
    CRectangle Third( Left, Second.Bottom( ) + 2.0f * Scale, Col, Row );
    Busy = DrawSwitch( Third, "Visible only", "silent.vis", Mute.vis, Point, Click, Scale ) || Busy;
    CRectangle Fourth( Left, Third.Bottom( ) + 2.0f * Scale, Col, Row );
    Busy = DrawSwitch( Fourth, "Prediction", "silent.pred", Mute.pred, Point, Click, Scale ) || Busy;
    Busy = DrawBind( Left, Fourth.Bottom( ) + 10.0f * Scale, "Silent key", "silent.listen", Mute.key, Mute.listen, Point, Click, Scale ) || Busy;

    float Slide = Top;
    static const char* Sorts[ ] = { "FOV", "Distance", "Combine" };
    Busy = DrawDrop( Right, Slide, Col, "Priority", "silent.sort", Sorts, 3, Mute.sort, Point, Click, Scale ) || Busy;
    Slide += Font->LineSpan + 38.0f * Scale;
    Busy = DrawDropBits( Right, Slide, Col, "Target", "silent.bone", Bones, 6, Mute.bones, Point, Click, Scale ) || Busy;
    return Busy;
}

static bool DrawAimTarget( const CRectangle& Body, const CVector& Point, bool Click, bool Press, float Scale ) {
    static const char* Bones[ ] = { "Head", "Neck", "Chest", "Stomach", "Body", "Legs" };
    float Left = Body.Left + 14.0f * Scale;
    float Wide = Body.Width - 28.0f * Scale;
    float Top = Body.Top + 12.0f * Scale;
    float Row = 30.0f * Scale;
    bool Busy = false;

    CRectangle Draw( Left, Top, Wide, Row );
    Busy = DrawSwitch( Draw, "Draw FOV", "aim.drawfov", Aim.drawFov, Point, Click, Scale ) || Busy;
    Top = Draw.Bottom( ) + 8.0f * Scale;

    float ShowFov = ur::motion::toward( "aim.fov.show", Aim.drawFov ? 1.0f : 0.0f, 32.0f );
    if ( ShowFov > 0.02f ) {
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * ShowFov;
        Busy = DrawSlider( Left, Top, Wide, "FOV", "aim.fov", Aim.fov, 10.0f, 360.0f, Point, Click, Press, Scale ) || Busy;
        Canvas->Opacity = Amount;
        Top += 30.0f * Scale * ShowFov;
    }

    Busy = DrawSlider( Left, Top, Wide, "Smooth", "aim.smooth", Aim.smooth, 0.0f, 100.0f, Point, Click, Press, Scale ) || Busy;
    Top += 32.0f * Scale;
    static const char* Sorts[ ] = { "FOV", "Distance", "Combine" };
    Busy = DrawDrop( Left, Top, Wide, "Priority", "aim.sort", Sorts, 3, Aim.sort, Point, Click, Scale ) || Busy;
    Top += Font->LineSpan + 38.0f * Scale;
    Busy = DrawDropBits( Left, Top, Wide, "Target", "aim.bone", Bones, 6, Aim.bones, Point, Click, Scale ) || Busy;
    Top += Font->LineSpan + 38.0f * Scale;
    Busy = DrawBind( Left, Top, "Aim key", "aim.listen", Aim.key, Aim.listen, Point, Click, Scale ) || Busy;
    return Busy;
}

static bool DrawEspOverlay( const CRectangle& Body, const CVector& Point, bool Click, float Scale ) {
    float Left = Body.Left + 14.0f * Scale;
    float Wide = Body.Width - 28.0f * Scale;
    float Top = Body.Top + 12.0f * Scale;
    float Row = 32.0f * Scale;
    bool Busy = false;
    CRectangle First( Left, Top, Wide, Row );
    Busy = DrawSwitch( First, "Enabled", "esp.on", Esp.on, Point, Click, Scale ) || Busy;
    CRectangle Second( Left, First.Bottom( ) + 2.0f * Scale, Wide, Row );
    Busy = DrawSwitch( Second, "Box", "esp.box", Esp.box, Point, Click, Scale ) || Busy;
    CRectangle Third( Left, Second.Bottom( ) + 2.0f * Scale, Wide, Row );
    Busy = DrawSwitch( Third, "Name", "esp.name", Esp.name, Point, Click, Scale ) || Busy;
    CRectangle Fourth( Left, Third.Bottom( ) + 2.0f * Scale, Wide, Row );
    Busy = DrawSwitch( Fourth, "Health", "esp.health", Esp.health, Point, Click, Scale ) || Busy;
    CRectangle Fifth( Left, Fourth.Bottom( ) + 2.0f * Scale, Wide, Row );
    Busy = DrawSwitch( Fifth, "Distance", "esp.dist", Esp.dist, Point, Click, Scale ) || Busy;
    CRectangle Sixth( Left, Fifth.Bottom( ) + 2.0f * Scale, Wide, Row );
    Busy = DrawSwitch( Sixth, "Team check", "esp.team", Esp.team, Point, Click, Scale ) || Busy;
    return Busy;
}

static bool DrawEspVisual( const CRectangle& Body, const CVector& Point, bool Click, float Scale ) {
    float Left = Body.Left + 14.0f * Scale;
    float Wide = Body.Width - 28.0f * Scale;
    float Top = Body.Top + 12.0f * Scale;
    float Row = 32.0f * Scale;
    bool Busy = false;
    CRectangle First( Left, Top, Wide, Row );
    Busy = DrawSwitch( First, "Skeleton", "esp.skel", Esp.skeleton, Point, Click, Scale ) || Busy;
    CRectangle Second( Left, First.Bottom( ) + 2.0f * Scale, Wide, Row );
    Busy = DrawSwitch( Second, "Snaplines", "esp.snap", Esp.snap, Point, Click, Scale ) || Busy;
    return Busy;
}

static bool DrawSwatches( float Left, float Top, float Wide, int Count, const CColor* Colors, int& Pick, const char* Prefix, const CVector& Point, bool Click, float Scale );

static float SwatchSize( float Scale ) {
    return 16.0f * Scale;
}

static float SwatchGap( float Scale ) {
    return 3.0f * Scale;
}

static int SwatchColumns( float Wide, int Count, float Scale ) {
    float Size = SwatchSize( Scale );
    float Gap = SwatchGap( Scale );
    int Columns = Count;
    float Need = Size * ( float )Count + Gap * ( float )( Count - 1 );
    if ( Need > Wide )
        Columns = ( int )( ( Wide + Gap ) / ( Size + Gap ) );
    if ( Columns < 1 )
        Columns = 1;
    return Columns;
}

static float SwatchTall( float Wide, int Count, float Scale ) {
    float Size = SwatchSize( Scale );
    float Gap = SwatchGap( Scale );
    int Columns = SwatchColumns( Wide, Count, Scale );
    int Rows = ( Count + Columns - 1 ) / Columns;
    if ( Rows < 1 )
        Rows = 1;
    return Size * ( float )Rows + Gap * ( float )( Rows - 1 );
}

static bool DrawEspCustom( const CRectangle& Body, const CVector& Point, bool Click, float Scale ) {
    static const char* Feats[ ] = { "Box", "Name", "Health", "Distance", "Skeleton", "Snaplines", "All" };
    float Left = Body.Left + 16.0f * Scale;
    float Wide = Body.Width - 32.0f * Scale;
    float Top = Body.Top + 14.0f * Scale;
    bool Busy = false;
    Busy = DrawDrop( Left, Top, Wide, "Feature", "esp.feat", Feats, FeatCount + 1, Dye.feat, Point, Click, Scale ) || Busy;
    Top += Font->LineSpan + 42.0f * Scale;
    if ( Dye.feat < 0 || Dye.feat > FeatCount )
        Dye.feat = 0;
    bool All = Dye.feat == FeatCount;
    int& VisPick = All ? Dye.globVis : Dye.vis[ Dye.feat ];
    int& HidPick = All ? Dye.globHid : Dye.hid[ Dye.feat ];
    Canvas->Text( CVector( Left, Top ), Style->Faint, All ? "Visible (all)" : "Visible" );
    Top += Font->LineSpan + 8.0f * Scale;
    int WasVis = VisPick;
    Busy = DrawSwatches( Left, Top, Wide, 13, EspTints, VisPick, "esp.vis", Point, Click, Scale ) || Busy;
    if ( All && VisPick != WasVis ) {
        for ( int Index = 0; Index < FeatCount; Index++ )
            Dye.vis[ Index ] = VisPick;
    }
    Top += SwatchTall( Wide, 13, Scale ) + 14.0f * Scale;
    Canvas->Text( CVector( Left, Top ), Style->Faint, All ? "Hidden (all)" : "Hidden" );
    Top += Font->LineSpan + 8.0f * Scale;
    int WasHid = HidPick;
    Busy = DrawSwatches( Left, Top, Wide, 13, EspTints, HidPick, "esp.hid", Point, Click, Scale ) || Busy;
    if ( All && HidPick != WasHid ) {
        for ( int Index = 0; Index < FeatCount; Index++ )
            Dye.hid[ Index ] = HidPick;
    }
    return Busy;
}

static bool DrawSwatches( float Left, float Top, float Wide, int Count, const CColor* Colors, int& Pick, const char* Prefix, const CVector& Point, bool Click, float Scale ) {
    float Size = SwatchSize( Scale );
    float Gap = SwatchGap( Scale );
    int Columns = SwatchColumns( Wide, Count, Scale );
    bool Busy = false;
    for ( int Index = 0; Index < Count; Index++ ) {
        int Col = Index % Columns;
        int Row = Index / Columns;
        CRectangle Chip( Left + ( Size + Gap ) * ( float )Col, Top + ( Size + Gap ) * ( float )Row, Size, Size );
        bool Over = Chip.Contains( Point ) && !Moving( ) && !Menu.slide;
        char Id[ 64 ];
        snprintf( Id, sizeof( Id ), "%s.%d", Prefix, Index );
        float Tone = ur::motion::toward( Id, Pick == Index ? 1.0f : ( Over ? 0.5f : 0.0f ), 26.0f );
        Canvas->Rectangle( Chip, Colors[ Index ], 4.0f * Scale );
        Canvas->Border( Chip, Mix( CColor( 80, 96, 120, 140 ), CColor( 236, 242, 252 ), Tone ), 4.0f * Scale, Pick == Index ? 1.6f * Scale : 1.0f * Scale );
        if ( Over && Click )
            Pick = Index;
        Busy = Busy || Over;
    }
    return Busy;
}

struct PageFit {
    float inset;
    float gap;
    float head;
    float general;
    float target;
    float silent;
    float rageJump;
    float rageNoclip;
    float overlay;
    float visual;
    float theme;
    float custom;
    float misc;
    float game;
    float setOverlay;
};

static PageFit FitOf( float Scale ) {
    PageFit Fit;
    Fit.inset = 10.0f * Scale;
    Fit.gap = 8.0f * Scale;
    Fit.head = 36.0f * Scale;
    Fit.general = 200.0f * Scale;
    Fit.silent = 220.0f * Scale;
    Fit.target = ( 276.0f + ( Aim.drawFov ? 32.0f : 0.0f ) ) * Scale;
    Fit.rageJump = 128.0f * Scale;
    Fit.rageNoclip = 58.0f * Scale;
    Fit.overlay = 232.0f * Scale;
    Fit.visual = 88.0f * Scale;
    Fit.theme = 228.0f * Scale;
    Fit.custom = 236.0f * Scale;
    Fit.misc = ( 138.0f + ( Menu.limit ? 28.0f : 0.0f ) ) * Scale;
    Fit.game = 160.0f * Scale;
    Fit.setOverlay = 204.0f * Scale;
    return Fit;
}

static bool DrawAimbot( const CRectangle& Content, const CVector& Point, bool Click, bool Press, float Scale, float Ease ) {
    float Keep = Canvas->Opacity;
    Canvas->Opacity = Keep * Ease;

    PageFit Fit = FitOf( Scale );
    float Inset = Fit.inset;
    float Gap = Fit.gap;
    float Left = Content.Left + Inset;
    float Top = Content.Top + Inset;
    float Wide = ( Content.Width - Inset * 2.0f - Gap ) * 0.5f;
    float Head = Fit.head;
    float Round = 8.0f * Scale;
    float GeneralBody = Fit.general;
    float TargetBody = Fit.target;

    CRectangle Body;
    float OpenGen = 0.0f;
    float OpenTgt = 0.0f;
    float OpenMute = 0.0f;
    bool Busy = DrawFold( Left, Top, Wide, Head, GeneralBody, Round, Scale, "General", "fold.aim.general", Aim.general, Point, Click, Body, OpenGen );
    if ( OpenGen > 0.08f ) {
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * OpenGen;
        Canvas->PushClip( CRectangle( Left, Top, Wide, Head + GeneralBody * OpenGen ) );
        Busy = DrawAimGeneral( Body, Point, Click, Scale ) || Busy;
        Canvas->PopClip( );
        Canvas->Opacity = Amount;
    }

    float Right = Left + Wide + Gap;
    Busy = DrawFold( Right, Top, Wide, Head, TargetBody, Round, Scale, "Targeting", "fold.aim.target", Aim.targeting, Point, Click, Body, OpenTgt ) || Busy;
    if ( OpenTgt > 0.08f ) {
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * OpenTgt;
        Canvas->PushClip( CRectangle( Right, Top, Wide, Head + TargetBody * OpenTgt ) );
        Busy = DrawAimTarget( Body, Point, Click, Press, Scale ) || Busy;
        Canvas->PopClip( );
        Canvas->Opacity = Amount;
    }

    float LeftH = Head + GeneralBody * OpenGen;
    float RightH = Head + TargetBody * OpenTgt;
    float Stack = LeftH > RightH ? LeftH : RightH;
    float SilentTop = Top + Stack + Gap;
    float SilentWide = Wide * 2.0f + Gap;
    float SilentBody = Fit.silent;
    float SilentRoom = Content.Bottom( ) - SilentTop - Head - 8.0f * Scale;
    if ( SilentRoom < 0.0f )
        SilentRoom = 0.0f;
    if ( SilentBody > SilentRoom )
        SilentBody = SilentRoom;
    Canvas->PushClip( Content );
    Busy = DrawFold( Left, SilentTop, SilentWide, Head, SilentBody, Round, Scale, "Silent Aim", "fold.aim.silent", Mute.fold, Point, Click, Body, OpenMute ) || Busy;
    if ( OpenMute > 0.08f ) {
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * OpenMute;
        Canvas->PushClip( CRectangle( Left, SilentTop, SilentWide, Head + SilentBody * OpenMute ) );
        Busy = DrawAimSilent( Body, Point, Click, Press, Scale ) || Busy;
        Canvas->PopClip( );
        Canvas->Opacity = Amount;
    }
    Canvas->PopClip( );

    Canvas->Opacity = Keep;
    return Busy;
}

static bool DrawRageJump( const CRectangle& Body, const CVector& Point, bool Click, bool Press, float Scale ) {
    move::Cfg& Move = move::Live( );
    float Left = Body.Left + 14.0f * Scale;
    float Wide = Body.Width - 28.0f * Scale;
    float Top = Body.Top + 12.0f * Scale;
    bool Busy = false;
    CRectangle First( Left, Top, Wide, 32.0f * Scale );
    Busy = DrawSwitch( First, "Enabled", "move.jump", Move.jump, Point, Click, Scale ) || Busy;
    Busy = DrawSlider( Left, First.Bottom( ) + 6.0f * Scale, Wide, "Power", "move.jump.power", Move.jumpPower, 1.0f, 500.0f, Point, Click, Press, Scale ) || Busy;
    CRectangle Inf( Left, First.Bottom( ) + 38.0f * Scale, Wide, 32.0f * Scale );
    Busy = DrawSwitch( Inf, "Inf Jump", "move.infjump", Move.infJump, Point, Click, Scale ) || Busy;
    return Busy;
}

static bool DrawRageNoclip( const CRectangle& Body, const CVector& Point, bool Click, float Scale ) {
    move::Cfg& Move = move::Live( );
    float Left = Body.Left + 14.0f * Scale;
    float Wide = Body.Width - 28.0f * Scale;
    CRectangle First( Left, Body.Top + 12.0f * Scale, Wide, 32.0f * Scale );
    return DrawSwitch( First, "Enabled", "move.noclip", Move.noclip, Point, Click, Scale );
}

static bool DrawRage( const CRectangle& Content, const CVector& Point, bool Click, bool Press, float Scale, float Ease ) {
    float Keep = Canvas->Opacity;
    Canvas->Opacity = Keep * Ease;

    PageFit Fit = FitOf( Scale );
    float Inset = Fit.inset;
    float Gap = Fit.gap;
    float Left = Content.Left + Inset;
    float Top = Content.Top + Inset;
    float Wide = ( Content.Width - Inset * 2.0f - Gap ) * 0.5f;
    float Head = Fit.head;
    float Round = 8.0f * Scale;
    float Right = Left + Wide + Gap;

    CRectangle Body;
    float OpenJump = 0.0f;
    float OpenClip = 0.0f;
    bool Busy = DrawFold( Left, Top, Wide, Head, Fit.rageJump, Round, Scale, "Jump", "fold.rage.jump", Rage.jump, Point, Click, Body, OpenJump );
    if ( OpenJump > 0.08f ) {
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * OpenJump;
        Canvas->PushClip( CRectangle( Left, Top, Wide, Head + Fit.rageJump * OpenJump ) );
        Busy = DrawRageJump( Body, Point, Click, Press, Scale ) || Busy;
        Canvas->PopClip( );
        Canvas->Opacity = Amount;
    }

    Busy = DrawFold( Right, Top, Wide, Head, Fit.rageNoclip, Round, Scale, "Noclip", "fold.rage.noclip", Rage.noclip, Point, Click, Body, OpenClip ) || Busy;
    if ( OpenClip > 0.08f ) {
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * OpenClip;
        Canvas->PushClip( CRectangle( Right, Top, Wide, Head + Fit.rageNoclip * OpenClip ) );
        Busy = DrawRageNoclip( Body, Point, Click, Scale ) || Busy;
        Canvas->PopClip( );
        Canvas->Opacity = Amount;
    }

    Canvas->Opacity = Keep;
    return Busy;
}

static bool DrawEsp( const CRectangle& Content, const CVector& Point, bool Click, bool Press, float Scale, float Ease ) {
    ( void )Press;
    float Keep = Canvas->Opacity;
    Canvas->Opacity = Keep * Ease;

    PageFit Fit = FitOf( Scale );
    float Inset = Fit.inset;
    float Gap = Fit.gap;
    float Left = Content.Left + Inset;
    float Top = Content.Top + Inset;
    float Wide = ( Content.Width - Inset * 2.0f - Gap ) * 0.5f;
    float Head = Fit.head;
    float Round = 8.0f * Scale;
    float OverlayBody = Fit.overlay;
    float VisualBody = Fit.visual;
    float CustomBody = Fit.custom;
    float CustomRoom = Content.Bottom( ) - Inset - Top - Head;
    if ( CustomRoom < 0.0f )
        CustomRoom = 0.0f;
    if ( CustomBody > CustomRoom )
        CustomBody = CustomRoom;

    CRectangle Body;
    float Open = 0.0f;
    bool Busy = DrawFold( Left, Top, Wide, Head, OverlayBody, Round, Scale, "Overlay", "fold.esp.overlay", Esp.overlay, Point, Click, Body, Open );
    if ( Open > 0.08f ) {
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * Open;
        Canvas->PushClip( CRectangle( Left, Top, Wide, Head + OverlayBody * Open ) );
        Busy = DrawEspOverlay( Body, Point, Click, Scale ) || Busy;
        Canvas->PopClip( );
        Canvas->Opacity = Amount;
    }

    float VisualTop = Top + Head + OverlayBody * Open + Gap;
    Busy = DrawFold( Left, VisualTop, Wide, Head, VisualBody, Round, Scale, "Visuals", "fold.esp.visual", Esp.visual, Point, Click, Body, Open ) || Busy;
    if ( Open > 0.08f ) {
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * Open;
        Canvas->PushClip( CRectangle( Left, VisualTop, Wide, Head + VisualBody * Open ) );
        Busy = DrawEspVisual( Body, Point, Click, Scale ) || Busy;
        Canvas->PopClip( );
        Canvas->Opacity = Amount;
    }

    float Right = Left + Wide + Gap;
    Busy = DrawFold( Right, Top, Wide, Head, CustomBody, Round, Scale, "Customization", "fold.esp.custom", Esp.custom, Point, Click, Body, Open ) || Busy;
    if ( Open > 0.08f ) {
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * Open;
        Canvas->PushClip( CRectangle( Right, Top, Wide, Head + CustomBody * Open ) );
        Busy = DrawEspCustom( Body, Point, Click, Scale ) || Busy;
        Canvas->PopClip( );
        Canvas->Opacity = Amount;
    }

    Canvas->Opacity = Keep;
    return Busy;
}

static bool DrawOverlayBody( const CRectangle& Body, const CVector& Point, bool Click, bool Press, float Scale ) {
    float Left = Body.Left + 14.0f * Scale;
    float Wide = Body.Width - 28.0f * Scale;
    float Top = Body.Top + 12.0f * Scale;
    float Row = 30.0f * Scale;
    bool Busy = false;
    CRectangle First( Left, Top, Wide, Row );
    Busy = DrawSwitch( First, "Watermark", "set.watermark", Menu.watermark, Point, Click, Scale ) || Busy;
    CRectangle Second( Left, First.Bottom( ) + 4.0f * Scale, Wide, Row );
    Busy = DrawSwitch( Second, "Show FPS", "set.showfps", Menu.showFps, Point, Click, Scale ) || Busy;
    CRectangle Third( Left, Second.Bottom( ) + 4.0f * Scale, Wide, Row );
    Busy = DrawSwitch( Third, "Streamproof", "set.stream", Menu.stream, Point, Click, Scale ) || Busy;
    Busy = DrawSlider( Left, Third.Bottom( ) + 8.0f * Scale, Wide, "Menu opacity", "set.fade", Menu.fade, 40.0f, 100.0f, Point, Click, Press, Scale ) || Busy;
    Busy = DrawSlider( Left, Third.Bottom( ) + 40.0f * Scale, Wide, "ESP range", "esp.range", Esp.range, 25.0f, 2000.0f, Point, Click, Press, Scale ) || Busy;
    return Busy;
}

static bool DrawThemeBody( const CRectangle& Body, const CVector& Point, bool Click, float Scale ) {
    static const char* Tones[ 3 ] = { "Dark Knight", "Coffee", "Matcha" };
    static const char* Looks[ 6 ] = {
        "Ice", "Thunder", "Ether", "Snow", "Bends", "Clouds"
    };
    static const char* Weather[ 4 ] = { "Off", "Snow", "Rain", "Thunder" };

    float Left = Body.Left + 14.0f * Scale;
    float Wide = Body.Width - 28.0f * Scale;
    float Top = Body.Top + 10.0f * Scale;
    bool Busy = false;
    Busy = DrawDrop( Left, Top, Wide, "Colors", "skin.tone", Tones, 3, skin::tone( ), Point, Click, Scale ) || Busy;
    Top += Font->LineSpan + 42.0f * Scale;
    Busy = DrawDrop( Left, Top, Wide, "Shader", "skin.look", Looks, 6, skin::look( ), Point, Click, Scale ) || Busy;
    Top += Font->LineSpan + 42.0f * Scale;
    Busy = DrawDrop( Left, Top, Wide, "Particles", "skin.weather", Weather, 4, weather::mode( ), Point, Click, Scale ) || Busy;
    return Busy;
}

static bool DrawSettings( const CRectangle& Content, const CVector& Point, bool Click, bool Press, float Scale, float Ease ) {
    float Keep = Canvas->Opacity;
    Canvas->Opacity = Keep * Ease;

    PageFit Fit = FitOf( Scale );
    float Inset = Fit.inset;
    float Gap = Fit.gap;
    float Left = Content.Left + Inset;
    float Top = Content.Top + Inset;
    float Wide = ( Content.Width - Inset * 2.0f - Gap ) * 0.5f;
    float Head = Fit.head;
    float Round = 8.0f * Scale;
    float MiscBody = Fit.misc;
    float GameBody = Fit.game;
    float OverlayBody = Fit.setOverlay;
    float ThemeBody = Fit.theme;

    Canvas->PushClip( Content );
    CRectangle Body;
    float Open = 0.0f;
    bool Busy = DrawFold( Left, Top, Wide, Head, MiscBody, Round, Scale, "Misc", "fold.misc", Menu.misc, Point, Click, Body, Open );
    float MiscOpen = Open;
    if ( Open > 0.08f ) {
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * Open;
        Canvas->PushClip( CRectangle( Left, Top, Wide, Head + MiscBody * Open ) );
        Busy = DrawMiscBody( Body, Point, Click, Press, Scale ) || Busy;
        Canvas->PopClip( );
        Canvas->Opacity = Amount;
    }

    float GameLeft = Left + Wide + Gap;
    Busy = DrawFold( GameLeft, Top, Wide, Head, GameBody, Round, Scale, "Game", "fold.game", Menu.game, Point, Click, Body, Open ) || Busy;
    float GameOpen = Open;
    if ( Open > 0.08f ) {
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * Open;
        Canvas->PushClip( CRectangle( GameLeft, Top, Wide, Head + GameBody * Open ) );
        Busy = DrawGameBody( Body, Point, Click, Scale ) || Busy;
        Canvas->PopClip( );
        Canvas->Opacity = Amount;
    }

    float ThemeTop = Top + Head + MiscBody * MiscOpen + Gap;
    Busy = DrawFold( Left, ThemeTop, Wide, Head, ThemeBody, Round, Scale, "Theme", "fold.theme", Menu.theme, Point, Click, Body, Open ) || Busy;
    if ( Open > 0.08f ) {
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * Open;
        Canvas->PushClip( CRectangle( Left, ThemeTop, Wide, Head + ThemeBody * Open ) );
        Busy = DrawThemeBody( Body, Point, Click, Scale ) || Busy;
        Canvas->PopClip( );
        Canvas->Opacity = Amount;
    }

    float OverlayTop = Top + Head + GameBody * GameOpen + Gap;
    Busy = DrawFold( GameLeft, OverlayTop, Wide, Head, OverlayBody, Round, Scale, "Overlay", "fold.overlay", Menu.overlay, Point, Click, Body, Open ) || Busy;
    if ( Open > 0.08f ) {
        float Amount = Canvas->Opacity;
        Canvas->Opacity = Amount * Open;
        Canvas->PushClip( CRectangle( GameLeft, OverlayTop, Wide, Head + OverlayBody * Open ) );
        Busy = DrawOverlayBody( Body, Point, Click, Press, Scale ) || Busy;
        Canvas->PopClip( );
        Canvas->Opacity = Amount;
    }

    Canvas->PopClip( );
    Canvas->Opacity = Keep;
    return Busy;
}

static void PackPut( char* Out, int Cap, const char* Key, int Value ) {
    char Line[ 64 ];
    snprintf( Line, sizeof( Line ), "%s %d\n", Key, Value );
    size_t Have = strlen( Out );
    snprintf( Out + Have, ( size_t )Cap - Have, "%s", Line );
}

static void PackState( char* Out, int Cap ) {
    Out[ 0 ] = 0;
    PackPut( Out, Cap, "menuKey", Menu.menuKey );
    PackPut( Out, Cap, "limit", Menu.limit ? 1 : 0 );
    PackPut( Out, Cap, "fps", ( int )Menu.fps );
    PackPut( Out, Cap, "afk", Menu.afk ? 1 : 0 );
    PackPut( Out, Cap, "uncap", Menu.uncap ? 1 : 0 );
    PackPut( Out, Cap, "vsync", Menu.vsync ? 1 : 0 );
    PackPut( Out, Cap, "watermark", Menu.watermark ? 1 : 0 );
    PackPut( Out, Cap, "showFps", Menu.showFps ? 1 : 0 );
    PackPut( Out, Cap, "stream", Menu.stream ? 1 : 0 );
    PackPut( Out, Cap, "fade", ( int )Menu.fade );
    PackPut( Out, Cap, "tone", skin::tone( ) );
    PackPut( Out, Cap, "look", skin::look( ) );
    PackPut( Out, Cap, "weather", weather::mode( ) );
    PackPut( Out, Cap, "aim.on", Aim.on ? 1 : 0 );
    PackPut( Out, Cap, "aim.team", Aim.team ? 1 : 0 );
    PackPut( Out, Cap, "aim.vis", Aim.vis ? 1 : 0 );
    PackPut( Out, Cap, "aim.sticky", Aim.sticky ? 1 : 0 );
    PackPut( Out, Cap, "aim.pred", Aim.pred ? 1 : 0 );
    PackPut( Out, Cap, "aim.drawFov", Aim.drawFov ? 1 : 0 );
    PackPut( Out, Cap, "aim.fov", ( int )Aim.fov );
    PackPut( Out, Cap, "aim.smooth", ( int )Aim.smooth );
    PackPut( Out, Cap, "aim.key", Aim.key );
    PackPut( Out, Cap, "aim.bones", Aim.bones );
    PackPut( Out, Cap, "aim.sort", Aim.sort );
    PackPut( Out, Cap, "silent.on", Mute.on ? 1 : 0 );
    PackPut( Out, Cap, "silent.team", Mute.team ? 1 : 0 );
    PackPut( Out, Cap, "silent.vis", Mute.vis ? 1 : 0 );
    PackPut( Out, Cap, "silent.pred", Mute.pred ? 1 : 0 );
    PackPut( Out, Cap, "silent.key", Mute.key );
    PackPut( Out, Cap, "silent.bones", Mute.bones );
    PackPut( Out, Cap, "silent.sort", Mute.sort );
    {
        move::Cfg& Move = move::Live( );
        PackPut( Out, Cap, "move.jump", Move.jump ? 1 : 0 );
        PackPut( Out, Cap, "move.jumpPower", ( int )Move.jumpPower );
        PackPut( Out, Cap, "move.infJump", Move.infJump ? 1 : 0 );
        PackPut( Out, Cap, "move.noclip", Move.noclip ? 1 : 0 );
    }
    PackPut( Out, Cap, "esp.on", Esp.on ? 1 : 0 );
    PackPut( Out, Cap, "esp.box", Esp.box ? 1 : 0 );
    PackPut( Out, Cap, "esp.name", Esp.name ? 1 : 0 );
    PackPut( Out, Cap, "esp.health", Esp.health ? 1 : 0 );
    PackPut( Out, Cap, "esp.dist", Esp.dist ? 1 : 0 );
    PackPut( Out, Cap, "esp.skel", Esp.skeleton ? 1 : 0 );
    PackPut( Out, Cap, "esp.snap", Esp.snap ? 1 : 0 );
    PackPut( Out, Cap, "esp.team", Esp.team ? 1 : 0 );
    PackPut( Out, Cap, "esp.range", ( int )Esp.range );
    PackPut( Out, Cap, "esp.feat", Dye.feat );
    PackPut( Out, Cap, "esp.globVis", Dye.globVis );
    PackPut( Out, Cap, "esp.globHid", Dye.globHid );
    for ( int Index = 0; Index < FeatCount; Index++ ) {
        char Key[ 24 ];
        snprintf( Key, sizeof( Key ), "esp.vis.%d", Index );
        PackPut( Out, Cap, Key, Dye.vis[ Index ] );
        snprintf( Key, sizeof( Key ), "esp.hid.%d", Index );
        PackPut( Out, Cap, Key, Dye.hid[ Index ] );
    }
    PackPut( Out, Cap, "mark.x", ( int )( Badge.origin.Horizontal + 0.5f ) );
    PackPut( Out, Cap, "mark.y", ( int )( Badge.origin.Vertical + 0.5f ) );
}

static void ApplyState( const char* Body ) {
    if ( !Body )
        return;
    store::TakeB( Body, "limit", Menu.limit );
    store::TakeF( Body, "fps", Menu.fps );
    store::TakeB( Body, "afk", Menu.afk );
    store::TakeB( Body, "uncap", Menu.uncap );
    store::TakeB( Body, "vsync", Menu.vsync );
    store::TakeB( Body, "watermark", Menu.watermark );
    store::TakeB( Body, "showFps", Menu.showFps );
    store::TakeB( Body, "stream", Menu.stream );
    store::TakeF( Body, "fade", Menu.fade );
    store::Take( Body, "tone", skin::tone( ) );
    store::Take( Body, "look", skin::look( ) );
    store::Take( Body, "weather", weather::mode( ) );
    store::Take( Body, "menuKey", Menu.menuKey );
    store::TakeB( Body, "aim.on", Aim.on );
    store::TakeB( Body, "aim.team", Aim.team );
    store::TakeB( Body, "aim.vis", Aim.vis );
    store::TakeB( Body, "aim.sticky", Aim.sticky );
    store::TakeB( Body, "aim.pred", Aim.pred );
    store::TakeB( Body, "aim.drawFov", Aim.drawFov );
    store::TakeF( Body, "aim.fov", Aim.fov );
    store::TakeF( Body, "aim.smooth", Aim.smooth );
    store::Take( Body, "aim.key", Aim.key );
    store::Take( Body, "aim.bones", Aim.bones );
    store::Take( Body, "aim.sort", Aim.sort );
    store::TakeB( Body, "aim.silent", Mute.on );
    store::TakeB( Body, "silent.on", Mute.on );
    store::TakeB( Body, "silent.team", Mute.team );
    store::TakeB( Body, "silent.vis", Mute.vis );
    store::TakeB( Body, "silent.pred", Mute.pred );
    store::Take( Body, "silent.key", Mute.key );
    store::Take( Body, "silent.bones", Mute.bones );
    store::Take( Body, "silent.sort", Mute.sort );
    {
        move::Cfg& Move = move::Live( );
        store::TakeB( Body, "move.jump", Move.jump );
        store::TakeF( Body, "move.jumpPower", Move.jumpPower );
        store::TakeB( Body, "move.infJump", Move.infJump );
        store::TakeB( Body, "move.noclip", Move.noclip );
        move::Clamp( );
    }
    store::TakeB( Body, "esp.on", Esp.on );
    store::TakeB( Body, "esp.box", Esp.box );
    store::TakeB( Body, "esp.name", Esp.name );
    store::TakeB( Body, "esp.health", Esp.health );
    store::TakeB( Body, "esp.dist", Esp.dist );
    store::TakeB( Body, "esp.skel", Esp.skeleton );
    store::TakeB( Body, "esp.snap", Esp.snap );
    store::TakeB( Body, "esp.team", Esp.team );
    store::TakeF( Body, "esp.range", Esp.range );
    store::Take( Body, "esp.feat", Dye.feat );
    store::Take( Body, "esp.globVis", Dye.globVis );
    store::Take( Body, "esp.globHid", Dye.globHid );
    for ( int Index = 0; Index < FeatCount; Index++ ) {
        char Key[ 24 ];
        snprintf( Key, sizeof( Key ), "esp.vis.%d", Index );
        store::Take( Body, Key, Dye.vis[ Index ] );
        snprintf( Key, sizeof( Key ), "esp.hid.%d", Index );
        store::Take( Body, Key, Dye.hid[ Index ] );
    }
    int MarkX = ( int )Badge.origin.Horizontal;
    int MarkY = ( int )Badge.origin.Vertical;
    if ( store::Take( Body, "mark.x", MarkX ) && store::Take( Body, "mark.y", MarkY ) ) {
        Badge.origin = CVector( ( float )MarkX, ( float )MarkY );
        Badge.ready = true;
    }
    if ( skin::look( ) < 0 || skin::look( ) >= skin::LookCount )
        skin::look( ) = 0;
    if ( skin::tone( ) < 0 || skin::tone( ) >= skin::ToneCount )
        skin::tone( ) = 0;
    if ( weather::mode( ) < 0 || weather::mode( ) >= weather::ModeCount )
        weather::mode( ) = weather::Snow;
    if ( Menu.fps < 60.0f )
        Menu.fps = 60.0f;
    if ( Menu.fade < 40.0f )
        Menu.fade = 40.0f;
    if ( Esp.range < 25.0f )
        Esp.range = 25.0f;
    if ( Esp.range > 2000.0f )
        Esp.range = 2000.0f;
    if ( Aim.smooth < 0.0f )
        Aim.smooth = 0.0f;
    if ( Aim.smooth > 100.0f )
        Aim.smooth = 100.0f;
    if ( Aim.fov < 10.0f )
        Aim.fov = 10.0f;
    if ( Aim.fov > 360.0f )
        Aim.fov = 360.0f;
    if ( Aim.bones == 0 )
        Aim.bones = 1;
    if ( Aim.sort < 0 || Aim.sort > 2 )
        Aim.sort = 0;
    if ( Mute.bones == 0 )
        Mute.bones = 1;
    if ( Mute.sort < 0 || Mute.sort > 2 )
        Mute.sort = 0;
    if ( !Mute.key )
        Mute.key = 'M';
    move::Clamp( );
}

static void PackNote( const char* Text ) {
    lstrcpynA( Packs.note, Text ? Text : "", ( int )sizeof( Packs.note ) );
    Packs.noteAge = 2.4f;
}

static void PackRefresh( ) {
    Packs.count = store::List( Packs.names );
    if ( Packs.pick >= Packs.count )
        Packs.pick = Packs.count > 0 ? Packs.count - 1 : 0;
    for ( int Index = 0; Index < Packs.count; Index++ ) {
        if ( _stricmp( Packs.names[ Index ], Packs.live ) == 0 )
            Packs.pick = Index;
    }
}

static bool PackSave( const char* Name ) {
    char Body[ store::BodyCap ];
    PackState( Body, store::BodyCap );
    if ( !store::Write( Name, Body ) )
        return false;
    lstrcpynA( Packs.live, Name, store::NameCap );
    store::SetCurrent( Name );
    PackRefresh( );
    return true;
}

static bool PackLoad( const char* Name ) {
    char Body[ store::BodyCap ];
    if ( !store::Read( Name, Body, store::BodyCap ) )
        return false;
    ApplyState( Body );
    lstrcpynA( Packs.live, Name, store::NameCap );
    store::SetCurrent( Name );
    PackRefresh( );
    Tokens( );
    return true;
}

static void PackBoot( ) {
    if ( Packs.ready )
        return;
    PackRefresh( );
    char Last[ store::NameCap ] = { };
    if ( store::Current( Last, store::NameCap ) && PackLoad( Last ) ) {
        Packs.ready = true;
        if ( Menu.visible )
            OpenLiveFolds( );
        return;
    }
    if ( Packs.count > 0 && PackLoad( Packs.names[ 0 ] ) ) {
        Packs.ready = true;
        if ( Menu.visible )
            OpenLiveFolds( );
        return;
    }
    lstrcpynA( Packs.live, "Default", store::NameCap );
    PackSave( "Default" );
    Packs.ready = true;
    if ( Menu.visible )
        OpenLiveFolds( );
}

static void PackDraft( ) {
    if ( !Packs.type )
        return;
    if ( Edge( VK_BACK, KeyWas[ VK_BACK ] ) ) {
        size_t Len = strlen( Packs.draft );
        if ( Len )
            Packs.draft[ Len - 1 ] = 0;
        return;
    }
    if ( Edge( VK_RETURN, KeyWas[ VK_RETURN ] ) ) {
        store::Sanitize( Packs.draft );
        if ( store::Valid( Packs.draft ) && PackSave( Packs.draft ) )
            PackNote( "Created" );
        Packs.type = false;
        return;
    }
    bool Shift = Held( VK_SHIFT );
    for ( int Code = 'A'; Code <= 'Z'; Code++ ) {
        if ( !Edge( Code, KeyWas[ Code ] ) )
            continue;
        size_t Len = strlen( Packs.draft );
        if ( Len >= store::NameCap - 1 )
            return;
        Packs.draft[ Len ] = ( char )( Shift ? Code : Code + 32 );
        Packs.draft[ Len + 1 ] = 0;
        return;
    }
    for ( int Code = '0'; Code <= '9'; Code++ ) {
        if ( !Edge( Code, KeyWas[ Code ] ) )
            continue;
        size_t Len = strlen( Packs.draft );
        if ( Len >= store::NameCap - 1 )
            return;
        Packs.draft[ Len ] = ( char )Code;
        Packs.draft[ Len + 1 ] = 0;
        return;
    }
    char Extra = 0;
    if ( Edge( VK_SPACE, KeyWas[ VK_SPACE ] ) )
        Extra = ' ';
    else if ( Edge( VK_OEM_MINUS, KeyWas[ VK_OEM_MINUS ] ) )
        Extra = '-';
    if ( Extra ) {
        size_t Len = strlen( Packs.draft );
        if ( Len >= store::NameCap - 1 )
            return;
        Packs.draft[ Len ] = Extra;
        Packs.draft[ Len + 1 ] = 0;
    }
}

static bool DrawAction( const CRectangle& Row, const char* Label, const CVector& Point, bool Click, float Scale, bool Danger ) {
    bool Over = Row.Contains( Point ) && !Moving( ) && !Menu.slide;
    float Tone = ur::motion::toward( Label, Over ? 1.0f : 0.0f, 26.0f );
    float Round = 6.0f * Scale;
    if ( Over )
        DrawIce( Row, Row, Round, 0.45f + Tone * 0.35f );
    else
        Canvas->Rectangle( Row, Dress.elevated, Round );
    Canvas->Border( Row, Mix( Dress.foldLine, Danger ? CColor( 232, 64, 72 ) : Style->AccentSoft, Tone ), Round, 1.0f );
    CVector Size = Font->Measure( Label );
    CColor Ink = Danger
        ? Mix( CColor( 214, 220, 232 ), CColor( 232, 64, 72 ), Tone )
        : Mix( Style->Text, Dress.inkHot, Tone );
    Canvas->Text( CVector( Row.Left + ( Row.Width - Size.Horizontal ) * 0.5f, Row.Top + ( Row.Height - Font->LineSpan ) * 0.5f ), Ink, Label );
    return Over && Click;
}

static bool ReadClientVer( char* Out, int Cap ) {
    static const wchar_t* Names[ ] = { L"RobloxPlayerBeta.exe", L"RobloxPlayer.exe", L"Windows10Universal.exe" };
    if ( !Out || Cap < 8 )
        return false;
    Out[ 0 ] = 0;
    for ( const wchar_t* Name : Names ) {
        DWORD Pid = world::FindPid( Name );
        if ( !Pid )
            continue;
        HANDLE Handle = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, Pid );
        if ( !Handle )
            continue;
        char Path[ MAX_PATH ] = { };
        DWORD Size = ( DWORD )sizeof( Path );
        BOOL Ok = QueryFullProcessImageNameA( Handle, 0, Path, &Size );
        CloseHandle( Handle );
        if ( !Ok )
            continue;
        const char* Found = strstr( Path, "version-" );
        if ( !Found )
            continue;
        lstrcpynA( Out, Found, Cap );
        char* Cut = strpbrk( Out, "\\/" );
        if ( Cut )
            *Cut = 0;
        return Out[ 0 ] != 0;
    }
    return false;
}

static void TickChannel( ) {
    unsigned Now = GetTickCount( );
    if ( Now < LiveCh.nextScan )
        return;
    LiveCh.nextScan = Now + 2000;

    char Client[ 48 ] = { };
    bool HaveClient = ReadClientVer( Client, ( int )sizeof( Client ) );
    char Dump[ 48 ] = { };
    if ( offsets::Ready( ) )
        offsets::CopyVersion( Dump, ( int )sizeof( Dump ) );

    if ( HaveClient )
        lstrcpynA( LiveCh.client, Client, ( int )sizeof( LiveCh.client ) );
    else
        LiveCh.client[ 0 ] = 0;
    lstrcpynA( LiveCh.dump, Dump, ( int )sizeof( LiveCh.dump ) );

    LiveCh.mismatch = HaveClient && Dump[ 0 ] && _stricmp( Client, Dump ) != 0;
    if ( !LiveCh.mismatch ) {
        LiveCh.open = false;
        LiveCh.dismissed = false;
        return;
    }
    if ( !LiveCh.dismissed )
        LiveCh.open = true;
}

static void DrawChannelNotice( float Across, float Vertical, const CVector& Point, bool Click, float Scale ) {
    if ( !LiveCh.open || !Font )
        return;

    float Line = Font->LineSpan;
    float Pad = 18.0f * Scale;
    float HeadH = 42.0f * Scale;
    float ActH = 32.0f * Scale;
    float AfterSteps = 18.0f * Scale;
    float StepGap = 6.0f * Scale;
    float Wide = 448.0f * Scale;
    float Tall = HeadH + 14.0f * Scale + Line + 6.0f * Scale + Line + 12.0f * Scale + Line + 10.0f * Scale;
    for ( int Index = 0; Index < 7; Index++ )
        Tall += Line + StepGap;
    Tall += AfterSteps + ActH + Pad;
    CRectangle Shade( 0.0f, 0.0f, Across, Vertical );
    CRectangle Card( ( Across - Wide ) * 0.5f, ( Vertical - Tall ) * 0.5f, Wide, Tall );
    float Keep = Canvas->Opacity;
    Canvas->Opacity = 1.0f;
    Canvas->Rectangle( Shade, CColor( 6, 8, 12, 186 ), 0.0f );
    Canvas->Shadow( Card, CColor( 6, 10, 18, 130 ), 10.0f * Scale, 22.0f * Scale );
    Canvas->Rectangle( Card, Style->Surface, 10.0f * Scale );
    CRectangle Head( Card.Left, Card.Top, Card.Width, HeadH );
    DrawIce( Head, Card, 10.0f * Scale, 1.0f );

    CFont* Title = Heading.get( );
    if ( Title && Title->LineSpan > 1.0f )
        Canvas->Write( Title, CVector( Card.Left + Pad, Head.Top + ( Head.Height - Title->LineSpan ) * 0.5f ), Dress.inkHot, "Wrong Roblox channel" );
    else
        Canvas->Text( CVector( Card.Left + Pad, Head.Top + 12.0f * Scale ), Dress.inkHot, "Wrong Roblox channel" );

    float Y = Head.Bottom( ) + 14.0f * Scale;
    char LineText[ 96 ] = { };
    snprintf( LineText, sizeof( LineText ), "Your client  %s", LiveCh.client[ 0 ] ? LiveCh.client : "unknown" );
    Canvas->Text( CVector( Card.Left + Pad, Y ), Style->Text, LineText );
    Y += Line + 6.0f * Scale;
    snprintf( LineText, sizeof( LineText ), "LIVE dump    %s", LiveCh.dump[ 0 ] ? LiveCh.dump : "unknown" );
    Canvas->Text( CVector( Card.Left + Pad, Y ), Style->Faint, LineText );
    Y += Line + 12.0f * Scale;
    Canvas->Text( CVector( Card.Left + Pad, Y ), Style->Faint, "Offsets are dumped for the LIVE channel only." );
    Y += Line + 10.0f * Scale;

    static const char* Steps[ ] = {
        "1. Download Fishstrap from fishstrap.app",
        "2. Install it, then open Fishstrap from search",
        "3. Click Configure Settings",
        "4. Open the Deployment tab",
        "5. Set Channel to production and press Enter",
        "6. Set Automatic channel change to Never change",
        "7. Press Save and Launch"
    };
    for ( const char* Step : Steps ) {
        Canvas->Text( CVector( Card.Left + Pad, Y ), Style->Text, Step );
        Y += Line + StepGap;
    }

    Y += AfterSteps;
    float Gap = 8.0f * Scale;
    float Half = ( Card.Width - Pad * 2.0f - Gap ) * 0.5f;
    CRectangle Get( Card.Left + Pad, Y, Half, ActH );
    CRectangle Ok( Get.Right( ) + Gap, Y, Half, ActH );
    if ( DrawAction( Get, "Get Fishstrap", Point, Click, Scale, false ) )
        ShellExecuteA( nullptr, "open", "https://www.fishstrap.app/Fishstrap.exe", nullptr, nullptr, SW_SHOWNORMAL );
    if ( DrawAction( Ok, "Got it", Point, Click, Scale, false ) ) {
        LiveCh.open = false;
        LiveCh.dismissed = true;
    }

    ur::overlay::Options& Overlay = ur::app::overlay_options( );
    Overlay.click_through = false;
    Input->ApplyPosition( Point.Horizontal, Point.Vertical );
    Canvas->Opacity = Keep;
}

static bool DrawConfigs( const CRectangle& Content, const CVector& Point, bool Click, bool Press, float Scale, float Ease ) {
    ( void )Press;
    float Keep = Canvas->Opacity;
    Canvas->Opacity = Keep * Ease;

    if ( Packs.noteAge > 0.0f )
        Packs.noteAge -= Context->DeltaTime;

    float Inset = 10.0f * Scale;
    float Gap = 8.0f * Scale;
    float Head = 40.0f * Scale;
    float Round = 8.0f * Scale;
    float Left = Content.Left + Inset;
    float Top = Content.Top + Inset;
    float Full = Content.Width - Inset * 2.0f;
    float ListW = Full * 0.58f;
    float SideW = Full - ListW - Gap;
    float RowH = 34.0f * Scale;
    float LibPad = 24.0f * Scale;
    int Rows = Packs.count > 0 ? Packs.count : 1;
    float Room = Content.Height - Inset * 2.0f - Head;
    float LibBodyH = LibPad + RowH * ( float )Rows;
    if ( LibBodyH > Room )
        LibBodyH = Room;
    float Line = Font ? Font->LineSpan : 16.0f * Scale;
    float FieldH = 30.0f * Scale;
    float ActH = 32.0f * Scale;
    float ManNeed = 12.0f * Scale + Line + 2.0f * Scale + Line + 8.0f * Scale + Line + 4.0f * Scale + FieldH + 8.0f * Scale + ActH + 6.0f * Scale + ActH + 8.0f * Scale + Line + 14.0f * Scale;
    float ManBodyH = ManNeed;
    if ( ManBodyH > Room )
        ManBodyH = Room;
    if ( ManBodyH < 168.0f * Scale && Room > 168.0f * Scale )
        ManBodyH = 168.0f * Scale;

    CRectangle Lib( Left, Top, ListW, Head + LibBodyH );
    CRectangle LibBar( Left, Top, ListW, Head );
    CRectangle LibBody( Left, Top + Head, ListW, LibBodyH );
    Canvas->Rectangle( Lib, Dress.card, Round );
    DrawIce( LibBar, CRectangle( Left, Top, ListW, Head + Round ), Round, 1.0f );
    Canvas->Border( Lib, Dress.foldLine, Round, 1.0f );
    Canvas->Write( Heading.get( ), CVector( Left + 14.0f * Scale, LibBar.Top + ( Head - Heading->LineSpan ) * 0.5f ), Dress.inkHot, "Library" );
    char Count[ 16 ];
    snprintf( Count, sizeof( Count ), "%d", Packs.count );
    CVector CountSize = Font->Measure( Count );
    Canvas->Text( CVector( LibBar.Right( ) - 14.0f * Scale - CountSize.Horizontal, LibBar.Top + ( Head - Font->LineSpan ) * 0.5f ), Style->Faint, Count );

    float Pad = 10.0f * Scale;
    CRectangle Pane( LibBody.Left + Pad, LibBody.Top + 8.0f * Scale, LibBody.Width - Pad * 2.0f, LibBody.Height - 16.0f * Scale );
    if ( Pane.Contains( Point ) && Input->WheelDelta != 0.0f ) {
        Packs.scroll -= Input->WheelDelta * 36.0f * Scale;
        Input->WheelDelta = 0.0f;
    }
    float Need = ( float )Packs.count * RowH;
    float Most = Need - Pane.Height;
    if ( Most < 0.0f )
        Most = 0.0f;
    if ( Packs.scroll > Most )
        Packs.scroll = Most;
    if ( Packs.scroll < 0.0f )
        Packs.scroll = 0.0f;

    bool Busy = false;
    Canvas->PushClip( Pane );
    if ( Packs.count == 0 ) {
        Canvas->Text( CVector( Pane.Left + 6.0f * Scale, Pane.Top + 8.0f * Scale ), Style->Faint, "No configs yet." );
    }
    for ( int Index = 0; Index < Packs.count; Index++ ) {
        CRectangle Row( Pane.Left, Pane.Top + ( float )Index * RowH - Packs.scroll, Pane.Width, RowH - 4.0f * Scale );
        if ( Row.Bottom( ) < Pane.Top || Row.Top > Pane.Bottom( ) )
            continue;
        bool Over = Row.Contains( Point ) && Pane.Contains( Point ) && !Moving( );
        bool On = Packs.pick == Index;
        bool Live = _stricmp( Packs.names[ Index ], Packs.live ) == 0;
        float Tone = ur::motion::toward( Packs.names[ Index ], On ? 1.0f : ( Over ? 0.45f : 0.0f ), 26.0f );
        if ( On )
            DrawIce( Row, Row, 6.0f * Scale, 0.55f + Tone * 0.25f );
        else if ( Over )
            Canvas->Rectangle( Row, CColor( 255, 255, 255, 12 ), 6.0f * Scale );
        Canvas->Text( CVector( Row.Left + 12.0f * Scale, Row.Top + ( Row.Height - Font->LineSpan ) * 0.5f ), On ? Dress.inkHot : Style->Text, Packs.names[ Index ] );
        if ( Live ) {
            CVector Tag = Font->Measure( "loaded" );
            Canvas->Text( CVector( Row.Right( ) - 12.0f * Scale - Tag.Horizontal, Row.Top + ( Row.Height - Font->LineSpan ) * 0.5f ), Style->AccentSoft, "loaded" );
        }
        if ( Over && Click ) {
            if ( Packs.pick == Index )
                PackLoad( Packs.names[ Index ] );
            Packs.pick = Index;
            Packs.confirm = false;
            lstrcpynA( Packs.draft, Packs.names[ Index ], store::NameCap );
            Busy = true;
        }
        Busy = Busy || Over;
    }
    Canvas->PopClip( );

    float Side = Left + ListW + Gap;
    CRectangle Box( Side, Top, SideW, Head + ManBodyH );
    CRectangle Bar( Side, Top, SideW, Head );
    CRectangle Inner( Side, Top + Head, SideW, ManBodyH );
    Canvas->Rectangle( Box, Dress.card, Round );
    DrawIce( Bar, CRectangle( Side, Top, SideW, Head + Round ), Round, 1.0f );
    Canvas->Border( Box, Dress.foldLine, Round, 1.0f );
    Canvas->Write( Heading.get( ), CVector( Side + 14.0f * Scale, Bar.Top + ( Head - Heading->LineSpan ) * 0.5f ), Dress.inkHot, "Manage" );

    float PadX = 14.0f * Scale;
    float CursorY = Inner.Top + 10.0f * Scale;
    float InnerW = Inner.Width - PadX * 2.0f;
    Canvas->PushClip( Inner );
    Canvas->Text( CVector( Side + PadX, CursorY ), Style->Faint, "Loaded" );
    CursorY += Line + 2.0f * Scale;
    Canvas->Text( CVector( Side + PadX, CursorY ), Dress.inkHot, Packs.live[ 0 ] ? Packs.live : "None" );
    CursorY += Line + 8.0f * Scale;

    Canvas->Text( CVector( Side + PadX, CursorY ), Style->Faint, "Name" );
    CursorY += Line + 4.0f * Scale;
    float MakeW = 78.0f * Scale;
    CRectangle Field( Side + PadX, CursorY, InnerW - MakeW - 8.0f * Scale, FieldH );
    bool OverField = Field.Contains( Point ) && !Moving( );
    if ( OverField && Click ) {
        Packs.type = true;
        SyncBindKeys( );
    }
    float Wait = ur::motion::toward( "cfg.type", Packs.type ? 1.0f : ( OverField ? 0.4f : 0.0f ), 26.0f );
    DrawIce( Field, Field, 6.0f * Scale, 0.55f + Wait * 0.45f );
    Canvas->Border( Field, Mix( Dress.foldLine, Style->AccentSoft, Wait ), 6.0f * Scale, 1.0f );
    const char* Shown = Packs.draft[ 0 ] ? Packs.draft : ( Packs.type ? "" : "new config" );
    Canvas->Text( CVector( Field.Left + 10.0f * Scale, Field.Top + ( Field.Height - Font->LineSpan ) * 0.5f ), Packs.draft[ 0 ] ? Style->Text : Style->Faint, Shown );
    if ( Packs.type && ( ( int )( Context->Elapsed * 2.0 ) & 1 ) ) {
        CVector Caret = Font->Measure( Packs.draft );
        Canvas->Rectangle( CRectangle( Field.Left + 10.0f * Scale + Caret.Horizontal + 1.0f * Scale, Field.Top + 7.0f * Scale, 1.0f * Scale, Field.Height - 14.0f * Scale ), Dress.inkHot, 0.0f );
    }
    CRectangle Make( Field.Right( ) + 8.0f * Scale, CursorY, MakeW, FieldH );
    if ( DrawAction( Make, "Create", Point, Click, Scale, false ) ) {
        store::Sanitize( Packs.draft );
        if ( store::Valid( Packs.draft ) && PackSave( Packs.draft ) )
            PackNote( "Created" );
        else
            PackNote( "Need a name" );
        Packs.type = false;
        Packs.confirm = false;
        Busy = true;
    }
    CursorY += FieldH + 8.0f * Scale;

    float Half = ( InnerW - 8.0f * Scale ) * 0.5f;
    CRectangle Load( Side + PadX, CursorY, Half, ActH );
    CRectangle Save( Load.Right( ) + 8.0f * Scale, CursorY, Half, ActH );
    if ( DrawAction( Load, "Load", Point, Click, Scale, false ) ) {
        if ( Packs.count > 0 && PackLoad( Packs.names[ Packs.pick ] ) )
            PackNote( "Loaded" );
        else
            PackNote( "Nothing to load" );
        Packs.confirm = false;
        Busy = true;
    }
    if ( DrawAction( Save, "Save", Point, Click, Scale, false ) ) {
        const char* Target = Packs.count > 0 ? Packs.names[ Packs.pick ] : Packs.live;
        if ( store::Valid( Target ) && PackSave( Target ) )
            PackNote( "Saved" );
        else
            PackNote( "Save failed" );
        Packs.confirm = false;
        Busy = true;
    }
    CursorY += ActH + 6.0f * Scale;

    CRectangle Kill( Side + PadX, CursorY, Half, ActH );
    CRectangle Folder( Kill.Right( ) + 8.0f * Scale, CursorY, Half, ActH );
    if ( DrawAction( Kill, Packs.confirm ? "Sure?" : "Delete", Point, Click, Scale, true ) ) {
        if ( Packs.count > 0 ) {
            if ( !Packs.confirm ) {
                Packs.confirm = true;
                PackNote( "Click again to delete" );
            } else if ( store::Remove( Packs.names[ Packs.pick ] ) ) {
                PackNote( "Deleted" );
                Packs.confirm = false;
                if ( _stricmp( Packs.live, Packs.names[ Packs.pick ] ) == 0 )
                    Packs.live[ 0 ] = 0;
                PackRefresh( );
            }
        }
        Busy = true;
    }
    if ( DrawAction( Folder, "Folder", Point, Click, Scale, false ) ) {
        store::OpenFolder( );
        Busy = true;
    }
    CursorY += ActH + 8.0f * Scale;
    if ( Packs.noteAge > 0.0f && Packs.note[ 0 ] ) {
        float Fade = Packs.noteAge > 1.0f ? 1.0f : Packs.noteAge;
        float Hold = Canvas->Opacity;
        Canvas->Opacity = Hold * Fade;
        Canvas->Text( CVector( Side + PadX, CursorY ), Style->AccentSoft, Packs.note );
        Canvas->Opacity = Hold;
    }
    Canvas->PopClip( );

    if ( Click && !OverField )
        Packs.type = false;

    Canvas->Opacity = Keep;
    return Busy || OverField;
}

static void DrawPage( const CRectangle& Content, const CVector& Point, bool Click, bool Press, float Scale, bool& Busy ) {
    Menu.pageIn += Context->DeltaTime * 5.2f;
    if ( Menu.pageIn > 1.0f )
        Menu.pageIn = 1.0f;

    float Remain = 1.0f - Menu.pageIn;
    float Ease = 1.0f - Remain * Remain * Remain * Remain * Remain;
    float Slide = ( 1.0f - Ease ) * 36.0f * Scale * Menu.pageDir;
    bool Live = Menu.pageIn > 0.82f;

    CRectangle Shifted = Content;
    Shifted.Top += Slide;
    CVector Hit( Point.Horizontal, Point.Vertical - Slide );

    bool Block = DropHit( Hit, Scale );
    Canvas->PushClip( Content );
    float Keep = Canvas->Opacity;
    Canvas->Opacity = Keep * Ease;
    if ( Menu.tab == TabAimbot )
        Busy = DrawAimbot( Shifted, Hit, Click && Live && !Block, Press && Live && !Block, Scale, 1.0f ) || Busy;
    else if ( Menu.tab == TabRage )
        Busy = DrawRage( Shifted, Hit, Click && Live && !Block, Press && Live && !Block, Scale, 1.0f ) || Busy;
    else if ( Menu.tab == TabEsp )
        Busy = DrawEsp( Shifted, Hit, Click && Live && !Block, Press && Live && !Block, Scale, 1.0f ) || Busy;
    else if ( Menu.tab == TabConfigs )
        Busy = DrawConfigs( Shifted, Hit, Click && Live && !Block, Press && Live && !Block, Scale, 1.0f ) || Busy;
    else if ( Menu.tab == TabSettings )
        Busy = DrawSettings( Shifted, Hit, Click && Live && !Block, Press && Live && !Block, Scale, 1.0f ) || Busy;
    Canvas->Opacity = Keep;

    float Wash = ( 1.0f - Ease ) * 0.55f;
    if ( Wash > 0.02f ) {
        float Edge = Content.Top + ( Menu.pageDir > 0.0f ? 0.0f : Content.Height - 3.0f * Scale );
        DrawIce( Content, CRectangle( Content.Left, Edge, Content.Width, 3.0f * Scale ), 0.0f, Wash );
    }
    Canvas->PopClip( );
    Busy = DrawDropList( Hit, Click && Live, Scale ) || Busy;
}

static void PlaceExplore( float Across, float Vertical, float Scale ) {
    float Wide = ExploreWidth * Scale;
    float Tall = ExploreHeight * Scale;
    float Gap = 16.0f * Scale;
    if ( !Tree.ready ) {
        Tree.dock = CVector( MenuWidth * Scale + Gap, 0.0f );
        if ( Menu.origin.Horizontal + Tree.dock.Horizontal + Wide > Across - 8.0f )
            Tree.dock = CVector( -Wide - Gap, 0.0f );
        Tree.docked = true;
        Tree.ready = true;
    }
    if ( Tree.docked )
        Tree.origin = Menu.origin + Tree.dock;
    ClampBox( Tree.origin, Across, Vertical, Wide, Tall );
}

static void DragExplore( const CRectangle& Bounds, const CVector& Point, float Across, float Vertical, float Wide, float Tall, bool AllowStart ) {
    bool Press = Held( VK_LBUTTON );
    if ( Press && !Menu.mouse && AllowStart && !Menu.held && Bounds.Contains( Point ) && !Menu.slide ) {
        Tree.held = true;
        Tree.docked = false;
        Tree.grab = Point - Tree.origin;
    }

    if ( Tree.held ) {
        if ( Press )
            Tree.origin = Point - Tree.grab;
        else
            Tree.held = false;
    }

    ClampBox( Tree.origin, Across, Vertical, Wide, Tall );
}

static void ExploreChrome( const CRectangle& Bounds, float Scale, CRectangle& Header, CRectangle& Pane ) {
    float Cap = HeaderHeight * Scale;
    float Pad = 10.0f * Scale;
    Header = CRectangle( Bounds.Left, Bounds.Top, Bounds.Width, Cap );
    Pane = CRectangle( Bounds.Left + Pad, Header.Bottom( ) + Pad, Bounds.Width - Pad * 2.0f, Bounds.Height - Cap - Pad * 2.0f );
}

static void DrawCaret( CVector At, bool Down, float Scale, CColor Tint ) {
    float Span = 3.6f * Scale;
    CVector Tips[ 3 ];
    if ( Down ) {
        Tips[ 0 ] = CVector( At.Horizontal - Span, At.Vertical - Span * 0.45f );
        Tips[ 1 ] = CVector( At.Horizontal + Span, At.Vertical - Span * 0.45f );
        Tips[ 2 ] = CVector( At.Horizontal, At.Vertical + Span * 0.75f );
    } else {
        Tips[ 0 ] = CVector( At.Horizontal - Span * 0.35f, At.Vertical - Span );
        Tips[ 1 ] = CVector( At.Horizontal + Span * 0.8f, At.Vertical );
        Tips[ 2 ] = CVector( At.Horizontal - Span * 0.35f, At.Vertical + Span );
    }
    Canvas->Polygon( Tips, 3, Tint );
}

static void CopyText( const char* Text ) {
    if ( !Text || !Text[ 0 ] )
        return;
    size_t Bytes = strlen( Text ) + 1;
    HGLOBAL Block = GlobalAlloc( GMEM_MOVEABLE, Bytes );
    if ( !Block )
        return;
    void* Dest = GlobalLock( Block );
    if ( !Dest ) {
        GlobalFree( Block );
        return;
    }
    memcpy( Dest, Text, Bytes );
    GlobalUnlock( Block );
    if ( OpenClipboard( ( HWND )ur::app::window( ) ) ) {
        EmptyClipboard( );
        SetClipboardData( CF_TEXT, Block );
        CloseClipboard( );
    } else {
        GlobalFree( Block );
    }
}

static void TreeDraft( ) {
    if ( !Tree.type )
        return;
    if ( Edge( VK_BACK, KeyWas[ VK_BACK ] ) ) {
        size_t Len = strlen( Tree.find );
        if ( Len )
            Tree.find[ Len - 1 ] = 0;
        return;
    }
    if ( Edge( VK_RETURN, KeyWas[ VK_RETURN ] ) ) {
        Tree.type = false;
        return;
    }
    bool Shift = Held( VK_SHIFT );
    for ( int Code = 'A'; Code <= 'Z'; Code++ ) {
        if ( !Edge( Code, KeyWas[ Code ] ) )
            continue;
        size_t Len = strlen( Tree.find );
        if ( Len >= ( size_t )sizeof( Tree.find ) - 1 )
            return;
        Tree.find[ Len ] = ( char )( Shift ? Code : Code + 32 );
        Tree.find[ Len + 1 ] = 0;
        return;
    }
    for ( int Code = '0'; Code <= '9'; Code++ ) {
        if ( !Edge( Code, KeyWas[ Code ] ) )
            continue;
        size_t Len = strlen( Tree.find );
        if ( Len >= ( size_t )sizeof( Tree.find ) - 1 )
            return;
        Tree.find[ Len ] = ( char )Code;
        Tree.find[ Len + 1 ] = 0;
        return;
    }
    char Extra = 0;
    if ( Edge( VK_SPACE, KeyWas[ VK_SPACE ] ) )
        Extra = ' ';
    else if ( Edge( VK_OEM_PERIOD, KeyWas[ VK_OEM_PERIOD ] ) )
        Extra = '.';
    else if ( Edge( VK_OEM_MINUS, KeyWas[ VK_OEM_MINUS ] ) )
        Extra = '-';
    if ( Extra ) {
        size_t Len = strlen( Tree.find );
        if ( Len >= ( size_t )sizeof( Tree.find ) - 1 )
            return;
        Tree.find[ Len ] = Extra;
        Tree.find[ Len + 1 ] = 0;
    }
}

static int WalkLive( uintptr_t Parent, int Depth, float Scale, const CRectangle& Pane, const CVector& Point, bool Click, bool Locked, int& Row, bool Paint, bool& Busy ) {
    uintptr_t List[ browse::KidCap ];
    int Count = 0;
    if ( Parent == 0 ) {
        if ( browse::Root( ) )
            List[ Count++ ] = browse::Root( );
    } else {
        Count = browse::Children( Parent, List, browse::KidCap );
    }
    int Shown = 0;
    float RowH = 24.0f * Scale;
    float Indent = 12.0f * Scale;
    float Icon = 15.0f * Scale;
    for ( int Index = 0; Index < Count; Index++ ) {
        uintptr_t Addr = List[ Index ];
        if ( !browse::Visible( Addr ) )
            continue;
        browse::Node* Item = browse::Get( Addr );
        if ( !Item )
            continue;
        Shown += 1;
        float Top = Pane.Top + ( float )Row * RowH - Tree.scroll;
        CRectangle Line( Pane.Left, Top, Pane.Width, RowH );
        bool See = Top + RowH > Pane.Top && Top < Pane.Bottom( );
        bool Kids = browse::HasKids( Addr );
        if ( Paint && See ) {
            bool Over = Line.Contains( Point ) && Pane.Contains( Point ) && !Locked && !Tree.type;
            CRectangle Arm( Pane.Left + 4.0f * Scale + Indent * ( float )Depth, Line.Top, 14.0f * Scale, RowH );
            if ( Over && Click ) {
                if ( Kids && Arm.Contains( Point ) )
                    browse::Toggle( Addr );
                else {
                    browse::Select( Addr );
                    Tree.pick = Addr;
                    Tree.confirm = false;
                }
            }
            if ( Tree.pick == Addr )
                Canvas->Rectangle( CRectangle( Pane.Left + 2.0f * Scale, Line.Top + 1.0f * Scale, Pane.Width - 4.0f * Scale, RowH - 2.0f * Scale ), Dress.trackOn, 4.0f * Scale );
            else if ( Over )
                Canvas->Rectangle( CRectangle( Pane.Left + 2.0f * Scale, Line.Top + 1.0f * Scale, Pane.Width - 4.0f * Scale, RowH - 2.0f * Scale ), CColor( 255, 255, 255, 14 ), 4.0f * Scale );
            if ( Kids )
                DrawCaret( CVector( Arm.Left + Arm.Width * 0.5f, Line.Top + RowH * 0.5f ), Item->open, Scale, Mix( CColor( 168, 178, 194 ), CColor( 230, 236, 246 ), Tree.pick == Addr ? 1.0f : 0.0f ) );
            unsigned long long Glyph = TreeGlyph( browse::Glyph( Addr ) );
            CRectangle Mark( Arm.Right( ) + 2.0f * Scale, Line.Top + ( RowH - Icon ) * 0.5f, Icon, Icon );
            if ( Glyph )
                Canvas->Image( Mark, Glyph, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), CColor( 255, 255, 255 ), 0.0f );
            char Caption[ 96 ];
            if ( Item->extra > 0 && Item->open )
                snprintf( Caption, sizeof( Caption ), "%s [%s] +%d", Item->name, Item->klass, Item->extra );
            else
                snprintf( Caption, sizeof( Caption ), "%s [%s]", Item->name, Item->klass );
            CColor Ink = Tree.pick == Addr ? CColor( 240, 246, 255 ) : Style->Text;
            Canvas->Text( CVector( Mark.Right( ) + 6.0f * Scale, Line.Top + ( RowH - Font->LineSpan ) * 0.5f ), Ink, Caption );
            Busy = Busy || Over;
        }
        Row += 1;
        if ( Item->open && Kids )
            Shown += WalkLive( Addr, Depth + 1, Scale, Pane, Point, Click, Locked, Row, Paint, Busy );
    }
    return Shown;
}

static bool DrawExplorer( float Across, float Vertical, const CVector& Point, bool Click, bool Press, float Scale ) {
    ( void )Press;
    PlaceExplore( Across, Vertical, Scale );

    float Wide = ExploreWidth * Scale;
    float Tall = ExploreHeight * Scale;
    float Round = Style->Rounding * Scale;
    CRectangle Bounds( Tree.origin, CVector( Wide, Tall ) );
    CRectangle Header;
    CRectangle Body;
    ExploreChrome( Bounds, Scale, Header, Body );

    float SearchH = 28.0f * Scale;
    CRectangle Search( Body.Left, Body.Top, Body.Width, SearchH );
    float Gap = 8.0f * Scale;
    float TreeW = Body.Width * 0.56f;
    float SideW = Body.Width - TreeW - Gap;
    float WorkTop = Search.Bottom( ) + 6.0f * Scale;
    float WorkH = Body.Bottom( ) - WorkTop;
    CRectangle Pane( Body.Left, WorkTop, TreeW, WorkH );
    CRectangle Side( Body.Left + TreeW + Gap, WorkTop, SideW, WorkH );

    bool OverTree = Pane.Contains( Point );
    bool OverSide = Side.Contains( Point );
    bool Locked = Tree.held;

    uintptr_t Root = browse::Root( );
    int Count = 0;
    bool Busy = false;
    if ( Root )
        Count = WalkLive( 0, 0, Scale, Pane, Point, false, true, Count, false, Busy );
    float RowH = 24.0f * Scale;
    float Need = ( float )Count * RowH;
    float Most = Need - Pane.Height;
    if ( Most < 0.0f )
        Most = 0.0f;
    if ( OverTree && !Locked && !Tree.type && Input->WheelDelta != 0.0f ) {
        Tree.scroll -= Input->WheelDelta * 42.0f * Scale;
        Input->WheelDelta = 0.0f;
    }
    if ( Tree.scroll > Most )
        Tree.scroll = Most;
    if ( Tree.scroll < 0.0f )
        Tree.scroll = 0.0f;

    Canvas->Shadow( Bounds, CColor( 6, 10, 18, 130 ), Round, 24.0f * Scale );
    Canvas->Rectangle( Bounds, Style->Surface, Round );
    DrawIce( Header, Bounds, Round, 1.0f );
    DrawTitle( Header, Scale, "Explorer" );

    bool OverFind = Search.Contains( Point ) && !Locked;
    if ( OverFind && Click )
        Tree.type = true;
    else if ( Click && !OverFind && Tree.type )
        Tree.type = false;
    Canvas->Rectangle( Search, Style->Elevated, 8.0f * Scale );
    if ( Tree.type )
        DrawIce( Search, Search, 8.0f * Scale, 0.45f );
    Canvas->Border( Search, Mix( Dress.foldLine, Style->AccentSoft, Tree.type ? 1.0f : 0.0f ), 8.0f * Scale, 1.0f );
    const char* Shown = Tree.find[ 0 ] ? Tree.find : ( Tree.type ? "" : "Search name or class" );
    Canvas->Text( CVector( Search.Left + 10.0f * Scale, Search.Top + ( SearchH - Font->LineSpan ) * 0.5f ), Tree.find[ 0 ] ? Style->Text : Style->Faint, Shown );
    if ( Tree.type && ( ( int )( Context->Elapsed * 2.0 ) & 1 ) ) {
        CVector Caret = Font->Measure( Tree.find );
        Canvas->Rectangle( CRectangle( Search.Left + 10.0f * Scale + Caret.Horizontal, Search.Top + 6.0f * Scale, 1.0f * Scale, SearchH - 12.0f * Scale ), Style->AccentSoft, 0.0f );
    }

    Canvas->Rectangle( Pane, Style->Elevated, 10.0f * Scale );
    Canvas->PushClip( Pane );
    int Row = 0;
    Busy = false;
    if ( Root )
        WalkLive( 0, 0, Scale, Pane, Point, Click && !Tree.held && !Tree.type, Tree.held, Row, true, Busy );
    else
        Canvas->Text( CVector( Pane.Left + 8.0f * Scale, Pane.Top + 8.0f * Scale ), Style->Faint, browse::Core( ).note[ 0 ] ? browse::Core( ).note : "Not attached" );
    Canvas->PopClip( );

    Canvas->Rectangle( Side, Style->Elevated, 10.0f * Scale );
    float In = 8.0f * Scale;
    float Y = Side.Top + In;
    const char* Path = browse::Core( ).path;
    CRectangle PathRow( Side.Left + In, Y, Side.Width - In * 2.0f, 22.0f * Scale );
    bool OverPath = PathRow.Contains( Point ) && !Locked;
    Canvas->Text( CVector( PathRow.Left, PathRow.Top + ( PathRow.Height - Font->LineSpan ) * 0.5f ), OverPath ? Dress.inkHot : Style->Faint, Path[ 0 ] ? Path : "—" );
    if ( OverPath && Click && Path[ 0 ] ) {
        CopyText( Path );
        lstrcpynA( Tree.note, "Copied path", ( int )sizeof( Tree.note ) );
        Tree.noteAge = 1.6f;
    }
    Y = PathRow.Bottom( ) + 6.0f * Scale;

    float ActH = 22.0f * Scale;
    float ActW = ( Side.Width - In * 2.0f - 6.0f * Scale ) * 0.5f;
    CRectangle A1( Side.Left + In, Y, ActW, ActH );
    CRectangle A2( A1.Right( ) + 6.0f * Scale, Y, ActW, ActH );
    Y = A1.Bottom( ) + 4.0f * Scale;
    CRectangle A3( Side.Left + In, Y, ActW, ActH );
    CRectangle A4( A3.Right( ) + 6.0f * Scale, Y, ActW, ActH );
    if ( DrawAction( A1, "Copy Path", Point, Click && !Locked, Scale, false ) && Path[ 0 ] ) {
        CopyText( Path );
        lstrcpynA( Tree.note, "Copied path", ( int )sizeof( Tree.note ) );
        Tree.noteAge = 1.6f;
    }
    if ( DrawAction( A2, "Goto", Point, Click && !Locked, Scale, false ) ) {
        browse::Goto( Tree.pick );
        lstrcpynA( Tree.note, browse::Core( ).note, ( int )sizeof( Tree.note ) );
        Tree.noteAge = 1.6f;
    }
    const char* Kill = Tree.confirm ? "Confirm" : "Destroy";
    if ( DrawAction( A3, Kill, Point, Click && !Locked, Scale, true ) ) {
        if ( !Tree.confirm )
            Tree.confirm = true;
        else {
            browse::Destroy( Tree.pick );
            Tree.pick = browse::Pick( );
            Tree.confirm = false;
            lstrcpynA( Tree.note, browse::Core( ).note, ( int )sizeof( Tree.note ) );
            Tree.noteAge = 1.6f;
        }
    }
    if ( DrawAction( A4, Tree.mark ? "Unmark" : "Highlight", Point, Click && !Locked, Scale, false ) )
        Tree.mark = !Tree.mark;
    Y = A4.Bottom( ) + 8.0f * Scale;

    CRectangle PropPane( Side.Left + In, Y, Side.Width - In * 2.0f, Side.Bottom( ) - In - Y - 18.0f * Scale );
    if ( PropPane.Height < 20.0f * Scale )
        PropPane.Height = 20.0f * Scale;
    float PropH = 18.0f * Scale;
    int PropN = browse::Core( ).propN;
    if ( OverSide && !OverTree && Input->WheelDelta != 0.0f ) {
        Tree.propScroll -= Input->WheelDelta * 28.0f * Scale;
        Input->WheelDelta = 0.0f;
    }
    float PropNeed = ( float )PropN * PropH;
    float PropMost = PropNeed - PropPane.Height;
    if ( PropMost < 0.0f )
        PropMost = 0.0f;
    if ( Tree.propScroll > PropMost )
        Tree.propScroll = PropMost;
    if ( Tree.propScroll < 0.0f )
        Tree.propScroll = 0.0f;
    Canvas->PushClip( PropPane );
    for ( int Index = 0; Index < PropN; Index++ ) {
        const browse::Prop& Item = browse::Core( ).props[ Index ];
        float Top = PropPane.Top + ( float )Index * PropH - Tree.propScroll;
        CRectangle Line( PropPane.Left, Top, PropPane.Width, PropH );
        if ( Top + PropH < PropPane.Top || Top > PropPane.Bottom( ) )
            continue;
        Canvas->Text( CVector( Line.Left, Line.Top + 1.0f * Scale ), Style->Faint, Item.label );
        CVector Size = Font->Measure( Item.text );
        float TextX = Line.Right( ) - Size.Horizontal;
        if ( TextX < Line.Left + 64.0f * Scale )
            TextX = Line.Left + 64.0f * Scale;
        Canvas->Text( CVector( TextX, Line.Top + 1.0f * Scale ), Style->Text, Item.text );
        if ( Item.write && Line.Contains( Point ) && Click && !Locked ) {
            browse::Nudge( Tree.pick, Item.write, Held( VK_SHIFT ) ? -2.0f : 2.0f );
            lstrcpynA( Tree.note, browse::Core( ).note, ( int )sizeof( Tree.note ) );
            Tree.noteAge = 1.2f;
        }
    }
    Canvas->PopClip( );

    if ( Tree.noteAge > 0.0f ) {
        Tree.noteAge -= Context->DeltaTime;
        Canvas->Text( CVector( Side.Left + In, Side.Bottom( ) - 16.0f * Scale ), Style->Faint, Tree.note );
    }

    Canvas->Border( Bounds, Style->Outline.Blend( Style->Accent, 0.22f ), Round, Style->Thickness );
    if ( ( Busy || OverTree || OverFind || OverPath || OverSide ) && !Moving( ) )
        Input->Pointer = PointerHand;
    if ( Tree.held )
        Input->Pointer = PointerMove;
    return Bounds.Contains( Point ) || Tree.held;
}

static float LiveFps( ) {
    float Instant = Context->DeltaTime > 0.00005f ? 1.0f / Context->DeltaTime : Context->Framerate;
    if ( Instant < 1.0f )
        Instant = Context->Framerate;
    static float Smooth = 0.0f;
    static float Shown = 0.0f;
    static float Wait = 0.0f;
    if ( Smooth < 1.0f )
        Smooth = Instant;
    else {
        float Rate = Context->DeltaTime * 1.4f;
        if ( Rate > 0.08f )
            Rate = 0.08f;
        Smooth += ( Instant - Smooth ) * Rate;
    }
    Wait += Context->DeltaTime;
    if ( Wait >= 0.4f || Shown < 1.0f ) {
        Shown = Smooth;
        Wait = 0.0f;
    }
    return Shown;
}

static void MarkLine( char* Line, size_t Cap ) {
    float Fps = LiveFps( );
    if ( Menu.watermark && Menu.showFps )
        snprintf( Line, Cap, "ff0l   %.0f fps", ( double )Fps );
    else if ( Menu.watermark )
        snprintf( Line, Cap, "ff0l" );
    else
        snprintf( Line, Cap, "%.0f fps", ( double )Fps );
}

static CRectangle PlaceMark( float Across, float Vertical, float Scale, CFont* Face, const char* Line ) {
    CVector Size = Face->Measure( Line );
    float PadX = 12.0f * Scale;
    float PadY = 7.0f * Scale;
    float Wide = Size.Horizontal + PadX * 2.0f;
    float Tall = Size.Vertical + PadY * 2.0f;
    if ( !Badge.ready ) {
        Badge.origin = CVector( 14.0f * Scale, Vertical - Tall - 14.0f * Scale );
        Badge.ready = true;
    }
    ClampBox( Badge.origin, Across, Vertical, Wide, Tall );
    return CRectangle( Badge.origin, CVector( Wide, Tall ) );
}

static bool DragMark( const CRectangle& Chip, const CVector& Point, float Across, float Vertical, bool AllowStart ) {
    bool Press = Held( VK_LBUTTON );
    if ( AllowStart && Press && !Menu.mouse && !Menu.held && !Tree.held && Chip.Contains( Point ) && !Menu.slide && !Listening( ) ) {
        Badge.held = true;
        Badge.grab = Point - Badge.origin;
    }
    if ( Badge.held ) {
        if ( Press )
            Badge.origin = Point - Badge.grab;
        else
            Badge.held = false;
    }
    ClampBox( Badge.origin, Across, Vertical, Chip.Width, Chip.Height );
    return Chip.Contains( Point ) || Badge.held;
}

static CVector OverlayOf( float Across, float Down ) {
    HWND Handle = ( HWND )ur::app::window( );
    POINT Point{ ( LONG )( Across + 0.5f ), ( LONG )( Down + 0.5f ) };
    if ( Handle )
        ScreenToClient( Handle, &Point );
    return CVector( ( float )Point.x, ( float )Point.y );
}

static bool OverlayToView( const CVector& Overlay, float& X, float& Y ) {
    POINT Point{ ( LONG )( Overlay.Horizontal + 0.5f ), ( LONG )( Overlay.Vertical + 0.5f ) };
    HWND OverlayHwnd = ( HWND )ur::app::window( );
    if ( OverlayHwnd )
        ClientToScreen( OverlayHwnd, &Point );
    HWND Game = world::GameWindow( );
    if ( Game && IsWindow( Game ) ) {
        ScreenToClient( Game, &Point );
        X = ( float )Point.x;
        Y = ( float )Point.y;
        return X > 1.0f && Y > 1.0f;
    }
    const world::Snap& Live = world::View( );
    X = Overlay.Horizontal - ( float )Live.clientX;
    Y = Overlay.Vertical - ( float )Live.clientY;
    return X > 1.0f && Y > 1.0f;
}

static bool EspDot( const world::Vec3& World, CVector& Out );

static world::Vec3 AimPoint( const world::Actor& Item, bool UsePred, int Bones, bool AllowPred ) {
    static const int Slot[ 6 ] = {
        world::BoneHead, world::BoneUpper, world::BoneUpper,
        world::BoneLower, world::BoneRoot, world::BoneLLegU
    };
    world::Vec3 Pos = Item.head;
    if ( Bones & 1 ) {
        Pos = Item.head;
        if ( Item.high.y > Item.head.y )
            Pos.y += ( Item.high.y - Item.head.y ) * 0.35f;
    } else {
        for ( int Index = 0; Index < 6; Index++ ) {
            if ( ( Bones & ( 1 << Index ) ) == 0 )
                continue;
            int Bone = Slot[ Index ];
            if ( Item.boneOk[ Bone ] ) {
                Pos = Item.world[ Bone ];
                if ( Index == 1 && Item.boneOk[ world::BoneHead ] ) {
                    Pos.x = ( Item.head.x + Item.world[ Bone ].x ) * 0.5f;
                    Pos.y = ( Item.head.y + Item.world[ Bone ].y ) * 0.5f;
                    Pos.z = ( Item.head.z + Item.world[ Bone ].z ) * 0.5f;
                }
                break;
            }
        }
    }
    if ( !UsePred || !AllowPred )
        return Pos;
    float Speed = sqrtf( Item.vel.x * Item.vel.x + Item.vel.y * Item.vel.y + Item.vel.z * Item.vel.z );
    if ( Speed < 1.5f )
        return Pos;
    world::Vec3 Vel = Item.vel;
    if ( Speed > 90.0f ) {
        Vel.x *= 90.0f / Speed;
        Vel.y *= 90.0f / Speed;
        Vel.z *= 90.0f / Speed;
    }
    const world::Snap& Live = world::View( );
    float Mine = Live.localPing;
    float Theirs = Item.ping > 0.0f ? Item.ping : Mine;
    float Ping = ( Mine + Theirs ) * 0.5f;
    if ( Ping > 0.25f )
        Ping = 0.25f;
    float Dist = Item.dist;
    if ( Dist < 1.0f )
        Dist = 1.0f;
    float Time = Ping * 0.5f + Dist / 800.0f;
    Time *= 0.80f;
    if ( Time > 0.45f )
        Time = 0.45f;
    Pos.x += Vel.x * Time;
    Pos.y += Vel.y * Time * 0.25f;
    Pos.z += Vel.z * Time;
    return Pos;
}

static world::Vec3 AimPoint( const world::Actor& Item, bool UsePred = true ) {
    return AimPoint( Item, UsePred, Aim.bones, Aim.pred );
}

static bool AimDot( const world::Vec3& World, CVector& Out ) {
    world::Dot View;
    if ( !world::ToView( World, View ) )
        return false;
    const world::Snap& Live = world::View( );
    float Wide = ( float )Live.viewW;
    float Tall = ( float )Live.viewH;
    if ( Wide < 8.0f || Tall < 8.0f )
        return false;
    if ( View.x < -48.0f || View.y < -48.0f || View.x > Wide + 48.0f || View.y > Tall + 48.0f )
        return false;
    world::Dot Hit;
    if ( !world::ToScreen( World, Hit ) )
        return false;
    Out = OverlayOf( Hit.x, Hit.y );
    return true;
}

static CVector AimMid( ) {
    if ( CursorVisible( ) )
        return Cursor( );
    return ScreenMid( );
}

static float AimRadius( float Scale, float Fov ) {
    const world::Snap& Snap = world::View( );
    float Wide = ( float )Snap.clientW;
    float Tall = ( float )Snap.clientH;
    HWND Overlay = ( HWND )ur::app::window( );
    if ( Overlay && Snap.clientW > 64 && Snap.clientH > 64 ) {
        POINT A{ Snap.clientX, Snap.clientY };
        POINT B{ Snap.clientX + Snap.clientW, Snap.clientY + Snap.clientH };
        ScreenToClient( Overlay, &A );
        ScreenToClient( Overlay, &B );
        Wide = ( float )( B.x - A.x );
        Tall = ( float )( B.y - A.y );
        if ( Wide < 0.0f )
            Wide = -Wide;
        if ( Tall < 0.0f )
            Tall = -Tall;
    }
    if ( Wide < 64.0f )
        Wide = ( float )ur::app::width( );
    if ( Tall < 64.0f )
        Tall = ( float )ur::app::height( );
    float Half = sqrtf( Wide * Wide + Tall * Tall ) * 0.5f;
    if ( Scale <= 0.0f )
        Scale = 1.0f;
    if ( Fov >= 359.0f )
        return Half * Scale;
    return Half * ( Fov / 360.0f ) * Scale;
}

static world::Vec3 SilentBone( const world::Actor& Item ) {
    static const int Slot[ 6 ] = {
        world::BoneHead, world::BoneUpper, world::BoneUpper,
        world::BoneLower, world::BoneRoot, world::BoneLLegU
    };
    if ( Mute.bones & 1 ) {
        world::Vec3 Pos = Item.boneOk[ world::BoneHead ] ? Item.world[ world::BoneHead ] : Item.head;
        if ( Item.high.y > Pos.y )
            Pos.y += ( Item.high.y - Pos.y ) * 0.28f;
        return Pos;
    }
    for ( int Index = 1; Index < 6; Index++ ) {
        if ( ( Mute.bones & ( 1 << Index ) ) == 0 )
            continue;
        int Bone = Slot[ Index ];
        if ( Item.boneOk[ Bone ] )
            return Item.world[ Bone ];
    }
    if ( Item.boneOk[ world::BoneHead ] )
        return Item.world[ world::BoneHead ];
    return Item.head;
}

static void TickAim( float Scale ) {
    static float RestX = 0.0f;
    static float RestY = 0.0f;
    static uintptr_t Hold = 0;
    static LARGE_INTEGER Freq = { };
    static LARGE_INTEGER Last = { };
    if ( !Freq.QuadPart ) {
        QueryPerformanceFrequency( &Freq );
        QueryPerformanceCounter( &Last );
    }
    LARGE_INTEGER Now = { };
    QueryPerformanceCounter( &Now );
    float Dt = ( float )( Now.QuadPart - Last.QuadPart ) / ( float )Freq.QuadPart;
    Last = Now;
    if ( Dt < 0.00025f )
        Dt = 0.00025f;
    if ( Dt > 0.05f )
        Dt = 0.05f;

    bool ListenBusy = Mute.listen || Aim.listen || Menu.listen || Menu.slide;
    bool MuteHeld = Held( Mute.key );
    bool SilentOk = Mute.on && MuteHeld && !ListenBusy;
    bool MouseOk = Aim.on && Held( Aim.key ) && !ListenBusy;
    if ( !Mute.on )
        silent::Remove( );
    if ( !SilentOk && !MouseOk ) {
        RestX = 0.0f;
        RestY = 0.0f;
        if ( !Aim.sticky || !Held( Aim.key ) )
            Hold = 0;
        if ( Mute.on )
            silent::Off( );
        return;
    }

    const world::Snap& Snap = world::View( );
    if ( !Snap.ready || Snap.count <= 0 ) {
        silent::Off( );
        return;
    }

    CVector Mid = AimMid( );
    auto Pick = [ & ]( float Fov, bool Team, int Bones, bool Pred, bool NeedVis, int Sort, CVector* OutAt ) -> const world::Actor* {
        const world::Actor* Best = nullptr;
        float Limit = AimRadius( Scale, Fov );
        float BestScore = 1.0e9f;
        float Far = 1.0f;
        for ( int Index = 0; Index < Snap.count; Index++ ) {
            if ( Snap.list[ Index ].dist > Far )
                Far = Snap.list[ Index ].dist;
        }
        CVector Chosen;
        for ( int Index = 0; Index < Snap.count; Index++ ) {
            const world::Actor& Item = Snap.list[ Index ];
            if ( Team && Item.mate )
                continue;
            if ( NeedVis && !Item.vis )
                continue;
            CVector At;
            float Screen = 1.0e9f;
            bool OnScreen = false;
            auto Consider = [ & ]( const world::Vec3& World ) {
                CVector Point;
                if ( !EspDot( World, Point ) )
                    return;
                float Dx = Point.Horizontal - Mid.Horizontal;
                float Dy = Point.Vertical - Mid.Vertical;
                float Dist = sqrtf( Dx * Dx + Dy * Dy );
                if ( !OnScreen || Dist < Screen ) {
                    OnScreen = true;
                    Screen = Dist;
                    At = Point;
                }
            };
            Consider( AimPoint( Item, Pred, Bones, Pred ) );
            if ( !OnScreen )
                continue;
            float Score = 0.0f;
            if ( Sort <= 0 ) {
                if ( Screen > Limit )
                    continue;
                Score = Screen;
            } else if ( Sort == 1 ) {
                if ( Screen > Limit )
                    continue;
                Score = Item.dist;
            } else {
                if ( Screen > Limit )
                    continue;
                float Sn = Limit > 1.0f ? Screen / Limit : Screen;
                float Dn = Far > 1.0f ? Item.dist / Far : Item.dist;
                Score = Sn * 0.5f + Dn * 0.5f;
            }
            if ( Score < BestScore ) {
                BestScore = Score;
                Best = &Item;
                Chosen = At;
            }
        }
        if ( Best && OutAt )
            *OutAt = Chosen;
        return Best;
    };

    if ( SilentOk ) {
        CVector GhostAt;
        CVector Cross = ScreenMid( );
        CVector KeepMid = Mid;
        float KeepScale = Scale;
        Mid = Cross;
        Scale = 1.0f;
        const world::Actor* Ghost = Pick( 360.0f, Mute.team, Mute.bones, Mute.pred, Mute.vis, Mute.sort, &GhostAt );
        Mid = KeepMid;
        Scale = KeepScale;
        if ( Ghost ) {
            world::Vec3 AimAt = SilentBone( *Ghost );
            if ( Mute.pred ) {
                float Speed = sqrtf( Ghost->vel.x * Ghost->vel.x + Ghost->vel.y * Ghost->vel.y + Ghost->vel.z * Ghost->vel.z );
                if ( Speed >= 1.5f ) {
                    world::Vec3 Vel = Ghost->vel;
                    if ( Speed > 90.0f ) {
                        Vel.x *= 90.0f / Speed;
                        Vel.y *= 90.0f / Speed;
                        Vel.z *= 90.0f / Speed;
                    }
                    float Ping = Snap.localPing;
                    if ( Ghost->ping > 0.0f )
                        Ping = ( Ping + Ghost->ping ) * 0.5f;
                    if ( Ping > 0.25f )
                        Ping = 0.25f;
                    float Dist = Ghost->dist;
                    if ( Dist < 1.0f )
                        Dist = 1.0f;
                    float Time = Ping * 0.25f + Dist / 1200.0f;
                    if ( Time > 0.18f )
                        Time = 0.18f;
                    AimAt.x += Vel.x * Time;
                    AimAt.y += Vel.y * Time * 0.25f;
                    AimAt.z += Vel.z * Time;
                }
            }
            world::Dot View;
            float Sx = 0.0f;
            float Sy = 0.0f;
            CVector At;
            if ( world::ToView( AimAt, View ) ) {
                Sx = View.x;
                Sy = View.y;
            } else if ( EspDot( AimAt, At ) ) {
                OverlayToView( At, Sx, Sy );
            } else {
                OverlayToView( GhostAt, Sx, Sy );
            }
            silent::On( AimAt, Sx, Sy, true );
        } else {
            silent::Off( );
        }
        if ( !MouseOk )
            return;
    } else if ( Mute.on ) {
        silent::Off( );
    }
    if ( !MouseOk )
        return;

    CVector BestAt;
    const world::Actor* Best = nullptr;
    if ( Aim.sticky && Hold ) {
        for ( int Index = 0; Index < Snap.count; Index++ ) {
            const world::Actor& Item = Snap.list[ Index ];
            if ( Item.player != Hold )
                continue;
            if ( Aim.team && Item.mate )
                break;
            if ( Aim.vis && !Item.vis )
                break;
            if ( AimDot( AimPoint( Item ), BestAt ) )
                Best = &Item;
            break;
        }
    }
    if ( !Best )
        Best = Pick( Aim.fov, Aim.team, Aim.bones, Aim.pred, Aim.vis, Aim.sort, &BestAt );
    if ( !Best ) {
        Hold = 0;
        return;
    }
    Hold = Best->player;
    float Dx = BestAt.Horizontal - Mid.Horizontal;
    float Dy = BestAt.Vertical - Mid.Vertical;
    if ( Dx * Dx + Dy * Dy < 4.0f )
        return;
    float T = Aim.smooth / 100.0f;
    if ( T < 0.0f )
        T = 0.0f;
    if ( T > 1.0f )
        T = 1.0f;
    float Tau = 0.035f;
    float CapPx = 14000.0f;
    if ( T <= 0.05f ) {
        Tau = 0.012f + ( T / 0.05f ) * 0.028f;
        CapPx = 18000.0f - ( T / 0.05f ) * 4000.0f;
    } else if ( T <= 0.50f ) {
        float U = ( T - 0.05f ) / 0.45f;
        Tau = 0.040f + U * 0.36f;
        CapPx = 14000.0f - U * 12600.0f;
    } else {
        float U = ( T - 0.50f ) / 0.50f;
        Tau = 0.40f + U * 2.00f;
        CapPx = 1400.0f - U * 1320.0f;
    }
    if ( Tau < 0.008f )
        Tau = 0.008f;
    float Alpha = 1.0f - expf( -Dt / Tau );
    if ( T < 0.005f ) {
        Alpha = 1.0f;
        float Cap = 22.0f;
        float Step = sqrtf( Dx * Dx + Dy * Dy );
        if ( Step > Cap ) {
            Dx *= Cap / Step;
            Dy *= Cap / Step;
        }
    } else {
        float Step = sqrtf( Dx * Dx + Dy * Dy ) * Alpha;
        float MaxStep = CapPx * Dt;
        if ( MaxStep < 0.35f )
            MaxStep = 0.35f;
        if ( Step > MaxStep && Step > 0.001f ) {
            float ScaleStep = MaxStep / Step;
            Alpha *= ScaleStep;
        }
    }
    if ( Alpha < 0.0f )
        Alpha = 0.0f;
    if ( Alpha > 1.0f )
        Alpha = 1.0f;
    RestX += Dx * Alpha;
    RestY += Dy * Alpha;
    int MoveX = ( int )( RestX >= 0.0f ? RestX + 0.5f : RestX - 0.5f );
    int MoveY = ( int )( RestY >= 0.0f ? RestY + 0.5f : RestY - 0.5f );
    if ( MoveX == 0 && MoveY == 0 )
        return;
    RestX -= ( float )MoveX;
    RestY -= ( float )MoveY;
    INPUT Step{ };
    Step.type = INPUT_MOUSE;
    Step.mi.dx = MoveX;
    Step.mi.dy = MoveY;
    Step.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput( 1, &Step, sizeof( Step ) );
}

static bool EspDot( const world::Vec3& World, CVector& Out ) {
    world::Dot Hit;
    if ( !world::ToScreen( World, Hit ) )
        return false;
    Out = OverlayOf( Hit.x, Hit.y );
    return true;
}

static void DrawFovRings( float Scale ) {
    CVector Mid = AimMid( );
    float Pulse = 0.7f + 0.3f * ( 0.5f + 0.5f * sinf( ( float )Context->Elapsed * 1.8f ) );
    float Keep = Canvas->Opacity;
    if ( Aim.on && Aim.drawFov ) {
        float Ring = ur::motion::toward( "aim.fov.ring", 1.0f, 18.0f );
        float Radius = AimRadius( Scale, Aim.fov );
        Canvas->Opacity = Keep * Ring * Pulse;
        Canvas->Border( CRectangle( Mid.Horizontal - Radius, Mid.Vertical - Radius, Radius * 2.0f, Radius * 2.0f ), Mix( Style->Accent, Style->AccentSoft, 0.3f ), Radius, 1.6f * Scale );
    }
    Canvas->Opacity = Keep;
}

static void DrawWeather( float Across, float Vertical, float Scale ) {
    weather::Tick( ( float )Context->DeltaTime, Across, Vertical );
    if ( weather::mode( ) == weather::Off )
        return;

    float Keep = Canvas->Opacity;
    Canvas->Opacity = 1.0f;
    weather::State& Storm = weather::Live( );
    CColor Flake = Mix( CColor( 230, 236, 246 ), Style->AccentSoft, 0.15f );
    CColor Streak = Mix( CColor( 170, 190, 220 ), Style->AccentSoft, 0.25f );

    if ( weather::mode( ) == weather::Storm ) {
        unsigned int Former = Canvas->Effect( weather::ThunderFx( ) );
        Canvas->Rectangle( CRectangle( 0.0f, 0.0f, Across, Vertical ), CColor( 255, 255, 255, 210 ), 0.0f );
        Canvas->Effect( Former );
        Canvas->Opacity = Keep;
        return;
    }

    for ( int Index = 0; Index < Storm.used; Index++ ) {
        const weather::Drop& Item = Storm.list[ Index ];
        if ( weather::mode( ) == weather::Snow )
            Canvas->Circle( CVector( Item.x, Item.y ), Item.size * Scale, Flake.Fade( 0.72f ) );
        else
            Canvas->Line( CVector( Item.x, Item.y ), CVector( Item.x + Item.vx * 0.018f, Item.y + Item.size ), Streak.Fade( 0.45f ), 1.1f * Scale );
    }
    Canvas->Opacity = Keep;
}

static void DrawEspWorld( float Scale ) {
    if ( !Esp.on )
        return;
    const world::Snap& Snap = world::View( );
    if ( !Snap.ready || Snap.count <= 0 )
        return;

    float Keep = Canvas->Opacity;
    Canvas->Opacity = 1.0f;
    CColor Edge = CColor( 8, 10, 14, 210 );
    float Thick = 1.5f * Scale;
    CVector Foot( ( float )ur::app::width( ) * 0.5f, ( float )ur::app::height( ) - 4.0f * Scale );

    static const int Links[ ][ 2 ] = {
        { 0, 2 }, { 2, 3 },
        { 2, 4 }, { 4, 5 }, { 5, 6 },
        { 2, 7 }, { 7, 8 }, { 8, 9 },
        { 3, 10 }, { 10, 11 }, { 11, 12 },
        { 3, 13 }, { 13, 14 }, { 14, 15 },
        { 0, 2 }, { 2, 4 }, { 2, 7 }, { 2, 10 }, { 2, 13 }
    };

    for ( int Index = 0; Index < Snap.count; Index++ ) {
        const world::Actor& Item = Snap.list[ Index ];
        if ( Item.dist > Esp.range )
            continue;
        if ( Esp.team && Item.mate )
            continue;
        CVector Dots[ world::BoneMax ];
        bool On[ world::BoneMax ] = { };
        float MinX = 1.0e9f;
        float MaxX = -1.0e9f;
        float MinY = 1.0e9f;
        float MaxY = -1.0e9f;
        int Hits = 0;
        auto Push = [ & ]( const world::Vec3& World ) {
            CVector At;
            if ( !EspDot( World, At ) )
                return;
            if ( At.Horizontal < MinX )
                MinX = At.Horizontal;
            if ( At.Horizontal > MaxX )
                MaxX = At.Horizontal;
            if ( At.Vertical < MinY )
                MinY = At.Vertical;
            if ( At.Vertical > MaxY )
                MaxY = At.Vertical;
            Hits++;
        };
        auto PushOff = [ & ]( world::Vec3 Point, float Side, float Lift ) {
            Point.x += Snap.right.x * Side;
            Point.y += Lift;
            Point.z += Snap.right.z * Side;
            Push( Point );
        };
        Push( Item.head );
        Push( Item.low );
        Push( Item.root );
        PushOff( Item.head, 0.70f, 0.20f );
        PushOff( Item.head, -0.70f, 0.20f );
        PushOff( Item.low, 1.05f, 0.0f );
        PushOff( Item.low, -1.05f, 0.0f );
        for ( int Slot = 0; Slot < world::BoneMax; Slot++ ) {
            if ( !Item.boneOk[ Slot ] )
                continue;
            CVector At;
            if ( !EspDot( Item.world[ Slot ], At ) )
                continue;
            Dots[ Slot ] = At;
            On[ Slot ] = true;
            Push( Item.world[ Slot ] );
        }
        if ( Hits < 2 )
            continue;

        float Tall = MaxY - MinY;
        float Wide = MaxX - MinX;
        if ( Tall < 2.0f || Wide < 2.0f )
            continue;
        CRectangle Box( MinX, MinY, Wide, Tall );

        if ( Esp.snap )
            Canvas->Line( Foot, CVector( Box.Left + Box.Width * 0.5f, Box.Bottom( ) ), FeatColor( FeatSnap, Item.vis ).Fade( 0.55f ), Thick );

        if ( Esp.skeleton ) {
            CColor Joint = FeatColor( FeatSkel, Item.vis );
            int Start = Item.r15 ? 0 : 14;
            int Count = Item.r15 ? 14 : 5;
            for ( int Link = 0; Link < Count; Link++ ) {
                int A = Links[ Start + Link ][ 0 ];
                int B = Links[ Start + Link ][ 1 ];
                if ( !On[ A ] || !On[ B ] )
                    continue;
                Canvas->Line( Dots[ A ], Dots[ B ], Joint, Thick );
            }
        }

        if ( Esp.box )
            Canvas->Border( Box, FeatColor( FeatBox, Item.vis ), 0.0f, Thick );

        if ( Esp.health ) {
            float Ratio = Item.health / Item.maxHealth;
            if ( Ratio < 0.0f )
                Ratio = 0.0f;
            if ( Ratio > 1.0f )
                Ratio = 1.0f;
            float BarW = 3.0f * Scale;
            CRectangle Rail( Box.Left - 6.0f * Scale, Box.Top, BarW, Box.Height );
            Canvas->Rectangle( Rail, CColor( 10, 12, 16, 190 ), 0.0f );
            CRectangle Fill( Rail.Left, Rail.Bottom( ) - Rail.Height * Ratio, Rail.Width, Rail.Height * Ratio );
            Canvas->Rectangle( Fill, FeatColor( FeatHealth, Item.vis ), 0.0f );
        }

        if ( Font && Esp.name ) {
            CVector Size = Font->Measure( Item.name );
            CVector At( Box.Left + ( Box.Width - Size.Horizontal ) * 0.5f, Box.Top - Size.Vertical - 3.0f * Scale );
            Canvas->Outlined( At, FeatColor( FeatName, Item.vis ), Edge, 1.0f, Item.name );
        }
        if ( Font && Esp.dist ) {
            char Line[ 24 ];
            snprintf( Line, sizeof( Line ), "%.0fm", ( double )Item.dist );
            CVector Size = Font->Measure( Line );
            CVector At( Box.Left + ( Box.Width - Size.Horizontal ) * 0.5f, Box.Bottom( ) + 3.0f * Scale );
            Canvas->Outlined( At, FeatColor( FeatDist, Item.vis ), Edge, 1.0f, Line );
        }
    }
    Canvas->Opacity = Keep;
}

static void DrawExploreMark( float Scale ) {
    if ( !Tree.mark || !Tree.pick )
        return;
    uintptr_t Part = browse::AimPart( Tree.pick );
    world::Vec3 Pos;
    if ( !Part || !world::PartPos( Part, Pos ) )
        return;
    world::Vec3 Size{ };
    if ( !world::PartSize( Part, Size ) ) {
        Size.x = 1.0f;
        Size.y = 2.0f;
        Size.z = 1.0f;
    }
    world::Vec3 Hi = Pos;
    world::Vec3 Lo = Pos;
    Hi.y += Size.y * 0.5f + 0.15f;
    Lo.y -= Size.y * 0.5f;
    Hi.x += Size.x * 0.5f;
    Lo.x -= Size.x * 0.5f;
    CVector A;
    CVector B;
    CVector C;
    CVector D;
    if ( !EspDot( Hi, A ) || !EspDot( Lo, B ) )
        return;
    world::Vec3 Right = Pos;
    Right.x += Size.x * 0.5f;
    Right.z += Size.z * 0.5f;
    world::Vec3 Left = Pos;
    Left.x -= Size.x * 0.5f;
    Left.z -= Size.z * 0.5f;
    EspDot( Right, C );
    EspDot( Left, D );
    float MinX = A.Horizontal < B.Horizontal ? A.Horizontal : B.Horizontal;
    float MaxX = A.Horizontal > B.Horizontal ? A.Horizontal : B.Horizontal;
    float MinY = A.Vertical < B.Vertical ? A.Vertical : B.Vertical;
    float MaxY = A.Vertical > B.Vertical ? A.Vertical : B.Vertical;
    auto Push = [ & ]( const CVector& At ) {
        if ( At.Horizontal < MinX )
            MinX = At.Horizontal;
        if ( At.Horizontal > MaxX )
            MaxX = At.Horizontal;
        if ( At.Vertical < MinY )
            MinY = At.Vertical;
        if ( At.Vertical > MaxY )
            MaxY = At.Vertical;
    };
    Push( C );
    Push( D );
    float Wide = MaxX - MinX;
    float Tall = MaxY - MinY;
    if ( Wide < 2.0f || Tall < 2.0f )
        return;
    float Keep = Canvas->Opacity;
    Canvas->Opacity = 1.0f;
    Canvas->Border( CRectangle( MinX, MinY, Wide, Tall ), CColor( 255, 80, 200, 230 ), 0.0f, 2.0f * Scale );
    Canvas->Opacity = Keep;
}

static bool DrawMarks( float Across, float Vertical, float Scale, const CVector& Point, bool AllowDrag ) {
    if ( !Menu.watermark && !Menu.showFps ) {
        Badge.held = false;
        return false;
    }

    EnsureTitle( Scale );
    CFont* Face = ( Font && Font->LineSpan > 1.0f ) ? Font.get( ) : &TitleFace;
    if ( !Face || Face->LineSpan < 1.0f )
        return false;

    char Line[ 48 ] = { };
    MarkLine( Line, sizeof( Line ) );
    CRectangle Chip = PlaceMark( Across, Vertical, Scale, Face, Line );
    bool Over = DragMark( Chip, Point, Across, Vertical, AllowDrag );
    Chip = PlaceMark( Across, Vertical, Scale, Face, Line );

    float PadX = 12.0f * Scale;
    float Keep = Canvas->Opacity;
    Canvas->Opacity = 1.0f;
    Canvas->Rectangle( Chip, Dress.card, 7.0f * Scale );
    DrawIce( Chip, Chip, 7.0f * Scale, 0.55f );
    Canvas->Border( Chip, Dress.foldLine, 7.0f * Scale, 1.0f );
    Canvas->Write( Face, CVector( Chip.Left + PadX, Chip.Top + ( Chip.Height - Face->LineSpan ) * 0.5f ), Dress.inkHot, Line );
    Canvas->Opacity = Keep;
    if ( Over && !Moving( ) )
        Input->Pointer = PointerHand;
    if ( Badge.held )
        Input->Pointer = PointerMove;
    return Over;
}

static void TickStream( ) {
    HWND Handle = ( HWND )ur::app::window( );
    if ( !Handle )
        return;

    DWORD Want = Menu.stream ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
    DWORD Now = WDA_NONE;
    if ( GetWindowDisplayAffinity( Handle, &Now ) && Now == Want ) {
        Menu.streamed = Menu.stream;
        return;
    }

    if ( SetWindowDisplayAffinity( Handle, Want ) ) {
        Menu.streamed = Menu.stream;
        return;
    }

    if ( Menu.stream )
        SetWindowDisplayAffinity( Handle, 0x00000001 );
}

static void Draw( float Across, float Vertical ) {
    float Scale = Style->Scale > 0.0f ? Style->Scale : 1.0f;
    float Wide = MenuWidth * Scale;
    float Tall = MenuHeight * Scale;
    float Cap = HeaderHeight * Scale;
    float Round = Style->Rounding * Scale;
    float Pad = 10.0f * Scale;
    float RailW = RailWidth * Scale;

    if ( !Menu.ready ) {
        Center( Across, Vertical, Scale );
        Menu.ready = true;
    }

    CVector Point = Cursor( );
    CRectangle Bounds( Menu.origin, CVector( Wide, Tall ) );
    CRectangle Header( Bounds.Left, Bounds.Top, Bounds.Width, Cap );
    CRectangle Rail( Bounds.Left + Pad, Header.Bottom( ) + Pad, RailW, Bounds.Height - Cap - Pad * 2.0f );
    CRectangle Stack( Rail.Left, Rail.Top, Rail.Width, TabHeight * Scale * ( float )TabCount + TabGap * Scale * ( float )( TabCount - 1 ) );
    CRectangle Content( Rail.Right( ) + Pad, Header.Bottom( ) + Pad, Bounds.Right( ) - Rail.Right( ) - Pad * 2.0f, Bounds.Height - Cap - Pad * 2.0f );
    CRectangle Shut = CloseBounds( Header, Scale );

    bool Press = Held( VK_LBUTTON );
    bool Click = Press && !Menu.mouse && !LiveCh.open;
    bool OverExplore = false;
    if ( Tree.open ) {
        PlaceExplore( Across, Vertical, Scale );
        CRectangle ExploreBox( Tree.origin, CVector( ExploreWidth * Scale, ExploreHeight * Scale ) );
        CRectangle ExploreHead;
        CRectangle ExplorePane;
        ExploreChrome( ExploreBox, Scale, ExploreHead, ExplorePane );
        OverExplore = ExploreBox.Contains( Point );
        DragExplore( ExploreBox, Point, Across, Vertical, ExploreWidth * Scale, ExploreHeight * Scale, OverExplore && !ExplorePane.Contains( Point ) && !Listening( ) );
        OverExplore = CRectangle( Tree.origin, CVector( ExploreWidth * Scale, ExploreHeight * Scale ) ).Contains( Point ) || Tree.held;
    }

    if ( Click && !Moving( ) && !OverExplore ) {
        for ( int Index = 0; Index < TabCount; Index++ ) {
            if ( TabBounds( Rail, Scale, Index ).Contains( Point ) && Menu.tab != Index ) {
                Menu.pageDir = Index > Menu.tab ? 1.0f : -1.0f;
                Menu.tab = Index;
                Menu.pageIn = 0.0f;
                DropId = nullptr;
            }
        }
    }

    bool OverMark = false;
    if ( Menu.watermark || Menu.showFps ) {
        EnsureTitle( Scale );
        CFont* Face = ( Font && Font->LineSpan > 1.0f ) ? Font.get( ) : &TitleFace;
        if ( Face && Face->LineSpan > 1.0f ) {
            char Line[ 48 ] = { };
            MarkLine( Line, sizeof( Line ) );
            CRectangle Chip = PlaceMark( Across, Vertical, Scale, Face, Line );
            OverMark = DragMark( Chip, Point, Across, Vertical, !OverExplore && !Listening( ) );
        }
    }

    bool OverTab = Stack.Contains( Point );
    bool OverClose = Shut.Contains( Point );
    bool OverPage = ( ( Menu.tab == TabAimbot || Menu.tab == TabRage || Menu.tab == TabEsp || Menu.tab == TabConfigs || Menu.tab == TabSettings ) && Content.Contains( Point ) ) || DropHit( Point, Scale );
    Drag( Bounds, Point, Across, Vertical, Wide, Tall, !OverTab && !OverClose && !OverPage && !Listening( ) && !OverExplore && !Tree.held && !OverMark && !Badge.held );
    Bounds = CRectangle( Menu.origin, CVector( Wide, Tall ) );
    Header = CRectangle( Bounds.Left, Bounds.Top, Bounds.Width, Cap );
    Rail = CRectangle( Bounds.Left + Pad, Header.Bottom( ) + Pad, RailW, Bounds.Height - Cap - Pad * 2.0f );
    Stack = CRectangle( Rail.Left, Rail.Top, Rail.Width, TabHeight * Scale * ( float )TabCount + TabGap * Scale * ( float )( TabCount - 1 ) );
    Content = CRectangle( Rail.Right( ) + Pad, Header.Bottom( ) + Pad, Bounds.Right( ) - Rail.Right( ) - Pad * 2.0f, Bounds.Height - Cap - Pad * 2.0f );
    Shut = CloseBounds( Header, Scale );

    CRectangle Drawn[ TabCount ];
    OverTab = false;
    for ( int Index = 0; Index < TabCount; Index++ ) {
        Drawn[ Index ] = TabBounds( Rail, Scale, Index );
        if ( Drawn[ Index ].Contains( Point ) )
            OverTab = true;
    }

    Gate( Bounds.Contains( Point ) || OverExplore || OverMark || DropHit( Point, Scale ), Point );
    if ( ( OverTab || OverClose ) && !Moving( ) )
        Input->Pointer = PointerHand;

    Canvas->Route( 0 );
    DrawWeather( Across, Vertical, Scale );
    DrawEspWorld( Scale );
    DrawExploreMark( Scale );
    float Fade = Menu.fade / 100.0f;
    if ( Fade < 0.4f )
        Fade = 0.4f;
    if ( Fade > 1.0f )
        Fade = 1.0f;
    DrawFovRings( Scale );
    float Shell = Canvas->Opacity;
    Canvas->Opacity = Shell * Fade;
    Canvas->Shadow( Bounds, CColor( 6, 10, 18, 130 ), Round, 24.0f * Scale );
    Canvas->Rectangle( Bounds, Style->Surface, Round );

    DrawIce( Header, Bounds, Round, 1.0f );
    DrawTitle( Header, Scale, "ff0l" );
    bool CloseBusy = DrawClose( Header, Point, Click && !OverExplore, Scale, "close.hover", true );

    Canvas->Rectangle( Stack, Dress.rail, 12.0f * Scale );
    DrawTabSwipe( Rail, Scale );
    Canvas->Rectangle( Content, Style->Elevated, 10.0f * Scale );
    for ( int Index = 0; Index < TabCount; Index++ )
        DrawTab( Drawn[ Index ], Tabs[ Index ], Index, Scale, Drawn[ Index ].Contains( Point ) && !Moving( ) && !OverExplore );
    bool PageBusy = false;
    DrawPage( Content, Point, Click && !OverExplore, Press, Scale, PageBusy );
    if ( ( PageBusy || CloseBusy ) && !Moving( ) )
        Input->Pointer = PointerHand;
    Canvas->Border( Bounds, Style->Outline.Blend( Style->Accent, 0.22f ), Round, Style->Thickness );

    if ( Tree.open )
        DrawExplorer( Across, Vertical, Point, Click, Press, Scale );
    Canvas->Opacity = Shell;
    DrawMarks( Across, Vertical, Scale, Point, false );
}

static void TickMenuMouse( ) {
    static bool Freed = false;
    if ( !Freed ) {
        ClipCursor( nullptr );
        Freed = true;
    }
}

static void Tick( ) {
    if ( !Menu.vsync )
        ur::app::set_vsync( false );
    Tokens( );
    PackBoot( );
    offsets::Boot( );
    TickChannel( );
    world::Pulse( Esp.on || Aim.on || Mute.on, Aim.on || Mute.on, Esp.skeleton, Esp.range, Esp.on || Aim.vis || ( Mute.on && Mute.vis ) );
    TickMenuMouse( );
    TickAim( Style->Scale > 0.0f ? Style->Scale : 1.0f );
    move::Tick( Context->DeltaTime, Menu.listen || Aim.listen || Mute.listen );
    if ( !Mute.on ) {
        silent::Off( );
        silent::Remove( );
    }
    if ( Tree.open ) {
        if ( !browse::Live( ) )
            browse::Open( );
        if ( !Esp.on && !Aim.on )
            world::FrameView( );
        lstrcpynA( browse::Core( ).query, Tree.find, ( int )sizeof( browse::Core( ).query ) );
        browse::Tick( );
        Tree.pick = browse::Pick( );
        if ( Tree.type )
            TreeDraft( );
    }

    static bool Quiet = false;
    if ( !Quiet ) {
        ur::toast::clear( );
        Quiet = true;
    }

    if ( Edge( VK_ESCAPE, Menu.escape ) ) {
        if ( LiveCh.open ) {
            LiveCh.open = false;
            LiveCh.dismissed = true;
        } else if ( Packs.type )
            Packs.type = false;
        else if ( Tree.type )
            Tree.type = false;
        else if ( Tree.confirm )
            Tree.confirm = false;
        else if ( Menu.listen )
            Menu.listen = false;
        else if ( DropId )
            DropId = nullptr;
        else if ( Aim.listen )
            Aim.listen = false;
        else if ( Mute.listen )
            Mute.listen = false;
        else if ( Tree.open ) {
            Tree.open = false;
            browse::Close( );
        }
    }

    if ( Packs.type ) {
        PackDraft( );
    } else if ( Menu.listen || Aim.listen || Mute.listen ) {
        int Next = PollBind( ( Aim.listen || Mute.listen ) && !Menu.listen );
        if ( Next ) {
            if ( Menu.listen ) {
                Menu.menuKey = Next;
                Menu.listen = false;
                Menu.insert = true;
            } else if ( Mute.listen ) {
                Mute.key = Next;
                Mute.listen = false;
            } else {
                Aim.key = Next;
                Aim.listen = false;
            }
        }
    } else if ( Edge( Menu.menuKey, Menu.insert ) ) {
        Menu.visible = !Menu.visible;
        if ( Menu.visible )
            OpenLiveFolds( );
    }

    TickAfk( );
    TickStream( );

    bool Press = Held( VK_LBUTTON );
    bool ChanClick = Press && !ChanMouse;
    ChanMouse = Press;

    if ( !Menu.visible ) {
        Tree.held = false;
        float Across = ( float )ur::app::width( );
        float Vertical = ( float )ur::app::height( );
        float Scale = Style->Scale > 0.0f ? Style->Scale : 1.0f;
        CVector Point = Cursor( );
        Canvas->Route( 0 );
        DrawEspWorld( Scale );
        DrawFovRings( Scale );
        DrawExploreMark( Scale );
        bool OverMark = DrawMarks( Across, Vertical, Scale, Point, true );
        DrawChannelNotice( Across, Vertical, Point, ChanClick, Scale );
        ur::overlay::Options& Overlay = ur::app::overlay_options( );
        Overlay.click_through = !LiveCh.open && !OverMark && !Badge.held;
        Menu.mouse = Press;
        Pace( );
        return;
    }

    Draw( ( float )ur::app::width( ), ( float )ur::app::height( ) );
    DrawChannelNotice( ( float )ur::app::width( ), ( float )ur::app::height( ), Cursor( ), ChanClick, Style->Scale > 0.0f ? Style->Scale : 1.0f );
    if ( LiveCh.open )
        ur::app::overlay_options( ).click_through = false;
    Pace( );
}

static bool LoadFace( const char* Path ) {
    if ( AddFontResourceExA( Path, FR_PRIVATE, nullptr ) <= 0 )
        return false;
    if ( !FaceHandle ) {
        lstrcpynA( FacePath, Path, MAX_PATH );
        FaceHandle = ( HANDLE )1;
    }
    return true;
}

static void BindFace( ) {
    char Module[ MAX_PATH ] = { };
    GetModuleFileNameA( nullptr, Module, MAX_PATH );
    std::string Folder = Module;
    size_t Slash = Folder.find_last_of( "\\/" );
    if ( Slash != std::string::npos )
        Folder = Folder.substr( 0, Slash ) + "\\assets\\fonts\\";
    else
        Folder = "assets\\fonts\\";

    const char* Local[ ] = {
        "Poppins-Regular.ttf",
        "Poppins-Medium.ttf",
        "Poppins-SemiBold.ttf"
    };

    for ( const char* Name : Local )
        LoadFace( ( Folder + Name ).c_str( ) );
}

}

int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int ) {
    BindFace( );

    ur::overlay::Options& Overlay = ur::app::overlay_options( );
    Overlay.topmost = true;
    Overlay.borderless = true;
    Overlay.transparent = true;
    Overlay.layered = true;
    Overlay.click_through = true;
    Overlay.alpha = 255;

    ur::app::Config Config;
    Config.title = "ff0l";
    Config.width = 1280;
    Config.height = 720;
    Config.backend = ur::Backend::DX11;
    Config.overlay = true;
    Config.persist = false;
    Config.docking = false;
    Config.vsync = false;
    Config.fonts = Fonts;
    Config.font_count = 3;
    Config.font_size = 13.0f;

    offsets::Boot( );
    world::Boot( );
    int Code = ur::app::run( Config, Tick );
    TitleFace.Destroy( );
    if ( FaceHandle && FacePath[ 0 ] )
        RemoveFontResourceExA( FacePath, FR_PRIVATE, nullptr );
    return Code;
}
