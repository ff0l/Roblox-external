cmake_minimum_required( VERSION 3.20 )

project( ff0l LANGUAGES CXX )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

if( NOT WIN32 )
    message( FATAL_ERROR "ff0l builds on Windows." )
endif( )

set( UR_BUILD_DEMO OFF CACHE BOOL "" FORCE )
set( UR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/custom-framework" )
set( FONT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts" )

if( NOT EXISTS "${UR_ROOT}/CMakeLists.txt" )
    message( FATAL_ERROR "Missing ${UR_ROOT}. The source tree must include third_party/custom-framework." )
endif( )

add_subdirectory( "${UR_ROOT}" "${CMAKE_BINARY_DIR}/ur" )

add_executable( ff0l WIN32 src/Main.cpp )
target_link_libraries( ff0l PRIVATE ur winhttp )
target_include_directories( ff0l PRIVATE src )

if( MSVC )
    target_compile_options( ff0l PRIVATE /W4 /utf-8 /EHsc )
endif( )

set_target_properties( ff0l PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command( TARGET ff0l POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${UR_ROOT}/assets"
        "$<TARGET_FILE_DIR:ff0l>/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:ff0l>/assets/icons/fontawesome"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FONT_DIR}/fa-solid-900.woff2"
        "$<TARGET_FILE_DIR:ff0l>/assets/icons/fontawesome/fa-solid-900.woff2"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FONT_DIR}/fa-regular-400.woff2"
        "$<TARGET_FILE_DIR:ff0l>/assets/icons/fontawesome/fa-regular-400.woff2"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FONT_DIR}/fa-light-300.woff2"
        "$<TARGET_FILE_DIR:ff0l>/assets/icons/fontawesome/fa-light-300.woff2"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:ff0l>/assets/fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FONT_DIR}/Poppins-Regular.ttf"
        "$<TARGET_FILE_DIR:ff0l>/assets/fonts/Poppins-Regular.ttf"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FONT_DIR}/Poppins-Medium.ttf"
        "$<TARGET_FILE_DIR:ff0l>/assets/fonts/Poppins-Medium.ttf"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FONT_DIR}/Poppins-SemiBold.ttf"
        "$<TARGET_FILE_DIR:ff0l>/assets/fonts/Poppins-SemiBold.ttf"
)
