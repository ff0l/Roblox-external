#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "Canvas.h"

struct ID3D12Device;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct ID3D12GraphicsCommandList;
struct ID3D10Blob;

class CTwelveFrame {
public:
    ID3D12Resource* Storage = nullptr;
    unsigned char* Mapped = nullptr;

    int Capacity = 0;
    std::vector< ID3D12Resource* > Graveyard;
};

class CTwelveImage {
public:
    ID3D12Resource* Texture = nullptr;
    ID3D12Resource* Upload = nullptr;

    int Width = 0;
    int Height = 0;

    int Slot = 0;

    bool Owned = false;
    bool Dirty = false;
    bool Shown = false;
};

class CDirectTwelve : public CGraphics {
public:
    ~CDirectTwelve( );

    bool Create( ID3D12Device* Device, int Frames, int Kind );
    void Destroy( ) override;

    void Render( const CDrawData& Data, void* Stream ) override;

    unsigned long long CreateImage( const unsigned char* Pixels, int Width, int Height ) override;
    void UpdateImage( unsigned long long Image, const unsigned char* Pixels ) override;

    unsigned long long AdoptImage( void* Native ) override;
    void DestroyImage( unsigned long long Image ) override;

private:
    bool Reserve( CTwelveFrame& Ring, int Count );
    void Refresh( ID3D12GraphicsCommandList* Orders );

    void Bind( ID3D12GraphicsCommandList* Orders, const CDrawData& Data );
    ID3D12PipelineState* Pipe( unsigned int Effect );

    int TakeSlot( );
    void FillUpload( ID3D12Resource* Upload, const unsigned char* Pixels, int Width, int Height, int Stride );

    ID3D12Resource* CreateUpload( size_t Size );
    ID3D12Resource* CreateTexture( int Width, int Height, int Kind );

    void Bury( ID3D12Resource* Resource );

    ID3D12Device* Hardware = nullptr;
    ID3D12RootSignature* Signature = nullptr;

    std::vector< ID3D12PipelineState* > Pipelines;
    ID3D10Blob* VertexCode = nullptr;

    ID3D12DescriptorHeap* Descriptors = nullptr;

    ID3D12Resource* SheetTexture = nullptr;
    ID3D12Resource* SheetUpload = nullptr;

    std::vector< CTwelveFrame > Rings;
    std::unordered_map< unsigned long long, CTwelveImage > Images;

    std::vector< int > FreeSlots;
    unsigned long long NextImage = 1;

    unsigned int SheetVersion = 0;
    int SheetDepth = 0;

    bool SheetShown = false;

    int FrameCount = 0;
    int Cursor = 0;

    int NextSlot = 1;
    int Format = 0;

    unsigned int Increment = 0;
};

inline auto Twelve = std::make_unique< CDirectTwelve >( );