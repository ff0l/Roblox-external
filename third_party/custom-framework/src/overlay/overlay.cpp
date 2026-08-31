#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "ur/overlay.hpp"

#include <Windows.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace ur {
namespace overlay {

static Options Last{ };
static HWND LastHandle = nullptr;
static bool HaveLast = false;

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
        Extra |= WS_EX_TOOLWINDOW;
    }

    if ( Options.layered || Options.transparent || Options.click_through || Options.alpha < 255 )
        Extra |= WS_EX_LAYERED;
    else
        Extra &= ~WS_EX_LAYERED;

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
        MARGINS Glass = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea( Handle, &Glass );
        if ( Extra & WS_EX_LAYERED )
            SetLayeredWindowAttributes( Handle, 0, ( BYTE )Alpha, LWA_ALPHA );
    } else if ( Extra & WS_EX_LAYERED ) {
        SetLayeredWindowAttributes( Handle, 0, ( BYTE )Alpha, LWA_ALPHA );
    }

    UINT Place = SWP_NOACTIVATE | SWP_FRAMECHANGED;
    int Left = 0;
    int Top = 0;
    int Width = 0;
    int Height = 0;
    if ( Options.borderless ) {
        Left = 0;
        Top = 0;
        Width = GetSystemMetrics( SM_CXSCREEN );
        Height = GetSystemMetrics( SM_CYSCREEN );
    } else {
        Place |= SWP_NOMOVE | SWP_NOSIZE;
    }

    SetWindowPos( Handle, Options.topmost ? HWND_TOPMOST : HWND_NOTOPMOST, Left, Top, Width, Height, Place );
}

bool glass( ) {
    return HaveLast && Last.transparent;
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
