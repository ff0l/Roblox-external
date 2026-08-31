#pragma once

#include <memory>
#include <vector>

class CPictures {
public:
    bool Load( const char* Path, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Longest = 0 );
    bool Decode( const unsigned char* Bytes, size_t Length, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Longest = 0 );

    bool Vector( const char* Path, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Size );
    bool VectorBytes( const unsigned char* Bytes, size_t Length, std::vector< unsigned char >& Pixels, int& Width, int& Height, int Size );
};

inline auto Pictures = std::make_unique< CPictures >( );