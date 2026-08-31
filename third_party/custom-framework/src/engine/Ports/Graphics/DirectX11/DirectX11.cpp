#include <d3d11.h>
#include <cstring>
#include <d3dcompiler.h>
#include <unordered_map>

#include "Sheet.h"
#include "Context.h"
#include "Shaders.h"
#include "DirectX11.h"

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "d3dcompiler.lib" )

static_assert( sizeof( CInstance ) == 80, "The instance layout must match the shader" );

static ID3DBlob* CompileStage( const std::string& Source, const char* Entry, const char* Target ) {
    static std::unordered_map< std::string, ID3DBlob* > Keepsakes;

    std::string Key = std::string( Target ) + '\x01' + Source;
    auto Kept = Keepsakes.find( Key );

    if ( Kept != Keepsakes.end( ) ) {
        if ( Kept->second )
            Kept->second->AddRef( );

        return Kept->second;
    }

    ID3DBlob* Bytecode = nullptr;
    ID3DBlob* Faults = nullptr;

    D3DCompile( Source.c_str( ), Source.size( ), "UR", nullptr, nullptr, Entry, Target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &Bytecode, &Faults );

    if ( Faults ) {
        if ( !Bytecode )
            Context->Report( ( const char* )Faults->GetBufferPointer( ) );

        Faults->Release( );
    }

    Keepsakes[ Key ] = Bytecode;

    if ( Bytecode )
        Bytecode->AddRef( );

    return Bytecode;
}

CDirectEleven::~CDirectEleven( ) {
    Destroy( );
}

bool CDirectEleven::Create( ID3D11Device* Device, ID3D11DeviceContext* Orders ) {
    Destroy( );

    if ( !Device || !Orders )
        return false;

    Hardware = Device;
    Hardware->AddRef( );

    Commands = Orders;
    Commands->AddRef( );

    ID3DBlob* VertexCode = CompileStage( Shaders->Vertex( ShaderHlsl ), "MainVertex", "vs_5_0" );
    if ( !VertexCode ) {
        Destroy( );
        return false;
    }

    Hardware->CreateVertexShader( VertexCode->GetBufferPointer( ), VertexCode->GetBufferSize( ), nullptr, &VertexStage );
    VertexCode->Release( );

    if ( !VertexStage || !Stage( 0 ) ) {
        Destroy( );
        return false;
    }

    D3D11_BLEND_DESC Mixing = { };

    Mixing.RenderTarget[ 0 ].BlendEnable = TRUE;
    Mixing.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    Mixing.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

    Mixing.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_ADD;
    Mixing.RenderTarget[ 0 ].SrcBlendAlpha = D3D11_BLEND_ONE;

    Mixing.RenderTarget[ 0 ].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    Mixing.RenderTarget[ 0 ].BlendOpAlpha = D3D11_BLEND_OP_ADD;

    Mixing.RenderTarget[ 0 ].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    Hardware->CreateBlendState( &Mixing, &Blending );

    D3D11_RASTERIZER_DESC Shaping = { };

    Shaping.FillMode = D3D11_FILL_SOLID;
    Shaping.CullMode = D3D11_CULL_NONE;

    Shaping.DepthClipEnable = TRUE;
    Hardware->CreateRasterizerState( &Shaping, &Rasterizer );

    D3D11_SAMPLER_DESC Filtering = { };

    Filtering.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    Filtering.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;

    Filtering.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    Filtering.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    Filtering.MaxLOD = D3D11_FLOAT32_MAX;
    Hardware->CreateSamplerState( &Filtering, &Sampler );

    D3D11_BUFFER_DESC Slot = { };

    Slot.ByteWidth = 48;
    Slot.Usage = D3D11_USAGE_DEFAULT;

    Slot.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Hardware->CreateBuffer( &Slot, nullptr, &Constants );

    if ( !Blending || !Rasterizer || !Sampler || !Constants ) {
        Destroy( );
        return false;
    }

    return Reserve( 16384 );
}

void CDirectEleven::Destroy( ) {
    for ( auto& Entry : Images ) {
        if ( Entry.second.Owned && Entry.second.View )
            Entry.second.View->Release( );
    }

    Images.clear( );

    for ( ID3D11PixelShader* Piece : Stages ) {
        if ( Piece )
            Piece->Release( );
    }

    Stages.clear( );

    if ( SheetView ) {
        SheetView->Release( );
        SheetView = nullptr;
    }

    if ( View ) {
        View->Release( );
        View = nullptr;
    }

    if ( Storage ) {
        Storage->Release( );
        Storage = nullptr;
    }

    if ( Constants ) {
        Constants->Release( );
        Constants = nullptr;
    }

    if ( Sampler ) {
        Sampler->Release( );
        Sampler = nullptr;
    }

    if ( Rasterizer ) {
        Rasterizer->Release( );
        Rasterizer = nullptr;
    }

    if ( Blending ) {
        Blending->Release( );
        Blending = nullptr;
    }

    if ( VertexStage ) {
        VertexStage->Release( );
        VertexStage = nullptr;
    }

    if ( Commands ) {
        Commands->Release( );
        Commands = nullptr;
    }

    if ( Hardware ) {
        Hardware->Release( );
        Hardware = nullptr;
    }

    SheetVersion = 0;
    SheetDepth = 0;

    Capacity = 0;
    NextImage = 1;
}

ID3D11PixelShader* CDirectEleven::Stage( unsigned int Effect ) {
    size_t Wanted = ( size_t )Effect + 1;

    if ( Stages.size( ) < Wanted )
        Stages.resize( Wanted, nullptr );

    if ( Stages[ Effect ] )
        return Stages[ Effect ];

    ID3DBlob* Code = CompileStage( Shaders->Pixel( Effect, ShaderHlsl ), "MainFragment", "ps_5_0" );
    if ( !Code )
        return Effect == 0 ? nullptr : Stage( 0 );

    Hardware->CreatePixelShader( Code->GetBufferPointer( ), Code->GetBufferSize( ), nullptr, &Stages[ Effect ] );
    Code->Release( );

    if ( !Stages[ Effect ] )
        return Effect == 0 ? nullptr : Stage( 0 );

    return Stages[ Effect ];
}

bool CDirectEleven::Reserve( int Count ) {
    if ( Count <= Capacity )
        return true;

    int Goal = Capacity > 0 ? Capacity : 16384;
    while ( Goal < Count )
        Goal *= 2;

    if ( View ) {
        View->Release( );
        View = nullptr;
    }

    if ( Storage ) {
        Storage->Release( );
        Storage = nullptr;
    }

    Capacity = 0;
    D3D11_BUFFER_DESC Description = { };

    Description.ByteWidth = ( unsigned int )( Goal * ( int )sizeof( CInstance ) );
    Description.Usage = D3D11_USAGE_DYNAMIC;

    Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    Description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    Description.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    Description.StructureByteStride = sizeof( CInstance );

    Hardware->CreateBuffer( &Description, nullptr, &Storage );
    if ( !Storage ) {
        Context->Report( "Could not grow the DirectX 11 instance buffer" );
        return false;
    }

    Hardware->CreateShaderResourceView( Storage, nullptr, &View );
    if ( !View )
        return false;

    Capacity = Goal;
    return true;
}

void CDirectEleven::Refresh( ) {
    if ( SheetVersion == Sheet->Version && SheetView && SheetDepth == Sheet->Depth( ) )
        return;

    if ( Sheet->Depth( ) <= 0 )
        return;

    if ( SheetView ) {
        SheetView->Release( );
        SheetView = nullptr;
    }

    D3D11_TEXTURE2D_DESC Description = { };

    Description.Width = ( unsigned int )Sheet->Extent( );
    Description.Height = ( unsigned int )Sheet->Depth( );

    Description.MipLevels = 1;
    Description.ArraySize = 1;

    Description.Format = DXGI_FORMAT_R8_UNORM;
    Description.SampleDesc.Count = 1;

    Description.Usage = D3D11_USAGE_IMMUTABLE;
    Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA Content = { };

    Content.pSysMem = Sheet->Data( );
    Content.SysMemPitch = ( unsigned int )Sheet->Extent( );

    ID3D11Texture2D* Texture = nullptr;
    Hardware->CreateTexture2D( &Description, &Content, &Texture );

    if ( !Texture )
        return;

    Hardware->CreateShaderResourceView( Texture, nullptr, &SheetView );
    Texture->Release( );

    SheetVersion = Sheet->Version;
    SheetDepth = Sheet->Depth( );
}

unsigned long long CDirectEleven::CreateImage( const unsigned char* Pixels, int Width, int Height ) {
    if ( !Hardware || !Pixels || Width <= 0 || Height <= 0 )
        return 0;

    if ( Width > 16384 || Height > 16384 ) {
        Context->Report( "Rejected an image with unreasonable dimensions" );
        return 0;
    }

    D3D11_TEXTURE2D_DESC Description = { };

    Description.Width = ( unsigned int )Width;
    Description.Height = ( unsigned int )Height;

    Description.MipLevels = 1;
    Description.ArraySize = 1;

    Description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Description.SampleDesc.Count = 1;

    Description.Usage = D3D11_USAGE_DEFAULT;
    Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA Content = { };

    Content.pSysMem = Pixels;
    Content.SysMemPitch = ( unsigned int )Width * 4;

    ID3D11Texture2D* Texture = nullptr;
    Hardware->CreateTexture2D( &Description, &Content, &Texture );

    if ( !Texture ) {
        Context->Report( "Could not create a DirectX 11 image" );
        return 0;
    }

    CElevenImage Image;

    Image.Width = Width;
    Image.Height = Height;

    Image.Owned = true;
    Hardware->CreateShaderResourceView( Texture, nullptr, &Image.View );

    Texture->Release( );
    if ( !Image.View )
        return 0;

    unsigned long long Handle = NextImage;
    NextImage++;

    Images[ Handle ] = Image;
    return Handle;
}

void CDirectEleven::UpdateImage( unsigned long long Image, const unsigned char* Pixels ) {
    auto Entry = Images.find( Image );
    if ( Entry == Images.end( ) || !Entry->second.Owned || !Pixels )
        return;

    ID3D11Resource* Resource = nullptr;
    Entry->second.View->GetResource( &Resource );

    if ( !Resource )
        return;

    Commands->UpdateSubresource( Resource, 0, nullptr, Pixels, ( unsigned int )Entry->second.Width * 4, 0 );
    Resource->Release( );
}

unsigned long long CDirectEleven::AdoptImage( void* Native ) {
    if ( !Native )
        return 0;

    CElevenImage Image;

    Image.View = ( ID3D11ShaderResourceView* )Native;
    Image.Owned = false;

    unsigned long long Handle = NextImage;
    NextImage++;

    Images[ Handle ] = Image;
    return Handle;
}

void CDirectEleven::DestroyImage( unsigned long long Image ) {
    auto Entry = Images.find( Image );
    if ( Entry == Images.end( ) )
        return;

    if ( Entry->second.Owned && Entry->second.View )
        Entry->second.View->Release( );

    Images.erase( Entry );
}

void CDirectEleven::Bind( const CDrawData& Data ) {
    D3D11_VIEWPORT Viewport = { };

    Viewport.Width = Data.Extent.Horizontal;
    Viewport.Height = Data.Extent.Vertical;

    Viewport.MaxDepth = 1.0f;

    Commands->RSSetViewports( 1, &Viewport );
    Commands->RSSetState( Rasterizer );

    Commands->IASetInputLayout( nullptr );
    Commands->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

    Commands->VSSetShader( VertexStage, nullptr, 0 );
    Commands->VSSetShaderResources( 0, 1, &View );

    Commands->VSSetConstantBuffers( 0, 1, &Constants );
    Commands->PSSetConstantBuffers( 0, 1, &Constants );

    Commands->PSSetSamplers( 0, 1, &Sampler );
    Commands->OMSetBlendState( Blending, nullptr, 0xffffffff );
}

void CDirectEleven::Render( const CDrawData& Data, void* Stream ) {
    ( void )Stream;

    if ( !Commands || Data.Extent.Horizontal <= 0.0f || Data.Extent.Vertical <= 0.0f )
        return;

    Refresh( );

    if ( Data.BatchCount <= 0 )
        return;

    if ( Data.Count > 0 && Data.Items ) {
        if ( !Reserve( Data.Count ) )
            return;

        D3D11_MAPPED_SUBRESOURCE Mapping = { };
        if ( FAILED( Commands->Map( Storage, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapping ) ) )
            return;

        memcpy( Mapping.pData, Data.Items, ( size_t )Data.Count * sizeof( CInstance ) );
        Commands->Unmap( Storage, 0 );
    }

    ID3D11RasterizerState* KeepRasterizer = nullptr;
    Commands->RSGetState( &KeepRasterizer );

    unsigned int KeepViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT KeepViewport[ D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE ];

    Commands->RSGetViewports( &KeepViewports, KeepViewport );

    ID3D11InputLayout* KeepLayout = nullptr;
    Commands->IAGetInputLayout( &KeepLayout );

    D3D11_PRIMITIVE_TOPOLOGY KeepTopology;
    Commands->IAGetPrimitiveTopology( &KeepTopology );

    ID3D11VertexShader* KeepVertex = nullptr;
    Commands->VSGetShader( &KeepVertex, nullptr, nullptr );

    ID3D11PixelShader* KeepPixel = nullptr;
    Commands->PSGetShader( &KeepPixel, nullptr, nullptr );

    ID3D11ShaderResourceView* KeepVertexView = nullptr;
    Commands->VSGetShaderResources( 0, 1, &KeepVertexView );

    ID3D11ShaderResourceView* KeepPixelView = nullptr;
    Commands->PSGetShaderResources( 0, 1, &KeepPixelView );

    ID3D11Buffer* KeepVertexConstants = nullptr;
    Commands->VSGetConstantBuffers( 0, 1, &KeepVertexConstants );

    ID3D11Buffer* KeepPixelConstants = nullptr;
    Commands->PSGetConstantBuffers( 0, 1, &KeepPixelConstants );

    ID3D11SamplerState* KeepSampler = nullptr;
    Commands->PSGetSamplers( 0, 1, &KeepSampler );

    ID3D11BlendState* KeepBlend = nullptr;
    float KeepFactor[ 4 ] = { };

    unsigned int KeepMask = 0;
    Commands->OMGetBlendState( &KeepBlend, KeepFactor, &KeepMask );

    Bind( Data );

    for ( int Position = 0; Position < Data.BatchCount; Position++ ) {
        const CBatch& Batch = Data.Batches[ Position ];

        if ( Batch.Callback ) {
            Batch.Callback( Commands, Batch.Detail );
            Bind( Data );

            continue;
        }

        if ( Batch.Count <= 0 )
            continue;

        ID3D11ShaderResourceView* Bound = SheetView;

        if ( Batch.Image != 0 ) {
            auto Entry = Images.find( Batch.Image );
            Bound = Entry == Images.end( ) ? nullptr : Entry->second.View;
        }

        if ( !Bound )
            continue;

        float Values[ 12 ] = {
            2.0f / Data.Extent.Horizontal, -2.0f / Data.Extent.Vertical, -1.0f, 1.0f,
            0.0f, Data.Moment, Data.Mouse.Horizontal, Data.Mouse.Vertical,
            Data.PressLeft, Data.PressRight, 0.0f, 0.0f
        };
        ( ( unsigned int* )Values )[ 4 ] = ( unsigned int )Batch.Offset;

        Commands->UpdateSubresource( Constants, 0, nullptr, Values, 0, 0 );

        Commands->PSSetShader( Stage( Batch.Effect ), nullptr, 0 );
        Commands->PSSetShaderResources( 0, 1, &Bound );

        Commands->DrawInstanced( 4, ( unsigned int )Batch.Count, 0, 0 );
    }

    Commands->RSSetState( KeepRasterizer );
    Commands->RSSetViewports( KeepViewports, KeepViewport );

    Commands->IASetInputLayout( KeepLayout );
    Commands->IASetPrimitiveTopology( KeepTopology );

    Commands->VSSetShader( KeepVertex, nullptr, 0 );
    Commands->PSSetShader( KeepPixel, nullptr, 0 );

    Commands->VSSetShaderResources( 0, 1, &KeepVertexView );
    Commands->PSSetShaderResources( 0, 1, &KeepPixelView );

    Commands->VSSetConstantBuffers( 0, 1, &KeepVertexConstants );
    Commands->PSSetConstantBuffers( 0, 1, &KeepPixelConstants );

    Commands->PSSetSamplers( 0, 1, &KeepSampler );
    Commands->OMSetBlendState( KeepBlend, KeepFactor, KeepMask );

    if ( KeepRasterizer )
        KeepRasterizer->Release( );

    if ( KeepLayout )
        KeepLayout->Release( );

    if ( KeepVertex )
        KeepVertex->Release( );

    if ( KeepPixel )
        KeepPixel->Release( );

    if ( KeepVertexView )
        KeepVertexView->Release( );

    if ( KeepPixelView )
        KeepPixelView->Release( );

    if ( KeepVertexConstants )
        KeepVertexConstants->Release( );

    if ( KeepPixelConstants )
        KeepPixelConstants->Release( );

    if ( KeepSampler )
        KeepSampler->Release( );

    if ( KeepBlend )
        KeepBlend->Release( );
}