#pragma once

#include <memory>
#include <string>

class CNative {
public:
    bool Create( void* Window );
    void Destroy( );

    bool Translate( void* Window, unsigned int Message, unsigned long long Primary, long long Secondary );
    float Scale( ) const;

    std::string ClipboardGet( ) const;
    void ClipboardSet( const char* Text ) const;

private:
    void* Handle = nullptr;
    bool Tracking = false;
};

inline auto Native = std::make_unique< CNative >( );