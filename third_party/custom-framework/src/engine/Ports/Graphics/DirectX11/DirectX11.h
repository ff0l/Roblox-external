#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "Canvas.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11BlendState;
struct ID3D11RasterizerState;
struct ID3D11SamplerState;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;

class CElevenImage {
public:
    ID3D11ShaderResourceView* View = nullptr;

    int Width = 0;
    int Height = 0;

    bool Owned = false;
};

class CDirectEleven : public CGraphics {
public:
    ~CDirectEleven( );

    bool Create( ID3D11Device* Device, ID3D11DeviceContext* Orders );
    void Destroy( ) override;

    void Render( const CDrawData& Data, void* Stream ) override;

    unsigned long long CreateImage( const unsigned char* Pixels, int Width, int Height ) override;
    void UpdateImage( unsigned long long Image, const unsigned char* Pixels ) override;

    unsigned long long AdoptImage( void* Native ) override;
    void DestroyImage( unsigned long long Image ) override;

private:
    bool Reserve( int Count );
    void Refresh( );

    void Bind( const CDrawData& Data );
    ID3D11PixelShader* Stage( unsigned int Effect );

    ID3D11Device* Hardware = nullptr;
    ID3D11DeviceContext* Commands = nullptr;

    ID3D11VertexShader* VertexStage = nullptr;
    std::vector< ID3D11PixelShader* > Stages;

    ID3D11BlendState* Blending = nullptr;
    ID3D11RasterizerState* Rasterizer = nullptr;

    ID3D11SamplerState* Sampler = nullptr;
    ID3D11Buffer* Constants = nullptr;

    ID3D11Buffer* Storage = nullptr;
    ID3D11ShaderResourceView* View = nullptr;

    ID3D11ShaderResourceView* SheetView = nullptr;
    std::unordered_map< unsigned long long, CElevenImage > Images;

    unsigned long long NextImage = 1;
    unsigned int SheetVersion = 0;

    int SheetDepth = 0;
    int Capacity = 0;
};

inline auto Eleven = std::make_unique< CDirectEleven >( );