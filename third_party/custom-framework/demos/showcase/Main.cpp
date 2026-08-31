#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#include "ur/ur.hpp"

static const char* FolderSvg = "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'><path fill='#5684FF' d='M10 4H4c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2h-8l-2-2z'/></svg>";

static const char* Paints[ 4 ] = { "Indigo", "Teal", "Rose", "Amber" };
static const CColor Tints[ 4 ] = { CColor( 99, 102, 241 ), CColor( 45, 212, 191 ), CColor( 244, 114, 182 ), CColor( 245, 158, 11 ) };
static const char* Rooms[ 4 ] = { "Booth", "Stage", "Edit", "Live" };
static const char* Places[ 3 ] = { "Left", "Center", "Right" };
static const char* Busses[ 5 ] = { "Master", "Drums", "Vocal", "FX", "Cue" };
static const char* Rows[ 3 ] = { "Input", "Bus", "Send" };
static const char* HostNames[ 4 ] = { "DirectX 11", "DirectX 12", "OpenGL", "Vulkan" };

static int Paint = 0;
static int Presses = 0;
static bool FirstOption = true;
static bool Wireframe = false;
static int Portion = 1;
static int Room = 0;
static int Alignment = 1;
static char PersonName[ 64 ] = "Session";
static char Notebook[ 512 ] = "Cue notes stay with the mix.\nCtrl+S writes the layout.";
static int Quantity = 12;
static float Threshold = 0.35f;
static int Rating = 3;
static float Tuning[ 3 ] = { 1.0f, 0.5f, 0.25f };
static CColor Brush = CColor( 203, 166, 247 );
static bool DiagnosticsOpen = false;
static bool IdsOpen = false;
static bool AboutOpen = false;
static float Volume = 64.0f;
static float Wave[ 96 ] = { };
static float Bars[ 12 ] = { };
static float Slices[ 5 ] = { 32.0f, 24.0f, 18.0f, 14.0f, 10.0f };
static bool Tasks[ 5 ] = { true, false, true, false, false };
static char TaskBuffer[ 64 ] = "";
static int SceneSel = 0;
static char LogLines[ 8 ][ 72 ] = { };
static int LogCount = 0;
static bool SharePlaying = true;
static bool PreferApi = false;
static bool DiscordOn = false;
static int OverlayAlpha = 255;
static bool OverlayTop = false;
static bool OverlayClick = false;
static bool OverlayLayer = false;
static int QualityPick = 1;
static char IconQuery[ 64 ] = "";
static char DiscordDetails[ 80 ] = "UR";
static char DiscordState[ 80 ] = "idle";
static char FilterQuery[ 64 ] = "";
static int FilterPick = 0;
static int Shortcut = ( int )ur::Key::F8;
static bool Locked = false;
static float MixGain = 0.62f;
static bool Paletted = false;
static std::string LastShare;
static ur::view::Board Pack;

static void PushLog( const char* Line ) {
    auto Store = [ & ]( int Slot ) {
        size_t n = 0;
        while ( Line[ n ] && n < 71 ) {
            LogLines[ Slot ][ n ] = Line[ n ];
            n++;
        }
        LogLines[ Slot ][ n ] = 0;
    };
    if ( LogCount < 8 ) {
        Store( LogCount );
        LogCount++;
    } else {
        for ( int i = 0; i < 7; i++ )
            memcpy( LogLines[ i ], LogLines[ i + 1 ], 72 );
        Store( 7 );
    }
}

static void EnsurePalette( ) {
    if ( Paletted )
        return;
    Paletted = true;
    DiscordOn = ur::discord::enabled( );
    ur::palette::add( "Save layout", "Ctrl+S", [ ] { ur::app::save_layout( ); ur::toast::push( "Layout saved" ); } );
    ur::palette::add( "Profiler", "Toggle diagnostics", [ ] { DiagnosticsOpen = !DiagnosticsOpen; } );
    ur::palette::add( "ID debugger", "Widget hashes", [ ] { IdsOpen = !IdsOpen; } );
    ur::palette::add( "Toggle glass", nullptr, [ ] { Style->Glass = !Style->Glass; } );
    ur::palette::add( "Toggle docking", nullptr, [ ] { Docking->Enabled = !Docking->Enabled; } );
    ur::palette::add( "Command palette", "Ctrl+K", [ ] { ur::palette::open( ); } );
    for ( int Index = 0; Index < ur::theme::Count; Index++ ) {
        const char* Name = ur::theme::names( )[ Index ];
        ur::palette::add( Name, "Theme", [ Index ] { ur::theme::apply( Index ); } );
    }
    ur::palette::add( "Quit", nullptr, [ ] { ur::app::quit( ); } );
}

static void Showcase( ) {
    EnsurePalette( );
    ur::bind::set( "overlay.unlock", Shortcut );

    ur::overlay::Options& Overlay = ur::app::overlay_options( );
    Overlay.topmost = OverlayTop;
    Overlay.click_through = OverlayClick;
    Overlay.layered = OverlayLayer || OverlayAlpha < 255;
    Overlay.alpha = OverlayAlpha;

    if ( ur::bind::pressed( "overlay.unlock" ) || ur::pressed( ur::Key::F8 ) ) {
        OverlayClick = false;
        Overlay.click_through = false;
        ur::toast::push( "Clicks unlocked" );
    }

    if ( SharePlaying && ur::discord::enabled( ) ) {
        const ur::media::Track& Track = ur::media::current( );
        std::string Key = Track.id + "|" + Track.title + "|" + Track.artist + ( Track.playing ? "|1" : "|0" );
        if ( Key != LastShare ) {
            LastShare = Key;
            ur::discord::Presence Presence;
            Presence.details = Track.title.empty( ) ? DiscordDetails : Track.title;
            Presence.state = Track.artist.empty( ) ? DiscordState : Track.artist;
            Presence.large_text = Track.album;
            Presence.start = Track.playing ? ( std::int64_t )time( nullptr ) - ( std::int64_t )Track.position : 0;
            ur::discord::set_presence( Presence );
        }
    }

    if ( ur::effects::quality( ) != ur::effects::Quality::Off ) {
        ur::effects::draw_atmosphere( ( float )ur::app::width( ), ( float )ur::app::height( ) );
        ur::effects::draw_particles( ( float )ur::app::width( ), ( float )ur::app::height( ), Context->DeltaTime );
    }

    Pack.begin( );
    const unsigned int Card = ur::view::card( );
    CVector Origin, Extent;

    auto Open = [ & ]( const char* Title, float Guess, int Span = 1 ) -> bool {
        Pack.cell( Title, Guess, Origin, Extent, Span );
        return Frames->Begin( Title, nullptr, Card, Origin, Extent );
    };

    if ( Open( "Listening", 108 ) )
        ur::player::draw_chip( );
    Frames->End( );

    if ( Open( "Deck", 300 ) ) {
        ur::player::draw_expanded( );
        if ( ur::media::spotify_configured( ) ) {
            Widgets->Separator( );
            if ( !ur::media::spotify_ready( ) ) {
                if ( Widgets->Button( "Connect Spotify" ) )
                    ur::media::spotify_connect( );
            } else {
                Widgets->Faint( "Spotify API connected" );
                Widgets->Check( "Prefer Spotify API", PreferApi );
                ur::media::prefer_spotify( PreferApi );
            }
        }
    }
    Frames->End( );

    if ( Open( "Orbit", 280 ) ) {
        ur::orbit::Options Mesh;
        Mesh.height = 210.0f * Style->Scale;
        ur::orbit::draw( Mesh );
    }
    Frames->End( );

    if ( Open( "Desk", 148 ) )
        ur::desk::clock( );
    Frames->End( );

    if ( Open( "Mix", 280 ) )
        ur::desk::mix( );
    Frames->End( );

    if ( Open( "Pulse", 132 ) ) {
        Widgets->Label( Format->Print( "Bass  %.0f", ur::hear::bass( ) * 100.0f ) );
        Widgets->Meter( ur::hear::bass( ) );
        Widgets->Label( Format->Print( "Mid   %.0f", ur::hear::mid( ) * 100.0f ) );
        Widgets->Meter( ur::hear::mid( ) );
        Widgets->Label( Format->Print( "Treble %.0f", ur::hear::treble( ) * 100.0f ) );
        Widgets->Meter( ur::hear::treble( ) );
        Widgets->Faint( ur::hear::status( ) );
    }
    Frames->End( );

    if ( Open( "Today", 118 ) ) {
        SYSTEMTIME Local = { };
        GetLocalTime( &Local );
        const char* Days[ 7 ] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
        const char* Months[ 12 ] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
        Widgets->Heading( Format->Print( "%s", Days[ Local.wDayOfWeek ] ) );
        Widgets->Label( Format->Print( "%u %s", Local.wDay, Months[ Local.wMonth - 1 ] ) );
        int Minutes = ( int )( Context->Elapsed / 60.0 );
        Widgets->Faint( Format->Print( "session  %d:%02d", Minutes, ( int )Context->Elapsed % 60 ) );
    }
    Frames->End( );

    if ( Open( "Host", 128 ) ) {
        int HostIndex = ( int )ur::app::backend( );
        if ( HostIndex < 0 || HostIndex > 3 )
            HostIndex = 0;
        const char* Host = HostNames[ HostIndex ];
        Widgets->Label( Format->Print( "%.0f fps", Context->Framerate ) );
        Widgets->Faint( Format->Print( "%s  %dx%d", Host, ur::app::width( ), ur::app::height( ) ) );
        Widgets->Faint( ur::app::vsync( ) ? "VSync on" : "VSync off" );
        Widgets->Colored( ur::hear::live( ) ? Style->Success : Style->Faint, ur::hear::live( ) ? "Audio live" : "Audio idle" );
    }
    Frames->End( );

    if ( Open( "Studio", 420, 2 ) ) {
        if ( Widgets->BeginMenuBar( ) ) {
            if ( Widgets->BeginMenu( "File" ) ) {
                if ( Widgets->MenuItem( "Save layout", "Ctrl+S" ) )
                    ur::app::save_layout( );
                if ( Widgets->MenuItem( "Load layout" ) )
                    ur::app::load_layout( );
                if ( Widgets->MenuItem( "Commands", "Ctrl+K" ) )
                    ur::palette::open( );
                if ( Widgets->MenuItem( "Quit" ) )
                    ur::app::quit( );
                Widgets->EndMenu( );
            }
            if ( Widgets->BeginMenu( "View" ) ) {
                for ( int Index = 0; Index < ur::theme::Count; Index++ ) {
                    if ( Widgets->MenuItem( ur::theme::names( )[ Index ], nullptr, ur::theme::current( ) == Index ) )
                        ur::theme::apply( Index );
                }
                if ( Widgets->MenuItem( "Profiler", nullptr, DiagnosticsOpen ) )
                    DiagnosticsOpen = !DiagnosticsOpen;
                if ( Widgets->MenuItem( "IDs", nullptr, IdsOpen ) )
                    IdsOpen = !IdsOpen;
                if ( Widgets->MenuItem( "Glass", nullptr, Style->Glass ) )
                    Style->Glass = !Style->Glass;
                if ( Widgets->MenuItem( "Docking", nullptr, Docking->Enabled ) )
                    Docking->Enabled = !Docking->Enabled;
                Widgets->EndMenu( );
            }
            Widgets->EndMenuBar( );
        }

        if ( Widgets->BeginTabs( "MainTabs" ) ) {
            if ( Widgets->Tab( "Kit" ) ) {
                Widgets->Section( "Buttons" );
                float Half = ( Layout->Width( ) - Style->Spacing ) * 0.5f;
                if ( Widgets->Button( "Press", Half ) ) {
                    Presses++;
                    PushLog( Format->Print( "press %d", Presses ) );
                    ur::toast::push( Format->Print( "%d clicks", Presses ) );
                }
                Layout->SameLine( );
                if ( Widgets->Button( "Reset", Half ) )
                    Presses = 0;
                Widgets->Label( Format->Print( "%d clicks", Presses ) );
                Widgets->Tooltip( "Counts button presses" );

                Widgets->Section( "Selection" );
                Widgets->Check( "First option", FirstOption );
                Widgets->Toggle( "Wireframe", Wireframe );
                Widgets->Radio( "Small", Portion, 0 );
                Widgets->Radio( "Medium", Portion, 1 );
                Widgets->Radio( "Large", Portion, 2 );
                Widgets->Choice( "Room", Room, Rooms, 4 );
                Widgets->Segments( "Align", Alignment, Places, 3 );

                Widgets->Section( "Text" );
                Widgets->Field( "Name", PersonName, 64, "name" );
                Widgets->Area( "Notes", Notebook, 512, 76.0f * Style->Scale );

                Widgets->Section( "Values" );
                Widgets->Slider( "Volume", Volume, 0.0f, 100.0f );
                Widgets->Knob( "Gain", MixGain, 0.0f, 1.0f );
                Widgets->Stars( "Rating", Rating, 5 );
                Widgets->Number( "Quantity", Quantity );
                Widgets->Decimal( "Threshold", Threshold, 0.01f );
                Widgets->Drag( "Fine", Tuning[ 0 ], 0.01f, -10.0f, 10.0f );
                Widgets->Vector( "Position", Tuning, 3, -10.0f, 10.0f );
                Widgets->Color( "Brush", Brush );
                Widgets->Progress( 0.5f + 0.5f * sinf( ( float )Context->Elapsed * 1.4f ) );
                Widgets->Meter( MixGain );

                Widgets->Section( "State" );
                Widgets->Check( "Lock extras", Locked );
                Widgets->BeginDisabled( Locked );
                Widgets->Keybind( "Unlock", Shortcut );
                if ( Widgets->Button( "About" ) )
                    AboutOpen = true;
                Widgets->EndDisabled( );

                Widgets->Section( "Filter" );
                Widgets->FilterList( "Busses", FilterPick, Busses, 5, FilterQuery, 64, 4 );
            }

            if ( Widgets->Tab( "Look" ) ) {
                int HostCount = 3;
#if UR_VULKAN
                HostCount = 4;
#endif
                int Selected = ( int )ur::app::backend( );
                if ( Widgets->Choice( "Backend", Selected, HostNames, HostCount ) )
                    ur::app::set_backend( ( ur::Backend )Selected );

                bool VerticalSync = ur::app::vsync( );
                if ( Widgets->Check( "VSync", VerticalSync ) )
                    ur::app::set_vsync( VerticalSync );

                int Theme = ur::theme::current( );
                if ( Widgets->Choice( "Theme", Theme, ur::theme::names( ), ur::theme::Count ) )
                    ur::theme::apply( Theme );

                if ( Widgets->Segments( "Accent", Paint, Paints, 4 ) ) {
                    Style->Accent = Tints[ Paint ];
                    Style->AccentSoft = Tints[ Paint ].Fade( 0.25f );
                }
                Widgets->Slider( "Rounding", Style->Rounding, 0.0f, 24.0f );
                Widgets->Slider( "Shadow", Style->Softness, 1.0f, 44.0f );
                Widgets->Check( "Shadows", Style->Shadows );
                Widgets->Check( "Borders", Style->Borders );
                Widgets->Check( "Smooth scroll", Style->ScrollSmooth );
                Widgets->Check( "Adaptive", Style->Adaptive );
                if ( Widgets->Button( "Load example theme" ) ) {
                    ur::theme::load_file( ur::config::asset( "assets/themes/example.theme" ).c_str( ) );
                    ur::toast::push( "Theme file applied", Style->Success );
                }
            }

            if ( Widgets->Tab( "Stage" ) ) {
                int Count = 0;
                const char* const* List = ur::effects::background_names( Count );
                int Current = ur::effects::background( );
                if ( Widgets->Choice( "Background", Current, List, Count ) )
                    ur::effects::set_background( Current );
                const char* Qualities[ 3 ] = { "Off", "Low", "High" };
                if ( Widgets->Segments( "Quality", QualityPick, Qualities, 3 ) )
                    ur::effects::set_quality( ( ur::effects::Quality )QualityPick );
                Widgets->Faint( "Fullscreen pass, then the UI on top." );
                Widgets->Colored( Style->Success, "Success" );
                Layout->SameLine( );
                Widgets->Colored( Style->Warning, "Warning" );
                Layout->SameLine( );
                Widgets->Colored( Style->Danger, "Danger" );
            }

            if ( Widgets->Tab( "Presence" ) ) {
                if ( Widgets->Check( "Rich Presence", DiscordOn ) ) {
                    ur::discord::enable( DiscordOn );
                    LastShare.clear( );
                }
                Widgets->Field( "Details", DiscordDetails, 80 );
                Widgets->Field( "State", DiscordState, 80 );
                Widgets->Check( "Share now playing", SharePlaying );
                Widgets->Label( ur::discord::connected( ) ? "Connected" : "Disconnected" );
                if ( Widgets->Button( "Push presence" ) ) {
                    LastShare.clear( );
                    ur::discord::Presence Presence;
                    Presence.details = DiscordDetails;
                    Presence.state = DiscordState;
                    ur::discord::set_presence( Presence );
                }
                if ( Widgets->Button( "Clear" ) ) {
                    LastShare.clear( );
                    ur::discord::clear( );
                }
                Widgets->Faint( "Now playing stays on screen. Set UR_DISCORD_APP_ID in .env" );
            }

            if ( Widgets->Tab( "Overlay" ) ) {
                Widgets->Check( "Topmost", OverlayTop );
                Widgets->Check( "Click through", OverlayClick );
                Widgets->Check( "Layered", OverlayLayer );
                Widgets->SliderWhole( "Alpha", OverlayAlpha, 40, 255 );
                Widgets->Keybind( "Unlock clicks", Shortcut );
                Widgets->Faint( "The bound key turns click-through off if the window stops taking clicks." );
            }

            Widgets->EndTabs( );
        }
    }
    Frames->End( );

    for ( int Sample = 0; Sample < 96; Sample++ )
        Wave[ Sample ] = sinf( ( float )Context->Elapsed * 2.0f + ( float )Sample * 0.2f ) * ( 0.5f + 0.4f * sinf( ( float )Sample * 0.05f ) );
    for ( int Bucket = 0; Bucket < 12; Bucket++ )
        Bars[ Bucket ] = 0.5f + 0.5f * sinf( ( float )Context->Elapsed + ( float )Bucket * 0.5f );

    if ( Open( "Signals", 300 ) ) {
        if ( Widgets->BeginTabs( "ChartTabs" ) ) {
            if ( Widgets->Tab( "Graphs" ) ) {
                Widgets->Pie( "Share", Slices, 5 );
                Widgets->Pie( "Donut", Slices, 5, true );
                Widgets->Plot( "Signal", Wave, 96 );
                Widgets->Area( "Filled", Wave, 96 );
                Widgets->Histogram( "Buckets", Bars, 12 );
                Widgets->Waveform( Wave, 96 );
                Widgets->Spectrum( Bars, 12 );
            }
            if ( Widgets->Tab( "Data" ) ) {
                unsigned long long Folder = ur::image::svg( FolderSvg, 32 );
                if ( Widgets->Tree( "Scene", Folder, true ) ) {
                    if ( Widgets->TreeLeaf( "Camera", SceneSel == 0 ) )
                        SceneSel = 0;
                    if ( Widgets->TreeLeaf( "Light", SceneSel == 1 ) )
                        SceneSel = 1;
                    if ( Widgets->Tree( "Meshes", Folder, true ) ) {
                        if ( Widgets->TreeLeaf( "Cube", SceneSel == 2 ) )
                            SceneSel = 2;
                        if ( Widgets->TreeLeaf( "Sphere", SceneSel == 3 ) )
                            SceneSel = 3;
                        Widgets->TreePop( );
                    }
                    Widgets->TreePop( );
                }
                if ( Widgets->BeginCollapse( "List" ) ) {
                    int Fruit = 0;
                    Widgets->List( "Busses", Fruit, Busses, 5, 4 );
                    Widgets->EndCollapse( );
                }
                if ( Layout->BeginTable( "Grid", 3 ) ) {
                    Layout->TableSetup( "Name" );
                    Layout->TableSetup( "Value", 90.0f * Style->Scale );
                    Layout->TableSetup( "State" );
                    Layout->TableHeaders( );
                    for ( int Line = 0; Line < 3; Line++ ) {
                        Layout->TableRow( );
                        Layout->TableColumn( );
                        Widgets->Label( Rows[ Line ] );
                        Layout->TableColumn( );
                        Widgets->Label( Format->Print( "%d", Line * 17 ) );
                        Layout->TableColumn( );
                        Widgets->Faint( Line % 2 ? "on" : "off" );
                    }
                    Layout->EndTable( );
                }
            }
            Widgets->EndTabs( );
        }
    }
    Frames->End( );

    if ( Open( "Glyphs", 280 ) ) {
        Widgets->Field( "Search", IconQuery, 64, "home, gear, music" );
        int Shown = 0;
        float Cell = 56.0f * Style->Scale;
        float NameH = Font->LineSpan + 6.0f * Style->Scale;
        if ( Layout->BeginChild( "IconGrid", CVector( Layout->Width( ), 196.0f * Style->Scale ) ) ) {
            float Wide = Layout->Width( );
            int Cols = ( int )( Wide / Cell );
            if ( Cols < 1 )
                Cols = 1;
            int Col = 0;
            for ( size_t i = 0; i < ur::icons::kCatalogCount && Shown < 120; i++ ) {
                const ur::icons::Glyph& Item = ur::icons::kCatalog[ i ];
                if ( IconQuery[ 0 ] && !strstr( Item.name, IconQuery ) )
                    continue;

                ur::id_scope Scope( ( int )i );
                unsigned long long Image = ur::glyphs::image( Item.icon, ( int )( 22.0f * Style->Scale ) );
                CRectangle Box = Layout->Place( CVector( Cell, Cell + NameH ) );
                float Mark = 22.0f * Style->Scale;
                CRectangle Icon( Box.Left + ( Cell - Mark ) * 0.5f, Box.Top + 6.0f * Style->Scale, Mark, Mark );
                if ( Image )
                    Canvas->Image( Icon, Image, CRectangle( 0.0f, 0.0f, 1.0f, 1.0f ), Style->Text, 0.0f );

                Canvas->PushClip( CRectangle( Box.Left + 2.0f * Style->Scale, Box.Top + Cell - 4.0f * Style->Scale, Cell - 4.0f * Style->Scale, NameH ) );
                CVector Size = Font->Measure( Item.name );
                Canvas->Text( CVector( Box.Left + ( Cell - Size.Horizontal ) * 0.5f, Box.Top + Cell - 2.0f * Style->Scale ), Style->Faint, Item.name );
                Canvas->PopClip( );

                if ( Box.Contains( Input->MousePosition ) && Input->MousePressed( 0 ) )
                    Native->ClipboardSet( Item.name );

                Shown++;
                Col++;
                if ( Col < Cols )
                    Layout->SameLine( 0.0f, 0.0f );
                else
                    Col = 0;
            }
            Layout->EndChild( );
        }
        Widgets->Faint( Format->Print( "%d shown  ·  click copies the name", Shown ) );
    }
    Frames->End( );

    if ( Open( "Board", 220 ) ) {
        int Done = 0;
        for ( int Item = 0; Item < 5; Item++ )
            if ( Tasks[ Item ] )
                Done++;
        Widgets->Heading( "Cue" );
        Widgets->Faint( Format->Print( "%d of 5", Done ) );
        Widgets->Progress( ( float )Done / 5.0f );
        const char* TaskName[ 5 ] = { "Gain", "EQ", "Comp", "Send", "Print" };
        for ( int Item = 0; Item < 5; Item++ ) {
            ur::id_scope Scope( Item );
            Widgets->Check( TaskName[ Item ], Tasks[ Item ] );
        }
        float Half = ( Layout->Width( ) - Style->Spacing ) * 0.5f;
        if ( Widgets->Button( "Clear", Half ) ) {
            for ( int Item = 0; Item < 5; Item++ )
                Tasks[ Item ] = false;
        }
        Layout->SameLine( );
        if ( Widgets->Button( "Mark all", Half ) ) {
            for ( int Item = 0; Item < 5; Item++ )
                Tasks[ Item ] = true;
        }
        Widgets->Field( "", TaskBuffer, 64, "note" );
    }
    Frames->End( );

    if ( Open( "Log", 160 ) ) {
        for ( int Line = 0; Line < LogCount; Line++ )
            Widgets->Faint( LogLines[ Line ] );
        if ( Widgets->Button( "Stamp" ) )
            PushLog( Format->Print( "%.1fs tick", Context->Elapsed ) );
    }
    Frames->End( );

    if ( AboutOpen && Widgets->BeginModal( "About", &AboutOpen, CVector( 380.0f, 200.0f ) ) ) {
        Widgets->Heading( "Custom Framework" );
        Widgets->Wrapped( "Immediate-mode UI for Windows on Direct3D 11, Direct3D 12, and OpenGL. Ctrl+K opens the command palette. Ctrl+S saves the layout." );
        if ( Widgets->Button( "Close" ) )
            AboutOpen = false;
        Widgets->EndModal( );
    }

    if ( DiagnosticsOpen )
        Engine->Diagnostics( &DiagnosticsOpen );
    ur::debug::ids( &IdsOpen );
}

int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR CommandLine, int ) {
    ur::app::Config Config;
    Config.title = "Custom Framework";
    Config.width = 1480;
    Config.height = 860;
    Config.media = true;
    Config.hear = true;
    Config.discord = true;
    Config.overlay = true;
    Config.persist = true;
    Config.layout = "ur.layout";
    Config.settings = "ur.settings";

    if ( CommandLine && CommandLine[ 0 ] ) {
        int Index = atoi( CommandLine );
        if ( Index >= 0 && Index <= 3 )
            Config.backend = ( ur::Backend )Index;
    }

    PushLog( "ready" );
    return ur::app::run( Config, Showcase );
}
