#include "Style.h"

void CStyle::Dark( ) {
    Preset = 0;

    Backdrop = CColor( 10, 10, 13 );
    Surface = CColor( 21, 21, 26 );

    Elevated = CColor( 30, 30, 37 );
    Header = CColor( 26, 26, 32 );

    Outline = CColor( 255, 255, 255, 20 );
    Highlight = CColor( 255, 255, 255, 16 );

    Text = CColor( 237, 238, 243 );
    Faint = CColor( 138, 141, 153 );

    Accent = CColor( 74, 124, 255 );
    AccentSoft = CColor( 130, 166, 255 );

    Control = CColor( 32, 32, 39 );
    Selected = CColor( 32, 42, 66 );

    Hovered = CColor( 43, 43, 52 );
    Pressed = CColor( 26, 26, 32 );

    Groove = CColor( 42, 42, 51 );
    Knob = CColor( 240, 241, 247 );

    Shade = CColor( 0, 0, 0, 140 );
    Focus = CColor( 74, 124, 255 );

    Tab = CColor( 16, 16, 20 );
    TabActive = CColor( 30, 30, 37 );

    Popup = CColor( 28, 28, 34 );
    Danger = CColor( 240, 92, 104 );
    Success = CColor( 74, 222, 128 );
    Warning = CColor( 251, 191, 36 );

    ScrollTrack = CColor( 30, 30, 37, 150 );
    ScrollThumb = CColor( 62, 62, 74 );

    Apply( );
}

void CStyle::Light( ) {
    Preset = 1;

    Backdrop = CColor( 239, 240, 245 );
    Surface = CColor( 253, 253, 255 );

    Elevated = CColor( 247, 248, 251 );
    Header = CColor( 248, 249, 252 );

    Outline = CColor( 17, 20, 34, 26 );
    Highlight = CColor( 255, 255, 255, 130 );

    Text = CColor( 26, 28, 38 );
    Faint = CColor( 120, 124, 139 );

    Accent = CColor( 48, 108, 244 );
    AccentSoft = CColor( 130, 166, 255 );

    Control = CColor( 244, 245, 249 );
    Selected = CColor( 224, 233, 255 );

    Hovered = CColor( 235, 237, 243 );
    Pressed = CColor( 225, 227, 235 );

    Groove = CColor( 224, 226, 234 );
    Knob = CColor( 255, 255, 255 );

    Shade = CColor( 28, 32, 58, 40 );
    Focus = CColor( 48, 108, 244 );

    Tab = CColor( 232, 234, 241 );
    TabActive = CColor( 255, 255, 255 );

    Popup = CColor( 252, 252, 254 );
    Danger = CColor( 224, 66, 78 );
    Success = CColor( 22, 163, 74 );
    Warning = CColor( 217, 119, 6 );

    ScrollTrack = CColor( 225, 227, 235, 150 );
    ScrollThumb = CColor( 178, 182, 196 );

    Apply( );
}

void CStyle::Mocha( ) {
    Preset = 2;

    Backdrop = CColor( 17, 17, 27 );
    Surface = CColor( 30, 30, 46 );

    Elevated = CColor( 49, 50, 68 );
    Header = CColor( 24, 24, 37 );

    Outline = CColor( 108, 112, 134, 48 );
    Highlight = CColor( 205, 214, 244, 18 );

    Text = CColor( 205, 214, 244 );
    Faint = CColor( 166, 173, 200 );

    Accent = CColor( 203, 166, 247 );
    AccentSoft = CColor( 180, 190, 254 );

    Control = CColor( 49, 50, 68 );
    Selected = CColor( 69, 71, 90 );

    Hovered = CColor( 69, 71, 90 );
    Pressed = CColor( 24, 24, 37 );

    Groove = CColor( 69, 71, 90 );
    Knob = CColor( 205, 214, 244 );

    Shade = CColor( 17, 17, 27, 160 );
    Focus = CColor( 203, 166, 247 );

    Tab = CColor( 24, 24, 37 );
    TabActive = CColor( 49, 50, 68 );

    Popup = CColor( 36, 39, 58 );
    Danger = CColor( 243, 139, 168 );
    Success = CColor( 166, 227, 161 );
    Warning = CColor( 249, 226, 175 );

    ScrollTrack = CColor( 49, 50, 68, 150 );
    ScrollThumb = CColor( 108, 112, 134 );

    Apply( );
}

void CStyle::Nord( ) {
    Preset = 3;

    Backdrop = CColor( 46, 52, 64 );
    Surface = CColor( 59, 66, 82 );

    Elevated = CColor( 67, 76, 94 );
    Header = CColor( 59, 66, 82 );

    Outline = CColor( 216, 222, 233, 28 );
    Highlight = CColor( 236, 239, 244, 16 );

    Text = CColor( 236, 239, 244 );
    Faint = CColor( 216, 222, 233 );

    Accent = CColor( 136, 192, 208 );
    AccentSoft = CColor( 129, 161, 193 );

    Control = CColor( 67, 76, 94 );
    Selected = CColor( 76, 86, 106 );

    Hovered = CColor( 76, 86, 106 );
    Pressed = CColor( 46, 52, 64 );

    Groove = CColor( 76, 86, 106 );
    Knob = CColor( 236, 239, 244 );

    Shade = CColor( 46, 52, 64, 150 );
    Focus = CColor( 136, 192, 208 );

    Tab = CColor( 46, 52, 64 );
    TabActive = CColor( 67, 76, 94 );

    Popup = CColor( 59, 66, 82 );
    Danger = CColor( 191, 97, 106 );
    Success = CColor( 163, 190, 140 );
    Warning = CColor( 235, 203, 139 );

    ScrollTrack = CColor( 67, 76, 94, 150 );
    ScrollThumb = CColor( 129, 161, 193 );

    Apply( );
}

void CStyle::Midnight( ) {
    Preset = 4;

    Backdrop = CColor( 8, 10, 16 );
    Surface = CColor( 14, 17, 26 );

    Elevated = CColor( 22, 26, 38 );
    Header = CColor( 16, 19, 30 );

    Outline = CColor( 140, 160, 210, 28 );
    Highlight = CColor( 180, 200, 255, 14 );

    Text = CColor( 230, 236, 248 );
    Faint = CColor( 132, 144, 168 );

    Accent = CColor( 88, 166, 255 );
    AccentSoft = CColor( 140, 190, 255 );

    Control = CColor( 22, 26, 38 );
    Selected = CColor( 28, 40, 64 );

    Hovered = CColor( 32, 38, 54 );
    Pressed = CColor( 12, 14, 22 );

    Groove = CColor( 36, 42, 58 );
    Knob = CColor( 236, 240, 250 );

    Shade = CColor( 0, 0, 0, 170 );
    Focus = CColor( 88, 166, 255 );

    Tab = CColor( 10, 12, 20 );
    TabActive = CColor( 22, 26, 38 );

    Popup = CColor( 18, 22, 34 );
    Danger = CColor( 255, 107, 129 );
    Success = CColor( 52, 211, 153 );
    Warning = CColor( 251, 191, 36 );

    ScrollTrack = CColor( 22, 26, 38, 150 );
    ScrollThumb = CColor( 70, 82, 110 );

    Apply( );
}

void CStyle::Rose( ) {
    Preset = 5;

    Backdrop = CColor( 22, 14, 18 );
    Surface = CColor( 34, 22, 28 );

    Elevated = CColor( 48, 30, 38 );
    Header = CColor( 40, 24, 32 );

    Outline = CColor( 244, 180, 196, 32 );
    Highlight = CColor( 255, 214, 224, 16 );

    Text = CColor( 250, 236, 240 );
    Faint = CColor( 196, 156, 168 );

    Accent = CColor( 235, 111, 146 );
    AccentSoft = CColor( 246, 160, 180 );

    Control = CColor( 48, 30, 38 );
    Selected = CColor( 72, 40, 52 );

    Hovered = CColor( 64, 38, 48 );
    Pressed = CColor( 28, 16, 22 );

    Groove = CColor( 72, 44, 56 );
    Knob = CColor( 250, 236, 240 );

    Shade = CColor( 16, 8, 12, 160 );
    Focus = CColor( 235, 111, 146 );

    Tab = CColor( 28, 16, 22 );
    TabActive = CColor( 48, 30, 38 );

    Popup = CColor( 42, 26, 34 );
    Danger = CColor( 244, 92, 110 );
    Success = CColor( 134, 239, 172 );
    Warning = CColor( 251, 191, 36 );

    ScrollTrack = CColor( 48, 30, 38, 150 );
    ScrollThumb = CColor( 148, 88, 108 );

    Apply( );
}

void CStyle::Dracula( ) {
    Preset = 6;

    Backdrop = CColor( 40, 42, 54 );
    Surface = CColor( 68, 71, 90 );

    Elevated = CColor( 80, 84, 108 );
    Header = CColor( 48, 51, 66 );

    Outline = CColor( 98, 114, 164, 48 );
    Highlight = CColor( 248, 248, 242, 16 );

    Text = CColor( 248, 248, 242 );
    Faint = CColor( 98, 114, 164 );

    Accent = CColor( 189, 147, 249 );
    AccentSoft = CColor( 255, 121, 198 );

    Control = CColor( 68, 71, 90 );
    Selected = CColor( 80, 84, 108 );

    Hovered = CColor( 80, 84, 108 );
    Pressed = CColor( 40, 42, 54 );

    Groove = CColor( 80, 84, 108 );
    Knob = CColor( 248, 248, 242 );

    Shade = CColor( 18, 18, 26, 160 );
    Focus = CColor( 139, 233, 253, 80 );

    Tab = CColor( 68, 71, 90 );
    TabActive = CColor( 80, 84, 108 );

    Popup = CColor( 48, 51, 66 );
    Danger = CColor( 255, 85, 85 );
    Success = CColor( 80, 250, 123 );
    Warning = CColor( 241, 250, 140 );

    ScrollTrack = CColor( 40, 42, 54 );
    ScrollThumb = CColor( 189, 147, 249 );

    Apply( );
}

void CStyle::Tokyo( ) {
    Preset = 7;

    Backdrop = CColor( 26, 27, 38 );
    Surface = CColor( 36, 40, 59 );

    Elevated = CColor( 65, 72, 104 );
    Header = CColor( 30, 32, 48 );

    Outline = CColor( 86, 95, 137, 48 );
    Highlight = CColor( 192, 202, 245, 16 );

    Text = CColor( 192, 202, 245 );
    Faint = CColor( 86, 95, 137 );

    Accent = CColor( 122, 162, 247 );
    AccentSoft = CColor( 187, 154, 247 );

    Control = CColor( 36, 40, 59 );
    Selected = CColor( 65, 72, 104 );

    Hovered = CColor( 65, 72, 104 );
    Pressed = CColor( 26, 27, 38 );

    Groove = CColor( 65, 72, 104 );
    Knob = CColor( 192, 202, 245 );

    Shade = CColor( 15, 16, 24, 160 );
    Focus = CColor( 125, 207, 255, 80 );

    Tab = CColor( 36, 40, 59 );
    TabActive = CColor( 65, 72, 104 );

    Popup = CColor( 30, 32, 48 );
    Danger = CColor( 247, 118, 142 );
    Success = CColor( 158, 206, 106 );
    Warning = CColor( 224, 175, 104 );

    ScrollTrack = CColor( 26, 27, 38 );
    ScrollThumb = CColor( 122, 162, 247 );

    Apply( );
}

void CStyle::Grove( ) {
    Preset = 8;

    Backdrop = CColor( 40, 40, 40 );
    Surface = CColor( 60, 56, 54 );

    Elevated = CColor( 80, 73, 69 );
    Header = CColor( 50, 48, 47 );

    Outline = CColor( 168, 153, 132, 40 );
    Highlight = CColor( 235, 219, 178, 14 );

    Text = CColor( 235, 219, 178 );
    Faint = CColor( 168, 153, 132 );

    Accent = CColor( 254, 128, 25 );
    AccentSoft = CColor( 251, 73, 52 );

    Control = CColor( 60, 56, 54 );
    Selected = CColor( 80, 73, 69 );

    Hovered = CColor( 80, 73, 69 );
    Pressed = CColor( 40, 40, 40 );

    Groove = CColor( 80, 73, 69 );
    Knob = CColor( 235, 219, 178 );

    Shade = CColor( 20, 20, 20, 160 );
    Focus = CColor( 184, 187, 38, 80 );

    Tab = CColor( 60, 56, 54 );
    TabActive = CColor( 80, 73, 69 );

    Popup = CColor( 50, 48, 47 );
    Danger = CColor( 251, 73, 52 );
    Success = CColor( 184, 187, 38 );
    Warning = CColor( 250, 189, 47 );

    ScrollTrack = CColor( 40, 40, 40 );
    ScrollThumb = CColor( 254, 128, 25 );

    Apply( );
}

void CStyle::Forest( ) {
    Preset = 9;

    Backdrop = CColor( 45, 53, 59 );
    Surface = CColor( 61, 72, 77 );

    Elevated = CColor( 79, 93, 99 );
    Header = CColor( 52, 63, 68 );

    Outline = CColor( 133, 146, 137, 44 );
    Highlight = CColor( 211, 198, 170, 14 );

    Text = CColor( 211, 198, 170 );
    Faint = CColor( 133, 146, 137 );

    Accent = CColor( 167, 192, 128 );
    AccentSoft = CColor( 127, 187, 179 );

    Control = CColor( 61, 72, 77 );
    Selected = CColor( 79, 93, 99 );

    Hovered = CColor( 79, 93, 99 );
    Pressed = CColor( 45, 53, 59 );

    Groove = CColor( 79, 93, 99 );
    Knob = CColor( 211, 198, 170 );

    Shade = CColor( 23, 28, 31, 160 );
    Focus = CColor( 230, 180, 80, 80 );

    Tab = CColor( 61, 72, 77 );
    TabActive = CColor( 79, 93, 99 );

    Popup = CColor( 52, 63, 68 );
    Danger = CColor( 230, 126, 128 );
    Success = CColor( 135, 169, 107 );
    Warning = CColor( 230, 180, 80 );

    ScrollTrack = CColor( 45, 53, 59 );
    ScrollThumb = CColor( 167, 192, 128 );

    Apply( );
}

void CStyle::Solar( ) {
    Preset = 10;

    Backdrop = CColor( 0, 43, 54 );
    Surface = CColor( 7, 54, 66 );

    Elevated = CColor( 0, 69, 82 );
    Header = CColor( 0, 49, 60 );

    Outline = CColor( 88, 110, 117, 48 );
    Highlight = CColor( 131, 148, 150, 16 );

    Text = CColor( 131, 148, 150 );
    Faint = CColor( 88, 110, 117 );

    Accent = CColor( 38, 139, 210 );
    AccentSoft = CColor( 42, 161, 152 );

    Control = CColor( 7, 54, 66 );
    Selected = CColor( 0, 69, 82 );

    Hovered = CColor( 0, 69, 82 );
    Pressed = CColor( 0, 43, 54 );

    Groove = CColor( 0, 69, 82 );
    Knob = CColor( 238, 232, 213 );

    Shade = CColor( 0, 21, 27, 160 );
    Focus = CColor( 181, 137, 0, 80 );

    Tab = CColor( 7, 54, 66 );
    TabActive = CColor( 0, 69, 82 );

    Popup = CColor( 0, 49, 60 );
    Danger = CColor( 220, 50, 47 );
    Success = CColor( 133, 153, 0 );
    Warning = CColor( 181, 137, 0 );

    ScrollTrack = CColor( 0, 43, 54 );
    ScrollThumb = CColor( 38, 139, 210 );

    Apply( );
}

void CStyle::Latte( ) {
    Preset = 11;

    Backdrop = CColor( 239, 241, 245 );
    Surface = CColor( 230, 233, 239 );

    Elevated = CColor( 220, 224, 232 );
    Header = CColor( 230, 233, 239 );

    Outline = CColor( 156, 160, 176, 48 );
    Highlight = CColor( 76, 79, 105, 10 );

    Text = CColor( 76, 79, 105 );
    Faint = CColor( 108, 111, 133 );

    Accent = CColor( 136, 57, 239 );
    AccentSoft = CColor( 234, 118, 203 );

    Control = CColor( 220, 224, 232 );
    Selected = CColor( 204, 208, 218 );

    Hovered = CColor( 204, 208, 218 );
    Pressed = CColor( 188, 192, 204 );

    Groove = CColor( 188, 192, 204 );
    Knob = CColor( 76, 79, 105 );

    Shade = CColor( 76, 79, 105, 40 );
    Focus = CColor( 30, 102, 245, 70 );

    Tab = CColor( 220, 224, 232 );
    TabActive = CColor( 204, 208, 218 );

    Popup = CColor( 239, 241, 245 );
    Danger = CColor( 210, 15, 57 );
    Success = CColor( 64, 160, 43 );
    Warning = CColor( 223, 142, 29 );

    ScrollTrack = CColor( 230, 233, 239 );
    ScrollThumb = CColor( 136, 57, 239 );

    Apply( );
}

void CStyle::Apply( ) {
    Rounding = 12.0f * Scale;
    ControlRounding = 8.0f * Scale;

    PaddingWide = 12.0f * Scale;
    PaddingTall = 10.0f * Scale;

    Spacing = 8.0f * Scale;
    Thickness = 1.0f * Scale;

    Softness = 22.0f * Scale;
    Glow = 18.0f * Scale;

    TitleHeight = 36.0f * Scale;
    ControlHeight = 30.0f * Scale;

    TabHeight = 32.0f * Scale;
    SplitterSize = 5.0f * Scale;

    GripSize = 16.0f * Scale;
    CheckSize = 18.0f * Scale;

    KnobSize = 6.0f * Scale;
    GrooveWidth = 4.0f * Scale;

    Underline = 2.0f * Scale;
    IconStroke = 1.6f * Scale;

    FadeSpeed = 14.0f;
}

void CStyle::Rescale( float Factor ) {
    if ( Factor <= 0.0f )
        Factor = 1.0f;

    Scale = Factor;

    switch ( Preset ) {
    case 1:
        Light( );
        break;
    case 2:
        Mocha( );
        break;
    case 3:
        Nord( );
        break;
    case 4:
        Midnight( );
        break;
    case 5:
        Rose( );
        break;
    case 6:
        Dracula( );
        break;
    case 7:
        Tokyo( );
        break;
    case 8:
        Grove( );
        break;
    case 9:
        Forest( );
        break;
    case 10:
        Solar( );
        break;
    case 11:
        Latte( );
        break;
    default:
        Dark( );
        break;
    }
}