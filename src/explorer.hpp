#pragma once

#include <cstring>
#include "explorer_icons.h"

enum class TreeIcon : int {
    Model,
    Workspace,
    Folder,
    Gui,
    Stats,
    Sound,
    Players,
    Script,
    LocalScript,
    Module,
    Part,
    Spawn,
    Camera,
    Humanoid,
    Hat,
    Accessory,
    Player,
    ReplicatedStorage,
    ReplicatedFirst,
    StarterGui,
    StarterPack,
    StarterPlayer,
    CoreGui,
    Chat,
    RunService
};

struct TreeNode {
    const char* name;
    const char* type;
    int parent;
    TreeIcon icon;
};

inline constexpr TreeNode TreeNodes[ ] = {
    { "Ugc", "DataModel", -1, TreeIcon::Model },
    { "Workspace", "Workspace", 0, TreeIcon::Workspace },
    { "Baseplate", "Part", 1, TreeIcon::Part },
    { "SpawnLocation", "SpawnLocation", 1, TreeIcon::Spawn },
    { "Camera", "Camera", 1, TreeIcon::Camera },
    { "Terrain", "Terrain", 1, TreeIcon::Part },
    { "LocalHandler", "LocalScript", 1, TreeIcon::LocalScript },
    { "Dummy", "Model", 1, TreeIcon::Model },
    { "Humanoid", "Humanoid", 7, TreeIcon::Humanoid },
    { "CoolHat", "Hat", 7, TreeIcon::Hat },
    { "Face", "Accessory", 7, TreeIcon::Accessory },
    { "ReplicatedStorage", "ReplicatedStorage", 0, TreeIcon::ReplicatedStorage },
    { "Shared", "Folder", 11, TreeIcon::Folder },
    { "Net", "ModuleScript", 12, TreeIcon::Module },
    { "Util", "ModuleScript", 12, TreeIcon::Module },
    { "ServerScriptService", "ServerScriptService", 0, TreeIcon::Folder },
    { "Game", "Script", 15, TreeIcon::Script },
    { "Combat", "Script", 15, TreeIcon::Script },
    { "StarterPlayer", "StarterPlayer", 0, TreeIcon::StarterPlayer },
    { "StarterPlayerScripts", "StarterPlayerScripts", 18, TreeIcon::Folder },
    { "Client", "LocalScript", 19, TreeIcon::LocalScript },
    { "StarterCharacterScripts", "StarterCharacterScripts", 18, TreeIcon::Folder },
    { "Animate", "LocalScript", 21, TreeIcon::LocalScript },
    { "Players", "Players", 0, TreeIcon::Players },
    { "Player1", "Player", 23, TreeIcon::Player },
    { "Lighting", "Lighting", 0, TreeIcon::Folder },
    { "SoundService", "SoundService", 0, TreeIcon::Sound },
    { "GuiService", "GuiService", 0, TreeIcon::Gui },
    { "StarterGui", "StarterGui", 0, TreeIcon::StarterGui },
    { "HUD", "ScreenGui", 28, TreeIcon::Gui },
    { "StarterPack", "StarterPack", 0, TreeIcon::StarterPack },
    { "CoreGui", "CoreGui", 0, TreeIcon::CoreGui },
    { "TextChatService", "TextChatService", 0, TreeIcon::Chat },
    { "Stats", "Stats", 0, TreeIcon::Stats },
    { "ReplicatedFirst", "ReplicatedFirst", 0, TreeIcon::ReplicatedFirst },
    { "RunService", "RunService", 0, TreeIcon::RunService }
};

inline constexpr int TreeNodeCount = ( int )( sizeof( TreeNodes ) / sizeof( TreeNodes[ 0 ] ) );

inline unsigned long long PngIcon( const std::vector< uint8_t >& Bytes ) {
    CGraphics* Gfx = ur::app::graphics( );
    if ( !Gfx || Bytes.empty( ) )
        return 0;

    std::vector< unsigned char > Pixels;
    int Width = 0;
    int Height = 0;
    if ( !Pictures->Decode( Bytes.data( ), Bytes.size( ), Pixels, Width, Height, 0 ) )
        return 0;
    if ( Width <= 0 || Height <= 0 || Pixels.empty( ) )
        return 0;
    return Gfx->CreateImage( Pixels.data( ), Width, Height );
}

inline unsigned long long TreeGlyph( TreeIcon Icon ) {
    static unsigned long long Cache[ 32 ] = { };
    int Index = ( int )Icon;
    if ( Index < 0 || Index >= 32 )
        return 0;
    if ( Cache[ Index ] )
        return Cache[ Index ];

    const std::vector< uint8_t >* Bytes = nullptr;
    switch ( Icon ) {
    case TreeIcon::Model: Bytes = &ExplorerIcons::model_image_data; break;
    case TreeIcon::Workspace: Bytes = &ExplorerIcons::workspace_image_data; break;
    case TreeIcon::Folder: Bytes = &ExplorerIcons::folder_image_data; break;
    case TreeIcon::Gui: Bytes = &ExplorerIcons::gui_service_image_data; break;
    case TreeIcon::Stats: Bytes = &ExplorerIcons::stats_image_data; break;
    case TreeIcon::Sound: Bytes = &ExplorerIcons::sound_image_data; break;
    case TreeIcon::Players: Bytes = &ExplorerIcons::players_image_data; break;
    case TreeIcon::Script: Bytes = &ExplorerIcons::script_image_data; break;
    case TreeIcon::LocalScript: Bytes = &ExplorerIcons::local_script_image_data; break;
    case TreeIcon::Module: Bytes = &ExplorerIcons::module_script_image_data; break;
    case TreeIcon::Part: Bytes = &ExplorerIcons::part_image_data; break;
    case TreeIcon::Spawn: Bytes = &ExplorerIcons::spawn_location_image_data; break;
    case TreeIcon::Camera: Bytes = &ExplorerIcons::camera_image_data; break;
    case TreeIcon::Humanoid: Bytes = &ExplorerIcons::humanoid_image_data; break;
    case TreeIcon::Hat: Bytes = &ExplorerIcons::hat_image_data; break;
    case TreeIcon::Accessory: Bytes = &ExplorerIcons::accessory_image_data; break;
    case TreeIcon::Player: Bytes = &ExplorerIcons::player_image_data; break;
    case TreeIcon::ReplicatedStorage: Bytes = &ExplorerIcons::replicated_storage_image_data; break;
    case TreeIcon::ReplicatedFirst: Bytes = &ExplorerIcons::replicated_first_image_data; break;
    case TreeIcon::StarterGui: Bytes = &ExplorerIcons::starter_gui_image_data; break;
    case TreeIcon::StarterPack: Bytes = &ExplorerIcons::starter_pack_image_data; break;
    case TreeIcon::StarterPlayer: Bytes = &ExplorerIcons::starter_player_image_data; break;
    case TreeIcon::CoreGui: Bytes = &ExplorerIcons::core_gui_image_data; break;
    case TreeIcon::Chat: Bytes = &ExplorerIcons::chat_image_data; break;
    case TreeIcon::RunService: Bytes = &ExplorerIcons::run_service_image_data; break;
    }
    if ( !Bytes )
        return 0;
    Cache[ Index ] = PngIcon( *Bytes );
    return Cache[ Index ];
}

inline TreeIcon IconFor( const char* Klass ) {
    if ( !Klass || !Klass[ 0 ] )
        return TreeIcon::Folder;
    if ( !_stricmp( Klass, "Workspace" ) )
        return TreeIcon::Workspace;
    if ( !_stricmp( Klass, "Players" ) )
        return TreeIcon::Players;
    if ( !_stricmp( Klass, "Player" ) )
        return TreeIcon::Player;
    if ( !_stricmp( Klass, "Humanoid" ) )
        return TreeIcon::Humanoid;
    if ( !_stricmp( Klass, "Camera" ) )
        return TreeIcon::Camera;
    if ( !_stricmp( Klass, "Folder" ) )
        return TreeIcon::Folder;
    if ( !_stricmp( Klass, "Model" ) )
        return TreeIcon::Model;
    if ( !_stricmp( Klass, "SpawnLocation" ) )
        return TreeIcon::Spawn;
    if ( !_stricmp( Klass, "Hat" ) )
        return TreeIcon::Hat;
    if ( !_stricmp( Klass, "Accessory" ) || !_stricmp( Klass, "Accoutrement" ) )
        return TreeIcon::Accessory;
    if ( !_stricmp( Klass, "LocalScript" ) )
        return TreeIcon::LocalScript;
    if ( !_stricmp( Klass, "ModuleScript" ) )
        return TreeIcon::Module;
    if ( !_stricmp( Klass, "Script" ) )
        return TreeIcon::Script;
    if ( !_stricmp( Klass, "ReplicatedStorage" ) )
        return TreeIcon::ReplicatedStorage;
    if ( !_stricmp( Klass, "ReplicatedFirst" ) )
        return TreeIcon::ReplicatedFirst;
    if ( !_stricmp( Klass, "StarterGui" ) )
        return TreeIcon::StarterGui;
    if ( !_stricmp( Klass, "StarterPack" ) )
        return TreeIcon::StarterPack;
    if ( !_stricmp( Klass, "StarterPlayer" ) )
        return TreeIcon::StarterPlayer;
    if ( !_stricmp( Klass, "CoreGui" ) )
        return TreeIcon::CoreGui;
    if ( !_stricmp( Klass, "SoundService" ) || !_stricmp( Klass, "Sound" ) )
        return TreeIcon::Sound;
    if ( !_stricmp( Klass, "Stats" ) )
        return TreeIcon::Stats;
    if ( !_stricmp( Klass, "RunService" ) )
        return TreeIcon::RunService;
    if ( !_stricmp( Klass, "TextChatService" ) || !_stricmp( Klass, "Chat" ) )
        return TreeIcon::Chat;
    if ( !_stricmp( Klass, "GuiService" ) || strstr( Klass, "Gui" ) || strstr( Klass, "Frame" ) || strstr( Klass, "Label" ) || strstr( Klass, "Button" ) )
        return TreeIcon::Gui;
    if ( !_stricmp( Klass, "Part" ) || !_stricmp( Klass, "MeshPart" ) || !_stricmp( Klass, "WedgePart" )
        || !_stricmp( Klass, "CornerWedgePart" ) || !_stricmp( Klass, "TrussPart" ) || !_stricmp( Klass, "UnionOperation" )
        || !_stricmp( Klass, "Terrain" ) || strstr( Klass, "Part" ) )
        return TreeIcon::Part;
    return TreeIcon::Folder;
}

inline bool TreeHasKids( int Index ) {
    for ( int Node = 0; Node < TreeNodeCount; Node++ ) {
        if ( TreeNodes[ Node ].parent == Index )
            return true;
    }
    return false;
}
