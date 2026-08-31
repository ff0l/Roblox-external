#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "ur/app.hpp"
#include "ur/config.hpp"
#include "ur/keys.hpp"
#include "ur/debug.hpp"
#include "ur/discord.hpp"
#include "ur/effects.hpp"
#include "ur/glyphs.hpp"
#include "ur/hear.hpp"
#include "ur/image.hpp"
#include "ur/media.hpp"
#include "ur/palette.hpp"
#include "ur/player.hpp"
#include "ur/settings.hpp"
#include "ur/theme.hpp"
#include "ur/toast.hpp"

#include "ElevenHost.h"
#include "OpenGLHost.h"
#include "TwelveHost.h"
#include "VulkanHost.h"
#include "Native.h"

#include <Windows.h>
#include <dwmapi.h>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <string>
#include <timeapi.h>
#include <vector>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace ur {
namespace app {

static Config Active;
static bool Quit = false;
static bool Resized = false;
static int ClientWidth = 0;
static int ClientHeight = 0;
static HWND Handle = nullptr;
static std::function< void( ) >* Ticker = nullptr;
static std::vector< CHost* > Roster;
static std::vector< const char* > Names;
static std::vector< void ( * )( CGraphics* ) > GraphicsHooks;
static int Running = 0;
static int Selected = 0;
static bool VerticalSync = true;
static CGraphics* Gfx = nullptr;
static overlay::Options Overlay;
static bool OverlayReady = false;

static void TitleBar( HWND Window, bool Dark ) {
    BOOL Enabled = Dark ? TRUE : FALSE;
    if ( FAILED( DwmSetWindowAttribute( Window, DWMWA_USE_IMMERSIVE_DARK_MODE, &Enabled, sizeof( Enabled ) ) ) )
        DwmSetWindowAttribute( Window, 19, &Enabled, sizeof( Enabled ) );
}

static void SyncTitle( ) {
    int Sum = ( int )Style->Backdrop.Red + ( int )Style->Backdrop.Green + ( int )Style->Backdrop.Blue;
    TitleBar( Handle, Sum < 420 );
}

static void BindGraphics( ) {
    Gfx = Roster[ ( size_t )Running ]->Graphics( );
    glyphs::bind( Gfx );
    player::bind( Gfx );
    effects::bind( Gfx );
    image::bind( Gfx );
    for ( auto Hook : GraphicsHooks )
        Hook( Gfx );
}

static void FillRoster( ) {
    Roster.clear( );
    Names.clear( );
    Roster.push_back( ElevenHost.get( ) );
    Names.push_back( "DirectX 11" );
    Roster.push_back( TwelveHost.get( ) );
    Names.push_back( "DirectX 12" );
    Roster.push_back( OpenGLHost.get( ) );
    Names.push_back( "OpenGL" );
#if UR_VULKAN
    Roster.push_back( VulkanHost.get( ) );
    Names.push_back( "Vulkan" );
#endif
}

static int ClampBackend( int Index ) {
    if ( Index < 0 || Index >= ( int )Roster.size( ) )
        return 0;
    return Index;
}

static bool StartHost( int Index ) {
    Index = ClampBackend( Index );
    if ( Roster[ ( size_t )Index ]->Create( Handle, ClientWidth, ClientHeight ) ) {
        Running = Index;
        Selected = Index;
        BindGraphics( );
        return true;
    }
    Roster[ ( size_t )Index ]->Destroy( );
    return false;
}

static bool PickHost( Backend Wanted ) {
    if ( Active.overlay ) {
        if ( StartHost( ( int )Backend::DX11 ) )
            return true;
        Context->Report( "Overlay needs DirectX 11" );
        return false;
    }

    if ( Wanted != Backend::Auto ) {
        int Index = ( int )Wanted;
        if ( StartHost( Index ) )
            return true;
        Context->Report( "Requested backend failed, trying others" );
    }

    for ( int Index = 0; Index < ( int )Roster.size( ); Index++ ) {
        if ( StartHost( Index ) ) {
            toast::push( Names[ ( size_t )Index ] );
            return true;
        }
    }

    return false;
}

static LRESULT CALLBACK ProcessMessage( HWND Origin, UINT Message, WPARAM Primary, LPARAM Secondary ) {
    if ( Native->Translate( Origin, Message, Primary, Secondary ) )
        return Message == WM_SETCURSOR ? TRUE : 0;

    switch ( Message ) {
    case WM_CLOSE:
        Quit = true;
        return 0;
    case WM_SIZE: {
        int Across = ( int )( unsigned short )LOWORD( Secondary );
        int Down = ( int )( unsigned short )HIWORD( Secondary );
        if ( Across > 0 && Down > 0 ) {
            ClientWidth = Across;
            ClientHeight = Down;
            Resized = true;
            if ( Ticker )
                ( *Ticker )( );
        }
        return 0;
    }
    case WM_MOVE:
        if ( Ticker )
            ( *Ticker )( );
        return 0;
    case WM_DPICHANGED: {
        RECT* Suggested = ( RECT* )Secondary;
        SetWindowPos( Origin, nullptr, Suggested->left, Suggested->top, Suggested->right - Suggested->left, Suggested->bottom - Suggested->top, SWP_NOZORDER | SWP_NOACTIVATE );
        Engine->Rescale( Native->Scale( ) );
        return 0;
    }
    case WM_ENTERSIZEMOVE:
        SetTimer( Origin, 1, 8, nullptr );
        return 0;
    case WM_EXITSIZEMOVE:
        KillTimer( Origin, 1 );
        return 0;
    case WM_TIMER:
        if ( Ticker )
            ( *Ticker )( );
        return 0;
    }

    return DefWindowProcW( Origin, Message, Primary, Secondary );
}

static void PersistWrite( ) {
    if ( !Active.persist )
        return;

    settings::set_int( "backend", Selected );
    settings::set_bool( "vsync", VerticalSync );
    settings::set_int( "theme", theme::current( ) );
    settings::set_bool( "glass", Style->Glass );
    settings::set_bool( "docking", Docking->Enabled );
    settings::set_float( "rounding", Style->Rounding / ( Style->Scale > 0.0f ? Style->Scale : 1.0f ) );
    settings::save( Active.settings );
    Engine->Save( Active.layout );
}

static void PersistRead( ) {
    if ( !Active.persist )
        return;

    settings::load( Active.settings );
    theme::apply( settings::get_int( "theme", 2 ) );
    Style->Glass = settings::get_bool( "glass", Style->Glass );
    Docking->Enabled = settings::get_bool( "docking", Active.docking );
    VerticalSync = settings::get_bool( "vsync", Active.vsync );
    float Round = settings::get_float( "rounding", 0.0f );
    if ( Round > 0.0f )
        Style->Rounding = Round * Style->Scale;
    Engine->Load( Active.layout );
}

static void TickFrame( std::function< void( ) >& User ) {
    if ( Resized ) {
        Resized = false;
        Roster[ ( size_t )Running ]->Resize( ClientWidth, ClientHeight );
    }

    if ( Active.persist && Input->Control( ) && Input->KeyPressed( ( int )Key::S ) ) {
        PersistWrite( );
        toast::push( "Layout saved" );
    }

    if ( Active.media )
        media::tick( );
    if ( Active.hear )
        hear::tick( );
    if ( Active.discord )
        discord::tick( );

    if ( OverlayReady )
        overlay::apply( Handle, Overlay );

    Engine->Begin( CVector( ( float )ClientWidth, ( float )ClientHeight ) );

    if ( Docking->Enabled )
        Docking->Space( CRectangle( 0.0f, 0.0f, ( float )ClientWidth, ( float )ClientHeight ) );

    User( );

    palette::draw( );
    toast::draw( );

    Engine->End( );

    SyncTitle( );

    CColor Wash = Style->Backdrop;
    if ( Active.overlay && Overlay.transparent )
        Wash = CColor( 0, 0, 0, 0 );
    Roster[ ( size_t )Running ]->Begin( Wash );
    Roster[ ( size_t )Running ]->Graphics( )->Render( Engine->Data( ), Roster[ ( size_t )Running ]->Stream( ) );
    Roster[ ( size_t )Running ]->End( VerticalSync );

    if ( Selected != Running ) {
        Roster[ ( size_t )Running ]->Destroy( );
        if ( Roster[ ( size_t )Selected ]->Create( Handle, ClientWidth, ClientHeight ) ) {
            Running = Selected;
            BindGraphics( );
            toast::push( Names[ ( size_t )Running ] );
        } else {
            Roster[ ( size_t )Selected ]->Destroy( );
            Selected = Running;
            if ( !Roster[ ( size_t )Running ]->Create( Handle, ClientWidth, ClientHeight ) ) {
                Quit = true;
                return;
            }
            BindGraphics( );
            toast::push( "Backend switch failed", Style->Danger );
        }
    }
}

void quit( ) {
    Quit = true;
}

void* window( ) {
    return Handle;
}

int width( ) {
    return ClientWidth;
}

int height( ) {
    return ClientHeight;
}

CGraphics* graphics( ) {
    return Gfx;
}

Backend backend( ) {
    return ( Backend )Running;
}

void set_backend( Backend Next ) {
    if ( Next == Backend::Auto )
        return;
    Selected = ClampBackend( ( int )Next );
}

bool vsync( ) {
    return VerticalSync;
}

void set_vsync( bool On ) {
    VerticalSync = On;
}

void on_graphics( void ( *Fn )( CGraphics* ) ) {
    if ( Fn )
        GraphicsHooks.push_back( Fn );
}

void save_layout( ) {
    PersistWrite( );
}

void load_layout( ) {
    Engine->Load( Active.layout );
}

const Config& config( ) {
    return Active;
}

overlay::Options& overlay_options( ) {
    return Overlay;
}

int run( const Config& Wanted, std::function< void( ) > User ) {
    Active = Wanted;
    Quit = false;
    VerticalSync = Active.vsync;

    timeBeginPeriod( 1 );

    WNDCLASSEXW Description = { };
    Description.cbSize = sizeof( WNDCLASSEXW );
    Description.style = CS_DBLCLKS | CS_OWNDC;
    Description.lpfnWndProc = ProcessMessage;
    Description.hInstance = GetModuleHandleW( nullptr );
    Description.hCursor = LoadCursorW( nullptr, ( const wchar_t* )IDC_ARROW );
    Description.lpszClassName = L"UR";

    if ( !RegisterClassExW( &Description ) )
        return 1;

    DWORD WindowStyle = WS_OVERLAPPEDWINDOW;
    DWORD ExtraStyle = 0;
    int FullWidth = Active.width;
    int FullHeight = Active.height;
    int AnchorLeft = 0;
    int AnchorTop = 0;

    if ( Active.overlay && Overlay.borderless ) {
        WindowStyle = WS_POPUP;
        ExtraStyle |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
        if ( Overlay.topmost )
            ExtraStyle |= WS_EX_TOPMOST;
        if ( Overlay.click_through )
            ExtraStyle |= WS_EX_TRANSPARENT;
        overlay::primary_monitor( AnchorLeft, AnchorTop, FullWidth, FullHeight );
    } else {
        RECT Frame = { 0, 0, Active.width, Active.height };
        AdjustWindowRect( &Frame, WS_OVERLAPPEDWINDOW, FALSE );
        FullWidth = ( int )( Frame.right - Frame.left );
        FullHeight = ( int )( Frame.bottom - Frame.top );
        AnchorLeft = ( GetSystemMetrics( SM_CXSCREEN ) - FullWidth ) / 2;
        AnchorTop = ( GetSystemMetrics( SM_CYSCREEN ) - FullHeight ) / 2;
    }

    if ( Active.overlay && !Overlay.transparent && ( Overlay.layered || Overlay.click_through || Overlay.alpha < 255 ) )
        ExtraStyle |= WS_EX_LAYERED;

    int TitleCount = MultiByteToWideChar( CP_UTF8, 0, Active.title ? Active.title : "UR", -1, nullptr, 0 );
    std::wstring Title( TitleCount > 0 ? ( size_t )TitleCount : 1, 0 );
    if ( TitleCount > 0 )
        MultiByteToWideChar( CP_UTF8, 0, Active.title ? Active.title : "UR", -1, Title.data( ), TitleCount );

    Handle = CreateWindowExW( ExtraStyle, L"UR", Title.c_str( ), WindowStyle, AnchorLeft, AnchorTop, FullWidth, FullHeight, nullptr, nullptr, Description.hInstance, nullptr );
    if ( !Handle )
        return 1;

    RECT Client = { };
    GetClientRect( Handle, &Client );
    ClientWidth = ( int )Client.right;
    ClientHeight = ( int )Client.bottom;

    Native->Create( Handle );
    if ( !Engine->Create( Active.fonts, Active.font_count, Active.font_size ) )
        return 1;

    Engine->Rescale( Native->Scale( ) );
    config::load( );
    PersistRead( );
    Docking->Enabled = Active.persist ? Docking->Enabled : Active.docking;

    if ( Active.overlay ) {
        overlay::attach( Handle );
        OverlayReady = true;
        overlay::apply( Handle, Overlay );
        RECT ClientAfter = { };
        GetClientRect( Handle, &ClientAfter );
        ClientWidth = ( int )ClientAfter.right;
        ClientHeight = ( int )ClientAfter.bottom;
    }

    TitleBar( Handle, true );
    ShowWindow( Handle, SW_SHOW );

    FillRoster( );

    Backend WantedBackend = Active.backend;
    if ( Active.persist && WantedBackend == Backend::Auto )
        WantedBackend = ( Backend )settings::get_int( "backend", ( int )Backend::Auto );

    if ( !PickHost( WantedBackend ) )
        return 1;

    if ( Active.media )
        media::start( );
    if ( Active.hear )
        hear::start( );
    effects::compose( );

    if ( Active.discord && config::get( "UR_DISCORD_APP_ID" )[ 0 ] ) {
        discord::set_app_id( config::get( "UR_DISCORD_APP_ID" ) );
        discord::enable( true );
    }

    Context->Alarm = [ ]( const char* Message ) {
        toast::push( Message, Style->Warning );
    };

    std::function< void( ) > Tick = [ &User ]( ) {
        TickFrame( User );
    };

    Ticker = &Tick;
    while ( !Quit ) {
        MSG Message;
        while ( PeekMessageW( &Message, nullptr, 0, 0, PM_REMOVE ) ) {
            TranslateMessage( &Message );
            DispatchMessageW( &Message );
        }
        if ( Quit )
            break;
        Tick( );
    }

    Ticker = nullptr;
    PersistWrite( );

    if ( Active.discord )
        discord::shutdown( );
    if ( Active.hear )
        hear::shutdown( );
    if ( Active.media )
        media::shutdown( );

    glyphs::sweep( );
    image::sweep( );
    palette::clear( );
    toast::clear( );

    Roster[ ( size_t )Running ]->Destroy( );
    Engine->Destroy( );
    Native->Destroy( );
    DestroyWindow( Handle );
    UnregisterClassW( L"UR", Description.hInstance );
    Handle = nullptr;
    timeEndPeriod( 1 );
    return 0;
}

}
}
