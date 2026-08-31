#pragma once

#include <memory>
#include <string>
#include <vector>

inline constexpr int ShaderHlsl = 0;
inline constexpr int ShaderGlsl = 1;
inline constexpr int ShaderVulkan = 2;
inline constexpr int ShaderMetal = 3;

class CEffect {
public:
    std::string Name;
    std::string Body;
};

class CShaders {
public:
    unsigned int Compose( const char* Name, const char* Body );

    int Count( ) const;
    const CEffect* Fetch( unsigned int Effect ) const;

    std::string Vertex( int Language ) const;
    std::string Pixel( unsigned int Effect, int Language ) const;

    unsigned int Version = 1;

private:
    std::vector< CEffect > Effects;
};

inline auto Shaders = std::make_unique< CShaders >( );