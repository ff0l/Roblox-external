#pragma once

#include <Windows.h>
#include <winhttp.h>
#include <ShlObj.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace offsets {

inline constexpr wchar_t Host[ ] = L"offsets.imtheo.lol";
inline constexpr wchar_t VersionPath[ ] = L"/roblox/version";
inline constexpr wchar_t OffsetsPath[ ] = L"/offsets.json";

enum class Stage : int {
    Idle = 0,
    Check = 1,
    Fetch = 2,
    Apply = 3
};

struct Table {
    std::string version;
    std::string dumped;
    int total = 0;
    bool ready = false;
    bool stale = false;
    char error[ 160 ] = { };
    std::atomic< bool > busy{ false };
    std::atomic< int > stage{ 0 };
    std::atomic< unsigned > doneTick{ 0 };
    std::unordered_map< std::string, std::unordered_map< std::string, uintptr_t > > map;
};

inline Table& Data( ) {
    static Table Store;
    return Store;
}

inline std::recursive_mutex& Gate( ) {
    static std::recursive_mutex Lock;
    return Lock;
}

inline void SetError( const char* Text ) {
    std::lock_guard< std::recursive_mutex > Hold( Gate( ) );
    lstrcpynA( Data( ).error, Text ? Text : "", ( int )sizeof( Data( ).error ) );
}

inline void SetStage( Stage Work ) {
    Data( ).stage.store( ( int )Work );
}

inline void Trim( std::string& Text ) {
    while ( !Text.empty( ) && ( unsigned char )Text.back( ) <= 32 )
        Text.pop_back( );
    size_t Start = 0;
    while ( Start < Text.size( ) && ( unsigned char )Text[ Start ] <= 32 )
        Start++;
    if ( Start )
        Text.erase( 0, Start );
}

inline bool Folder( char* Out, int Cap ) {
    char App[ MAX_PATH ] = { };
    if ( FAILED( SHGetFolderPathA( nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, App ) ) )
        return false;
    snprintf( Out, Cap, "%s\\ff0l", App );
    CreateDirectoryA( Out, nullptr );
    return true;
}

inline bool FilePath( const char* Name, char* Out, int Cap ) {
    char Root[ MAX_PATH ] = { };
    if ( !Folder( Root, MAX_PATH ) )
        return false;
    snprintf( Out, Cap, "%s\\%s", Root, Name );
    return true;
}

inline bool ReadDisk( const char* Name, std::string& Body ) {
    char Path[ MAX_PATH ] = { };
    if ( !FilePath( Name, Path, MAX_PATH ) )
        return false;
    std::ifstream File( Path, std::ios::binary );
    if ( !File )
        return false;
    Body.assign( ( std::istreambuf_iterator< char >( File ) ), std::istreambuf_iterator< char >( ) );
    return !Body.empty( );
}

inline bool WriteDisk( const char* Name, const std::string& Body ) {
    char Path[ MAX_PATH ] = { };
    if ( !FilePath( Name, Path, MAX_PATH ) )
        return false;
    std::ofstream File( Path, std::ios::trunc | std::ios::binary );
    if ( !File )
        return false;
    File << Body;
    return true;
}

inline bool HttpGet( const wchar_t* Path, std::string& Body ) {
    Body.clear( );
    HINTERNET Session = WinHttpOpen( L"OffsetsClient/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
    if ( !Session ) {
        SetError( "WinHttpOpen failed" );
        return false;
    }
    DWORD Proto = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    WinHttpSetOption( Session, WINHTTP_OPTION_SECURE_PROTOCOLS, &Proto, sizeof( Proto ) );

    HINTERNET Connect = WinHttpConnect( Session, Host, INTERNET_DEFAULT_HTTPS_PORT, 0 );
    if ( !Connect ) {
        WinHttpCloseHandle( Session );
        SetError( "WinHttpConnect failed" );
        return false;
    }

    HINTERNET Request = WinHttpOpenRequest( Connect, L"GET", Path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE );
    if ( !Request ) {
        WinHttpCloseHandle( Connect );
        WinHttpCloseHandle( Session );
        SetError( "WinHttpOpenRequest failed" );
        return false;
    }

    BOOL Ok = WinHttpSendRequest( Request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0 );
    if ( !Ok || !WinHttpReceiveResponse( Request, nullptr ) ) {
        WinHttpCloseHandle( Request );
        WinHttpCloseHandle( Connect );
        WinHttpCloseHandle( Session );
        SetError( "Request failed" );
        return false;
    }

    DWORD Status = 0;
    DWORD StatusSize = sizeof( Status );
    WinHttpQueryHeaders( Request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &Status, &StatusSize, WINHTTP_NO_HEADER_INDEX );
    if ( Status != 200 ) {
        char Line[ 48 ];
        snprintf( Line, sizeof( Line ), "HTTP %u", Status );
        SetError( Line );
        WinHttpCloseHandle( Request );
        WinHttpCloseHandle( Connect );
        WinHttpCloseHandle( Session );
        return false;
    }

    DWORD Available = 0;
    while ( WinHttpQueryDataAvailable( Request, &Available ) && Available > 0 ) {
        std::string Chunk( Available, 0 );
        DWORD Read = 0;
        if ( !WinHttpReadData( Request, Chunk.data( ), Available, &Read ) )
            break;
        Body.append( Chunk.data( ), Read );
    }

    WinHttpCloseHandle( Request );
    WinHttpCloseHandle( Connect );
    WinHttpCloseHandle( Session );
    return !Body.empty( );
}

struct Parser {
    const char* At = nullptr;
    const char* End = nullptr;
    bool Fail = false;

    void Skip( ) {
        while ( At < End && ( unsigned char )*At <= 32 )
            At++;
    }

    bool Eat( char Mark ) {
        Skip( );
        if ( At >= End || *At != Mark )
            return false;
        At++;
        return true;
    }

    std::string String( ) {
        Skip( );
        std::string Out;
        if ( !Eat( '"' ) ) {
            Fail = true;
            return Out;
        }
        while ( At < End && *At != '"' ) {
            if ( *At == '\\' && At + 1 < End ) {
                At++;
                char Esc = *At++;
                if ( Esc == 'n' )
                    Out += '\n';
                else if ( Esc == 't' )
                    Out += '\t';
                else
                    Out += Esc;
            } else {
                Out += *At++;
            }
        }
        if ( !Eat( '"' ) )
            Fail = true;
        return Out;
    }

    uintptr_t Number( ) {
        Skip( );
        char* Next = nullptr;
        unsigned long long Value = _strtoui64( At, &Next, 10 );
        if ( Next == At ) {
            Fail = true;
            return 0;
        }
        At = Next;
        return ( uintptr_t )Value;
    }

    void SkipValue( ) {
        Skip( );
        if ( At >= End ) {
            Fail = true;
            return;
        }
        if ( *At == '"' ) {
            String( );
            return;
        }
        if ( *At == '{' ) {
            At++;
            Skip( );
            if ( Eat( '}' ) )
                return;
            for ( ;; ) {
                String( );
                if ( !Eat( ':' ) ) {
                    Fail = true;
                    return;
                }
                SkipValue( );
                Skip( );
                if ( Eat( '}' ) )
                    return;
                if ( !Eat( ',' ) ) {
                    Fail = true;
                    return;
                }
            }
        }
        if ( *At == '[' ) {
            At++;
            Skip( );
            if ( Eat( ']' ) )
                return;
            for ( ;; ) {
                SkipValue( );
                Skip( );
                if ( Eat( ']' ) )
                    return;
                if ( !Eat( ',' ) ) {
                    Fail = true;
                    return;
                }
            }
        }
        if ( strncmp( At, "true", 4 ) == 0 ) {
            At += 4;
            return;
        }
        if ( strncmp( At, "false", 5 ) == 0 ) {
            At += 5;
            return;
        }
        if ( strncmp( At, "null", 4 ) == 0 ) {
            At += 4;
            return;
        }
        Number( );
    }
};

inline bool Parse( const std::string& Body ) {
    std::lock_guard< std::recursive_mutex > Hold( Gate( ) );
    Table& Store = Data( );
    Store.map.clear( );
    Store.total = 0;
    Store.version.clear( );
    Store.dumped.clear( );

    Parser Read;
    Read.At = Body.c_str( );
    Read.End = Read.At + Body.size( );
    if ( !Read.Eat( '{' ) ) {
        SetError( "offsets.json is not an object" );
        return false;
    }

    while ( !Read.Fail && !Read.Eat( '}' ) ) {
        std::string Key = Read.String( );
        if ( !Read.Eat( ':' ) ) {
            SetError( "bad offsets.json key" );
            return false;
        }
        Read.Skip( );
        if ( Key == "Roblox Version" ) {
            Store.version = Read.String( );
        } else if ( Key == "Dumped At" ) {
            Store.dumped = Read.String( );
        } else if ( Key == "Total Offsets" ) {
            Store.total = ( int )Read.Number( );
        } else if ( Key == "Offsets" && Read.Eat( '{' ) ) {
            while ( !Read.Fail && !Read.Eat( '}' ) ) {
                std::string Class = Read.String( );
                if ( !Read.Eat( ':' ) ) {
                    SetError( "bad class" );
                    return false;
                }
                if ( !Read.Eat( '{' ) ) {
                    Read.SkipValue( );
                } else {
                    auto& Fields = Store.map[ Class ];
                    while ( !Read.Fail && !Read.Eat( '}' ) ) {
                        std::string Field = Read.String( );
                        if ( !Read.Eat( ':' ) ) {
                            SetError( "bad field" );
                            return false;
                        }
                        Read.Skip( );
                        if ( Read.At < Read.End && ( *Read.At == '{' || *Read.At == '[' || *Read.At == '"' ) )
                            Read.SkipValue( );
                        else
                            Fields[ Field ] = Read.Number( );
                        Read.Skip( );
                        Read.Eat( ',' );
                    }
                }
                Read.Skip( );
                Read.Eat( ',' );
            }
        } else {
            Read.SkipValue( );
        }
        Read.Skip( );
        Read.Eat( ',' );
    }

    if ( Read.Fail || Store.map.empty( ) ) {
        SetError( "failed to parse offsets.json" );
        return false;
    }
    if ( !Store.total ) {
        int Count = 0;
        for ( const auto& Class : Store.map )
            Count += ( int )Class.second.size( );
        Store.total = Count;
    }
    Store.ready = true;
    Store.error[ 0 ] = 0;
    return true;
}

inline bool LoadCache( ) {
    std::string Body;
    if ( !ReadDisk( "offsets.json", Body ) )
        return false;
    return Parse( Body );
}

inline bool Sync( bool Force ) {
    {
        std::lock_guard< std::recursive_mutex > Hold( Gate( ) );
        Data( ).stale = false;
    }

    SetStage( Stage::Check );
    std::string Live;
    bool GotLive = HttpGet( VersionPath, Live );
    if ( GotLive )
        Trim( Live );

    std::string Cached;
    ReadDisk( "offsets.version", Cached );
    Trim( Cached );

    bool Need = Force || !GotLive || Live.empty( ) || Live != Cached;
    if ( !Need ) {
        char Path[ MAX_PATH ] = { };
        if ( !FilePath( "offsets.json", Path, MAX_PATH ) || GetFileAttributesA( Path ) == INVALID_FILE_ATTRIBUTES )
            Need = true;
    }

    if ( Need && GotLive ) {
        SetStage( Stage::Fetch );
        std::string Body;
        if ( HttpGet( OffsetsPath, Body ) ) {
            SetStage( Stage::Apply );
            if ( Parse( Body ) ) {
                WriteDisk( "offsets.json", Body );
                WriteDisk( "offsets.version", Live );
                std::lock_guard< std::recursive_mutex > Hold( Gate( ) );
                Data( ).version = Live;
                Data( ).doneTick.store( GetTickCount( ) );
                return true;
            }
        }
    }

    SetStage( Stage::Apply );
    if ( LoadCache( ) ) {
        std::lock_guard< std::recursive_mutex > Hold( Gate( ) );
        Data( ).stale = !GotLive || Live != Data( ).version;
        if ( Data( ).stale )
            SetError( GotLive ? "using cached offsets" : "offline, using cache" );
        else
            Data( ).error[ 0 ] = 0;
        Data( ).doneTick.store( GetTickCount( ) );
        return true;
    }

    if ( !Data( ).error[ 0 ] )
        SetError( "no offsets available" );
    std::lock_guard< std::recursive_mutex > Hold( Gate( ) );
    Data( ).ready = false;
    Data( ).doneTick.store( GetTickCount( ) );
    return false;
}

inline bool Boot( ) {
    static bool Done = false;
    if ( Done )
        return Data( ).ready;
    Done = true;
    return Sync( false );
}

inline void Request( bool Force ) {
    bool Expected = false;
    if ( !Data( ).busy.compare_exchange_strong( Expected, true ) )
        return;
    SetStage( Stage::Check );
    std::thread( [ Force ]( ) {
        unsigned Start = GetTickCount( );
        Sync( Force );
        unsigned Spent = GetTickCount( ) - Start;
        if ( Spent < 420 )
            Sleep( 420 - Spent );
        SetStage( Stage::Idle );
        Data( ).busy.store( false );
    } ).detach( );
}

inline uintptr_t Get( const char* Class, const char* Field ) {
    std::lock_guard< std::recursive_mutex > Hold( Gate( ) );
    if ( !Class || !Field || !Data( ).ready )
        return 0;
    auto Found = Data( ).map.find( Class );
    if ( Found == Data( ).map.end( ) )
        return 0;
    auto Item = Found->second.find( Field );
    if ( Item == Found->second.end( ) )
        return 0;
    return Item->second;
}

inline void CopyVersion( char* Out, int Cap ) {
    std::lock_guard< std::recursive_mutex > Hold( Gate( ) );
    lstrcpynA( Out, Data( ).version.c_str( ), Cap );
}

inline void CopyError( char* Out, int Cap ) {
    std::lock_guard< std::recursive_mutex > Hold( Gate( ) );
    lstrcpynA( Out, Data( ).error, Cap );
}

inline const char* StageText( ) {
    switch ( ( Stage )Data( ).stage.load( ) ) {
    case Stage::Check: return "checking";
    case Stage::Fetch: return "downloading";
    case Stage::Apply: return "applying";
    default: return "";
    }
}

inline bool Busy( ) {
    return Data( ).busy.load( );
}

inline bool Fresh( unsigned WindowMs = 900 ) {
    unsigned Tick = Data( ).doneTick.load( );
    if ( !Tick )
        return false;
    return GetTickCount( ) - Tick < WindowMs;
}

inline int Total( ) {
    std::lock_guard< std::recursive_mutex > Hold( Gate( ) );
    return Data( ).total;
}

inline bool Ready( ) {
    std::lock_guard< std::recursive_mutex > Hold( Gate( ) );
    return Data( ).ready;
}

inline bool Stale( ) {
    std::lock_guard< std::recursive_mutex > Hold( Gate( ) );
    return Data( ).stale;
}

}
