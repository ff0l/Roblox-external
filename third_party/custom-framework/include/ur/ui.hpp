#pragma once

#include <string>

#include "Engine.h"
#include "ur/keys.hpp"
#include "ur/toast.hpp"
#include "ur/widget.hpp"

namespace ur {
namespace ui {

inline void label( const char* Message ) {
    Widgets->Label( Message );
}

inline void faint( const char* Message ) {
    Widgets->Faint( Message );
}

inline void heading( const char* Message ) {
    Widgets->Heading( Message );
}

inline void section( const char* Message ) {
    Widgets->Section( Message );
}

inline bool button( const char* Title, float Width = 0.0f ) {
    return Widgets->Button( Title, Width );
}

inline bool check( const char* Title, bool& State ) {
    return Widgets->Check( Title, State );
}

inline bool toggle( const char* Title, bool& State ) {
    return Widgets->Toggle( Title, State );
}

inline bool slider( const char* Title, float& Current, float Minimum, float Maximum ) {
    return Widgets->Slider( Title, Current, Minimum, Maximum );
}

inline bool field( const char* Title, std::string& Value, const char* Hint = nullptr ) {
    return Widgets->Field( Title, Value, Hint );
}

inline bool field( const char* Title, char* Buffer, int Capacity, const char* Hint = nullptr ) {
    return Widgets->Field( Title, Buffer, Capacity, Hint );
}

inline void progress( float Fraction ) {
    Widgets->Progress( Fraction );
}

inline void separator( ) {
    Widgets->Separator( );
}

inline void tooltip( const char* Message ) {
    Widgets->Tooltip( Message );
}

inline void disabled( bool Off = true ) {
    Widgets->BeginDisabled( Off );
}

inline void enabled( ) {
    Widgets->EndDisabled( );
}

inline void notice( const char* Message ) {
    toast::push( Message );
}

struct window {
    bool Opened = false;

    explicit window( const char* Title, bool* Visible = nullptr, unsigned int Options = FrameMove | FrameResize | FrameCollapse, CVector Anchor = CVector( 60.0f, 60.0f ), CVector Extent = CVector( 380.0f, 320.0f ) ) {
        Opened = Frames->Begin( Title, Visible, Options, Anchor, Extent );
    }

    ~window( ) {
        Frames->End( );
    }

    explicit operator bool( ) const {
        return Opened;
    }

    window( const window& ) = delete;
    window& operator=( const window& ) = delete;
};

struct disabled_scope {
    explicit disabled_scope( bool Off = true ) {
        Widgets->BeginDisabled( Off );
    }

    ~disabled_scope( ) {
        Widgets->EndDisabled( );
    }

    disabled_scope( const disabled_scope& ) = delete;
    disabled_scope& operator=( const disabled_scope& ) = delete;
};

}
}
