#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS

#include "ur/media.hpp"
#include "ur/config.hpp"

#include <Winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <shellapi.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <wrl/client.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Storage.Streams.h>

#include "Pictures.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <atomic>

#pragma comment( lib, "bcrypt.lib" )

namespace ur {
namespace media {

using winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager;
using winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession;
using winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus;
using winrt::Windows::Storage::Streams::DataReader;
using winrt::Windows::Storage::Streams::InputStreamOptions;

static std::mutex Guard;
static Track Now;
static Track Published;
static bool Started = false;
static bool PreferSpotify = false;
static bool SpotifyReady = false;
static std::atomic< bool > StopWorker{ false };
static std::thread Worker;
static std::atomic< int > Command{ 0 };
static std::atomic< double > SeekTo{ -1.0 };
static std::string AccessToken;
static std::string RefreshToken;
static std::chrono::steady_clock::time_point TokenExpiry;
static GlobalSystemMediaTransportControlsSessionManager Manager{ nullptr };
static GlobalSystemMediaTransportControlsSession Session{ nullptr };

static std::string WideToUtf8( const std::wstring& Text ) {
    if ( Text.empty( ) )
        return { };
    int Need = WideCharToMultiByte( CP_UTF8, 0, Text.c_str( ), ( int )Text.size( ), nullptr, 0, nullptr, nullptr );
    std::string Out( Need, '\0' );
    WideCharToMultiByte( CP_UTF8, 0, Text.c_str( ), ( int )Text.size( ), Out.data( ), Need, nullptr, nullptr );
    return Out;
}

static std::string Hstring( const winrt::hstring& Text ) {
    return WideToUtf8( std::wstring( Text ) );
}

static std::string TokenPath( ) {
    char Root[ MAX_PATH ] = { };
    ExpandEnvironmentStringsA( "%APPDATA%\\UR", Root, MAX_PATH );
    CreateDirectoryA( Root, nullptr );
    return std::string( Root ) + "\\spotify.json";
}

static std::string JsonField( const std::string& Body, const char* Key ) {
    std::string Needle = std::string( "\"" ) + Key + "\":";
    size_t At = Body.find( Needle );
    if ( At == std::string::npos )
        return { };
    At += Needle.size( );
    while ( At < Body.size( ) && ( Body[ At ] == ' ' ) )
        At++;
    if ( At < Body.size( ) && Body[ At ] == '"' ) {
        At++;
        size_t End = At;
        while ( End < Body.size( ) && Body[ End ] != '"' )
            End++;
        return Body.substr( At, End - At );
    }
    size_t End = At;
    while ( End < Body.size( ) && Body[ End ] != ',' && Body[ End ] != '}' && Body[ End ] != ' ' )
        End++;
    return Body.substr( At, End - At );
}

static void SaveTokens( ) {
    std::ofstream Stream( TokenPath( ) );
    if ( !Stream )
        return;
    Stream << "{\"access_token\":\"" << AccessToken << "\",\"refresh_token\":\"" << RefreshToken << "\"}";
}

static void LoadTokens( ) {
    std::ifstream Stream( TokenPath( ) );
    if ( !Stream )
        return;
    std::string Body( ( std::istreambuf_iterator< char >( Stream ) ), std::istreambuf_iterator< char >( ) );
    AccessToken = JsonField( Body, "access_token" );
    RefreshToken = JsonField( Body, "refresh_token" );
    SpotifyReady = !AccessToken.empty( );
}

static std::string Base64Url( const unsigned char* Data, size_t Size ) {
    static const char* Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string Out;
    int Val = 0;
    int Valb = -6;
    for ( size_t i = 0; i < Size; i++ ) {
        Val = ( Val << 8 ) + Data[ i ];
        Valb += 8;
        while ( Valb >= 0 ) {
            Out.push_back( Table[ ( Val >> Valb ) & 0x3F ] );
            Valb -= 6;
        }
    }
    if ( Valb > -6 )
        Out.push_back( Table[ ( ( Val << 8 ) >> ( Valb + 8 ) ) & 0x3F ] );
    return Out;
}

static std::string RandomUrl( size_t Length ) {
    std::random_device Rd;
    std::mt19937 Gen( Rd( ) );
    std::uniform_int_distribution< int > Dist( 0, 61 );
    static const char* Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string Out( Length, 'x' );
    for ( size_t i = 0; i < Length; i++ )
        Out[ i ] = Table[ Dist( Gen ) ];
    return Out;
}

static std::string Sha256B64( const std::string& Text ) {
    BCRYPT_ALG_HANDLE Alg = nullptr;
    BCRYPT_HASH_HANDLE Hash = nullptr;
    unsigned char Digest[ 32 ] = { };
    if ( BCryptOpenAlgorithmProvider( &Alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0 ) < 0 )
        return { };
    if ( BCryptCreateHash( Alg, &Hash, nullptr, 0, nullptr, 0, 0 ) < 0 ) {
        BCryptCloseAlgorithmProvider( Alg, 0 );
        return { };
    }
    BCryptHashData( Hash, ( PUCHAR )Text.data( ), ( ULONG )Text.size( ), 0 );
    BCryptFinishHash( Hash, Digest, 32, 0 );
    BCryptDestroyHash( Hash );
    BCryptCloseAlgorithmProvider( Alg, 0 );
    return Base64Url( Digest, 32 );
}

static std::string Http( const wchar_t* Host, const wchar_t* Path, const wchar_t* Method, const std::string& Body, const wchar_t* Auth, INTERNET_PORT Port, bool Secure ) {
    HINTERNET HttpSession = WinHttpOpen( L"UR/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
    if ( !HttpSession )
        return { };
    HINTERNET Connect = WinHttpConnect( HttpSession, Host, Port, 0 );
    if ( !Connect ) {
        WinHttpCloseHandle( HttpSession );
        return { };
    }
    DWORD Flags = Secure ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET Request = WinHttpOpenRequest( Connect, Method, Path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, Flags );
    if ( !Request ) {
        WinHttpCloseHandle( Connect );
        WinHttpCloseHandle( HttpSession );
        return { };
    }
    std::wstring Headers = L"Content-Type: application/x-www-form-urlencoded\r\n";
    if ( Auth && *Auth ) {
        Headers += L"Authorization: Bearer ";
        Headers += Auth;
        Headers += L"\r\n";
    }
    BOOL Ok = WinHttpSendRequest( Request, Headers.c_str( ), ( DWORD )-1, ( LPVOID )Body.data( ), ( DWORD )Body.size( ), ( DWORD )Body.size( ), 0 );
    if ( Ok )
        Ok = WinHttpReceiveResponse( Request, nullptr );
    std::string Result;
    if ( Ok ) {
        DWORD Avail = 0;
        while ( WinHttpQueryDataAvailable( Request, &Avail ) && Avail ) {
            std::string Chunk( Avail, '\0' );
            DWORD Got = 0;
            WinHttpReadData( Request, Chunk.data( ), Avail, &Got );
            Result.append( Chunk.data( ), Got );
        }
    }
    WinHttpCloseHandle( Request );
    WinHttpCloseHandle( Connect );
    WinHttpCloseHandle( HttpSession );
    return Result;
}

static std::wstring Utf16( const std::string& Text ) {
    if ( Text.empty( ) )
        return { };
    int Need = MultiByteToWideChar( CP_UTF8, 0, Text.c_str( ), ( int )Text.size( ), nullptr, 0 );
    std::wstring Out( Need, L'\0' );
    MultiByteToWideChar( CP_UTF8, 0, Text.c_str( ), ( int )Text.size( ), Out.data( ), Need );
    return Out;
}

static bool SpotifyCommand( const wchar_t* Path, const wchar_t* Method ) {
    if ( AccessToken.empty( ) )
        return false;
    std::string Reply = Http( L"api.spotify.com", Path, Method, { }, Utf16( AccessToken ).c_str( ), INTERNET_DEFAULT_HTTPS_PORT, true );
    return Reply.find( "error" ) == std::string::npos || Reply.empty( );
}

static void DecodeArt( Track& Item, const unsigned char* Bytes, size_t Length ) {
    if ( !Bytes || Length == 0 )
        return;
    Pictures->Decode( Bytes, Length, Item.art, Item.art_width, Item.art_height, 0 );
}

static bool PullSession( Track& Item ) {
    if ( !Manager )
        return false;

    auto Sessions = Manager.GetSessions( );
    GlobalSystemMediaTransportControlsSession Best{ nullptr };
    int BestScore = -100;
    for ( auto Candidate : Sessions ) {
        std::string App = Hstring( Candidate.SourceAppUserModelId( ) );
        auto Playback = Candidate.GetPlaybackInfo( );
        auto Props = Candidate.TryGetMediaPropertiesAsync( ).get( );
        std::string Title = Hstring( Props.Title( ) );
        bool Playing = Playback.PlaybackStatus( ) == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
        int Score = 0;
        if ( Playing )
            Score += 5;
        if ( !Title.empty( ) )
            Score += 3;
        if ( App.find( "Spotify" ) != std::string::npos || App.find( "Chrome" ) != std::string::npos || App.find( "YouTube" ) != std::string::npos || App.find( "msedge" ) != std::string::npos )
            Score += 2;
        if ( App.find( "discord" ) != std::string::npos || App.find( "Discord" ) != std::string::npos ) {
            if ( Title.empty( ) || !Playing )
                continue;
        }
        if ( Score > BestScore ) {
            BestScore = Score;
            Best = Candidate;
        }
    }
    if ( !Best )
        Best = Manager.GetCurrentSession( );
    if ( !Best )
        return false;

    Session = Best;
    auto Props = Best.TryGetMediaPropertiesAsync( ).get( );
    auto Playback = Best.GetPlaybackInfo( );
    auto Timeline = Best.GetTimelineProperties( );

    Item.title = Hstring( Props.Title( ) );
    Item.artist = Hstring( Props.Artist( ) );
    Item.album = Hstring( Props.AlbumTitle( ) );
    Item.app = Hstring( Best.SourceAppUserModelId( ) );
    Item.playing = Playback.PlaybackStatus( ) == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
    Item.can_play = Playback.Controls( ).IsPlayEnabled( );
    Item.can_pause = Playback.Controls( ).IsPauseEnabled( );
    Item.can_next = Playback.Controls( ).IsNextEnabled( );
    Item.can_prev = Playback.Controls( ).IsPreviousEnabled( );
    Item.can_seek = true;
    Item.position = std::chrono::duration< double >( Timeline.Position( ) ).count( );
    Item.duration = std::chrono::duration< double >( Timeline.EndTime( ) ).count( );
    Item.source = Item.app.find( "Spotify" ) != std::string::npos ? Source::Spotify : Source::System;
    Item.id = Item.app + "|" + Item.title + "|" + Item.artist;

    auto Thumb = Props.Thumbnail( );
    if ( Thumb ) {
        auto Stream = Thumb.OpenReadAsync( ).get( );
        unsigned int Size = ( unsigned int )Stream.Size( );
        if ( Size > 0 && Size < 8 * 1024 * 1024 ) {
            DataReader Reader( Stream );
            Reader.LoadAsync( Size ).get( );
            std::vector< unsigned char > Bytes( Size );
            Reader.ReadBytes( Bytes );
            DecodeArt( Item, Bytes.data( ), Bytes.size( ) );
        }
    }
    return !Item.title.empty( );
}

static bool PullSpotify( Track& Item ) {
    if ( AccessToken.empty( ) )
        return false;
    std::string Body = Http( L"api.spotify.com", L"/v1/me/player/currently-playing", L"GET", { }, Utf16( AccessToken ).c_str( ), INTERNET_DEFAULT_HTTPS_PORT, true );
    if ( Body.empty( ) )
        return false;
    if ( Body.find( "\"is_playing\"" ) == std::string::npos )
        return false;

    Item.title = JsonField( Body, "name" );
    size_t ArtistAt = Body.find( "\"artists\"" );
    if ( ArtistAt != std::string::npos )
        Item.artist = JsonField( Body.substr( ArtistAt ), "name" );
    Item.album = JsonField( Body, "album" ) ;
    Item.playing = Body.find( "\"is_playing\":true" ) != std::string::npos;
    std::string Progress = JsonField( Body, "progress_ms" );
    std::string Duration = JsonField( Body, "duration_ms" );
    if ( !Progress.empty( ) )
        Item.position = std::stod( Progress ) / 1000.0;
    if ( !Duration.empty( ) )
        Item.duration = std::stod( Duration ) / 1000.0;
    Item.can_play = true;
    Item.can_pause = true;
    Item.can_next = true;
    Item.can_prev = true;
    Item.source = Source::Spotify;
    Item.app = "Spotify";
    Item.id = "spotify|" + Item.title + "|" + Item.artist;
    return !Item.title.empty( );
}

static void ApplyCommand( int What ) {
    if ( What == 1 ) {
        if ( Session ) { try { Session.TryPlayAsync( ).get( ); } catch ( ... ) { } }
        else SpotifyCommand( L"/v1/me/player/play", L"PUT" );
    } else if ( What == 2 ) {
        if ( Session ) { try { Session.TryPauseAsync( ).get( ); } catch ( ... ) { } }
        else SpotifyCommand( L"/v1/me/player/pause", L"PUT" );
    } else if ( What == 3 ) {
        if ( Session ) { try { Session.TrySkipNextAsync( ).get( ); } catch ( ... ) { } }
        else SpotifyCommand( L"/v1/me/player/next", L"POST" );
    } else if ( What == 4 ) {
        if ( Session ) { try { Session.TrySkipPreviousAsync( ).get( ); } catch ( ... ) { } }
        else SpotifyCommand( L"/v1/me/player/previous", L"POST" );
    }
}

static void WorkerLoop( ) {
    winrt::init_apartment( winrt::apartment_type::multi_threaded );
    try {
        Manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync( ).get( );
    } catch ( ... ) {
        Manager = nullptr;
    }

    while ( !StopWorker ) {
        int What = Command.exchange( 0 );
        if ( What )
            ApplyCommand( What );
        double Seek = SeekTo.exchange( -1.0 );
        if ( Seek >= 0.0 && Session ) {
            try { Session.TryChangePlaybackPositionAsync( ( int64_t )( Seek * 10000000.0 ) ).get( ); } catch ( ... ) { }
        }

        Track Fresh;
        bool Ok = false;
        if ( PreferSpotify && SpotifyReady )
            Ok = PullSpotify( Fresh );
        if ( !Ok )
            Ok = PullSession( Fresh );
        if ( !Ok && SpotifyReady )
            Ok = PullSpotify( Fresh );
        {
            std::lock_guard< std::mutex > Lock( Guard );
            Now = Ok ? std::move( Fresh ) : Track{ };
        }
        Sleep( 400 );
    }
}

void start( ) {
    if ( Started )
        return;
    Started = true;
    LoadTokens( );
    StopWorker = false;
    Worker = std::thread( WorkerLoop );
}

void shutdown( ) {
    StopWorker = true;
    if ( Worker.joinable( ) )
        Worker.join( );
    Started = false;
    Session = nullptr;
    Manager = nullptr;
}

void refresh( ) {
}

void tick( ) {
    std::lock_guard< std::mutex > Lock( Guard );
    Published = Now;
}

const Track& current( ) {
    return Published;
}

bool play( ) {
    Command = 1;
    return true;
}

bool pause( ) {
    Command = 2;
    return true;
}

bool toggle( ) {
    return Published.playing ? pause( ) : play( );
}

bool next( ) {
    Command = 3;
    return true;
}

bool prev( ) {
    Command = 4;
    return true;
}

bool seek( double Seconds ) {
    SeekTo = Seconds;
    return true;
}

bool spotify_configured( ) {
    return config::get( "UR_SPOTIFY_CLIENT_ID" )[ 0 ] != 0;
}

bool spotify_ready( ) {
    return SpotifyReady;
}

void prefer_spotify( bool On ) {
    PreferSpotify = On;
}

void spotify_connect( ) {
    const char* Client = config::get( "UR_SPOTIFY_CLIENT_ID" );
    if ( !Client || !Client[ 0 ] )
        return;

    std::string Verifier = RandomUrl( 64 );
    std::string Challenge = Sha256B64( Verifier );
    std::string State = RandomUrl( 16 );
    std::ostringstream Url;
    Url << "https://accounts.spotify.com/authorize?client_id=" << Client
        << "&response_type=code&redirect_uri=http://127.0.0.1:17832/callback"
        << "&code_challenge_method=S256&code_challenge=" << Challenge
        << "&state=" << State
        << "&scope=user-read-playback-state%20user-modify-playback-state%20user-read-currently-playing";

    ShellExecuteA( nullptr, "open", Url.str( ).c_str( ), nullptr, nullptr, SW_SHOWNORMAL );

    SOCKET Listen = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( Listen == INVALID_SOCKET )
        return;
    sockaddr_in Addr{ };
    Addr.sin_family = AF_INET;
    Addr.sin_port = htons( 17832 );
    Addr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    if ( bind( Listen, ( sockaddr* )&Addr, sizeof( Addr ) ) != 0 ) {
        closesocket( Listen );
        return;
    }
    listen( Listen, 1 );
    DWORD Timeout = 60000;
    setsockopt( Listen, SOL_SOCKET, SO_RCVTIMEO, ( char* )&Timeout, sizeof( Timeout ) );
    SOCKET ClientSock = accept( Listen, nullptr, nullptr );
    if ( ClientSock == INVALID_SOCKET ) {
        closesocket( Listen );
        return;
    }
    char Buffer[ 2048 ] = { };
    recv( ClientSock, Buffer, 2047, 0 );
    std::string Request = Buffer;
    const char* Page = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body>UR can close this tab.</body></html>";
    send( ClientSock, Page, ( int )strlen( Page ), 0 );
    closesocket( ClientSock );
    closesocket( Listen );

    size_t CodeAt = Request.find( "code=" );
    if ( CodeAt == std::string::npos )
        return;
    CodeAt += 5;
    size_t CodeEnd = Request.find_first_of( "& ", CodeAt );
    std::string Code = Request.substr( CodeAt, CodeEnd - CodeAt );

    std::ostringstream Body;
    Body << "client_id=" << Client
         << "&grant_type=authorization_code&code=" << Code
         << "&redirect_uri=http://127.0.0.1:17832/callback"
         << "&code_verifier=" << Verifier;
    std::string TokenBody = Http( L"accounts.spotify.com", L"/api/token", L"POST", Body.str( ), nullptr, INTERNET_DEFAULT_HTTPS_PORT, true );
    AccessToken = JsonField( TokenBody, "access_token" );
    RefreshToken = JsonField( TokenBody, "refresh_token" );
    SpotifyReady = !AccessToken.empty( );
    if ( SpotifyReady )
        SaveTokens( );
}

}
}

static int WinsockOnce = []( ) {
    WSADATA Data{ };
    WSAStartup( MAKEWORD( 2, 2 ), &Data );
    return 0;
}( );
