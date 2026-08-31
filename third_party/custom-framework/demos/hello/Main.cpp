#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include "ur/ur.hpp"

int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int ) {
    ur::app::Config Config;
    Config.title = "Hello";
    Config.width = 720;
    Config.height = 420;

    return ur::app::run( Config, [ ] {
        if ( ur::ui::window Window( "Hello", nullptr, FrameMove | FrameResize, CVector( 24.0f, 24.0f ), CVector( 360.0f, 240.0f ) ); Window ) {
            ur::ui::heading( "Custom Framework" );
            ur::ui::faint( "Direct3D 11, Direct3D 12, or OpenGL." );

            if ( ur::ui::button( "Toast" ) )
                ur::ui::notice( "Hello" );

            float& Volume = ur::view::number( "volume", 0.6f );
            ur::ui::slider( "Volume", Volume, 0.0f, 1.0f );

            std::string& Name = ur::view::text( "name", "Session" );
            ur::ui::field( "Name", Name, "your name" );

            if ( ur::ui::button( "Quit" ) )
                ur::app::quit( );
        }
    } );
}
