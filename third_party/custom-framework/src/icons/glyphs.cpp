#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "ur/glyphs.hpp"
#include "ur/config.hpp"

#include <Windows.h>
#include <d2d1.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#pragma comment( lib, "d2d1.lib" )
#pragma comment( lib, "dwrite.lib" )

namespace ur {
namespace glyphs {

using Microsoft::WRL::ComPtr;

struct Slot {
    unsigned long long Image = 0;
    int Width = 0;
    int Height = 0;
};

static CGraphics* Gfx = nullptr;
static ComPtr< IDWriteFactory5 > Write;
static ComPtr< IDWriteFontFace > Faces[ 3 ];
static ComPtr< ID2D1Factory > Draw;
static ComPtr< IWICImagingFactory > Imaging;
static std::vector< std::wstring > TempFiles;
static std::unordered_map< unsigned long long, Slot > Cache;

static unsigned long long PackKey( icons::Icon Icon, int Size, Weight Weight ) {
    return ( ( unsigned long long )( unsigned int )Icon << 32 ) | ( ( unsigned long long )( unsigned int )Weight << 16 ) | ( unsigned int )Size;
}

static std::vector< unsigned char > ReadFileBytes( const std::string& Path ) {
    std::ifstream Stream( Path, std::ios::binary );
    if ( !Stream )
        return { };
    return std::vector< unsigned char >( ( std::istreambuf_iterator< char >( Stream ) ), std::istreambuf_iterator< char >( ) );
}

static bool FaceFromFile( const std::string& Path, ComPtr< IDWriteFontFace >& Face ) {
    std::vector< unsigned char > Bytes = ReadFileBytes( Path );
    if ( Bytes.empty( ) || !Write )
        return false;

    ComPtr< IDWriteFontFileStream > Unpacked;
    DWRITE_CONTAINER_TYPE Kind = Write->AnalyzeContainerType( Bytes.data( ), ( UINT32 )Bytes.size( ) );
    HRESULT Status = E_FAIL;
    if ( Kind == DWRITE_CONTAINER_TYPE_WOFF2 || Kind == DWRITE_CONTAINER_TYPE_WOFF )
        Status = Write->UnpackFontFile( Kind, Bytes.data( ), ( UINT32 )Bytes.size( ), &Unpacked );

    std::vector< unsigned char > FontBytes;
    if ( SUCCEEDED( Status ) && Unpacked ) {
        UINT64 Size = 0;
        if ( FAILED( Unpacked->GetFileSize( &Size ) ) || Size == 0 || Size > 16ull * 1024ull * 1024ull )
            return false;
        const void* Piece = nullptr;
        void* Cookie = nullptr;
        if ( FAILED( Unpacked->ReadFileFragment( &Piece, 0, Size, &Cookie ) ) )
            return false;
        FontBytes.assign( ( const unsigned char* )Piece, ( const unsigned char* )Piece + ( size_t )Size );
        Unpacked->ReleaseFileFragment( Cookie );
    } else {
        FontBytes = std::move( Bytes );
    }

    wchar_t TempDir[ MAX_PATH ] = { };
    wchar_t TempFile[ MAX_PATH ] = { };
    GetTempPathW( MAX_PATH, TempDir );
    GetTempFileNameW( TempDir, L"ur", 0, TempFile );
    HANDLE Disk = CreateFileW( TempFile, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr );
    if ( Disk == INVALID_HANDLE_VALUE )
        return false;
    DWORD Wrote = 0;
    WriteFile( Disk, FontBytes.data( ), ( DWORD )FontBytes.size( ), &Wrote, nullptr );
    CloseHandle( Disk );

    ComPtr< IDWriteFontFile > File;
    HRESULT FileStatus = Write->CreateFontFileReference( TempFile, nullptr, &File );
    if ( FAILED( FileStatus ) || !File ) {
        DeleteFileW( TempFile );
        return false;
    }
    TempFiles.push_back( TempFile );

    BOOL Supported = FALSE;
    DWRITE_FONT_FILE_TYPE FileType = DWRITE_FONT_FILE_TYPE_UNKNOWN;
    DWRITE_FONT_FACE_TYPE FaceType = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    UINT32 FacesOnFile = 0;
    if ( FAILED( File->Analyze( &Supported, &FileType, &FaceType, &FacesOnFile ) ) || !Supported || FacesOnFile == 0 )
        return false;

    IDWriteFontFile* Files[ 1 ] = { File.Get( ) };
    return SUCCEEDED( Write->CreateFontFace( FaceType, 1, Files, 0, DWRITE_FONT_SIMULATIONS_NONE, &Face ) );
}

static bool Boot( ) {
    if ( Write )
        return Faces[ 0 ] != nullptr;

    if ( FAILED( DWriteCreateFactory( DWRITE_FACTORY_TYPE_SHARED, __uuidof( IDWriteFactory5 ), &Write ) ) )
        return false;
    if ( FAILED( D2D1CreateFactory( D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS( &Draw ) ) ) )
        return false;
    if ( FAILED( CoCreateInstance( CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( &Imaging ) ) ) )
        return false;

    FaceFromFile( config::asset( "assets\\icons\\fontawesome\\fa-solid-900.woff2" ), Faces[ 0 ] );
    FaceFromFile( config::asset( "assets\\icons\\fontawesome\\fa-regular-400.woff2" ), Faces[ 1 ] );
    FaceFromFile( config::asset( "assets\\icons\\fontawesome\\fa-light-300.woff2" ), Faces[ 2 ] );
    return Faces[ 0 ] != nullptr;
}

static bool Raster( IDWriteFontFace* Face, UINT32 Code, int Size, std::vector< unsigned char >& Pixels, int& Width, int& Height ) {
    if ( !Face || Size <= 0 )
        return false;

    UINT16 Glyph = 0;
    Face->GetGlyphIndices( &Code, 1, &Glyph );
    if ( Glyph == 0 )
        return false;

    Width = Size;
    Height = Size;

    ComPtr< IWICBitmap > Bitmap;
    if ( FAILED( Imaging->CreateBitmap( Size, Size, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &Bitmap ) ) )
        return false;

    D2D1_RENDER_TARGET_PROPERTIES Props = D2D1::RenderTargetProperties( D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat( DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED ) );
    ComPtr< ID2D1RenderTarget > Target;
    if ( FAILED( Draw->CreateWicBitmapRenderTarget( Bitmap.Get( ), Props, &Target ) ) )
        return false;

    ComPtr< ID2D1SolidColorBrush > Brush;
    Target->CreateSolidColorBrush( D2D1::ColorF( 1.0f, 1.0f, 1.0f, 1.0f ), &Brush );

    DWRITE_FONT_METRICS Metrics{ };
    Face->GetMetrics( &Metrics );
    DWRITE_GLYPH_METRICS Box{ };
    Face->GetDesignGlyphMetrics( &Glyph, 1, &Box, FALSE );

    float Em = ( float )Size * 0.82f;
    float Units = Em / ( float )Metrics.designUnitsPerEm;
    float InkW = ( float )( Box.advanceWidth - Box.leftSideBearing - Box.rightSideBearing ) * Units;
    float InkH = ( float )( Box.advanceHeight - Box.topSideBearing - Box.bottomSideBearing ) * Units;
    if ( InkW < 1.0f )
        InkW = Em * 0.75f;
    if ( InkH < 1.0f )
        InkH = Em * 0.75f;
    float OriginX = ( ( float )Size - InkW ) * 0.5f - ( float )Box.leftSideBearing * Units;
    float TopFromBase = -( ( float )Box.verticalOriginY - ( float )Box.topSideBearing ) * Units;
    float OriginY = ( ( float )Size - InkH ) * 0.5f - TopFromBase;

    DWRITE_GLYPH_RUN Run{ };
    Run.fontFace = Face;
    Run.fontEmSize = Em;
    Run.glyphCount = 1;
    Run.glyphIndices = &Glyph;
    DWRITE_GLYPH_OFFSET Offset{ };
    float Advance = 0.0f;
    Run.glyphAdvances = &Advance;
    Run.glyphOffsets = &Offset;

    Target->BeginDraw( );
    Target->Clear( D2D1::ColorF( 0, 0, 0, 0 ) );
    Target->DrawGlyphRun( D2D1::Point2F( OriginX, OriginY ), &Run, Brush.Get( ) );
    if ( FAILED( Target->EndDraw( ) ) )
        return false;

    ComPtr< IWICBitmapLock > Lock;
    WICRect Rect{ 0, 0, Size, Size };
    if ( FAILED( Bitmap->Lock( &Rect, WICBitmapLockRead, &Lock ) ) )
        return false;

    UINT Stride = 0;
    UINT BufferSize = 0;
    BYTE* Data = nullptr;
    Lock->GetStride( &Stride );
    Lock->GetDataPointer( &BufferSize, &Data );
    Pixels.resize( ( size_t )Size * Size * 4 );
    for ( int Y = 0; Y < Size; Y++ ) {
        BYTE* Row = Data + Y * Stride;
        for ( int X = 0; X < Size; X++ ) {
            size_t Dst = ( ( size_t )Y * Size + X ) * 4;
            Pixels[ Dst + 0 ] = Row[ X * 4 + 2 ];
            Pixels[ Dst + 1 ] = Row[ X * 4 + 1 ];
            Pixels[ Dst + 2 ] = Row[ X * 4 + 0 ];
            Pixels[ Dst + 3 ] = Row[ X * 4 + 3 ];
        }
    }
    return true;
}

void bind( CGraphics* Graphics ) {
    Cache.clear( );
    Gfx = Graphics;
    Boot( );
}

unsigned long long image( icons::Icon Icon, int Size, Weight Weight ) {
    if ( !Gfx || Icon == icons::Icon::None || Size <= 0 )
        return 0;
    if ( !Boot( ) )
        return 0;

    unsigned long long Key = PackKey( Icon, Size, Weight );
    auto Found = Cache.find( Key );
    if ( Found != Cache.end( ) )
        return Found->second.Image;

    int Index = ( int )Weight;
    if ( Index < 0 || Index > 2 )
        Index = 0;
    IDWriteFontFace* Face = Faces[ Index ] ? Faces[ Index ].Get( ) : Faces[ 0 ].Get( );
    std::vector< unsigned char > Pixels;
    int Width = 0;
    int Height = 0;
    if ( !Raster( Face, ( UINT32 )Icon, Size, Pixels, Width, Height ) )
        return 0;

    Slot Made;
    Made.Width = Width;
    Made.Height = Height;
    Made.Image = Gfx->CreateImage( Pixels.data( ), Width, Height );
    Cache[ Key ] = Made;
    return Made.Image;
}

void sweep( ) {
    for ( const std::wstring& Path : TempFiles )
        DeleteFileW( Path.c_str( ) );
    TempFiles.clear( );
}

}
}
