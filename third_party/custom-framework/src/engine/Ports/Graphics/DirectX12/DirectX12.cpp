#include <d3d12.h>
#include <cstring>
#include <d3dcompiler.h>
#include <unordered_map>

#include "Sheet.h"
#include "Context.h"
#include "Shaders.h"
#include "DirectX12.h"

#pragma comment( lib, "d3d12.lib" )
#pragma comment( lib, "d3dcompiler.lib" )

static_assert( sizeof( CInstance ) == 80, "The instance layout must match the shader" );

static constexpr int HeapSlots = 64;

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

static void Transition( ID3D12GraphicsCommandList* Orders, ID3D12Resource* Resource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After ) {
    D3D12_RESOURCE_BARRIER Barrier = { };

    Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Transition.pResource = Resource;

    Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    Barrier.Transition.StateBefore = Before;

    Barrier.Transition.StateAfter = After;
    Orders->ResourceBarrier( 1, &Barrier );
}

CDirectTwelve::~CDirectTwelve( ) {
    Destroy( );
}

ID3D12Resource* CDirectTwelve::CreateUpload( size_t Size ) {
    D3D12_HEAP_PROPERTIES Heap = { };
    Heap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC Description = { };

    Description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    Description.Width = Size;

    Description.Height = 1;
    Description.DepthOrArraySize = 1;

    Description.MipLevels = 1;
    Description.SampleDesc.Count = 1;

    Description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ID3D12Resource* Resource = nullptr;
    Hardware->CreateCommittedResource( &Heap, D3D12_HEAP_FLAG_NONE, &Description, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof( ID3D12Resource ), ( void** )&Resource );

    return Resource;
}

ID3D12Resource* CDirectTwelve::CreateTexture( int Width, int Height, int Kind ) {
    D3D12_HEAP_PROPERTIES Heap = { };
    Heap.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC Description = { };

    Description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    Description.Width = ( unsigned long long )Width;

    Description.Height = ( unsigned int )Height;
    Description.DepthOrArraySize = 1;

    Description.MipLevels = 1;
    Description.Format = ( DXGI_FORMAT )Kind;

    Description.SampleDesc.Count = 1;

    ID3D12Resource* Resource = nullptr;
    Hardware->CreateCommittedResource( &Heap, D3D12_HEAP_FLAG_NONE, &Description, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof( ID3D12Resource ), ( void** )&Resource );

    return Resource;
}

void CDirectTwelve::Bury( ID3D12Resource* Resource ) {
    if ( !Resource )
        return;

    if ( Rings.empty( ) ) {
        Resource->Release( );
        return;
    }

    Rings[ Cursor ].Graveyard.push_back( Resource );
}

bool CDirectTwelve::Create( ID3D12Device* Device, int Frames, int Kind ) {
    Destroy( );

    if ( !Device || Frames < 1 || Frames > 8 )
        return false;

    Hardware = Device;
    Hardware->AddRef( );

    FrameCount = Frames;
    Format = Kind;

    Rings.resize( Frames );
    Increment = Hardware->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    D3D12_DESCRIPTOR_RANGE Range = { };

    Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    Range.NumDescriptors = 1;

    D3D12_ROOT_PARAMETER Parameters[ 3 ] = { };

    Parameters[ 0 ].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    Parameters[ 0 ].Constants.Num32BitValues = 6;
    Parameters[ 0 ].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    Parameters[ 1 ].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    Parameters[ 1 ].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    Parameters[ 2 ].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    Parameters[ 2 ].DescriptorTable.NumDescriptorRanges = 1;

    Parameters[ 2 ].DescriptorTable.pDescriptorRanges = &Range;
    Parameters[ 2 ].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC Filtering = { };

    Filtering.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    Filtering.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    Filtering.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    Filtering.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    Filtering.MaxLOD = 1000.0f;
    Filtering.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC Description = { };

    Description.NumParameters = 3;
    Description.pParameters = Parameters;

    Description.NumStaticSamplers = 1;
    Description.pStaticSamplers = &Filtering;

    ID3DBlob* Serialized = nullptr;
    ID3DBlob* Faults = nullptr;

    if ( FAILED( D3D12SerializeRootSignature( &Description, D3D_ROOT_SIGNATURE_VERSION_1, &Serialized, &Faults ) ) ) {
        if ( Faults ) {
            Context->Report( ( const char* )Faults->GetBufferPointer( ) );
            Faults->Release( );
        }

        Destroy( );
        return false;
    }

    Hardware->CreateRootSignature( 0, Serialized->GetBufferPointer( ), Serialized->GetBufferSize( ), __uuidof( ID3D12RootSignature ), ( void** )&Signature );
    Serialized->Release( );

    if ( !Signature ) {
        Destroy( );
        return false;
    }

    VertexCode = CompileStage( Shaders->Vertex( ShaderHlsl ), "MainVertex", "vs_5_0" );

    if ( !VertexCode || !Pipe( 0 ) ) {
        Destroy( );
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC HeapRecipe = { };

    HeapRecipe.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    HeapRecipe.NumDescriptors = HeapSlots;

    HeapRecipe.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    Hardware->CreateDescriptorHeap( &HeapRecipe, __uuidof( ID3D12DescriptorHeap ), ( void** )&Descriptors );

    if ( !Descriptors ) {
        Destroy( );
        return false;
    }

    return true;
}

ID3D12PipelineState* CDirectTwelve::Pipe( unsigned int Effect ) {
    size_t Wanted = ( size_t )Effect + 1;

    if ( Pipelines.size( ) < Wanted )
        Pipelines.resize( Wanted, nullptr );

    if ( Pipelines[ Effect ] )
        return Pipelines[ Effect ];

    ID3DBlob* PixelCode = CompileStage( Shaders->Pixel( Effect, ShaderHlsl ), "MainFragment", "ps_5_0" );
    if ( !PixelCode )
        return Effect == 0 ? nullptr : Pipe( 0 );

    D3D12_GRAPHICS_PIPELINE_STATE_DESC Recipe = { };

    Recipe.pRootSignature = Signature;

    Recipe.VS.pShaderBytecode = VertexCode->GetBufferPointer( );
    Recipe.VS.BytecodeLength = VertexCode->GetBufferSize( );

    Recipe.PS.pShaderBytecode = PixelCode->GetBufferPointer( );
    Recipe.PS.BytecodeLength = PixelCode->GetBufferSize( );

    Recipe.BlendState.RenderTarget[ 0 ].BlendEnable = TRUE;
    Recipe.BlendState.RenderTarget[ 0 ].SrcBlend = D3D12_BLEND_SRC_ALPHA;

    Recipe.BlendState.RenderTarget[ 0 ].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    Recipe.BlendState.RenderTarget[ 0 ].BlendOp = D3D12_BLEND_OP_ADD;

    Recipe.BlendState.RenderTarget[ 0 ].SrcBlendAlpha = D3D12_BLEND_ONE;
    Recipe.BlendState.RenderTarget[ 0 ].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

    Recipe.BlendState.RenderTarget[ 0 ].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    Recipe.BlendState.RenderTarget[ 0 ].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    Recipe.SampleMask = 0xffffffff;

    Recipe.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    Recipe.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    Recipe.RasterizerState.DepthClipEnable = TRUE;
    Recipe.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    Recipe.NumRenderTargets = 1;
    Recipe.RTVFormats[ 0 ] = ( DXGI_FORMAT )Format;

    Recipe.SampleDesc.Count = 1;

    Hardware->CreateGraphicsPipelineState( &Recipe, __uuidof( ID3D12PipelineState ), ( void** )&Pipelines[ Effect ] );
    PixelCode->Release( );

    if ( !Pipelines[ Effect ] ) {
        Context->Report( "Could not build a DirectX 12 pipeline" );
        return Effect == 0 ? nullptr : Pipe( 0 );
    }

    return Pipelines[ Effect ];
}

void CDirectTwelve::Destroy( ) {
    for ( auto& Entry : Images ) {
        if ( Entry.second.Owned ) {
            if ( Entry.second.Texture )
                Entry.second.Texture->Release( );

            if ( Entry.second.Upload )
                Entry.second.Upload->Release( );
        }
    }

    Images.clear( );

    for ( CTwelveFrame& Ring : Rings ) {
        for ( ID3D12Resource* Resource : Ring.Graveyard )
            Resource->Release( );

        Ring.Graveyard.clear( );

        if ( Ring.Storage ) {
            Ring.Storage->Release( );
            Ring.Storage = nullptr;
        }
    }

    Rings.clear( );

    for ( ID3D12PipelineState* Piece : Pipelines ) {
        if ( Piece )
            Piece->Release( );
    }

    Pipelines.clear( );

    if ( VertexCode ) {
        VertexCode->Release( );
        VertexCode = nullptr;
    }

    if ( SheetUpload ) {
        SheetUpload->Release( );
        SheetUpload = nullptr;
    }

    if ( SheetTexture ) {
        SheetTexture->Release( );
        SheetTexture = nullptr;
    }

    if ( Descriptors ) {
        Descriptors->Release( );
        Descriptors = nullptr;
    }

    if ( Signature ) {
        Signature->Release( );
        Signature = nullptr;
    }

    if ( Hardware ) {
        Hardware->Release( );
        Hardware = nullptr;
    }

    FreeSlots.clear( );
    NextImage = 1;

    SheetVersion = 0;
    SheetDepth = 0;

    SheetShown = false;

    FrameCount = 0;
    Cursor = 0;

    NextSlot = 1;
}

int CDirectTwelve::TakeSlot( ) {
    if ( !FreeSlots.empty( ) ) {
        int Slot = FreeSlots.back( );
        FreeSlots.pop_back( );

        return Slot;
    }

    if ( NextSlot >= HeapSlots )
        return 0;

    int Slot = NextSlot;
    NextSlot++;

    return Slot;
}

bool CDirectTwelve::Reserve( CTwelveFrame& Ring, int Count ) {
    if ( Count <= Ring.Capacity )
        return true;

    int Goal = Ring.Capacity > 0 ? Ring.Capacity : 16384;
    while ( Goal < Count )
        Goal *= 2;

    if ( Ring.Storage ) {
        Ring.Storage->Release( );

        Ring.Storage = nullptr;
        Ring.Mapped = nullptr;
    }

    Ring.Capacity = 0;
    Ring.Storage = CreateUpload( ( size_t )Goal * sizeof( CInstance ) );

    if ( !Ring.Storage ) {
        Context->Report( "Could not grow the DirectX 12 instance ring" );
        return false;
    }

    D3D12_RANGE Nothing = { };
    if ( FAILED( Ring.Storage->Map( 0, &Nothing, ( void** )&Ring.Mapped ) ) )
        return false;

    Ring.Capacity = Goal;
    return true;
}

void CDirectTwelve::FillUpload( ID3D12Resource* Upload, const unsigned char* Pixels, int Width, int Height, int Stride ) {
    unsigned char* Mapped = nullptr;
    D3D12_RANGE Nothing = { };

    if ( FAILED( Upload->Map( 0, &Nothing, ( void** )&Mapped ) ) )
        return;

    int Aligned = ( Width * Stride + 255 ) & ~255;

    for ( int Row = 0; Row < Height; Row++ )
        memcpy( Mapped + ( size_t )Row * Aligned, Pixels + ( size_t )Row * Width * Stride, ( size_t )Width * Stride );

    Upload->Unmap( 0, nullptr );
}

void CDirectTwelve::Refresh( ID3D12GraphicsCommandList* Orders ) {
    if ( Sheet->Depth( ) > 0 && ( !SheetTexture || SheetDepth != Sheet->Depth( ) ) ) {
        Bury( SheetTexture );
        Bury( SheetUpload );

        SheetTexture = CreateTexture( Sheet->Extent( ), Sheet->Depth( ), ( int )DXGI_FORMAT_R8_UNORM );

        int Aligned = ( Sheet->Extent( ) + 255 ) & ~255;
        SheetUpload = CreateUpload( ( size_t )Aligned * Sheet->Depth( ) );

        SheetDepth = Sheet->Depth( );
        SheetShown = false;

        SheetVersion = 0;

        if ( SheetTexture ) {
            D3D12_CPU_DESCRIPTOR_HANDLE Handle = Descriptors->GetCPUDescriptorHandleForHeapStart( );
            Hardware->CreateShaderResourceView( SheetTexture, nullptr, Handle );
        }
    }

    if ( SheetTexture && SheetUpload && SheetVersion != Sheet->Version ) {
        FillUpload( SheetUpload, Sheet->Data( ), Sheet->Extent( ), Sheet->Depth( ), 1 );

        if ( SheetShown )
            Transition( Orders, SheetTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST );

        D3D12_TEXTURE_COPY_LOCATION Target = { };

        Target.pResource = SheetTexture;
        Target.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        D3D12_TEXTURE_COPY_LOCATION Origin = { };

        Origin.pResource = SheetUpload;
        Origin.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

        Origin.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8_UNORM;
        Origin.PlacedFootprint.Footprint.Width = ( unsigned int )Sheet->Extent( );

        Origin.PlacedFootprint.Footprint.Height = ( unsigned int )Sheet->Depth( );
        Origin.PlacedFootprint.Footprint.Depth = 1;

        Origin.PlacedFootprint.Footprint.RowPitch = ( unsigned int )( ( Sheet->Extent( ) + 255 ) & ~255 );

        Orders->CopyTextureRegion( &Target, 0, 0, 0, &Origin, nullptr );
        Transition( Orders, SheetTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );

        SheetShown = true;
        SheetVersion = Sheet->Version;
    }

    for ( auto& Entry : Images ) {
        CTwelveImage& Image = Entry.second;

        if ( !Image.Dirty || !Image.Texture || !Image.Upload )
            continue;

        if ( Image.Shown )
            Transition( Orders, Image.Texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST );

        D3D12_TEXTURE_COPY_LOCATION Target = { };

        Target.pResource = Image.Texture;
        Target.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        D3D12_TEXTURE_COPY_LOCATION Origin = { };

        Origin.pResource = Image.Upload;
        Origin.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

        Origin.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        Origin.PlacedFootprint.Footprint.Width = ( unsigned int )Image.Width;

        Origin.PlacedFootprint.Footprint.Height = ( unsigned int )Image.Height;
        Origin.PlacedFootprint.Footprint.Depth = 1;

        Origin.PlacedFootprint.Footprint.RowPitch = ( unsigned int )( ( Image.Width * 4 + 255 ) & ~255 );

        Orders->CopyTextureRegion( &Target, 0, 0, 0, &Origin, nullptr );
        Transition( Orders, Image.Texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );

        Image.Shown = true;
        Image.Dirty = false;
    }
}

unsigned long long CDirectTwelve::CreateImage( const unsigned char* Pixels, int Width, int Height ) {
    if ( !Hardware || !Pixels || Width <= 0 || Height <= 0 )
        return 0;

    if ( Width > 16384 || Height > 16384 ) {
        Context->Report( "Rejected an image with unreasonable dimensions" );
        return 0;
    }

    int Slot = TakeSlot( );
    if ( Slot == 0 ) {
        Context->Report( "Ran out of DirectX 12 image slots" );
        return 0;
    }

    CTwelveImage Image;

    Image.Width = Width;
    Image.Height = Height;

    Image.Slot = Slot;
    Image.Owned = true;

    Image.Texture = CreateTexture( Width, Height, ( int )DXGI_FORMAT_R8G8B8A8_UNORM );

    int Aligned = ( Width * 4 + 255 ) & ~255;
    Image.Upload = CreateUpload( ( size_t )Aligned * Height );

    if ( !Image.Texture || !Image.Upload ) {
        Bury( Image.Texture );
        Bury( Image.Upload );

        FreeSlots.push_back( Slot );
        return 0;
    }

    FillUpload( Image.Upload, Pixels, Width, Height, 4 );
    Image.Dirty = true;

    D3D12_CPU_DESCRIPTOR_HANDLE Handle = Descriptors->GetCPUDescriptorHandleForHeapStart( );
    Handle.ptr += ( size_t )Slot * Increment;

    Hardware->CreateShaderResourceView( Image.Texture, nullptr, Handle );

    unsigned long long Result = NextImage;
    NextImage++;

    Images[ Result ] = Image;
    return Result;
}

void CDirectTwelve::UpdateImage( unsigned long long Image, const unsigned char* Pixels ) {
    auto Entry = Images.find( Image );
    if ( Entry == Images.end( ) || !Entry->second.Owned || !Pixels )
        return;

    CTwelveImage& Picture = Entry->second;
    int Aligned = ( Picture.Width * 4 + 255 ) & ~255;

    ID3D12Resource* Fresh = CreateUpload( ( size_t )Aligned * Picture.Height );
    if ( !Fresh )
        return;

    Bury( Picture.Upload );
    Picture.Upload = Fresh;

    FillUpload( Picture.Upload, Pixels, Picture.Width, Picture.Height, 4 );
    Picture.Dirty = true;
}

unsigned long long CDirectTwelve::AdoptImage( void* Native ) {
    if ( !Native || !Hardware )
        return 0;

    int Slot = TakeSlot( );
    if ( Slot == 0 )
        return 0;

    CTwelveImage Image;

    Image.Slot = Slot;
    Image.Owned = false;

    Image.Shown = true;

    D3D12_CPU_DESCRIPTOR_HANDLE Handle = Descriptors->GetCPUDescriptorHandleForHeapStart( );
    Handle.ptr += ( size_t )Slot * Increment;

    Hardware->CreateShaderResourceView( ( ID3D12Resource* )Native, nullptr, Handle );

    unsigned long long Result = NextImage;
    NextImage++;

    Images[ Result ] = Image;
    return Result;
}

void CDirectTwelve::DestroyImage( unsigned long long Image ) {
    auto Entry = Images.find( Image );
    if ( Entry == Images.end( ) )
        return;

    if ( Entry->second.Owned ) {
        Bury( Entry->second.Texture );
        Bury( Entry->second.Upload );
    }

    FreeSlots.push_back( Entry->second.Slot );
    Images.erase( Entry );
}

void CDirectTwelve::Bind( ID3D12GraphicsCommandList* Orders, const CDrawData& Data ) {
    Orders->SetGraphicsRootSignature( Signature );
    Orders->SetDescriptorHeaps( 1, &Descriptors );

    Orders->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

    D3D12_VIEWPORT Viewport = { };

    Viewport.Width = Data.Extent.Horizontal;
    Viewport.Height = Data.Extent.Vertical;

    Viewport.MaxDepth = 1.0f;
    Orders->RSSetViewports( 1, &Viewport );

    D3D12_RECT Scissor = { };

    Scissor.right = ( long )Data.Extent.Horizontal;
    Scissor.bottom = ( long )Data.Extent.Vertical;

    Orders->RSSetScissorRects( 1, &Scissor );

    float Values[ 6 ] = { 2.0f / Data.Extent.Horizontal, -2.0f / Data.Extent.Vertical, -1.0f, 1.0f, 0.0f, Data.Moment };
    Orders->SetGraphicsRoot32BitConstants( 0, 6, Values, 0 );

    if ( !Rings.empty( ) && Rings[ Cursor ].Storage )
        Orders->SetGraphicsRootShaderResourceView( 1, Rings[ Cursor ].Storage->GetGPUVirtualAddress( ) );
}

void CDirectTwelve::Render( const CDrawData& Data, void* Stream ) {
    ID3D12GraphicsCommandList* Orders = ( ID3D12GraphicsCommandList* )Stream;

    if ( !Hardware || !Orders || Rings.empty( ) || Data.Extent.Horizontal <= 0.0f || Data.Extent.Vertical <= 0.0f )
        return;

    Cursor = ( Cursor + 1 ) % FrameCount;
    CTwelveFrame& Ring = Rings[ Cursor ];

    for ( ID3D12Resource* Resource : Ring.Graveyard )
        Resource->Release( );

    Ring.Graveyard.clear( );
    Refresh( Orders );

    if ( Data.BatchCount <= 0 )
        return;

    if ( Data.Count > 0 && Data.Items ) {
        if ( !Reserve( Ring, Data.Count ) )
            return;

        memcpy( Ring.Mapped, Data.Items, ( size_t )Data.Count * sizeof( CInstance ) );
    }

    Bind( Orders, Data );

    for ( int Position = 0; Position < Data.BatchCount; Position++ ) {
        const CBatch& Batch = Data.Batches[ Position ];

        if ( Batch.Callback ) {
            Batch.Callback( Orders, Batch.Detail );
            Bind( Orders, Data );

            continue;
        }

        if ( Batch.Count <= 0 )
            continue;

        int Slot = 0;

        if ( Batch.Image != 0 ) {
            auto Entry = Images.find( Batch.Image );
            if ( Entry == Images.end( ) )
                continue;

            Slot = Entry->second.Slot;
        } else if ( !SheetTexture ) {
            continue;
        }

        ID3D12PipelineState* Chosen = Pipe( Batch.Effect );
        if ( !Chosen )
            continue;

        D3D12_GPU_DESCRIPTOR_HANDLE Handle = Descriptors->GetGPUDescriptorHandleForHeapStart( );
        Handle.ptr += ( unsigned long long )Slot * Increment;

        Orders->SetPipelineState( Chosen );
        Orders->SetGraphicsRootDescriptorTable( 2, Handle );

        Orders->SetGraphicsRoot32BitConstant( 0, ( unsigned int )Batch.Offset, 4 );
        Orders->DrawInstanced( 4, ( unsigned int )Batch.Count, 0, 0 );
    }
}