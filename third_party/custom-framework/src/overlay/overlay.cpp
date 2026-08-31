#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "ur/overlay.hpp"

#include <Windows.h>
#include <dwmapi.h>
#include <wingdi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef WS_EX_NOREDIRECTIONBITMAP
#define WS_EX_NOREDIRECTIONBITMAP 0x00200000L
#endif

namespace ur {
namespace overlay {

static Options Last{ };
static HWND LastHandle = nullptr;
static bool HaveLast = false;

static void CoverPrimary( int& Left, int& Top, int& Width, int& Height ) {
    HMONITOR Monitor = MonitorFromPoint( POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY );
    MONITORINFO Info = { sizeof( Info ) };
    if ( Monitor && GetMonitorInfo( Monitor, &Info ) ) {
        Left = Info.rcMonitor.left;
        Top = Info.rcMonitor.top;
        Width = Info.rcMonitor.right - Info.rcMonitor.left;
        Height = Info.rcMonitor.bottom - Info.rcMonitor.top;
        return;
    }
    Left = 0;
    Top = 0;
    Width = GetSystemMetrics( SM_CXSCREEN );
    Height = GetSystemMetrics( SM_CYSCREEN );
}

static void GlassFrame( HWND Handle ) {
    MARGINS Glass = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea( Handle, &Glass );
}

void apply( void* Window, const Options& Options ) {
    HWND Handle = ( HWND )Window;
    if ( !Handle )
        return;

    if ( HaveLast && LastHandle == Handle
        && Last.topmost == Options.topmost
        && Last.click_through == Options.click_through
        && Last.layered == Options.layered
        && Last.borderless == Options.borderless
        && Last.transparent == Options.transparent
        && Last.alpha == Options.alpha )
        return;

    if ( HaveLast && LastHandle == Handle
        && Last.topmost == Options.topmost
        && Last.layered == Options.layered
        && Last.borderless == Options.borderless
        && Last.transparent == Options.transparent
        && Last.alpha == Options.alpha
        && Last.click_through != Options.click_through ) {
        Last.click_through = Options.click_through;
        LONG Extra = GetWindowLongW( Handle, GWL_EXSTYLE );
        if ( Options.click_through )
            Extra |= WS_EX_TRANSPARENT;
        else
            Extra &= ~WS_EX_TRANSPARENT;
        SetWindowLongW( Handle, GWL_EXSTYLE, Extra );
        return;
    }

    Last = Options;
    LastHandle = Handle;
    HaveLast = true;

    LONG Style = GetWindowLongW( Handle, GWL_STYLE );
    LONG Extra = GetWindowLongW( Handle, GWL_EXSTYLE );

    if ( Options.borderless ) {
        Style &= ~( WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU );
        Style |= WS_POPUP;
        Extra |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    }

    if ( Options.transparent ) {
        Extra &= ~WS_EX_LAYERED;
        Extra |= WS_EX_NOREDIRECTIONBITMAP;
    } else if ( Options.layered || Options.click_through || Options.alpha < 255 ) {
        Extra |= WS_EX_LAYERED;
        Extra &= ~WS_EX_NOREDIRECTIONBITMAP;
    } else {
        Extra &= ~WS_EX_LAYERED;
        Extra &= ~WS_EX_NOREDIRECTIONBITMAP;
    }

    if ( Options.click_through )
        Extra |= WS_EX_TRANSPARENT;
    else
        Extra &= ~WS_EX_TRANSPARENT;

    SetWindowLongW( Handle, GWL_STYLE, Style );
    SetWindowLongW( Handle, GWL_EXSTYLE, Extra );

    int Alpha = Options.alpha;
    if ( Alpha < 0 )
        Alpha = 0;
    if ( Alpha > 255 )
        Alpha = 255;

    if ( Options.transparent ) {
        GlassFrame( Handle );
    } else if ( Extra & WS_EX_LAYERED ) {
        SetLayeredWindowAttributes( Handle, 0, ( BYTE )Alpha, LWA_ALPHA );
    }

    UINT Place = SWP_NOACTIVATE | SWP_FRAMECHANGED;
    int Left = 0;
    int Top = 0;
    int Width = 0;
    int Height = 0;
    if ( Options.borderless ) {
        CoverPrimary( Left, Top, Width, Height );
    } else {
        Place |= SWP_NOMOVE | SWP_NOSIZE;
    }

    SetWindowPos( Handle, Options.topmost ? HWND_TOPMOST : HWND_NOTOPMOST, Left, Top, Width, Height, Place );
}

void seal( void* Window ) {
    HWND Handle = ( HWND )Window;
    if ( !Handle || !Last.transparent )
        return;
    GlassFrame( Handle );
}

bool glass( ) {
    return HaveLast && Last.transparent;
}

void primary_monitor( int& left, int& top, int& width, int& height ) {
    CoverPrimary( left, top, width, height );
}

void attach( void* Window ) {
    HWND Handle = ( HWND )Window;
    if ( !Handle )
        return;

    HaveLast = false;
    LastHandle = nullptr;

    BOOL Dark = TRUE;
    if ( FAILED( DwmSetWindowAttribute( Handle, DWMWA_USE_IMMERSIVE_DARK_MODE, &Dark, sizeof( Dark ) ) ) )
        DwmSetWindowAttribute( Handle, 19, &Dark, sizeof( Dark ) );
}

}
}
