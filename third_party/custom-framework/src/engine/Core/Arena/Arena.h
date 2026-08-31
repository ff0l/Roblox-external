#pragma once

#include <memory>
#include <vector>

class CBlock {
public:
    std::unique_ptr< unsigned char[] > Bytes;
    size_t Capacity = 0;
};

class CArena {
public:
    bool Create( size_t Capacity );
    void Destroy( );

    void* Allocate( size_t Size );
    void Reset( );

private:
    std::vector< CBlock > Blocks;
    size_t Offset = 0;
};

inline auto Arena = std::make_unique< CArena >( );