#pragma once

#include <cstdint>
#include <string>

namespace ur {
namespace discord {

struct Presence {
    std::string details;
    std::string state;
    std::string large_image;
    std::string large_text;
    std::string small_image;
    std::string small_text;
    std::int64_t start = 0;
    std::int64_t end = 0;
};

void enable( bool on );
bool enabled( );
void set_app_id( const char* id );
bool connected( );
void set_presence( const Presence& presence );
void clear( );
void tick( );
void shutdown( );

}
}
