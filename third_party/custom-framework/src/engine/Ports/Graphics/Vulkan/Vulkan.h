#pragma once

#ifndef UR_VULKAN
#define UR_VULKAN 0
#endif

inline constexpr bool VulkanReady = UR_VULKAN != 0;

#if UR_VULKAN

#include <memory>
#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "Canvas.h"

class CVulkanTomb {
public:
    VkBuffer Buffer = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;
};

class CVulkanFrame {
public:
    VkBuffer Storage = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;

    unsigned char* Mapped = nullptr;
    int Capacity = 0;

    VkDescriptorSet Set = VK_NULL_HANDLE;
    std::vector< CVulkanTomb > Graveyard;
};

class CVulkanImage {
public:
    VkImage Texture = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;

    VkImageView View = VK_NULL_HANDLE;

    VkBuffer Staging = VK_NULL_HANDLE;
    VkDeviceMemory StagingMemory = VK_NULL_HANDLE;

    unsigned char* Mapped = nullptr;
    VkDescriptorSet Set = VK_NULL_HANDLE;

    int Width = 0;
    int Height = 0;

    bool Owned = false;
    bool Dirty = false;
    bool Shown = false;
};

class CVulkan : public CGraphics {
public:
    ~CVulkan( );

    bool Create( VkPhysicalDevice Adapter, VkDevice Owner, VkQueue Worker, unsigned int Family, VkRenderPass Target, int Frames );
    void Destroy( ) override;

    void Prepare( void* Stream );
    void Render( const CDrawData& Data, void* Stream ) override;

    unsigned long long CreateImage( const unsigned char* Pixels, int Width, int Height ) override;
    void UpdateImage( unsigned long long Image, const unsigned char* Pixels ) override;

    unsigned long long AdoptImage( void* Native ) override;
    void DestroyImage( unsigned long long Image ) override;

private:
    unsigned int Locate( unsigned int Mask, unsigned int Wanted ) const;

    bool CreateBuffer( size_t Size, unsigned int Usage, VkBuffer& Buffer, VkDeviceMemory& Memory, unsigned char** Mapped );
    bool CreateStore( CVulkanImage& Image, int Width, int Height, unsigned int Kind );

    VkDescriptorSet CreateSet( VkDescriptorSetLayout Shape );
    bool Reserve( CVulkanFrame& Ring, int Count );

    VkPipeline Pipe( unsigned int Effect );

    VkPhysicalDevice Physical = VK_NULL_HANDLE;
    VkDevice Device = VK_NULL_HANDLE;

    VkQueue Queue = VK_NULL_HANDLE;
    VkRenderPass Pass = VK_NULL_HANDLE;

    VkPipelineLayout Layout = VK_NULL_HANDLE;
    std::vector< VkPipeline > Pipelines;

    VkDescriptorSetLayout BufferShape = VK_NULL_HANDLE;
    VkDescriptorSetLayout ImageShape = VK_NULL_HANDLE;

    VkDescriptorPool Pool = VK_NULL_HANDLE;
    VkSampler Sampler = VK_NULL_HANDLE;

    CVulkanImage Album;

    std::vector< CVulkanFrame > Rings;
    std::unordered_map< unsigned long long, CVulkanImage > Images;

    unsigned long long NextImage = 1;
    unsigned int SheetVersion = 0;

    int SheetDepth = 0;

    int FrameCount = 0;
    int Cursor = 0;
};

inline auto Vulkan = std::make_unique< CVulkan >( );

#endif