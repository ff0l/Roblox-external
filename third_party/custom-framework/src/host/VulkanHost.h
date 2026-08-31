#pragma once

#include <memory>
#include <vector>

#include "Hosts.h"
#include "Vulkan.h"

#if UR_VULKAN

class CVulkanHost : public CHost {
public:
    bool Create( void* Window, int Width, int Height ) override;
    void Destroy( ) override;

    void Resize( int Width, int Height ) override;
    void Begin( CColor Backdrop ) override;

    void* Stream( ) override;
    CGraphics* Graphics( ) override;

    void End( bool VerticalSync ) override;

private:
    bool CreateChain( );
    void ReleaseChain( );

    VkInstance Instance = VK_NULL_HANDLE;
    VkSurfaceKHR Surface = VK_NULL_HANDLE;

    VkPhysicalDevice Physical = VK_NULL_HANDLE;
    VkDevice Device = VK_NULL_HANDLE;

    VkQueue Queue = VK_NULL_HANDLE;
    VkRenderPass Pass = VK_NULL_HANDLE;

    VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
    VkCommandPool Pool = VK_NULL_HANDLE;

    VkCommandBuffer Orders[ 2 ] = { };
    VkSemaphore Ready[ 2 ] = { };

    VkFence Guards[ 2 ] = { };

    std::vector< VkImage > Images;
    std::vector< VkImageView > Sights;

    std::vector< VkFramebuffer > Buffers;
    std::vector< VkSemaphore > Finished;

    unsigned int Family = 0;
    unsigned int Format = 0;

    unsigned int Slot = 0;
    int Frame = 0;

    int SurfaceWidth = 0;
    int SurfaceHeight = 0;

    bool Rebuild = false;
    bool Skipping = false;

    bool CurrentSync = true;
};

inline auto VulkanHost = std::make_unique< CVulkanHost >( );

#endif