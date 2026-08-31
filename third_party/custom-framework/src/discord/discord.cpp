#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "ur/discord.hpp"
#include "ur/config.hpp"

#include <Windows.h>
#include <mutex>
#include <sstream>
#include <atomic>

namespace ur {
namespace discord {

enum Opcode {
    Handshake = 0,
    Frame = 1,
    Close = 2,
    Ping = 3,
    Pong = 4
};

static HANDLE Pipe = INVALID_HANDLE_VALUE;
static std::mutex Guard;
static std::atomic< bool > Enabled{ false };
static std::atomic< bool > Connected{ false };
static std::string AppId;
static Presence Last;
static unsigned int Nonce = 1;
static DWORD Pid = 0;

static std::string Escape( const std::string& Text ) {
    std::string Out;
    Out.reserve( Text.size( ) + 8 );
    for ( char Ch : Text ) {
        if ( Ch == '"' || Ch == '\\' )
            Out.push_back( '\\' );
        if ( Ch == '\n' ) {
            Out += "\\n";
            continue;
        }
        Out.push_back( Ch );
    }
    return Out;
}

static bool WriteRaw( Opcode Code, const std::string& Json ) {
    if ( Pipe == INVALID_HANDLE_VALUE )
        return false;

    unsigned int Header[ 2 ] = { ( unsigned int )Code, ( unsigned int )Json.size( ) };
    DWORD Written = 0;
    if ( !WriteFile( Pipe, Header, 8, &Written, nullptr ) || Written != 8 )
        return false;
    if ( Json.empty( ) )
        return true;
    return WriteFile( Pipe, Json.data( ), ( DWORD )Json.size( ), &Written, nullptr ) && Written == Json.size( );
}

static bool ReadRaw( Opcode& Code, std::string& Json ) {
    unsigned int Header[ 2 ] = { };
    DWORD Read = 0;
    if ( !ReadFile( Pipe, Header, 8, &Read, nullptr ) || Read != 8 )
        return false;
    Code = ( Opcode )Header[ 0 ];
    unsigned int Size = Header[ 1 ];
    if ( Size > 65536 )
        return false;
    Json.assign( Size, '\0' );
    if ( Size == 0 )
        return true;
    return ReadFile( Pipe, Json.data( ), Size, &Read, nullptr ) && Read == Size;
}

static void Disconnect( ) {
    if ( Pipe != INVALID_HANDLE_VALUE ) {
        CloseHandle( Pipe );
        Pipe = INVALID_HANDLE_VALUE;
    }
    Connected = false;
}

static bool ConnectPipe( ) {
    Disconnect( );
    for ( int Index = 0; Index < 10; Index++ ) {
        char Name[ 64 ] = { };
        wsprintfA( Name, "\\\\.\\pipe\\discord-ipc-%d", Index );
        Pipe = CreateFileA( Name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr );
        if ( Pipe != INVALID_HANDLE_VALUE )
            return true;
    }
    return false;
}

static bool HandshakeNow( ) {
    std::ostringstream Json;
    Json << "{\"v\":1,\"client_id\":\"" << Escape( AppId ) << "\"}";
    if ( !WriteRaw( Handshake, Json.str( ) ) )
        return false;

    Opcode Code = Close;
    std::string Reply;
    DWORD Mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
    SetNamedPipeHandleState( Pipe, &Mode, nullptr, nullptr );
    DWORD Began = GetTickCount( );
    while ( GetTickCount( ) - Began < 1500 ) {
        if ( ReadRaw( Code, Reply ) )
            break;
        Sleep( 20 );
    }
    Mode = PIPE_READMODE_BYTE;
    SetNamedPipeHandleState( Pipe, &Mode, nullptr, nullptr );
    return Code == Frame || Code == Handshake;
}

static std::string BuildActivity( const Presence& Item ) {
    std::ostringstream Json;
    Json << "{\"cmd\":\"SET_ACTIVITY\",\"nonce\":\"" << Nonce++ << "\",\"args\":{\"pid\":" << Pid << ",\"activity\":{";
    bool Comma = false;
    auto Field = [ & ]( const char* Name, const std::string& Value ) {
        if ( Value.empty( ) )
            return;
        if ( Comma )
            Json << ",";
        Json << "\"" << Name << "\":\"" << Escape( Value ) << "\"";
        Comma = true;
    };
    Field( "details", Item.details );
    Field( "state", Item.state );
    if ( !Item.large_image.empty( ) || !Item.small_image.empty( ) ) {
        if ( Comma )
            Json << ",";
        Json << "\"assets\":{";
        bool Inner = false;
        auto Asset = [ & ]( const char* Name, const std::string& Value ) {
            if ( Value.empty( ) )
                return;
            if ( Inner )
                Json << ",";
            Json << "\"" << Name << "\":\"" << Escape( Value ) << "\"";
            Inner = true;
        };
        Asset( "large_image", Item.large_image );
        Asset( "large_text", Item.large_text );
        Asset( "small_image", Item.small_image );
        Asset( "small_text", Item.small_text );
        Json << "}";
        Comma = true;
    }
    if ( Item.start > 0 || Item.end > 0 ) {
        if ( Comma )
            Json << ",";
        Json << "\"timestamps\":{";
        if ( Item.start > 0 )
            Json << "\"start\":" << Item.start;
        if ( Item.end > 0 ) {
            if ( Item.start > 0 )
                Json << ",";
            Json << "\"end\":" << Item.end;
        }
        Json << "}";
    }
    Json << "}}}";
    return Json.str( );
}

static bool SendPresence( const Presence& Item ) {
    if ( Pipe == INVALID_HANDLE_VALUE )
        return false;
    return WriteRaw( Frame, BuildActivity( Item ) );
}

void enable( bool On ) {
    Enabled = On;
    if ( !On ) {
        std::lock_guard< std::mutex > Lock( Guard );
        Disconnect( );
        return;
    }
    if ( AppId.empty( ) )
        AppId = config::get( "UR_DISCORD_APP_ID" );
}

bool enabled( ) {
    return Enabled;
}

void set_app_id( const char* Id ) {
    AppId = Id ? Id : "";
}

bool connected( ) {
    return Connected;
}

void set_presence( const Presence& Item ) {
    Last = Item;
    if ( !Enabled || AppId.empty( ) )
        return;
    std::lock_guard< std::mutex > Lock( Guard );
    if ( Connected )
        SendPresence( Item );
}

void clear( ) {
    Last = Presence{ };
    if ( !Connected )
        return;
    std::lock_guard< std::mutex > Lock( Guard );
    std::ostringstream Json;
    Json << "{\"cmd\":\"SET_ACTIVITY\",\"nonce\":\"" << Nonce++ << "\",\"args\":{\"pid\":" << Pid << ",\"activity\":null}}";
    WriteRaw( Frame, Json.str( ) );
}

void tick( ) {
    if ( !Enabled || AppId.empty( ) ) {
        if ( Connected ) {
            std::lock_guard< std::mutex > Lock( Guard );
            Disconnect( );
        }
        return;
    }

    std::lock_guard< std::mutex > Lock( Guard );
    if ( !Connected ) {
        Pid = GetCurrentProcessId( );
        if ( ConnectPipe( ) && HandshakeNow( ) ) {
            Connected = true;
            if ( !Last.details.empty( ) || !Last.state.empty( ) )
                SendPresence( Last );
        } else {
            Disconnect( );
        }
        return;
    }

    DWORD Avail = 0;
    if ( PeekNamedPipe( Pipe, nullptr, 0, nullptr, &Avail, nullptr ) && Avail >= 8 ) {
        Opcode Code = Close;
        std::string Json;
        if ( !ReadRaw( Code, Json ) || Code == Close )
            Disconnect( );
        else if ( Code == Ping )
            WriteRaw( Pong, Json );
    }
}

void shutdown( ) {
    Enabled = false;
    std::lock_guard< std::mutex > Lock( Guard );
    Disconnect( );
}

}
}
