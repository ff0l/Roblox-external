#include "Arena.h"

bool CArena::Create( size_t Capacity ) {
    Destroy( );

    if ( Capacity == 0 )
        return false;

    CBlock Block;

    Block.Bytes = std::make_unique< unsigned char[] >( Capacity );
    Block.Capacity = Capacity;

    Blocks.push_back( std::move( Block ) );
    return true;
}

void CArena::Destroy( ) {
    Blocks.clear( );
    Offset = 0;
}

void* CArena::Allocate( size_t Size ) {
    size_t Aligned = ( Size + 15 ) & ~( size_t )15;

    if ( Blocks.empty( ) )
        Create( Aligned > 1048576 ? Aligned : 1048576 );

    if ( Offset + Aligned > Blocks.back( ).Capacity ) {
        size_t Doubled = Blocks.back( ).Capacity * 2;
        CBlock Block;

        Block.Capacity = Doubled > Aligned ? Doubled : Aligned;
        Block.Bytes = std::make_unique< unsigned char[] >( Block.Capacity );

        Blocks.push_back( std::move( Block ) );
        Offset = 0;
    }

    void* Result = Blocks.back( ).Bytes.get( ) + Offset;
    Offset += Aligned;

    return Result;
}

void CArena::Reset( ) {
    if ( Blocks.size( ) > 1 ) {
        size_t Total = 0;

        for ( const CBlock& Block : Blocks )
            Total += Block.Capacity;

        Destroy( );
        Create( Total );
    }

    Offset = 0;
}