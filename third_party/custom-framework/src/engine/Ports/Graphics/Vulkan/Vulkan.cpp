#include "Vulkan.h"

#if UR_VULKAN

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <cstring>
#include <unordered_map>
#include <shaderc/shaderc.h>

#include "Sheet.h"
#include "Context.h"
#include "Shaders.h"

#pragma comment( lib, "vulkan-1.lib" )
#pragma comment( lib, "shaderc_shared.lib" )

static_assert( sizeof( CInstance ) == 80, "The instance layout must match the shader" );

static VkShaderModule BuildStage( VkDevice Device, const std::string& Source, bool Fragment ) {
    static std::unordered_map< std::string, std::vector< unsigned int > > Keepsakes;

    std::string Key = ( Fragment ? "F" : "V" ) + Source;
    auto Kept = Keepsakes.find( Key );

    if ( Kept != Keepsakes.end( ) ) {
        if ( Kept->second.empty( ) )
            return VK_NULL_HANDLE;

        VkShaderModuleCreateInfo Recipe = { };

        Recipe.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        Recipe.codeSize = Kept->second.size( ) * sizeof( unsigned int );

        Recipe.pCode = Kept->second.data( );

        VkShaderModule Stage = VK_NULL_HANDLE;
        vkCreateShaderModule( Device, &Recipe, nullptr, &Stage );

        return Stage;
    }

    Keepsakes[ Key ] = std::vector< unsigned int >( );

    if ( !LoadLibraryA( "shaderc_shared.dll" ) ) {
        Context->Report( "Could not load the shader compiler" );
        return VK_NULL_HANDLE;
    }

    shaderc_compiler_t Compiler = shaderc_compiler_initialize( );
    if ( !Compiler )
        return VK_NULL_HANDLE;

    shaderc_compilation_result_t Outcome = shaderc_compile_into_spv( Compiler, Source.c_str( ), Source.size( ), Fragment ? shaderc_glsl_fragment_shader : shaderc_glsl_vertex_shader, "UR", "main", nullptr );
    VkShaderModule Stage = VK_NULL_HANDLE;

    if ( shaderc_result_get_compilation_status( Outcome ) == shaderc_compilation_status_success ) {
        VkShaderModuleCreateInfo Recipe = { };

        Recipe.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        Recipe.codeSize = shaderc_result_get_length( Outcome );

        Recipe.pCode = ( const unsigned int* )shaderc_result_get_bytes( Outcome );
        vkCreateShaderModule( Device, &Recipe, nullptr, &Stage );

        size_t Words = shaderc_result_get_length( Outcome ) / sizeof( unsigned int );
        const unsigned int* Bytes = ( const unsigned int* )shaderc_result_get_bytes( Outcome );

        Keepsakes[ Key ] = std::vector< unsigned int >( Bytes, Bytes + Words );
    } else {
        Context->Report( shaderc_result_get_error_message( Outcome ) );
    }

    shaderc_result_release( Outcome );
    shaderc_compiler_release( Compiler );

    return Stage;
}

CVulkan::~CVulkan( ) {
    Destroy( );
}

unsigned int CVulkan::Locate( unsigned int Mask, unsigned int Wanted ) const {
    VkPhysicalDeviceMemoryProperties Properties;
    vkGetPhysicalDeviceMemoryProperties( Physical, &Properties );

    for ( unsigned int Kind = 0; Kind < Properties.memoryTypeCount; Kind++ ) {
        if ( ( Mask & ( 1u << Kind ) ) && ( Properties.memoryTypes[ Kind ].propertyFlags & Wanted ) == Wanted )
            return Kind;
    }

    return 0;
}

bool CVulkan::CreateBuffer( size_t Size, unsigned int Usage, VkBuffer& Buffer, VkDeviceMemory& Memory, unsigned char** Mapped ) {
    VkBufferCreateInfo Recipe = { };

    Recipe.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    Recipe.size = Size;

    Recipe.usage = Usage;
    Recipe.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if ( vkCreateBuffer( Device, &Recipe, nullptr, &Buffer ) != VK_SUCCESS )
        return false;

    VkMemoryRequirements Needs = { };
    vkGetBufferMemoryRequirements( Device, Buffer, &Needs );

    VkMemoryAllocateInfo Portion = { };

    Portion.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    Portion.allocationSize = Needs.size;

    Portion.memoryTypeIndex = Locate( Needs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    if ( vkAllocateMemory( Device, &Portion, nullptr, &Memory ) != VK_SUCCESS ) {
        vkDestroyBuffer( Device, Buffer, nullptr );
        Buffer = VK_NULL_HANDLE;

        return false;
    }

    vkBindBufferMemory( Device, Buffer, Memory, 0 );

    if ( Mapped ) {
        if ( vkMapMemory( Device, Memory, 0, VK_WHOLE_SIZE, 0, ( void** )Mapped ) != VK_SUCCESS )
            return false;
    }

    return true;
}

bool CVulkan::CreateStore( CVulkanImage& Image, int Width, int Height, unsigned int Kind ) {
    VkImageCreateInfo Recipe = { };

    Recipe.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    Recipe.imageType = VK_IMAGE_TYPE_2D;

    Recipe.format = ( VkFormat )Kind;

    Recipe.extent.width = ( unsigned int )Width;
    Recipe.extent.height = ( unsigned int )Height;

    Recipe.extent.depth = 1;
    Recipe.mipLevels = 1;

    Recipe.arrayLayers = 1;
    Recipe.samples = VK_SAMPLE_COUNT_1_BIT;

    Recipe.tiling = VK_IMAGE_TILING_OPTIMAL;
    Recipe.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    Recipe.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if ( vkCreateImage( Device, &Recipe, nullptr, &Image.Texture ) != VK_SUCCESS )
        return false;

    VkMemoryRequirements Needs = { };
    vkGetImageMemoryRequirements( Device, Image.Texture, &Needs );

    VkMemoryAllocateInfo Portion = { };

    Portion.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    Portion.allocationSize = Needs.size;

    Portion.memoryTypeIndex = Locate( Needs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    if ( vkAllocateMemory( Device, &Portion, nullptr, &Image.Memory ) != VK_SUCCESS )
        return false;

    vkBindImageMemory( Device, Image.Texture, Image.Memory, 0 );

    VkImageViewCreateInfo Sight = { };

    Sight.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    Sight.image = Image.Texture;

    Sight.viewType = VK_IMAGE_VIEW_TYPE_2D;
    Sight.format = ( VkFormat )Kind;

    Sight.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Sight.subresourceRange.levelCount = 1;

    Sight.subresourceRange.layerCount = 1;

    if ( vkCreateImageView( Device, &Sight, nullptr, &Image.View ) != VK_SUCCESS )
        return false;

    Image.Width = Width;
    Image.Height = Height;

    return true;
}

VkDescriptorSet CVulkan::CreateSet( VkDescriptorSetLayout Shape ) {
    VkDescriptorSetAllocateInfo Recipe = { };

    Recipe.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    Recipe.descriptorPool = Pool;

    Recipe.descriptorSetCount = 1;
    Recipe.pSetLayouts = &Shape;

    VkDescriptorSet Set = VK_NULL_HANDLE;
    vkAllocateDescriptorSets( Device, &Recipe, &Set );

    return Set;
}

VkPipeline CVulkan::Pipe( unsigned int Effect ) {
    size_t Wanted = ( size_t )Effect + 1;

    if ( Pipelines.size( ) < Wanted )
        Pipelines.resize( Wanted, VK_NULL_HANDLE );

    if ( Pipelines[ Effect ] )
        return Pipelines[ Effect ];

    VkShaderModule VertexStage = BuildStage( Device, Shaders->Vertex( ShaderVulkan ), false );
    VkShaderModule FragmentStage = BuildStage( Device, Shaders->Pixel( Effect, ShaderVulkan ), true );

    if ( !VertexStage || !FragmentStage ) {
        if ( VertexStage )
            vkDestroyShaderModule( Device, VertexStage, nullptr );

        if ( FragmentStage )
            vkDestroyShaderModule( Device, FragmentStage, nullptr );

        return Effect == 0 ? VK_NULL_HANDLE : Pipe( 0 );
    }

    VkPipelineShaderStageCreateInfo Stages[ 2 ] = { };

    Stages[ 0 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    Stages[ 0 ].stage = VK_SHADER_STAGE_VERTEX_BIT;

    Stages[ 0 ].module = VertexStage;
    Stages[ 0 ].pName = "main";

    Stages[ 1 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    Stages[ 1 ].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    Stages[ 1 ].module = FragmentStage;
    Stages[ 1 ].pName = "main";

    VkPipelineVertexInputStateCreateInfo Feeding = { };
    Feeding.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo Weaving = { };

    Weaving.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    Weaving.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    VkPipelineViewportStateCreateInfo Framing = { };

    Framing.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    Framing.viewportCount = 1;

    Framing.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo Shaping = { };

    Shaping.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    Shaping.polygonMode = VK_POLYGON_MODE_FILL;

    Shaping.cullMode = VK_CULL_MODE_NONE;
    Shaping.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo Sampling = { };

    Sampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    Sampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState Mixing = { };

    Mixing.blendEnable = VK_TRUE;
    Mixing.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;

    Mixing.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    Mixing.colorBlendOp = VK_BLEND_OP_ADD;

    Mixing.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    Mixing.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    Mixing.alphaBlendOp = VK_BLEND_OP_ADD;
    Mixing.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo Blending = { };

    Blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    Blending.attachmentCount = 1;

    Blending.pAttachments = &Mixing;

    VkDynamicState Dynamics[ 2 ] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo Changing = { };

    Changing.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    Changing.dynamicStateCount = 2;

    Changing.pDynamicStates = Dynamics;

    VkGraphicsPipelineCreateInfo Recipe = { };

    Recipe.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    Recipe.stageCount = 2;

    Recipe.pStages = Stages;
    Recipe.pVertexInputState = &Feeding;

    Recipe.pInputAssemblyState = &Weaving;
    Recipe.pViewportState = &Framing;

    Recipe.pRasterizationState = &Shaping;
    Recipe.pMultisampleState = &Sampling;

    Recipe.pColorBlendState = &Blending;
    Recipe.pDynamicState = &Changing;

    Recipe.layout = Layout;
    Recipe.renderPass = Pass;

    VkResult Outcome = vkCreateGraphicsPipelines( Device, VK_NULL_HANDLE, 1, &Recipe, nullptr, &Pipelines[ Effect ] );

    vkDestroyShaderModule( Device, VertexStage, nullptr );
    vkDestroyShaderModule( Device, FragmentStage, nullptr );

    if ( Outcome != VK_SUCCESS ) {
        Context->Report( "Could not build a Vulkan pipeline" );

        Pipelines[ Effect ] = VK_NULL_HANDLE;
        return Effect == 0 ? VK_NULL_HANDLE : Pipe( 0 );
    }

    return Pipelines[ Effect ];
}

bool CVulkan::Create( VkPhysicalDevice Adapter, VkDevice Owner, VkQueue Worker, unsigned int Family, VkRenderPass Target, int Frames ) {
    ( void )Family;
    Destroy( );

    if ( !Adapter || !Owner || !Worker || !Target || Frames < 1 || Frames > 8 )
        return false;

    Physical = Adapter;
    Device = Owner;

    Queue = Worker;
    Pass = Target;

    FrameCount = Frames;
    Rings.resize( Frames );

    VkDescriptorSetLayoutBinding BufferSlot = { };

    BufferSlot.binding = 0;
    BufferSlot.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    BufferSlot.descriptorCount = 1;
    BufferSlot.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo BufferRecipe = { };

    BufferRecipe.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    BufferRecipe.bindingCount = 1;

    BufferRecipe.pBindings = &BufferSlot;
    vkCreateDescriptorSetLayout( Device, &BufferRecipe, nullptr, &BufferShape );

    VkDescriptorSetLayoutBinding ImageSlot = { };

    ImageSlot.binding = 0;
    ImageSlot.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    ImageSlot.descriptorCount = 1;
    ImageSlot.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo ImageRecipe = { };

    ImageRecipe.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ImageRecipe.bindingCount = 1;

    ImageRecipe.pBindings = &ImageSlot;
    vkCreateDescriptorSetLayout( Device, &ImageRecipe, nullptr, &ImageShape );

    VkPushConstantRange Reach = { };

    Reach.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    Reach.size = 20;

    VkDescriptorSetLayout Shapes[ 2 ] = { BufferShape, ImageShape };
    VkPipelineLayoutCreateInfo Plan = { };

    Plan.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    Plan.setLayoutCount = 2;

    Plan.pSetLayouts = Shapes;
    Plan.pushConstantRangeCount = 1;

    Plan.pPushConstantRanges = &Reach;
    vkCreatePipelineLayout( Device, &Plan, nullptr, &Layout );

    VkSamplerCreateInfo Filtering = { };

    Filtering.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    Filtering.magFilter = VK_FILTER_LINEAR;

    Filtering.minFilter = VK_FILTER_LINEAR;
    Filtering.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    Filtering.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    Filtering.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    Filtering.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler( Device, &Filtering, nullptr, &Sampler );

    VkDescriptorPoolSize Sizes[ 2 ] = { };

    Sizes[ 0 ].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    Sizes[ 0 ].descriptorCount = 8;

    Sizes[ 1 ].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    Sizes[ 1 ].descriptorCount = 72;

    VkDescriptorPoolCreateInfo PoolRecipe = { };

    PoolRecipe.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolRecipe.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    PoolRecipe.maxSets = 80;
    PoolRecipe.poolSizeCount = 2;

    PoolRecipe.pPoolSizes = Sizes;
    vkCreateDescriptorPool( Device, &PoolRecipe, nullptr, &Pool );

    if ( !BufferShape || !ImageShape || !Layout || !Sampler || !Pool ) {
        Destroy( );
        return false;
    }

    if ( !Pipe( 0 ) ) {
        Destroy( );
        return false;
    }

    for ( CVulkanFrame& Ring : Rings )
        Ring.Set = CreateSet( BufferShape );

    Album.Set = CreateSet( ImageShape );
    return true;
}

static void Clean( VkDevice Device, CVulkanImage& Image ) {
    if ( Image.View ) {
        vkDestroyImageView( Device, Image.View, nullptr );
        Image.View = VK_NULL_HANDLE;
    }

    if ( Image.Texture ) {
        vkDestroyImage( Device, Image.Texture, nullptr );
        Image.Texture = VK_NULL_HANDLE;
    }

    if ( Image.Memory ) {
        vkFreeMemory( Device, Image.Memory, nullptr );
        Image.Memory = VK_NULL_HANDLE;
    }

    if ( Image.Staging ) {
        vkDestroyBuffer( Device, Image.Staging, nullptr );
        Image.Staging = VK_NULL_HANDLE;
    }

    if ( Image.StagingMemory ) {
        vkFreeMemory( Device, Image.StagingMemory, nullptr );
        Image.StagingMemory = VK_NULL_HANDLE;
    }

    Image.Mapped = nullptr;
    Image.Shown = false;
}

void CVulkan::Destroy( ) {
    if ( !Device )
        return;

    vkDeviceWaitIdle( Device );

    for ( auto& Entry : Images ) {
        if ( Entry.second.Owned )
            Clean( Device, Entry.second );
    }

    Images.clear( );
    Clean( Device, Album );

    for ( CVulkanFrame& Ring : Rings ) {
        for ( CVulkanTomb& Tomb : Ring.Graveyard ) {
            vkDestroyBuffer( Device, Tomb.Buffer, nullptr );
            vkFreeMemory( Device, Tomb.Memory, nullptr );
        }

        Ring.Graveyard.clear( );

        if ( Ring.Storage ) {
            vkDestroyBuffer( Device, Ring.Storage, nullptr );
            vkFreeMemory( Device, Ring.Memory, nullptr );
        }
    }

    Rings.clear( );

    for ( VkPipeline Piece : Pipelines ) {
        if ( Piece )
            vkDestroyPipeline( Device, Piece, nullptr );
    }

    Pipelines.clear( );

    if ( Pool ) {
        vkDestroyDescriptorPool( Device, Pool, nullptr );
        Pool = VK_NULL_HANDLE;
    }

    if ( Sampler ) {
        vkDestroySampler( Device, Sampler, nullptr );
        Sampler = VK_NULL_HANDLE;
    }

    if ( Layout ) {
        vkDestroyPipelineLayout( Device, Layout, nullptr );
        Layout = VK_NULL_HANDLE;
    }

    if ( BufferShape ) {
        vkDestroyDescriptorSetLayout( Device, BufferShape, nullptr );
        BufferShape = VK_NULL_HANDLE;
    }

    if ( ImageShape ) {
        vkDestroyDescriptorSetLayout( Device, ImageShape, nullptr );
        ImageShape = VK_NULL_HANDLE;
    }

    Album = CVulkanImage( );

    Physical = VK_NULL_HANDLE;
    Device = VK_NULL_HANDLE;

    Queue = VK_NULL_HANDLE;
    Pass = VK_NULL_HANDLE;

    NextImage = 1;
    SheetVersion = 0;

    SheetDepth = 0;

    FrameCount = 0;
    Cursor = 0;
}

static void Shift( VkCommandBuffer Orders, VkImage Texture, VkImageLayout Before, VkImageLayout After ) {
    VkImageMemoryBarrier Barrier = { };

    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.oldLayout = Before;

    Barrier.newLayout = After;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.image = Texture;

    Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Barrier.subresourceRange.levelCount = 1;

    Barrier.subresourceRange.layerCount = 1;

    Barrier.srcAccessMask = Before == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ? VK_ACCESS_SHADER_READ_BIT : 0;
    Barrier.dstAccessMask = After == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ? VK_ACCESS_TRANSFER_WRITE_BIT : VK_ACCESS_SHADER_READ_BIT;

    VkPipelineStageFlags Origin = Before == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags Target = After == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ? VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    vkCmdPipelineBarrier( Orders, Origin, Target, 0, 0, nullptr, 0, nullptr, 1, &Barrier );
}

static void Convey( VkCommandBuffer Orders, CVulkanImage& Image ) {
    Shift( Orders, Image.Texture, Image.Shown ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

    VkBufferImageCopy Region = { };

    Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Region.imageSubresource.layerCount = 1;

    Region.imageExtent.width = ( unsigned int )Image.Width;
    Region.imageExtent.height = ( unsigned int )Image.Height;

    Region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage( Orders, Image.Staging, Image.Texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region );
    Shift( Orders, Image.Texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

    Image.Shown = true;
    Image.Dirty = false;
}

void CVulkan::Prepare( void* Stream ) {
    VkCommandBuffer Orders = ( VkCommandBuffer )Stream;

    if ( !Device || !Orders )
        return;

    if ( Sheet->Depth( ) > 0 && ( !Album.Texture || SheetDepth != Sheet->Depth( ) ) ) {
        vkQueueWaitIdle( Queue );

        VkDescriptorSet Kept = Album.Set;
        Clean( Device, Album );

        Album.Set = Kept;

        if ( !CreateStore( Album, Sheet->Extent( ), Sheet->Depth( ), ( unsigned int )VK_FORMAT_R8_UNORM ) )
            return;

        CreateBuffer( ( size_t )Sheet->Extent( ) * Sheet->Depth( ), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, Album.Staging, Album.StagingMemory, &Album.Mapped );

        VkDescriptorImageInfo Sight = { };

        Sight.sampler = Sampler;
        Sight.imageView = Album.View;

        Sight.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet Writing = { };

        Writing.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        Writing.dstSet = Album.Set;

        Writing.descriptorCount = 1;
        Writing.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        Writing.pImageInfo = &Sight;
        vkUpdateDescriptorSets( Device, 1, &Writing, 0, nullptr );

        SheetDepth = Sheet->Depth( );
        SheetVersion = 0;
    }

    if ( Album.Texture && Album.Mapped && SheetVersion != Sheet->Version ) {
        memcpy( Album.Mapped, Sheet->Data( ), ( size_t )Sheet->Extent( ) * Sheet->Depth( ) );
        Convey( Orders, Album );

        SheetVersion = Sheet->Version;
    }

    for ( auto& Entry : Images ) {
        if ( Entry.second.Dirty && Entry.second.Owned && Entry.second.Texture )
            Convey( Orders, Entry.second );
    }
}

bool CVulkan::Reserve( CVulkanFrame& Ring, int Count ) {
    if ( Count <= Ring.Capacity )
        return true;

    int Goal = Ring.Capacity > 0 ? Ring.Capacity : 16384;
    while ( Goal < Count )
        Goal *= 2;

    if ( Ring.Storage ) {
        CVulkanTomb Tomb;

        Tomb.Buffer = Ring.Storage;
        Tomb.Memory = Ring.Memory;

        Ring.Graveyard.push_back( Tomb );

        Ring.Storage = VK_NULL_HANDLE;
        Ring.Memory = VK_NULL_HANDLE;

        Ring.Mapped = nullptr;
    }

    Ring.Capacity = 0;

    if ( !CreateBuffer( ( size_t )Goal * sizeof( CInstance ), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, Ring.Storage, Ring.Memory, &Ring.Mapped ) ) {
        Context->Report( "Could not grow the Vulkan instance ring" );
        return false;
    }

    VkDescriptorBufferInfo Holding = { };

    Holding.buffer = Ring.Storage;
    Holding.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet Writing = { };

    Writing.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    Writing.dstSet = Ring.Set;

    Writing.descriptorCount = 1;
    Writing.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    Writing.pBufferInfo = &Holding;
    vkUpdateDescriptorSets( Device, 1, &Writing, 0, nullptr );

    Ring.Capacity = Goal;
    return true;
}

unsigned long long CVulkan::CreateImage( const unsigned char* Pixels, int Width, int Height ) {
    if ( !Device || !Pixels || Width <= 0 || Height <= 0 )
        return 0;

    if ( Width > 16384 || Height > 16384 ) {
        Context->Report( "Rejected an image with unreasonable dimensions" );
        return 0;
    }

    CVulkanImage Image;
    Image.Owned = true;

    if ( !CreateStore( Image, Width, Height, ( unsigned int )VK_FORMAT_R8G8B8A8_UNORM ) ) {
        Clean( Device, Image );

        Context->Report( "Could not create a Vulkan image" );
        return 0;
    }

    if ( !CreateBuffer( ( size_t )Width * Height * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, Image.Staging, Image.StagingMemory, &Image.Mapped ) ) {
        Clean( Device, Image );
        return 0;
    }

    memcpy( Image.Mapped, Pixels, ( size_t )Width * Height * 4 );
    Image.Dirty = true;

    Image.Set = CreateSet( ImageShape );

    VkDescriptorImageInfo Sight = { };

    Sight.sampler = Sampler;
    Sight.imageView = Image.View;

    Sight.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet Writing = { };

    Writing.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    Writing.dstSet = Image.Set;

    Writing.descriptorCount = 1;
    Writing.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    Writing.pImageInfo = &Sight;
    vkUpdateDescriptorSets( Device, 1, &Writing, 0, nullptr );

    unsigned long long Handle = NextImage;
    NextImage++;

    Images[ Handle ] = Image;
    return Handle;
}

void CVulkan::UpdateImage( unsigned long long Image, const unsigned char* Pixels ) {
    auto Entry = Images.find( Image );
    if ( Entry == Images.end( ) || !Entry->second.Owned || !Pixels )
        return;

    CVulkanImage& Picture = Entry->second;

    if ( Picture.Staging && !Rings.empty( ) ) {
        CVulkanTomb Tomb;

        Tomb.Buffer = Picture.Staging;
        Tomb.Memory = Picture.StagingMemory;

        Rings[ Cursor ].Graveyard.push_back( Tomb );

        Picture.Staging = VK_NULL_HANDLE;
        Picture.StagingMemory = VK_NULL_HANDLE;

        Picture.Mapped = nullptr;
    }

    if ( !CreateBuffer( ( size_t )Picture.Width * Picture.Height * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, Picture.Staging, Picture.StagingMemory, &Picture.Mapped ) )
        return;

    if ( !Picture.Mapped )
        return;

    memcpy( Picture.Mapped, Pixels, ( size_t )Picture.Width * Picture.Height * 4 );
    Picture.Dirty = true;
}

unsigned long long CVulkan::AdoptImage( void* Native ) {
    if ( !Device || !Native )
        return 0;

    CVulkanImage Image;

    Image.Owned = false;
    Image.Shown = true;

    Image.Set = CreateSet( ImageShape );

    VkDescriptorImageInfo Sight = { };

    Sight.sampler = Sampler;
    Sight.imageView = ( VkImageView )Native;

    Sight.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet Writing = { };

    Writing.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    Writing.dstSet = Image.Set;

    Writing.descriptorCount = 1;
    Writing.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    Writing.pImageInfo = &Sight;
    vkUpdateDescriptorSets( Device, 1, &Writing, 0, nullptr );

    unsigned long long Handle = NextImage;
    NextImage++;

    Images[ Handle ] = Image;
    return Handle;
}

void CVulkan::DestroyImage( unsigned long long Image ) {
    auto Entry = Images.find( Image );
    if ( Entry == Images.end( ) )
        return;

    vkQueueWaitIdle( Queue );

    if ( Entry->second.Owned )
        Clean( Device, Entry->second );

    if ( Entry->second.Set )
        vkFreeDescriptorSets( Device, Pool, 1, &Entry->second.Set );

    Images.erase( Entry );
}

void CVulkan::Render( const CDrawData& Data, void* Stream ) {
    VkCommandBuffer Orders = ( VkCommandBuffer )Stream;

    if ( !Device || !Orders || Rings.empty( ) || Data.Extent.Horizontal <= 0.0f || Data.Extent.Vertical <= 0.0f )
        return;

    Cursor = ( Cursor + 1 ) % FrameCount;
    CVulkanFrame& Ring = Rings[ Cursor ];

    for ( CVulkanTomb& Tomb : Ring.Graveyard ) {
        vkDestroyBuffer( Device, Tomb.Buffer, nullptr );
        vkFreeMemory( Device, Tomb.Memory, nullptr );
    }

    Ring.Graveyard.clear( );

    if ( Data.BatchCount <= 0 )
        return;

    if ( Data.Count > 0 && Data.Items ) {
        if ( !Reserve( Ring, Data.Count ) )
            return;

        memcpy( Ring.Mapped, Data.Items, ( size_t )Data.Count * sizeof( CInstance ) );
    }

    VkViewport Viewport = { };

    Viewport.width = Data.Extent.Horizontal;
    Viewport.height = Data.Extent.Vertical;

    Viewport.maxDepth = 1.0f;

    VkRect2D Scissor = { };

    Scissor.extent.width = ( unsigned int )Data.Extent.Horizontal;
    Scissor.extent.height = ( unsigned int )Data.Extent.Vertical;

    float Pushed[ 5 ] = { 2.0f / Data.Extent.Horizontal, 2.0f / Data.Extent.Vertical, -1.0f, -1.0f, Data.Moment };

    vkCmdSetViewport( Orders, 0, 1, &Viewport );
    vkCmdSetScissor( Orders, 0, 1, &Scissor );

    vkCmdPushConstants( Orders, Layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 20, Pushed );
    vkCmdBindDescriptorSets( Orders, VK_PIPELINE_BIND_POINT_GRAPHICS, Layout, 0, 1, &Ring.Set, 0, nullptr );

    for ( int Position = 0; Position < Data.BatchCount; Position++ ) {
        const CBatch& Batch = Data.Batches[ Position ];

        if ( Batch.Callback ) {
            Batch.Callback( Orders, Batch.Detail );

            vkCmdSetViewport( Orders, 0, 1, &Viewport );
            vkCmdSetScissor( Orders, 0, 1, &Scissor );

            vkCmdPushConstants( Orders, Layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 20, Pushed );
            vkCmdBindDescriptorSets( Orders, VK_PIPELINE_BIND_POINT_GRAPHICS, Layout, 0, 1, &Ring.Set, 0, nullptr );

            continue;
        }

        if ( Batch.Count <= 0 )
            continue;

        VkDescriptorSet Bound = Album.Set;

        if ( Batch.Image != 0 ) {
            auto Entry = Images.find( Batch.Image );
            if ( Entry == Images.end( ) )
                continue;

            Bound = Entry->second.Set;
        } else if ( !Album.Texture ) {
            continue;
        }

        VkPipeline Chosen = Pipe( Batch.Effect );
        if ( !Chosen )
            continue;

        vkCmdBindPipeline( Orders, VK_PIPELINE_BIND_POINT_GRAPHICS, Chosen );
        vkCmdBindDescriptorSets( Orders, VK_PIPELINE_BIND_POINT_GRAPHICS, Layout, 1, 1, &Bound, 0, nullptr );

        vkCmdDraw( Orders, 4, ( unsigned int )Batch.Count, 0, ( unsigned int )Batch.Offset );
    }
}

#endif