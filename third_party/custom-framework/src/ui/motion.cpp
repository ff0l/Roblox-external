#include "ur/motion.hpp"

#include "Context.h"
#include "Style.h"

#include <unordered_map>

namespace ur {
namespace motion {

static std::unordered_map< unsigned int, float > Values;

float toward( const char* Id, float Target, float Speed ) {
    unsigned int Key = Context->Hash( Id ? Id : "" );
    float& Current = Values[ Key ];
    float Rate = Speed > 0.0f ? Speed : Style->FadeSpeed;
    float Step = Rate * Context->DeltaTime;
    if ( Step > 1.0f )
        Step = 1.0f;
    Current += ( Target - Current ) * Step;
    return Current;
}

void clear( ) {
    Values.clear( );
}

}
}
