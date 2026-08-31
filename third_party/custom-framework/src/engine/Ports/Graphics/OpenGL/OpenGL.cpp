#if defined( _WIN32 )

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <gl/GL.h>

#pragma comment( lib, "opengl32.lib" )

#define UR_GL_CALL __stdcall

#elif defined( __APPLE__ )

#include <OpenGL/gl3.h>

#define UR_GL_CALL

#endif

#include <cstddef>
#include <cstring>

#include "Sheet.h"
#include "OpenGL.h"
#include "Context.h"
#include "Shaders.h"

#if defined( _WIN32 )

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#endif

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_TEXTURE0 0x84C0
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#define GL_SHADER_STORAGE_BUFFER_BINDING 0x90D3
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_R8 0x8229
#define GL_FUNC_ADD 0x8006
#define GL_BLEND_SRC_RGB 0x80C9
#define GL_BLEND_DST_RGB 0x80C8
#define GL_BLEND_SRC_ALPHA 0x80CB
#define GL_BLEND_DST_ALPHA 0x80CA

static GLuint( UR_GL_CALL* UrCreateShader )( GLenum ) = nullptr;
static void( UR_GL_CALL* UrShaderSource )( GLuint, GLsizei, const GLchar* const*, const GLint* ) = nullptr;
static void( UR_GL_CALL* UrCompileShader )( GLuint ) = nullptr;
static void( UR_GL_CALL* UrGetShaderiv )( GLuint, GLenum, GLint* ) = nullptr;
static void( UR_GL_CALL* UrGetShaderInfoLog )( GLuint, GLsizei, GLsizei*, GLchar* ) = nullptr;
static GLuint( UR_GL_CALL* UrCreateProgram )( void ) = nullptr;
static void( UR_GL_CALL* UrAttachShader )( GLuint, GLuint ) = nullptr;
static void( UR_GL_CALL* UrLinkProgram )( GLuint ) = nullptr;
static void( UR_GL_CALL* UrGetProgramiv )( GLuint, GLenum, GLint* ) = nullptr;
static void( UR_GL_CALL* UrGetProgramInfoLog )( GLuint, GLsizei, GLsizei*, GLchar* ) = nullptr;
static void( UR_GL_CALL* UrDeleteShader )( GLuint ) = nullptr;
static void( UR_GL_CALL* UrDeleteProgram )( GLuint ) = nullptr;
static void( UR_GL_CALL* UrUseProgram )( GLuint ) = nullptr;
static GLint( UR_GL_CALL* UrGetUniformLocation )( GLuint, const GLchar* ) = nullptr;
static void( UR_GL_CALL* UrUniform4f )( GLint, GLfloat, GLfloat, GLfloat, GLfloat ) = nullptr;
static void( UR_GL_CALL* UrUniform1ui )( GLint, GLuint ) = nullptr;
static void( UR_GL_CALL* UrUniform1i )( GLint, GLint ) = nullptr;
static void( UR_GL_CALL* UrUniform1f )( GLint, GLfloat ) = nullptr;
static void( UR_GL_CALL* UrGenVertexArrays )( GLsizei, GLuint* ) = nullptr;
static void( UR_GL_CALL* UrBindVertexArray )( GLuint ) = nullptr;
static void( UR_GL_CALL* UrDeleteVertexArrays )( GLsizei, const GLuint* ) = nullptr;
static void( UR_GL_CALL* UrGenBuffers )( GLsizei, GLuint* ) = nullptr;
static void( UR_GL_CALL* UrBindBuffer )( GLenum, GLuint ) = nullptr;
static void( UR_GL_CALL* UrBufferData )( GLenum, GLsizeiptr, const void*, GLenum ) = nullptr;
static void( UR_GL_CALL* UrBufferSubData )( GLenum, GLintptr, GLsizeiptr, const void* ) = nullptr;
static void( UR_GL_CALL* UrDeleteBuffers )( GLsizei, const GLuint* ) = nullptr;
static void( UR_GL_CALL* UrBindBufferBase )( GLenum, GLuint, GLuint ) = nullptr;
static void( UR_GL_CALL* UrDrawArraysInstanced )( GLenum, GLint, GLsizei, GLsizei ) = nullptr;
static void( UR_GL_CALL* UrActiveTexture )( GLenum ) = nullptr;
static void( UR_GL_CALL* UrBlendFuncSeparate )( GLenum, GLenum, GLenum, GLenum ) = nullptr;
static void( UR_GL_CALL* UrBlendEquation )( GLenum ) = nullptr;

#if defined( _WIN32 )

static void* Resolve( const char* Name ) {
    void* Found = ( void* )wglGetProcAddress( Name );
    if ( Found )
        return Found;

    HMODULE Module = GetModuleHandleA( "opengl32.dll" );
    if ( !Module )
        return nullptr;

    return ( void* )GetProcAddress( Module, Name );
}

#define UR_GL_RESOLVE( Name ) Resolve( #Name )

#else

#define UR_GL_RESOLVE( Name ) ( void* )&Name

#endif

static bool Gather( ) {
    UrCreateShader = ( GLuint( UR_GL_CALL* )( GLenum ) )UR_GL_RESOLVE( glCreateShader );
    UrShaderSource = ( void( UR_GL_CALL* )( GLuint, GLsizei, const GLchar* const*, const GLint* ) )UR_GL_RESOLVE( glShaderSource );
    UrCompileShader = ( void( UR_GL_CALL* )( GLuint ) )UR_GL_RESOLVE( glCompileShader );
    UrGetShaderiv = ( void( UR_GL_CALL* )( GLuint, GLenum, GLint* ) )UR_GL_RESOLVE( glGetShaderiv );
    UrGetShaderInfoLog = ( void( UR_GL_CALL* )( GLuint, GLsizei, GLsizei*, GLchar* ) )UR_GL_RESOLVE( glGetShaderInfoLog );
    UrCreateProgram = ( GLuint( UR_GL_CALL* )( void ) )UR_GL_RESOLVE( glCreateProgram );
    UrAttachShader = ( void( UR_GL_CALL* )( GLuint, GLuint ) )UR_GL_RESOLVE( glAttachShader );
    UrLinkProgram = ( void( UR_GL_CALL* )( GLuint ) )UR_GL_RESOLVE( glLinkProgram );
    UrGetProgramiv = ( void( UR_GL_CALL* )( GLuint, GLenum, GLint* ) )UR_GL_RESOLVE( glGetProgramiv );
    UrGetProgramInfoLog = ( void( UR_GL_CALL* )( GLuint, GLsizei, GLsizei*, GLchar* ) )UR_GL_RESOLVE( glGetProgramInfoLog );
    UrDeleteShader = ( void( UR_GL_CALL* )( GLuint ) )UR_GL_RESOLVE( glDeleteShader );
    UrDeleteProgram = ( void( UR_GL_CALL* )( GLuint ) )UR_GL_RESOLVE( glDeleteProgram );
    UrUseProgram = ( void( UR_GL_CALL* )( GLuint ) )UR_GL_RESOLVE( glUseProgram );
    UrGetUniformLocation = ( GLint( UR_GL_CALL* )( GLuint, const GLchar* ) )UR_GL_RESOLVE( glGetUniformLocation );
    UrUniform4f = ( void( UR_GL_CALL* )( GLint, GLfloat, GLfloat, GLfloat, GLfloat ) )UR_GL_RESOLVE( glUniform4f );
    UrUniform1ui = ( void( UR_GL_CALL* )( GLint, GLuint ) )UR_GL_RESOLVE( glUniform1ui );
    UrUniform1i = ( void( UR_GL_CALL* )( GLint, GLint ) )UR_GL_RESOLVE( glUniform1i );
    UrUniform1f = ( void( UR_GL_CALL* )( GLint, GLfloat ) )UR_GL_RESOLVE( glUniform1f );
    UrGenVertexArrays = ( void( UR_GL_CALL* )( GLsizei, GLuint* ) )UR_GL_RESOLVE( glGenVertexArrays );
    UrBindVertexArray = ( void( UR_GL_CALL* )( GLuint ) )UR_GL_RESOLVE( glBindVertexArray );
    UrDeleteVertexArrays = ( void( UR_GL_CALL* )( GLsizei, const GLuint* ) )UR_GL_RESOLVE( glDeleteVertexArrays );
    UrGenBuffers = ( void( UR_GL_CALL* )( GLsizei, GLuint* ) )UR_GL_RESOLVE( glGenBuffers );
    UrBindBuffer = ( void( UR_GL_CALL* )( GLenum, GLuint ) )UR_GL_RESOLVE( glBindBuffer );
    UrBufferData = ( void( UR_GL_CALL* )( GLenum, GLsizeiptr, const void*, GLenum ) )UR_GL_RESOLVE( glBufferData );
    UrBufferSubData = ( void( UR_GL_CALL* )( GLenum, GLintptr, GLsizeiptr, const void* ) )UR_GL_RESOLVE( glBufferSubData );
    UrDeleteBuffers = ( void( UR_GL_CALL* )( GLsizei, const GLuint* ) )UR_GL_RESOLVE( glDeleteBuffers );
    UrBindBufferBase = ( void( UR_GL_CALL* )( GLenum, GLuint, GLuint ) )UR_GL_RESOLVE( glBindBufferBase );
    UrDrawArraysInstanced = ( void( UR_GL_CALL* )( GLenum, GLint, GLsizei, GLsizei ) )UR_GL_RESOLVE( glDrawArraysInstanced );
    UrActiveTexture = ( void( UR_GL_CALL* )( GLenum ) )UR_GL_RESOLVE( glActiveTexture );
    UrBlendFuncSeparate = ( void( UR_GL_CALL* )( GLenum, GLenum, GLenum, GLenum ) )UR_GL_RESOLVE( glBlendFuncSeparate );
    UrBlendEquation = ( void( UR_GL_CALL* )( GLenum ) )UR_GL_RESOLVE( glBlendEquation );

    return UrCreateShader && UrShaderSource && UrCompileShader && UrGetShaderiv && UrCreateProgram && UrAttachShader && UrLinkProgram && UrGetProgramiv && UrDeleteShader && UrDeleteProgram && UrUseProgram && UrGetUniformLocation && UrUniform4f && UrUniform1ui && UrUniform1i && UrUniform1f && UrGenVertexArrays && UrBindVertexArray && UrDeleteVertexArrays && UrGenBuffers && UrBindBuffer && UrBufferData && UrBufferSubData && UrDeleteBuffers && UrBindBufferBase && UrDrawArraysInstanced && UrActiveTexture && UrBlendFuncSeparate && UrBlendEquation;
}

static unsigned int BuildStage( GLenum Kind, const std::string& Source ) {
    GLuint Stage = UrCreateShader( Kind );
    const char* Text = Source.c_str( );

    UrShaderSource( Stage, 1, &Text, nullptr );
    UrCompileShader( Stage );

    GLint Status = 0;
    UrGetShaderiv( Stage, GL_COMPILE_STATUS, &Status );

    if ( !Status ) {
        char Report[ 1024 ] = { };
        UrGetShaderInfoLog( Stage, 1024, nullptr, Report );

        Context->Report( Report );
        UrDeleteShader( Stage );

        return 0;
    }

    return Stage;
}

COpenGL::~COpenGL( ) {
    Destroy( );
}

const COpenSlots* COpenGL::Program( unsigned int Effect ) {
    size_t Wanted = ( size_t )Effect + 1;

    if ( Programs.size( ) < Wanted )
        Programs.resize( Wanted );

    if ( Programs[ Effect ].Program )
        return &Programs[ Effect ];

    unsigned int VertexStage = BuildStage( GL_VERTEX_SHADER, Shaders->Vertex( ShaderGlsl ) );
    unsigned int FragmentStage = BuildStage( GL_FRAGMENT_SHADER, Shaders->Pixel( Effect, ShaderGlsl ) );

    if ( !VertexStage || !FragmentStage ) {
        if ( VertexStage )
            UrDeleteShader( VertexStage );

        if ( FragmentStage )
            UrDeleteShader( FragmentStage );

        return Effect == 0 ? nullptr : Program( 0 );
    }

    GLuint Handle = UrCreateProgram( );

    UrAttachShader( Handle, VertexStage );
    UrAttachShader( Handle, FragmentStage );

    UrLinkProgram( Handle );

    UrDeleteShader( VertexStage );
    UrDeleteShader( FragmentStage );

    GLint Status = 0;
    UrGetProgramiv( Handle, GL_LINK_STATUS, &Status );

    if ( !Status ) {
        char Report[ 1024 ] = { };
        UrGetProgramInfoLog( Handle, 1024, nullptr, Report );

        Context->Report( Report );
        UrDeleteProgram( Handle );

        return Effect == 0 ? nullptr : Program( 0 );
    }

    COpenSlots& Slots = Programs[ Effect ];

    Slots.Program = Handle;
    Slots.Projection = UrGetUniformLocation( Handle, "Projection" );

    Slots.Origin = UrGetUniformLocation( Handle, "Origin" );
    Slots.Moment = UrGetUniformLocation( Handle, "Moment" );

    GLint SheetSlot = UrGetUniformLocation( Handle, "Sheet" );
    GLint Former = 0;

    glGetIntegerv( GL_CURRENT_PROGRAM, &Former );

    UrUseProgram( Handle );
    UrUniform1i( SheetSlot, 0 );

    UrUseProgram( ( GLuint )Former );
    return &Slots;
}

bool COpenGL::Create( ) {
    Destroy( );

    if ( !Gather( ) ) {
        Context->Report( "Could not load the OpenGL functions" );
        return false;
    }

    if ( !Program( 0 ) )
        return false;

    UrGenVertexArrays( 1, &Vertices );
    UrGenBuffers( 1, &Storage );

    return Vertices != 0 && Storage != 0;
}

void COpenGL::Destroy( ) {
    for ( auto& Entry : Images ) {
        if ( Entry.second.Owned && Entry.second.Texture )
            glDeleteTextures( 1, &Entry.second.Texture );
    }

    Images.clear( );

    if ( SheetTexture ) {
        glDeleteTextures( 1, &SheetTexture );
        SheetTexture = 0;
    }

    if ( Storage && UrDeleteBuffers ) {
        UrDeleteBuffers( 1, &Storage );
        Storage = 0;
    }

    if ( Vertices && UrDeleteVertexArrays ) {
        UrDeleteVertexArrays( 1, &Vertices );
        Vertices = 0;
    }

    for ( COpenSlots& Slots : Programs ) {
        if ( Slots.Program && UrDeleteProgram )
            UrDeleteProgram( Slots.Program );
    }

    Programs.clear( );

    SheetVersion = 0;
    SheetDepth = 0;

    Capacity = 0;
    NextImage = 1;
}

void COpenGL::Refresh( ) {
    if ( SheetVersion == Sheet->Version && SheetTexture && SheetDepth == Sheet->Depth( ) )
        return;

    if ( Sheet->Depth( ) <= 0 )
        return;

    if ( !SheetTexture )
        glGenTextures( 1, &SheetTexture );

    glBindTexture( GL_TEXTURE_2D, SheetTexture );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, Sheet->Extent( ), Sheet->Depth( ), 0, GL_RED, GL_UNSIGNED_BYTE, Sheet->Data( ) );

    SheetVersion = Sheet->Version;
    SheetDepth = Sheet->Depth( );
}

unsigned long long COpenGL::CreateImage( const unsigned char* Pixels, int Width, int Height ) {
    if ( Programs.empty( ) || !Pixels || Width <= 0 || Height <= 0 )
        return 0;

    if ( Width > 16384 || Height > 16384 ) {
        Context->Report( "Rejected an image with unreasonable dimensions" );
        return 0;
    }

    COpenImage Image;

    Image.Width = Width;
    Image.Height = Height;

    Image.Owned = true;

    GLint Former = 0;
    glGetIntegerv( GL_TEXTURE_BINDING_2D, &Former );

    glGenTextures( 1, &Image.Texture );
    glBindTexture( GL_TEXTURE_2D, Image.Texture );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Pixels );

    glBindTexture( GL_TEXTURE_2D, ( GLuint )Former );

    unsigned long long Handle = NextImage;
    NextImage++;

    Images[ Handle ] = Image;
    return Handle;
}

void COpenGL::UpdateImage( unsigned long long Image, const unsigned char* Pixels ) {
    auto Entry = Images.find( Image );
    if ( Entry == Images.end( ) || !Entry->second.Owned || !Pixels )
        return;

    GLint Former = 0;
    glGetIntegerv( GL_TEXTURE_BINDING_2D, &Former );

    glBindTexture( GL_TEXTURE_2D, Entry->second.Texture );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, Entry->second.Width, Entry->second.Height, GL_RGBA, GL_UNSIGNED_BYTE, Pixels );
    glBindTexture( GL_TEXTURE_2D, ( GLuint )Former );
}

unsigned long long COpenGL::AdoptImage( void* Native ) {
    COpenImage Image;

    Image.Texture = ( unsigned int )( size_t )Native;
    Image.Owned = false;

    if ( Image.Texture == 0 )
        return 0;

    unsigned long long Handle = NextImage;
    NextImage++;

    Images[ Handle ] = Image;
    return Handle;
}

void COpenGL::DestroyImage( unsigned long long Image ) {
    auto Entry = Images.find( Image );
    if ( Entry == Images.end( ) )
        return;

    if ( Entry->second.Owned && Entry->second.Texture )
        glDeleteTextures( 1, &Entry->second.Texture );

    Images.erase( Entry );
}

void COpenGL::Apply( const CDrawData& Data, const COpenSlots& Slots, unsigned int Offset ) {
    UrUseProgram( Slots.Program );

    UrUniform4f( Slots.Projection, 2.0f / Data.Extent.Horizontal, -2.0f / Data.Extent.Vertical, -1.0f, 1.0f );
    UrUniform1ui( Slots.Origin, Offset );

    UrUniform1f( Slots.Moment, Data.Moment );
}

void COpenGL::Render( const CDrawData& Data, void* Stream ) {
    ( void )Stream;

    if ( Programs.empty( ) || Data.Extent.Horizontal <= 0.0f || Data.Extent.Vertical <= 0.0f )
        return;

    GLint FormerProgram = 0;
    GLint FormerArray = 0;

    GLint FormerActive = 0;
    GLint FormerTexture = 0;

    GLint FormerStorage = 0;
    GLint FormerAlignment = 0;

    GLint FormerSourceColor = 0;
    GLint FormerTargetColor = 0;

    GLint FormerSourceAlpha = 0;
    GLint FormerTargetAlpha = 0;

    glGetIntegerv( GL_UNPACK_ALIGNMENT, &FormerAlignment );
    glGetIntegerv( GL_CURRENT_PROGRAM, &FormerProgram );

    glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &FormerArray );
    glGetIntegerv( GL_ACTIVE_TEXTURE, &FormerActive );

    glGetIntegerv( GL_BLEND_SRC_RGB, &FormerSourceColor );
    glGetIntegerv( GL_BLEND_DST_RGB, &FormerTargetColor );

    glGetIntegerv( GL_BLEND_SRC_ALPHA, &FormerSourceAlpha );
    glGetIntegerv( GL_BLEND_DST_ALPHA, &FormerTargetAlpha );

    glGetIntegerv( GL_SHADER_STORAGE_BUFFER_BINDING, &FormerStorage );

    bool FormerBlend = glIsEnabled( GL_BLEND ) != 0;
    bool FormerCull = glIsEnabled( GL_CULL_FACE ) != 0;

    bool FormerDepth = glIsEnabled( GL_DEPTH_TEST ) != 0;
    bool FormerScissor = glIsEnabled( GL_SCISSOR_TEST ) != 0;

    UrActiveTexture( GL_TEXTURE0 );
    glGetIntegerv( GL_TEXTURE_BINDING_2D, &FormerTexture );

    Refresh( );

    if ( Data.BatchCount <= 0 ) {
        glBindTexture( GL_TEXTURE_2D, ( GLuint )FormerTexture );
        UrActiveTexture( ( GLenum )FormerActive );

        glPixelStorei( GL_UNPACK_ALIGNMENT, FormerAlignment );
        return;
    }

    if ( Data.Count > 0 && Data.Items ) {
        UrBindBuffer( GL_SHADER_STORAGE_BUFFER, Storage );
        int Needed = Data.Count * ( int )sizeof( CInstance );

        if ( Needed > Capacity ) {
            int Goal = Capacity > 0 ? Capacity : 16384 * ( int )sizeof( CInstance );

            while ( Goal < Needed )
                Goal *= 2;

            UrBufferData( GL_SHADER_STORAGE_BUFFER, Goal, nullptr, GL_DYNAMIC_DRAW );
            Capacity = Goal;
        }

        UrBufferSubData( GL_SHADER_STORAGE_BUFFER, 0, Needed, Data.Items );
        UrBindBufferBase( GL_SHADER_STORAGE_BUFFER, 0, Storage );
    }

    glViewport( 0, 0, ( GLsizei )Data.Extent.Horizontal, ( GLsizei )Data.Extent.Vertical );
    glEnable( GL_BLEND );

    UrBlendEquation( GL_FUNC_ADD );
    UrBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA );

    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );

    glDisable( GL_SCISSOR_TEST );
    UrBindVertexArray( Vertices );

    for ( int Position = 0; Position < Data.BatchCount; Position++ ) {
        const CBatch& Batch = Data.Batches[ Position ];

        if ( Batch.Callback ) {
            Batch.Callback( nullptr, Batch.Detail );

            glEnable( GL_BLEND );
            UrBlendEquation( GL_FUNC_ADD );

            UrBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA );

            glDisable( GL_CULL_FACE );
            glDisable( GL_DEPTH_TEST );

            glDisable( GL_SCISSOR_TEST );
            glViewport( 0, 0, ( GLsizei )Data.Extent.Horizontal, ( GLsizei )Data.Extent.Vertical );

            UrBindVertexArray( Vertices );
            UrBindBufferBase( GL_SHADER_STORAGE_BUFFER, 0, Storage );

            UrActiveTexture( GL_TEXTURE0 );
            continue;
        }

        if ( Batch.Count <= 0 )
            continue;

        unsigned int Bound = SheetTexture;

        if ( Batch.Image != 0 ) {
            auto Entry = Images.find( Batch.Image );
            Bound = Entry == Images.end( ) ? 0 : Entry->second.Texture;
        }

        if ( !Bound )
            continue;

        const COpenSlots* Slots = Program( Batch.Effect );
        if ( !Slots )
            continue;

        Apply( Data, *Slots, ( unsigned int )Batch.Offset );

        glBindTexture( GL_TEXTURE_2D, Bound );
        UrDrawArraysInstanced( GL_TRIANGLE_STRIP, 0, 4, ( GLsizei )Batch.Count );
    }

    glBindTexture( GL_TEXTURE_2D, ( GLuint )FormerTexture );
    UrActiveTexture( ( GLenum )FormerActive );

    UrBindVertexArray( ( GLuint )FormerArray );
    UrUseProgram( ( GLuint )FormerProgram );

    UrBindBuffer( GL_SHADER_STORAGE_BUFFER, ( GLuint )FormerStorage );
    UrBlendFuncSeparate( ( GLenum )FormerSourceColor, ( GLenum )FormerTargetColor, ( GLenum )FormerSourceAlpha, ( GLenum )FormerTargetAlpha );

    glPixelStorei( GL_UNPACK_ALIGNMENT, FormerAlignment );

    if ( !FormerBlend )
        glDisable( GL_BLEND );

    if ( FormerCull )
        glEnable( GL_CULL_FACE );

    if ( FormerDepth )
        glEnable( GL_DEPTH_TEST );

    if ( FormerScissor )
        glEnable( GL_SCISSOR_TEST );
}