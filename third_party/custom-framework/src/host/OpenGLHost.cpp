#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <gl/GL.h>

#include "OpenGL.h"
#include "OpenGLHost.h"

#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x0001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x0002
#endif

static const wchar_t* const GlClassName = L"UR.GL";

static LRESULT CALLBACK GlSurfaceProc( HWND Origin, UINT Message, WPARAM Primary, LPARAM Secondary ) {
    return DefWindowProcW( Origin, Message, Primary, Secondary );
}

static void RegisterSurface( HINSTANCE Instance ) {
    static bool Ready = false;
    if ( Ready )
        return;

    WNDCLASSEXW Description = { };
    Description.cbSize = sizeof( WNDCLASSEXW );
    Description.style = CS_OWNDC | CS_DBLCLKS;
    Description.lpfnWndProc = GlSurfaceProc;
    Description.hInstance = Instance;
    Description.lpszClassName = GlClassName;
    RegisterClassExW( &Description );
    Ready = true;
}

static HGLRC MakeContext( HDC Paper, HGLRC Legacy ) {
    HGLRC( WINAPI * CreateModern )( HDC, HGLRC, const int* ) = ( HGLRC( WINAPI* )( HDC, HGLRC, const int* ) )wglGetProcAddress( "wglCreateContextAttribsARB" );
    if ( !CreateModern )
        return nullptr;

    const int Profiles[ 2 ] = { WGL_CONTEXT_CORE_PROFILE_BIT_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB };
    for ( int Profile : Profiles ) {
        int Wishes[ 7 ] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, Profile,
            0
        };
        HGLRC Modern = CreateModern( Paper, nullptr, Wishes );
        if ( Modern )
            return Modern;
    }

    ( void )Legacy;
    return nullptr;
}

bool COpenGLHost::Create( void* Window, int Width, int Height ) {
    Destroy( );

    if ( !Window || Width <= 0 || Height <= 0 )
        return false;

    HWND Parent = ( HWND )Window;
    Owner = Parent;
    LONG_PTR ParentStyle = GetWindowLongPtrW( Parent, GWL_STYLE );
    SetWindowLongPtrW( Parent, GWL_STYLE, ParentStyle | WS_CLIPCHILDREN );
    Clipped = true;

    RegisterSurface( ( HINSTANCE )GetWindowLongPtrW( Parent, GWLP_HINSTANCE ) );

    HWND Child = CreateWindowExW( WS_EX_TRANSPARENT, GlClassName, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0, Width, Height, Parent, nullptr, ( HINSTANCE )GetWindowLongPtrW( Parent, GWLP_HINSTANCE ), nullptr );
    if ( !Child ) {
        Destroy( );
        return false;
    }

    Handle = Child;
    Surface = GetDC( Child );
    if ( !Surface ) {
        Destroy( );
        return false;
    }

    HDC Paper = ( HDC )Surface;
    PIXELFORMATDESCRIPTOR Recipe = { };
    Recipe.nSize = sizeof( PIXELFORMATDESCRIPTOR );
    Recipe.nVersion = 1;
    Recipe.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    Recipe.iPixelType = PFD_TYPE_RGBA;
    Recipe.cColorBits = 32;
    Recipe.cAlphaBits = 8;
    Recipe.cDepthBits = 24;
    Recipe.cStencilBits = 8;
    Recipe.iLayerType = PFD_MAIN_PLANE;

    int Chosen = ChoosePixelFormat( Paper, &Recipe );
    if ( !Chosen || !SetPixelFormat( Paper, Chosen, &Recipe ) ) {
        Destroy( );
        return false;
    }

    HGLRC Legacy = wglCreateContext( Paper );
    if ( !Legacy || !wglMakeCurrent( Paper, Legacy ) ) {
        if ( Legacy )
            wglDeleteContext( Legacy );
        Destroy( );
        return false;
    }

    HGLRC Modern = MakeContext( Paper, Legacy );
    if ( !Modern ) {
        wglMakeCurrent( nullptr, nullptr );
        wglDeleteContext( Legacy );
        MessageBoxW( Parent, L"OpenGL 4.3 is required for this backend.", L"UR", MB_OK | MB_ICONWARNING );
        Destroy( );
        return false;
    }

    if ( !wglMakeCurrent( Paper, Modern ) ) {
        wglMakeCurrent( nullptr, nullptr );
        wglDeleteContext( Modern );
        wglDeleteContext( Legacy );
        Destroy( );
        return false;
    }

    wglDeleteContext( Legacy );
    Setting = Modern;

    SurfaceWidth = Width;
    SurfaceHeight = Height;
    Interval = -1;

    if ( !OpenGL->Create( ) ) {
        Destroy( );
        return false;
    }

    return true;
}

void COpenGLHost::Destroy( ) {
    if ( Setting && Surface )
        wglMakeCurrent( ( HDC )Surface, ( HGLRC )Setting );

    OpenGL->Destroy( );

    if ( Setting ) {
        wglMakeCurrent( nullptr, nullptr );
        wglDeleteContext( ( HGLRC )Setting );
        Setting = nullptr;
    }

    if ( Surface && Handle )
        ReleaseDC( ( HWND )Handle, ( HDC )Surface );

    Surface = nullptr;

    if ( Handle ) {
        DestroyWindow( ( HWND )Handle );
        Handle = nullptr;
    }

    if ( Owner && Clipped ) {
        LONG_PTR ParentStyle = GetWindowLongPtrW( ( HWND )Owner, GWL_STYLE );
        SetWindowLongPtrW( ( HWND )Owner, GWL_STYLE, ParentStyle & ~WS_CLIPCHILDREN );
        Clipped = false;
    }

    Owner = nullptr;
    SurfaceWidth = 0;
    SurfaceHeight = 0;
    Interval = -1;
}

void COpenGLHost::Resize( int Width, int Height ) {
    if ( Width <= 0 || Height <= 0 )
        return;

    SurfaceWidth = Width;
    SurfaceHeight = Height;

    if ( Handle )
        SetWindowPos( ( HWND )Handle, nullptr, 0, 0, Width, Height, SWP_NOZORDER | SWP_NOACTIVATE );
}

void COpenGLHost::Begin( CColor Backdrop ) {
    if ( !Setting || !Surface )
        return;

    wglMakeCurrent( ( HDC )Surface, ( HGLRC )Setting );
    glViewport( 0, 0, SurfaceWidth, SurfaceHeight );
    glClearColor( Backdrop.Red / 255.0f, Backdrop.Green / 255.0f, Backdrop.Blue / 255.0f, Backdrop.Alpha / 255.0f );
    glClear( GL_COLOR_BUFFER_BIT );
}

void* COpenGLHost::Stream( ) {
    return nullptr;
}

CGraphics* COpenGLHost::Graphics( ) {
    return OpenGL.get( );
}

void COpenGLHost::End( bool VerticalSync ) {
    if ( !Setting || !Surface )
        return;

    wglMakeCurrent( ( HDC )Surface, ( HGLRC )Setting );

    int Wanted = VerticalSync ? 1 : 0;
    if ( Wanted != Interval ) {
        BOOL( WINAPI * SwapPace )( int ) = ( BOOL( WINAPI* )( int ) )wglGetProcAddress( "wglSwapIntervalEXT" );
        if ( SwapPace )
            SwapPace( Wanted );
        Interval = Wanted;
    }

    SwapBuffers( ( HDC )Surface );
}
