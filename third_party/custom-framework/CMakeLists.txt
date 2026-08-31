cmake_minimum_required( VERSION 3.20 )

project( CustomFramework LANGUAGES CXX )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

if( NOT WIN32 )
    message( FATAL_ERROR "Custom Framework builds on Windows." )
endif( )

option( UR_DIRECTX11 "DirectX 11 backend" ON )
option( UR_DIRECTX12 "DirectX 12 backend" ON )
option( UR_OPENGL "OpenGL backend" ON )
option( UR_VULKAN "Vulkan backend" OFF )
option( UR_BUILD_DEMO "Build hello and showcase" ON )
option( UR_DISCORD "Discord Rich Presence" ON )
option( UR_SPOTIFY_API "Spotify Web API path" ON )

set( Engine ${CMAKE_CURRENT_SOURCE_DIR}/src/engine )

set( UrSources
    ${Engine}/Core/Arena/Arena.cpp
    ${Engine}/Core/Context/Context.cpp
    ${Engine}/Core/Input/Input.cpp
    ${Engine}/Text/Font/Font.cpp
    ${Engine}/Text/Format/Format.cpp
    ${Engine}/Text/Sheet/Sheet.cpp
    ${Engine}/Interface/Layout/Layout.cpp
    ${Engine}/Interface/Style/Style.cpp
    ${Engine}/Interface/Widgets/Widgets.cpp
    ${Engine}/Interface/Desktop/Docking/Docking.cpp
    ${Engine}/Interface/Desktop/Frames/Frames.cpp
    ${Engine}/Ports/Canvas/Canvas.cpp
    ${Engine}/Ports/Shaders/Shaders.cpp
    ${Engine}/Ports/Platform/Native/Native.cpp
    ${Engine}/Ports/Platform/Pictures/Pictures.cpp
    src/app/app.cpp
    src/app/settings.cpp
    src/app/theme.cpp
    src/audio/hear.cpp
    src/config/config.cpp
    src/discord/discord.cpp
    src/effects/effects.cpp
    src/host/ElevenHost.cpp
    src/host/TwelveHost.cpp
    src/host/OpenGLHost.cpp
    src/host/VulkanHost.cpp
    src/icons/glyphs.cpp
    src/media/media.cpp
    src/overlay/overlay.cpp
    src/ui/bind.cpp
    src/ui/debug.cpp
    src/ui/image.cpp
    src/ui/motion.cpp
    src/ui/palette.cpp
    src/ui/toast.cpp
    src/ui/view.cpp
    src/ui/widget.cpp
    src/widgets/desk.cpp
    src/widgets/orbit.cpp
    src/widgets/player.cpp
)

if( UR_DIRECTX11 )
    list( APPEND UrSources ${Engine}/Ports/Graphics/DirectX11/DirectX11.cpp )
endif( )

if( UR_DIRECTX12 )
    list( APPEND UrSources ${Engine}/Ports/Graphics/DirectX12/DirectX12.cpp )
endif( )

if( UR_OPENGL )
    list( APPEND UrSources ${Engine}/Ports/Graphics/OpenGL/OpenGL.cpp )
endif( )

if( UR_VULKAN )
    list( APPEND UrSources ${Engine}/Ports/Graphics/Vulkan/Vulkan.cpp )
endif( )

add_library( ur STATIC ${UrSources} )
add_library( UR::UR ALIAS ur )

set( UrIncludes
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/host
    ${Engine}
    ${Engine}/Core
    ${Engine}/Core/Arena
    ${Engine}/Core/Context
    ${Engine}/Core/Input
    ${Engine}/Text/Font
    ${Engine}/Text/Format
    ${Engine}/Text/Sheet
    ${Engine}/Interface/Layout
    ${Engine}/Interface/Style
    ${Engine}/Interface/Widgets
    ${Engine}/Interface/Desktop/Docking
    ${Engine}/Interface/Desktop/Frames
    ${Engine}/Ports/Canvas
    ${Engine}/Ports/Shaders
    ${Engine}/Ports/Platform/Native
    ${Engine}/Ports/Platform/Pictures
    ${Engine}/Ports/Graphics/DirectX11
    ${Engine}/Ports/Graphics/DirectX12
    ${Engine}/Ports/Graphics/OpenGL
    ${Engine}/Ports/Graphics/Vulkan
)

target_include_directories( ur PUBLIC ${UrIncludes} )

target_compile_definitions( ur PUBLIC
    UR_DISCORD=$<BOOL:${UR_DISCORD}>
    UR_SPOTIFY_API=$<BOOL:${UR_SPOTIFY_API}>
    UR_VULKAN=$<BOOL:${UR_VULKAN}>
)

if( MSVC )
    target_compile_options( ur PRIVATE /W4 /utf-8 /EHsc )
endif( )

target_link_libraries( ur PUBLIC
    user32 gdi32 ole32 uuid windowscodecs d2d1 dwrite dwmapi winmm
    runtimeobject windowsapp winhttp bcrypt ws2_32 shell32
)

if( UR_DIRECTX11 )
    target_link_libraries( ur PUBLIC d3d11 d3dcompiler dxgi )
endif( )

if( UR_DIRECTX12 )
    target_link_libraries( ur PUBLIC d3d12 d3dcompiler dxgi )
endif( )

if( UR_OPENGL )
    target_link_libraries( ur PUBLIC opengl32 )
endif( )

if( UR_VULKAN )
    find_package( Vulkan REQUIRED )
    target_link_libraries( ur PUBLIC Vulkan::Vulkan )
endif( )

function( ur_add_app Target )
    add_executable( ${Target} WIN32 ${ARGN} )
    target_link_libraries( ${Target} PRIVATE ur )
    if( MSVC )
        target_compile_options( ${Target} PRIVATE /W4 /utf-8 /EHsc )
    endif( )
    set_target_properties( ${Target} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_custom_command( TARGET ${Target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:${Target}>/assets
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/.env.example
            $<TARGET_FILE_DIR:${Target}>/.env.example
    )
endfunction( )

if( UR_BUILD_DEMO )
    ur_add_app( ur_showcase demos/showcase/Main.cpp )
    set_target_properties( ur_showcase PROPERTIES OUTPUT_NAME Showcase )

    ur_add_app( ur_hello demos/hello/Main.cpp )
    set_target_properties( ur_hello PROPERTIES OUTPUT_NAME Hello )
endif( )
