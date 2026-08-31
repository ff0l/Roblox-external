#if defined( _WIN32 )

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <wincodec.h>
#include <d3d11.h>
#include <d2d1_1.h>
#include <d2d1_3.h>
#include <dxgi.h>

#include "Context.h"
#include "Pictures.h"

#pragma comment( lib, "ole32.lib" )
#pragma comment( lib, "windowscodecs.lib" )
#pragma comment( lib, "d2d1.lib" )
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxgi.lib" )

static IWICImagingFactory* Workshop( ) {
    static IWICImagingFactory* Factory = nullptr;

    if ( Factory )
        return Factory;

    CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED );
    CoCreateInstance( CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, __uuidof( IWICImagingFactory ), ( void** )&Factory );

    return Factory;
}

static bool Harvest( IWICImagingFactory* Factory, IWICBitmapDecoder* Decoder, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Longest ) {
    IWICBitmapFrameDecode* Frame = nullptr;
    Decoder->GetFrame( 0, &Frame );

    if ( !Frame )
        return false;

    IWICBitmapSource* Source = Frame;

    unsigned int Across = 0;
    unsigned int Down = 0;

    Source->GetSize( &Across, &Down );

    if ( Across == 0 || Down == 0 || Across > 16384 || Down > 16384 ) {
        Source->Release( );
        return false;
    }

    unsigned int Widest = Across > Down ? Across : Down;

    if ( Longest > 0 && Widest > ( unsigned int )Longest ) {
        unsigned int SmallAcross = Across * ( unsigned int )Longest / Widest;
        unsigned int SmallDown = Down * ( unsigned int )Longest / Widest;

        if ( SmallAcross == 0 )
            SmallAcross = 1;

        if ( SmallDown == 0 )
            SmallDown = 1;

        IWICBitmapScaler* Scaler = nullptr;
        Factory->CreateBitmapScaler( &Scaler );

        if ( Scaler && SUCCEEDED( Scaler->Initialize( Source, SmallAcross, SmallDown, WICBitmapInterpolationModeFant ) ) ) {
            Source->Release( );
            Source = Scaler;

            Across = SmallAcross;
            Down = SmallDown;
        } else if ( Scaler ) {
            Scaler->Release( );
        }
    }

    if ( ( size_t )Across * Down > 67108864 ) {
        Source->Release( );
        return false;
    }

    IWICBitmapSource* Converted = nullptr;
    WICConvertBitmapSource( GUID_WICPixelFormat32bppRGBA, Source, &Converted );

    Source->Release( );
    if ( !Converted )
        return false;

    try {
        Pixels.resize( ( size_t )Across * Down * 4 );
    } catch ( ... ) {
        Converted->Release( );
        return false;
    }

    HRESULT Outcome = Converted->CopyPixels( nullptr, Across * 4, ( unsigned int )Pixels.size( ), Pixels.data( ) );
    Converted->Release( );

    if ( FAILED( Outcome ) )
        return false;

    Width = ( int )Across;
    Height = ( int )Down;

    return true;
}

bool CPictures::Load( const char* Path, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Longest ) {
    Width = 0;
    Height = 0;

    if ( !Path )
        return false;

    IWICImagingFactory* Factory = Workshop( );

    if ( !Factory ) {
        Context->Report( "Could not start the imaging system" );
        return false;
    }

    wchar_t Wide[ 1024 ] = { };

    if ( !MultiByteToWideChar( CP_UTF8, 0, Path, -1, Wide, 1024 ) ) {
        Context->Report( "Could not read the picture path" );
        return false;
    }

    IWICBitmapDecoder* Decoder = nullptr;
    Factory->CreateDecoderFromFilename( Wide, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &Decoder );

    if ( !Decoder ) {
        Context->Report( "Could not open the requested picture" );
        return false;
    }

    bool Loaded = Harvest( Factory, Decoder, Pixels, Width, Height, Longest );
    Decoder->Release( );

    if ( !Loaded )
        Context->Report( "Could not decode the requested picture" );

    return Loaded;
}

static ID2D1DeviceContext5* Easel( bool Rebuild ) {
    static ID2D1DeviceContext5* Cached = nullptr;
    static bool Tried = false;

    if ( Rebuild ) {
        if ( Cached )
            Cached->Release( );

        Cached = nullptr;
        Tried = false;

        return nullptr;
    }

    if ( Tried )
        return Cached;

    Tried = true;

    ID3D11Device* Device = nullptr;
    D3D_FEATURE_LEVEL Levels[ 3 ] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };

    if ( FAILED( D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, Levels, 3, D3D11_SDK_VERSION, &Device, nullptr, nullptr ) ) ) {
        if ( FAILED( D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, Levels, 3, D3D11_SDK_VERSION, &Device, nullptr, nullptr ) ) )
            return nullptr;
    }

    IDXGIDevice* Adapter = nullptr;
    Device->QueryInterface( __uuidof( IDXGIDevice ), ( void** )&Adapter );

    ID2D1Factory1* Factory = nullptr;
    D2D1_FACTORY_OPTIONS Options = { };

    D2D1CreateFactory( D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof( ID2D1Factory1 ), &Options, ( void** )&Factory );

    ID2D1Device* Surface = nullptr;
    if ( Factory && Adapter )
        Factory->CreateDevice( Adapter, &Surface );

    ID2D1DeviceContext* Orders = nullptr;
    if ( Surface )
        Surface->CreateDeviceContext( D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &Orders );

    if ( Orders )
        Orders->QueryInterface( __uuidof( ID2D1DeviceContext5 ), ( void** )&Cached );

    if ( Orders )
        Orders->Release( );

    if ( Surface )
        Surface->Release( );

    if ( Factory )
        Factory->Release( );

    if ( Adapter )
        Adapter->Release( );

    if ( Device )
        Device->Release( );

    return Cached;
}

static bool Paint( const unsigned char* Bytes, size_t Length, std::vector< unsigned char >& Pixels, int Size ) {
    ID2D1DeviceContext5* Painter = Easel( false );

    bool Done = false;
    bool Lost = false;

    if ( Painter ) {
        D2D1_SIZE_U Dimension = { ( unsigned int )Size, ( unsigned int )Size };

        D2D1_BITMAP_PROPERTIES1 TargetPlan = { };
        TargetPlan.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;

        TargetPlan.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        TargetPlan.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;

        ID2D1Bitmap1* Target = nullptr;
        Painter->CreateBitmap( Dimension, nullptr, 0, &TargetPlan, &Target );

        HGLOBAL Block = GlobalAlloc( GMEM_MOVEABLE, Length );
        IStream* Stream = nullptr;

        if ( Block ) {
            void* Room = GlobalLock( Block );

            if ( Room ) {
                memcpy( Room, Bytes, Length );
                GlobalUnlock( Block );

                CreateStreamOnHGlobal( Block, TRUE, &Stream );
            }

            if ( !Stream )
                GlobalFree( Block );
        }

        ID2D1SvgDocument* Document = nullptr;
        D2D1_SIZE_F Viewport = { ( float )Size, ( float )Size };

        if ( Stream )
            Painter->CreateSvgDocument( Stream, Viewport, &Document );

        if ( Target && Document ) {
            Painter->SetTarget( Target );
            Painter->BeginDraw( );

            D2D1_COLOR_F Clear = { 0.0f, 0.0f, 0.0f, 0.0f };
            Painter->Clear( Clear );

            Painter->DrawSvgDocument( Document );
            Lost = FAILED( Painter->EndDraw( ) );

            D2D1_BITMAP_PROPERTIES1 ReadPlan = { };
            ReadPlan.pixelFormat = TargetPlan.pixelFormat;

            ReadPlan.bitmapOptions = D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

            ID2D1Bitmap1* Readback = nullptr;

            if ( !Lost )
                Painter->CreateBitmap( Dimension, nullptr, 0, &ReadPlan, &Readback );

            if ( Readback && SUCCEEDED( Readback->CopyFromBitmap( nullptr, Target, nullptr ) ) ) {
                D2D1_MAPPED_RECT Mapped = { };

                if ( SUCCEEDED( Readback->Map( D2D1_MAP_OPTIONS_READ, &Mapped ) ) ) {
                    Pixels.resize( ( size_t )Size * Size * 4 );

                    for ( int Row = 0; Row < Size; Row++ ) {
                        const unsigned char* Line = Mapped.bits + ( size_t )Row * Mapped.pitch;

                        for ( int Column = 0; Column < Size; Column++ ) {
                            const unsigned char* Source = Line + ( size_t )Column * 4;
                            unsigned char* Out = Pixels.data( ) + ( ( size_t )Row * Size + Column ) * 4;

                            int Alpha = Source[ 3 ];

                            if ( Alpha > 0 ) {
                                Out[ 0 ] = ( unsigned char )( Source[ 2 ] * 255 / Alpha );
                                Out[ 1 ] = ( unsigned char )( Source[ 1 ] * 255 / Alpha );

                                Out[ 2 ] = ( unsigned char )( Source[ 0 ] * 255 / Alpha );
                            } else {
                                Out[ 0 ] = 0;
                                Out[ 1 ] = 0;

                                Out[ 2 ] = 0;
                            }

                            Out[ 3 ] = ( unsigned char )Alpha;
                        }
                    }

                    Readback->Unmap( );
                    Done = true;
                }
            }

            if ( Readback )
                Readback->Release( );
        }

        Painter->SetTarget( nullptr );

        if ( Document )
            Document->Release( );

        if ( Stream )
            Stream->Release( );

        if ( Target )
            Target->Release( );
    }

    if ( Lost )
        Easel( true );

    return Done;
}

bool CPictures::VectorBytes( const unsigned char* Bytes, size_t Length, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Size ) {
    Width = 0;
    Height = 0;

    if ( !Bytes || Length == 0 || Length > 0x4000000 || Size <= 0 )
        return false;

    if ( Size > 4096 )
        Size = 4096;

    bool Balanced = SUCCEEDED( CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED ) );
    bool Done = Paint( Bytes, Length, Pixels, Size );

    if ( Balanced )
        CoUninitialize( );

    if ( !Done ) {
        Context->Report( "Could not rasterize the SVG" );
        return false;
    }

    Width = Size;
    Height = Size;

    return true;
}

bool CPictures::Vector( const char* Path, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Size ) {
    Width = 0;
    Height = 0;

    if ( !Path )
        return false;

    wchar_t Wide[ 1024 ] = { };

    if ( !MultiByteToWideChar( CP_UTF8, 0, Path, -1, Wide, 1024 ) ) {
        Context->Report( "Could not read the SVG path" );
        return false;
    }

    HANDLE File = CreateFileW( Wide, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( File == INVALID_HANDLE_VALUE ) {
        Context->Report( "Could not open the SVG file" );
        return false;
    }

    DWORD Length = GetFileSize( File, nullptr );

    if ( Length == 0 || Length == INVALID_FILE_SIZE || Length > 0x4000000 ) {
        CloseHandle( File );
        return false;
    }

    std::vector< unsigned char > Bytes( Length );
    DWORD Taken = 0;

    ReadFile( File, Bytes.data( ), Length, &Taken, nullptr );
    CloseHandle( File );

    if ( Taken != Length )
        return false;

    return VectorBytes( Bytes.data( ), Bytes.size( ), Pixels, Width, Height, Size );
}

bool CPictures::Decode( const unsigned char* Bytes, size_t Length, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Longest ) {
    Width = 0;
    Height = 0;

    if ( !Bytes || Length == 0 || Length > 0x40000000 )
        return false;

    IWICImagingFactory* Factory = Workshop( );

    if ( !Factory )
        return false;

    IWICStream* Stream = nullptr;
    Factory->CreateStream( &Stream );

    if ( !Stream )
        return false;

    Stream->InitializeFromMemory( ( unsigned char* )Bytes, ( unsigned int )Length );

    IWICBitmapDecoder* Decoder = nullptr;
    Factory->CreateDecoderFromStream( Stream, nullptr, WICDecodeMetadataCacheOnDemand, &Decoder );

    bool Loaded = false;

    if ( Decoder ) {
        Loaded = Harvest( Factory, Decoder, Pixels, Width, Height, Longest );
        Decoder->Release( );
    }

    Stream->Release( );

    if ( !Loaded )
        Context->Report( "Could not decode the picture bytes given" );

    return Loaded;
}

#elif defined( __APPLE__ )

#include <cstring>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>

#include "Context.h"
#include "Pictures.h"

static bool Harvest( CGImageRef Picture, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Longest ) {
    if ( !Picture )
        return false;

    size_t Given = CGImageGetWidth( Picture );
    size_t Tall = CGImageGetHeight( Picture );

    if ( Given == 0 || Tall == 0 || Given > 16384 || Tall > 16384 )
        return false;

    unsigned int Across = ( unsigned int )Given;
    unsigned int Down = ( unsigned int )Tall;

    unsigned int Widest = Across > Down ? Across : Down;

    if ( Longest > 0 && Widest > ( unsigned int )Longest ) {
        unsigned int SmallAcross = Across * ( unsigned int )Longest / Widest;
        unsigned int SmallDown = Down * ( unsigned int )Longest / Widest;

        if ( SmallAcross == 0 )
            SmallAcross = 1;

        if ( SmallDown == 0 )
            SmallDown = 1;

        Across = SmallAcross;
        Down = SmallDown;
    }

    if ( ( size_t )Across * Down > 67108864 )
        return false;

    try {
        Pixels.resize( ( size_t )Across * Down * 4 );
    } catch ( ... ) {
        return false;
    }

    CGColorSpaceRef Space = CGColorSpaceCreateDeviceRGB( );
    if ( !Space )
        return false;

    CGContextRef Painter = CGBitmapContextCreate( Pixels.data( ), Across, Down, 8, ( size_t )Across * 4, Space, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big );
    CGColorSpaceRelease( Space );

    if ( !Painter )
        return false;

    CGRect Area = CGRectMake( 0.0, 0.0, ( CGFloat )Across, ( CGFloat )Down );

    CGContextClearRect( Painter, Area );
    CGContextSetBlendMode( Painter, kCGBlendModeCopy );

    CGContextSetInterpolationQuality( Painter, kCGInterpolationHigh );
    CGContextDrawImage( Painter, Area, Picture );

    CGContextFlush( Painter );
    CGContextRelease( Painter );

    size_t Total = ( size_t )Across * Down;

    for ( size_t Step = 0; Step < Total; Step++ ) {
        unsigned char* Dot = Pixels.data( ) + Step * 4;
        int Alpha = Dot[ 3 ];

        if ( Alpha == 255 )
            continue;

        if ( Alpha == 0 ) {
            Dot[ 0 ] = 0;
            Dot[ 1 ] = 0;

            Dot[ 2 ] = 0;
            continue;
        }

        for ( int Channel = 0; Channel < 3; Channel++ ) {
            int Level = Dot[ Channel ] * 255 / Alpha;
            if ( Level > 255 )
                Level = 255;

            Dot[ Channel ] = ( unsigned char )Level;
        }
    }

    Width = ( int )Across;
    Height = ( int )Down;

    return true;
}

bool CPictures::Load( const char* Path, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Longest ) {
    Width = 0;
    Height = 0;

    if ( !Path )
        return false;

    CFURLRef Location = CFURLCreateFromFileSystemRepresentation( kCFAllocatorDefault, ( const UInt8* )Path, ( CFIndex )strlen( Path ), false );

    if ( !Location ) {
        Context->Report( "Could not read the picture path" );
        return false;
    }

    CGImageSourceRef Reader = CGImageSourceCreateWithURL( Location, nullptr );
    CFRelease( Location );

    if ( !Reader ) {
        Context->Report( "Could not open the requested picture" );
        return false;
    }

    CGImageRef Picture = CGImageSourceCreateImageAtIndex( Reader, 0, nullptr );
    CFRelease( Reader );

    bool Loaded = Harvest( Picture, Pixels, Width, Height, Longest );

    if ( Picture )
        CGImageRelease( Picture );

    if ( !Loaded )
        Context->Report( "Could not decode the requested picture" );

    return Loaded;
}

bool CPictures::Decode( const unsigned char* Bytes, size_t Length, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Longest ) {
    Width = 0;
    Height = 0;

    if ( !Bytes || Length == 0 || Length > 0x40000000 )
        return false;

    CFDataRef Block = CFDataCreate( kCFAllocatorDefault, ( const UInt8* )Bytes, ( CFIndex )Length );

    if ( !Block )
        return false;

    CGImageSourceRef Reader = CGImageSourceCreateWithData( Block, nullptr );
    CFRelease( Block );

    bool Loaded = false;

    if ( Reader ) {
        CGImageRef Picture = CGImageSourceCreateImageAtIndex( Reader, 0, nullptr );
        Loaded = Harvest( Picture, Pixels, Width, Height, Longest );

        if ( Picture )
            CGImageRelease( Picture );

        CFRelease( Reader );
    }

    if ( !Loaded )
        Context->Report( "Could not decode the picture bytes given" );

    return Loaded;
}

bool CPictures::VectorBytes( const unsigned char* Bytes, size_t Length, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Size ) {
    Width = 0;
    Height = 0;

    Pixels.clear( );

    if ( !Bytes || Length == 0 || Length > 0x4000000 || Size <= 0 )
        return false;

    Context->Report( "Cannot rasterize SVG on this platform" );
    return false;
}

bool CPictures::Vector( const char* Path, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Size ) {
    Width = 0;
    Height = 0;

    Pixels.clear( );

    if ( !Path || Size <= 0 )
        return false;

    Context->Report( "Cannot rasterize SVG on this platform" );
    return false;
}

#endif