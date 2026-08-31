#if defined( _WIN32 )

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#elif defined( __APPLE__ )

#include <cstring>

#include <objc/message.h>
#include <objc/runtime.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <TargetConditionals.h>

#endif

#include "Input.h"
#include "Native.h"
#include "Context.h"

#if defined( _WIN32 )
#pragma comment( lib, "user32.lib" )
#endif

bool CNative::Create( void* Window ) {
    Handle = Window;
    Tracking = false;

    return Window != nullptr;
}

void CNative::Destroy( ) {
    Handle = nullptr;
    Tracking = false;
}

#if defined( _WIN32 )

float CNative::Scale( ) const {
    if ( !Handle )
        return 1.0f;

    return ( float )GetDpiForWindow( ( HWND )Handle ) / 96.0f;
}

static void ClaimCapture( HWND Origin ) {
    if ( GetCapture( ) != Origin )
        SetCapture( Origin );
}

static void SettleCapture( ) {
    if ( !Input->MouseDown( 0 ) && !Input->MouseDown( 1 ) && !Input->MouseDown( 2 ) )
        ReleaseCapture( );
}

bool CNative::Translate( void* Window, unsigned int Message, unsigned long long Primary, long long Secondary ) {
    HWND Origin = ( HWND )Window;

    switch ( Message ) {
    case WM_MOUSEMOVE:
        if ( !Tracking ) {
            TRACKMOUSEEVENT Tracker = { };

            Tracker.cbSize = sizeof( TRACKMOUSEEVENT );
            Tracker.dwFlags = TME_LEAVE;
            Tracker.hwndTrack = Origin;

            if ( TrackMouseEvent( &Tracker ) )
                Tracking = true;
        }

        Input->ApplyPosition( ( float )( int )( short )LOWORD( Secondary ), ( float )( int )( short )HIWORD( Secondary ) );
        return true;

    case WM_MOUSELEAVE:
        Tracking = false;

        if ( !Input->MouseDown( 0 ) && !Input->MouseDown( 1 ) && !Input->MouseDown( 2 ) )
            Input->ApplyPosition( -10000.0f, -10000.0f );

        return true;

    case WM_LBUTTONDOWN:
        ClaimCapture( Origin );
        Input->ApplyButton( 0, true );

        return true;

    case WM_LBUTTONDBLCLK:
        ClaimCapture( Origin );

        Input->ApplyButton( 0, true );
        Input->ApplyDouble( 0 );

        return true;

    case WM_LBUTTONUP:
        Input->ApplyButton( 0, false );
        SettleCapture( );

        return true;

    case WM_RBUTTONDOWN:
        ClaimCapture( Origin );
        Input->ApplyButton( 1, true );

        return true;

    case WM_RBUTTONUP:
        Input->ApplyButton( 1, false );
        SettleCapture( );

        return true;

    case WM_MBUTTONDOWN:
        ClaimCapture( Origin );
        Input->ApplyButton( 2, true );

        return true;

    case WM_MBUTTONUP:
        Input->ApplyButton( 2, false );
        SettleCapture( );

        return true;

    case WM_CAPTURECHANGED:
        if ( ( HWND )Secondary != Origin )
            Input->ApplyCancel( );

        return true;

    case WM_MOUSEWHEEL:
        Input->ApplyWheel( ( float )( short )HIWORD( Primary ) / 120.0f );
        return true;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        Input->ApplyKey( ( int )Primary, true );
        return Context->WantKeyboard;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        Input->ApplyKey( ( int )Primary, false );
        return Context->WantKeyboard;

    case WM_CHAR:
    case WM_IME_CHAR:
        Input->ApplyText( ( unsigned int )Primary );
        return Context->WantKeyboard;

    case WM_SETCURSOR:
        if ( LOWORD( Secondary ) == HTCLIENT ) {
            static HCURSOR Shapes[ 7 ] = { };

            if ( !Shapes[ 0 ] ) {
                const char* Names[ 7 ] = { ( const char* )IDC_ARROW, ( const char* )IDC_SIZEWE, ( const char* )IDC_SIZENS, ( const char* )IDC_SIZENWSE, ( const char* )IDC_SIZEALL, ( const char* )IDC_IBEAM, ( const char* )IDC_HAND };

                for ( int Shape = 0; Shape < 7; Shape++ )
                    Shapes[ Shape ] = LoadCursorA( nullptr, Names[ Shape ] );
            }

            int Pointer = Input->Pointer;
            if ( Pointer < 0 || Pointer > 6 )
                Pointer = 0;

            SetCursor( Shapes[ Pointer ] );
            return true;
        }

        return false;
    }

    return false;
}

std::string CNative::ClipboardGet( ) const {
    if ( !Handle || !OpenClipboard( ( HWND )Handle ) )
        return std::string( );

    std::string Result;
    HANDLE Data = GetClipboardData( CF_UNICODETEXT );

    if ( Data ) {
        const wchar_t* Wide = ( const wchar_t* )GlobalLock( Data );

        if ( Wide ) {
            int Length = WideCharToMultiByte( CP_UTF8, 0, Wide, -1, nullptr, 0, nullptr, nullptr );

            if ( Length > 1 ) {
                Result.resize( ( size_t )Length - 1 );
                WideCharToMultiByte( CP_UTF8, 0, Wide, -1, Result.data( ), Length, nullptr, nullptr );
            }

            GlobalUnlock( Data );
        }
    }

    CloseClipboard( );
    return Result;
}

void CNative::ClipboardSet( const char* Text ) const {
    if ( !Handle || !Text || !OpenClipboard( ( HWND )Handle ) )
        return;

    EmptyClipboard( );

    int Length = MultiByteToWideChar( CP_UTF8, 0, Text, -1, nullptr, 0 );

    if ( Length > 0 ) {
        HGLOBAL Block = GlobalAlloc( GMEM_MOVEABLE, ( size_t )Length * sizeof( wchar_t ) );

        if ( Block ) {
            wchar_t* Wide = ( wchar_t* )GlobalLock( Block );

            if ( Wide ) {
                MultiByteToWideChar( CP_UTF8, 0, Text, -1, Wide, Length );
                GlobalUnlock( Block );

                if ( !SetClipboardData( CF_UNICODETEXT, Block ) )
                    GlobalFree( Block );
            } else {
                GlobalFree( Block );
            }
        }
    }

    CloseClipboard( );
}

#elif defined( __APPLE__ )

static void* Shared( const char* Owner, const char* Message ) {
    void* Target = ( void* )objc_getClass( Owner );

    if ( !Target )
        return nullptr;

    typedef void* ( *CPlain )( void*, SEL );
    return ( ( CPlain )objc_msgSend )( Target, sel_registerName( Message ) );
}

static std::string Gather( CFStringRef Text ) {
    if ( !Text )
        return std::string( );

    CFIndex Length = CFStringGetLength( Text );
    if ( Length <= 0 )
        return std::string( );

    CFIndex Room = CFStringGetMaximumSizeForEncoding( Length, kCFStringEncodingUTF8 ) + 1;
    if ( Room <= 1 )
        return std::string( );

    std::string Result;
    Result.resize( ( size_t )Room );

    if ( !CFStringGetCString( Text, Result.data( ), Room, kCFStringEncodingUTF8 ) )
        return std::string( );

    Result.resize( strlen( Result.c_str( ) ) );
    return Result;
}

float CNative::Scale( ) const {
    if ( !Handle )
        return 1.0f;

#if TARGET_OS_IPHONE
    void* Screen = Shared( "UIScreen", "mainScreen" );

    if ( !Screen )
        return 1.0f;

    typedef double ( *CFactor )( void*, SEL );
    double Factor = ( ( CFactor )objc_msgSend )( Screen, sel_registerName( "scale" ) );

    if ( Factor <= 0.0 )
        return 1.0f;

    return ( float )Factor;
#else
    CGDisplayModeRef Mode = CGDisplayCopyDisplayMode( CGMainDisplayID( ) );

    if ( !Mode )
        return 1.0f;

    double Dots = ( double )CGDisplayModeGetPixelWidth( Mode );
    double Points = ( double )CGDisplayModeGetWidth( Mode );

    CGDisplayModeRelease( Mode );

    if ( Dots <= 0.0 || Points <= 0.0 )
        return 1.0f;

    return ( float )( Dots / Points );
#endif
}

bool CNative::Translate( void* Window, unsigned int Message, unsigned long long Primary, long long Secondary ) {
    ( void )Window;
    ( void )Message;

    ( void )Primary;
    ( void )Secondary;

    return false;
}

std::string CNative::ClipboardGet( ) const {
#if TARGET_OS_IPHONE
    void* Board = Shared( "UIPasteboard", "generalPasteboard" );

    if ( !Board )
        return std::string( );

    typedef void* ( *CPlain )( void*, SEL );
    CFStringRef Text = ( CFStringRef )( ( CPlain )objc_msgSend )( Board, sel_registerName( "string" ) );

    return Gather( Text );
#else
    void* Board = Shared( "NSPasteboard", "generalPasteboard" );

    if ( !Board )
        return std::string( );

    typedef void* ( *CTyped )( void*, SEL, CFStringRef );
    CFStringRef Text = ( CFStringRef )( ( CTyped )objc_msgSend )( Board, sel_registerName( "stringForType:" ), CFSTR( "public.utf8-plain-text" ) );

    return Gather( Text );
#endif
}

void CNative::ClipboardSet( const char* Text ) const {
    if ( !Text )
        return;

    CFStringRef Value = CFStringCreateWithCString( kCFAllocatorDefault, Text, kCFStringEncodingUTF8 );

    if ( !Value )
        return;

#if TARGET_OS_IPHONE
    void* Board = Shared( "UIPasteboard", "generalPasteboard" );

    if ( Board ) {
        typedef void ( *CWrite )( void*, SEL, CFStringRef );
        ( ( CWrite )objc_msgSend )( Board, sel_registerName( "setString:" ), Value );
    }
#else
    void* Board = Shared( "NSPasteboard", "generalPasteboard" );

    if ( Board ) {
        typedef long long ( *CClear )( void*, SEL );
        ( ( CClear )objc_msgSend )( Board, sel_registerName( "clearContents" ) );

        typedef bool ( *CWrite )( void*, SEL, CFStringRef, CFStringRef );
        ( ( CWrite )objc_msgSend )( Board, sel_registerName( "setString:forType:" ), Value, CFSTR( "public.utf8-plain-text" ) );
    }
#endif

    CFRelease( Value );
}

#endif
