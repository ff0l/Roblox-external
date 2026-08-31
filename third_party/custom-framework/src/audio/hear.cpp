#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "ur/hear.hpp"

#include "Widgets.h"

#include <Windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmreg.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

namespace ur {
namespace hear {

static constexpr int kRing = 2048;
static constexpr int kWin = 1024;
static constexpr int kBands = 32;
static constexpr int kWave = 96;

struct Snap {
    float Level = 0.0f;
    float Peak = 0.0f;
    float Bass = 0.0f;
    float Mid = 0.0f;
    float Treble = 0.0f;
    float Bands[ kBands ] = { };
    float Wave[ kWave ] = { };
    bool Live = false;
    bool Armed = false;
};

static std::mutex Guard;
static Snap Published;
static Snap Shown;
static std::atomic< bool > Stop{ false };
static std::atomic< int > Wanted{ ( int )Source::Both };
static std::atomic< float > Boost{ 4.5f };
static std::thread Worker;
static bool Started = false;
static float AutoGain = 10.0f;

static float Ring[ kRing ] = { };
static int Head = 0;
static int Filled = 0;

static float Smooth( float Current, float Target, float Up, float Down ) {
    if ( Target > Current )
        return Current + ( Target - Current ) * Up;
    return Current + ( Target - Current ) * Down;
}

static CColor Hsv( float Hue, float Sat, float Val ) {
    if ( Sat < 0.0f )
        Sat = 0.0f;
    if ( Sat > 1.0f )
        Sat = 1.0f;
    if ( Val < 0.0f )
        Val = 0.0f;
    if ( Val > 1.0f )
        Val = 1.0f;
    Hue = Hue - floorf( Hue );
    if ( Hue < 0.0f )
        Hue += 1.0f;
    float Sector = Hue * 6.0f;
    int Cell = ( int )Sector;
    float Frac = Sector - ( float )Cell;
    float P = Val * ( 1.0f - Sat );
    float Q = Val * ( 1.0f - Sat * Frac );
    float T = Val * ( 1.0f - Sat * ( 1.0f - Frac ) );
    float R = Val;
    float G = P;
    float B = P;
    switch ( Cell % 6 ) {
    case 0:
        R = Val;
        G = T;
        B = P;
        break;
    case 1:
        R = Q;
        G = Val;
        B = P;
        break;
    case 2:
        R = P;
        G = Val;
        B = T;
        break;
    case 3:
        R = P;
        G = Q;
        B = Val;
        break;
    case 4:
        R = T;
        G = P;
        B = Val;
        break;
    default:
        R = Val;
        G = P;
        B = Q;
        break;
    }
    return CColor( ( int )( R * 255.0f ), ( int )( G * 255.0f ), ( int )( B * 255.0f ) );
}

static void PushMono( float Sample ) {
    if ( Sample > 1.0f )
        Sample = 1.0f;
    if ( Sample < -1.0f )
        Sample = -1.0f;
    Ring[ Head ] = Sample;
    Head = ( Head + 1 ) % kRing;
    if ( Filled < kRing )
        Filled++;
}

static float SampleAt( int Back ) {
    int Index = Head - 1 - Back;
    while ( Index < 0 )
        Index += kRing;
    return Ring[ Index % kRing ];
}

static void Analyze( Snap& Out, int Rate ) {
    if ( Filled < 64 || Rate < 8000 ) {
        Out.Live = Filled > 0;
        return;
    }

    int Count = Filled < kWin ? Filled : kWin;
    float Window[ kWin ];
    float Energy = 0.0f;
    float Peak = 0.0f;
    for ( int i = 0; i < Count; i++ ) {
        float Hann = 0.5f - 0.5f * cosf( 6.2831853f * ( float )i / ( float )( Count - 1 ) );
        float V = SampleAt( Count - 1 - i ) * Hann;
        Window[ i ] = V;
        Energy += V * V;
        float Abs = fabsf( SampleAt( Count - 1 - i ) );
        if ( Abs > Peak )
            Peak = Abs;
    }
    for ( int i = Count; i < kWin; i++ )
        Window[ i ] = 0.0f;

    float Rms = sqrtf( Energy / ( float )Count );
    float Drive = Boost.load( );
    if ( Drive < 0.4f )
        Drive = 0.4f;
    if ( Drive > 16.0f )
        Drive = 16.0f;

    if ( Peak > 0.0008f ) {
        if ( Peak * AutoGain < 0.28f )
            AutoGain += 0.18f;
        if ( Peak * AutoGain > 0.88f )
            AutoGain -= 0.35f;
    }
    if ( AutoGain < 3.0f )
        AutoGain = 3.0f;
    if ( AutoGain > 36.0f )
        AutoGain = 36.0f;

    float Loud = Rms * AutoGain * Drive * 0.22f;
    Loud = 1.0f - expf( -Loud * 1.8f );
    Out.Level = Smooth( Out.Level, Loud, 0.78f, 0.20f );
    Out.Peak = Smooth( Out.Peak, 1.0f - expf( -Peak * AutoGain * Drive * 0.35f ), 0.82f, 0.16f );
    if ( Out.Level > 1.0f )
        Out.Level = 1.0f;
    if ( Out.Peak > 1.0f )
        Out.Peak = 1.0f;

    for ( int Band = 0; Band < kBands; Band++ ) {
        float T = ( float )Band / ( float )( kBands - 1 );
        float Hz = 45.0f * powf( 90.0f, T );
        if ( Hz > ( float )Rate * 0.45f )
            Hz = ( float )Rate * 0.45f;
        float Omega = 6.2831853f * Hz / ( float )Rate;
        float Coeff = 2.0f * cosf( Omega );
        float S0 = 0.0f;
        float S1 = 0.0f;
        float S2 = 0.0f;
        for ( int n = 0; n < Count; n++ ) {
            S0 = Window[ n ] + Coeff * S1 - S2;
            S2 = S1;
            S1 = S0;
        }
        float Power = S1 * S1 + S2 * S2 - Coeff * S1 * S2;
        if ( Power < 0.0f )
            Power = 0.0f;
        float Mag = sqrtf( Power ) / ( float )Count;
        Mag *= ( 14.0f + T * 22.0f ) * ( AutoGain * 0.12f ) * Drive;
        Mag = 1.0f - expf( -Mag );
        if ( Mag > 1.0f )
            Mag = 1.0f;
        Out.Bands[ Band ] = Smooth( Out.Bands[ Band ], Mag, 0.72f, 0.18f );
    }

    float Bass = 0.0f;
    float Mid = 0.0f;
    float Treble = 0.0f;
    for ( int i = 0; i < 6; i++ )
        Bass += Out.Bands[ i ];
    for ( int i = 6; i < 18; i++ )
        Mid += Out.Bands[ i ];
    for ( int i = 18; i < kBands; i++ )
        Treble += Out.Bands[ i ];
    Out.Bass = Smooth( Out.Bass, Bass / 6.0f, 0.50f, 0.12f );
    Out.Mid = Smooth( Out.Mid, Mid / 12.0f, 0.50f, 0.12f );
    Out.Treble = Smooth( Out.Treble, Treble / 14.0f, 0.50f, 0.12f );

    int Step = Count / kWave;
    if ( Step < 1 )
        Step = 1;
    for ( int i = 0; i < kWave; i++ )
        Out.Wave[ i ] = SampleAt( Count - 1 - i * Step );

    Out.Live = Out.Level > 0.012f || Out.Peak > 0.02f;
}

static bool Pump( IAudioCaptureClient* Capture, WAVEFORMATEX* Format ) {
    UINT32 Packet = 0;
    HRESULT Status = Capture->GetNextPacketSize( &Packet );
    if ( FAILED( Status ) )
        return false;

    int Channels = Format->nChannels > 0 ? Format->nChannels : 1;
    bool Ieee = Format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT;
    if ( Format->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
        Ieee = Format->wBitsPerSample == 32;

    while ( Packet > 0 ) {
        BYTE* Data = nullptr;
        UINT32 Frames = 0;
        DWORD Flags = 0;
        Status = Capture->GetBuffer( &Data, &Frames, &Flags, nullptr, nullptr );
        if ( FAILED( Status ) )
            return false;

        if ( !Frames )
            ;
        else if ( ( Flags & AUDCLNT_BUFFERFLAGS_SILENT ) || !Data ) {
            for ( UINT32 i = 0; i < Frames; i++ )
                PushMono( 0.0f );
        } else if ( Ieee ) {
            const float* Samples = reinterpret_cast< const float* >( Data );
            for ( UINT32 i = 0; i < Frames; i++ ) {
                float Mono = 0.0f;
                for ( int c = 0; c < Channels; c++ )
                    Mono += Samples[ i * Channels + c ];
                PushMono( Mono / ( float )Channels );
            }
        } else if ( Format->wBitsPerSample == 16 ) {
            const short* Samples = reinterpret_cast< const short* >( Data );
            for ( UINT32 i = 0; i < Frames; i++ ) {
                float Mono = 0.0f;
                for ( int c = 0; c < Channels; c++ )
                    Mono += ( float )Samples[ i * Channels + c ] * ( 1.0f / 32768.0f );
                PushMono( Mono / ( float )Channels );
            }
        }

        Capture->ReleaseBuffer( Frames );
        Status = Capture->GetNextPacketSize( &Packet );
        if ( FAILED( Status ) )
            return false;
    }

    return true;
}

struct Line {
    IMMDevice* Device = nullptr;
    IAudioClient* Client = nullptr;
    IAudioCaptureClient* Capture = nullptr;
    WAVEFORMATEX* Format = nullptr;
};

static void CloseLine( Line& Item ) {
    if ( Item.Client )
        Item.Client->Stop( );
    if ( Item.Capture )
        Item.Capture->Release( );
    if ( Item.Client )
        Item.Client->Release( );
    if ( Item.Device )
        Item.Device->Release( );
    if ( Item.Format )
        CoTaskMemFree( Item.Format );
    Item = { };
}

static bool OpenDevice( IMMDevice* Device, Line& Item, bool Loopback ) {
    Item.Device = Device;
    HRESULT Status = Device->Activate( __uuidof( IAudioClient ), CLSCTX_ALL, nullptr, reinterpret_cast< void** >( &Item.Client ) );
    if ( FAILED( Status ) || !Item.Client )
        return false;

    Status = Item.Client->GetMixFormat( &Item.Format );
    if ( FAILED( Status ) || !Item.Format )
        return false;

    DWORD Flags = Loopback ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0;
    Status = Item.Client->Initialize( AUDCLNT_SHAREMODE_SHARED, Flags, 10000000, 0, Item.Format, nullptr );
    if ( FAILED( Status ) )
        return false;

    Status = Item.Client->GetService( IID_PPV_ARGS( &Item.Capture ) );
    if ( FAILED( Status ) || !Item.Capture )
        return false;

    Status = Item.Client->Start( );
    return SUCCEEDED( Status );
}

static bool Collect( IMMDeviceEnumerator* Enumerator, EDataFlow Flow, bool Loopback, std::vector< Line >& Lines ) {
    IMMDeviceCollection* Collection = nullptr;
    HRESULT Status = Enumerator->EnumAudioEndpoints( Flow, DEVICE_STATE_ACTIVE, &Collection );
    if ( FAILED( Status ) || !Collection )
        return false;

    UINT Count = 0;
    Collection->GetCount( &Count );
    for ( UINT i = 0; i < Count; i++ ) {
        IMMDevice* Device = nullptr;
        if ( FAILED( Collection->Item( i, &Device ) ) || !Device )
            continue;
        Line Next;
        if ( OpenDevice( Device, Next, Loopback ) )
            Lines.push_back( Next );
        else
            CloseLine( Next );
    }
    Collection->Release( );
    return true;
}

static bool OpenAll( std::vector< Line >& Lines, Source Need ) {
    IMMDeviceEnumerator* Enumerator = nullptr;
    HRESULT Status = CoCreateInstance( __uuidof( MMDeviceEnumerator ), nullptr, CLSCTX_ALL, IID_PPV_ARGS( &Enumerator ) );
    if ( FAILED( Status ) || !Enumerator )
        return false;

    if ( Need == Source::Output || Need == Source::Both )
        Collect( Enumerator, eRender, true, Lines );
    if ( Need == Source::Mic || Need == Source::Both )
        Collect( Enumerator, eCapture, false, Lines );

    Enumerator->Release( );
    return !Lines.empty( );
}

static void Loop( ) {
    HRESULT Apartment = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
    Snap State;
    while ( !Stop.load( ) ) {
        Source Need = ( Source )Wanted.load( );
        std::vector< Line > Lines;
        Head = 0;
        Filled = 0;
        if ( !OpenAll( Lines, Need ) ) {
            {
                std::lock_guard< std::mutex > Lock( Guard );
                Published.Live = false;
                Published.Armed = false;
            }
            for ( int i = 0; i < 40 && !Stop.load( ) && Wanted.load( ) == ( int )Need; i++ )
                Sleep( 50 );
            continue;
        }

        State.Armed = true;
        int Rate = Lines[ 0 ].Format && Lines[ 0 ].Format->nSamplesPerSec ? ( int )Lines[ 0 ].Format->nSamplesPerSec : 48000;
        while ( !Stop.load( ) && Wanted.load( ) == ( int )Need ) {
            bool Any = false;
            for ( Line& Item : Lines ) {
                if ( Pump( Item.Capture, Item.Format ) )
                    Any = true;
            }
            if ( !Any )
                break;
            Analyze( State, Rate );
            {
                std::lock_guard< std::mutex > Lock( Guard );
                Published = State;
                Published.Armed = true;
            }
            Sleep( 8 );
        }

        for ( Line& Item : Lines )
            CloseLine( Item );
    }
    if ( SUCCEEDED( Apartment ) )
        CoUninitialize( );
}

void start( ) {
    if ( Started )
        return;
    Stop.store( false );
    Worker = std::thread( Loop );
    Started = true;
}

void shutdown( ) {
    if ( !Started )
        return;
    Stop.store( true );
    if ( Worker.joinable( ) )
        Worker.join( );
    Started = false;
}

void tick( ) {
    std::lock_guard< std::mutex > Lock( Guard );
    Shown = Published;
}

bool armed( ) {
    return Shown.Armed;
}

bool live( ) {
    return Shown.Live;
}

void set_source( Source Next ) {
    Wanted.store( ( int )Next );
}

Source source( ) {
    return ( Source )Wanted.load( );
}

const char* const* source_names( int& Count ) {
    static const char* Names[ ] = { "Output", "Mic", "Both" };
    Count = 3;
    return Names;
}

void set_gain( float Next ) {
    if ( Next < 0.4f )
        Next = 0.4f;
    if ( Next > 16.0f )
        Next = 16.0f;
    Boost.store( Next );
}

float gain( ) {
    return Boost.load( );
}

const char* status( ) {
    Source Kind = source( );
    if ( !Shown.Armed ) {
        if ( Kind == Source::Mic )
            return "no mic";
        if ( Kind == Source::Both )
            return "no devices";
        return "no output";
    }
    if ( !Shown.Live )
        return "silent";
    if ( Kind == Source::Mic )
        return "microphone";
    if ( Kind == Source::Both )
        return "output + mic";
    return "PC output";
}

float level( ) {
    return Shown.Level;
}

float peak( ) {
    return Shown.Peak;
}

float bass( ) {
    return Shown.Bass;
}

float mid( ) {
    return Shown.Mid;
}

float treble( ) {
    return Shown.Treble;
}

void bands( float* Dest, int Count ) {
    if ( !Dest || Count <= 0 )
        return;
    int Use = Count < kBands ? Count : kBands;
    memcpy( Dest, Shown.Bands, sizeof( float ) * Use );
    for ( int i = Use; i < Count; i++ )
        Dest[ i ] = 0.0f;
}

void wave( float* Dest, int Count ) {
    if ( !Dest || Count <= 0 )
        return;
    int Use = Count < kWave ? Count : kWave;
    memcpy( Dest, Shown.Wave, sizeof( float ) * Use );
    for ( int i = Use; i < Count; i++ )
        Dest[ i ] = 0.0f;
}

CColor tint( ) {
    float Hue = 0.78f - Shown.Treble * 0.22f - Shown.Mid * 0.08f + Shown.Bass * 0.04f;
    float Sat = 0.42f + Shown.Level * 0.48f;
    float Val = 0.50f + Shown.Peak * 0.42f;
    return Hsv( Hue, Sat, Val );
}

void draw_wave( ) {
    float Samples[ 96 ];
    wave( Samples, 96 );
    Widgets->Waveform( Samples, 96 );
}

void draw_spectrum( ) {
    float Bands[ 32 ];
    bands( Bands, 32 );
    Widgets->Spectrum( Bands, 32 );
}

void draw_meter( ) {
    Widgets->Meter( level( ) );
}

}
}
