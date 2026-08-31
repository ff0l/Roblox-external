#pragma once

#include "Engine.h"
#include "ur/icons.hpp"

namespace ur {
namespace glyphs {

enum class Weight {
    Solid,
    Regular,
    Light
};

void bind( CGraphics* graphics );
unsigned long long image( icons::Icon icon, int size, Weight weight = Weight::Solid );
void sweep( );

}
}
