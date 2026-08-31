#pragma once

namespace ur {
namespace orbit {

struct Options {
    int* shape = nullptr;
    bool* spin = nullptr;
    bool* wire = nullptr;
    float height = 0.0f;
};

void draw( const Options& options = {} );

}
}
