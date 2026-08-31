#pragma once

#include <memory>

#include "Geometry.h"

class CStyle {
public:
    void Dark( );
    void Light( );
    void Mocha( );
    void Nord( );
    void Midnight( );
    void Rose( );
    void Dracula( );
    void Tokyo( );
    void Grove( );
    void Forest( );
    void Solar( );
    void Latte( );

    void Rescale( float Factor );

    CColor Backdrop;
    CColor Surface;

    CColor Elevated;
    CColor Header;

    CColor Outline;
    CColor Highlight;

    CColor Text;
    CColor Faint;

    CColor Accent;
    CColor AccentSoft;

    CColor Control;
    CColor Selected;

    CColor Hovered;
    CColor Pressed;

    CColor Groove;
    CColor Knob;

    CColor Shade;
    CColor Focus;

    CColor Tab;
    CColor TabActive;

    CColor Popup;
    CColor Danger;
    CColor Success;
    CColor Warning;

    CColor ScrollTrack;
    CColor ScrollThumb;

    float Rounding = 16.0f;
    float ControlRounding = 10.0f;

    float PaddingWide = 20.0f;
    float PaddingTall = 18.0f;

    float Spacing = 13.0f;
    float Thickness = 1.0f;

    float Softness = 26.0f;
    float Glow = 22.0f;

    float TitleHeight = 46.0f;
    float TitleAlign = 0.0f;

    float ControlHeight = 34.0f;
    float TabHeight = 38.0f;

    float SplitterSize = 5.0f;
    float GripSize = 18.0f;

    float CheckSize = 20.0f;
    float KnobSize = 7.0f;

    float GrooveWidth = 4.0f;
    float Underline = 2.0f;

    float IconStroke = 1.7f;
    float FadeSpeed = 14.0f;

    float ScrollSpeed = 16.0f;
    float Elevation = 1.0f;

    float Scale = 1.0f;

    bool Shadows = true;
    bool Borders = true;

    bool Glass = true;
    bool ScrollSmooth = true;

    bool Adaptive = true;

    int Theme( ) const {
        return Preset;
    }

private:
    void Apply( );
    int Preset = 0;
};

inline auto Style = std::make_unique< CStyle >( );