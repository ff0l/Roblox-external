#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ur {
namespace media {

enum class Source {
    None,
    System,
    Spotify
};

struct Track {
    std::string id;
    std::string title;
    std::string artist;
    std::string album;
    std::string app;
    double position = 0.0;
    double duration = 0.0;
    bool playing = false;
    bool can_play = false;
    bool can_pause = false;
    bool can_next = false;
    bool can_prev = false;
    bool can_seek = false;
    std::vector< unsigned char > art;
    int art_width = 0;
    int art_height = 0;
    Source source = Source::None;
};

void start( );
void shutdown( );
void tick( );
void refresh( );

const Track& current( );

bool play( );
bool pause( );
bool toggle( );
bool next( );
bool prev( );
bool seek( double seconds );

bool spotify_configured( );
bool spotify_ready( );
void spotify_connect( );
void prefer_spotify( bool on );

}
}
