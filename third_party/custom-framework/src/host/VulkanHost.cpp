#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "VulkanHost.h"

#if UR_VULKAN

#include <Windows.h>
#include <vulkan/vulkan_win32.h>

bool CVulkanHost::Create( void* Window, int Width, int Height ) {
    Destroy( );

    if ( !LoadLibraryA( "vulkan-1.dll" ) )
        return false;

    VkApplicationInfo Naming = { };

    Naming.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    Naming.pApplicationName = "UR";

    Naming.apiVersion = VK_API_VERSION_1_1;

    const char* Extensions[ 2 ] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
    VkInstanceCreateInfo Opening = { };

    Opening.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    Opening.pApplicationInfo = &Naming;

    Opening.enabledExtensionCount = 2;
    Opening.ppEnabledExtensionNames = Extensions;

    if ( vkCreateInstance( &Opening, nullptr, &Instance ) != VK_SUCCESS )
        return false;

    VkWin32SurfaceCreateInfoKHR Glass = { };

    Glass.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    Glass.hinstance = GetModuleHandleA( nullptr );

    Glass.hwnd = ( HWND )Window;

    if ( vkCreateWin32SurfaceKHR( Instance, &Glass, nullptr, &Surface ) != VK_SUCCESS ) {
        Destroy( );
        return false;
    }

    unsigned int Count = 0;
    vkEnumeratePhysicalDevices( Instance, &Count, nullptr );

    if ( Count == 0 ) {
        Destroy( );
        return false;
    }

    std::vector< VkPhysicalDevice > Devices( Count );
    vkEnumeratePhysicalDevices( Instance, &Count, Devices.data( ) );

    for ( VkPhysicalDevice Candidate : Devices ) {
        unsigned int Households = 0;
        vkGetPhysicalDeviceQueueFamilyProperties( Candidate, &Households, nullptr );

        std::vector< VkQueueFamilyProperties > Properties( Households );
        vkGetPhysicalDeviceQueueFamilyProperties( Candidate, &Households, Properties.data( ) );

        for ( unsigned int Household = 0; Household < Households; Household++ ) {
            VkBool32 Showing = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR( Candidate, Household, Surface, &Showing );

            if ( ( Properties[ Household ].queueFlags & VK_QUEUE_GRAPHICS_BIT ) && Showing ) {
                Physical = Candidate;
                Family = Household;

                break;
            }
        }

        if ( Physical )
            break;
    }

    if ( !Physical ) {
        Destroy( );
        return false;
    }

    float Priority = 1.0f;

    VkDeviceQueueCreateInfo Lining = { };

    Lining.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    Lining.queueFamilyIndex = Family;

    Lining.queueCount = 1;
    Lining.pQueuePriorities = &Priority;

    const char* Needed[ 1 ] = { "VK_KHR_swapchain" };

    VkDeviceCreateInfo Fitting = { };

    Fitting.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    Fitting.queueCreateInfoCount = 1;

    Fitting.pQueueCreateInfos = &Lining;
    Fitting.enabledExtensionCount = 1;

    Fitting.ppEnabledExtensionNames = Needed;

    if ( vkCreateDevice( Physical, &Fitting, nullptr, &Device ) != VK_SUCCESS ) {
        Destroy( );
        return false;
    }

    vkGetDeviceQueue( Device, Family, 0, &Queue );

    unsigned int Choices = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR( Physical, Surface, &Choices, nullptr );

    std::vector< VkSurfaceFormatKHR > Palettes( Choices );
    vkGetPhysicalDeviceSurfaceFormatsKHR( Physical, Surface, &Choices, Palettes.data( ) );

    Format = ( unsigned int )Palettes[ 0 ].format;

    for ( const VkSurfaceFormatKHR& Palette : Palettes ) {
        if ( Palette.format == VK_FORMAT_B8G8R8A8_UNORM ) {
            Format = ( unsigned int )Palette.format;
            break;
        }
    }

    VkAttachmentDescription Coating = { };

    Coating.format = ( VkFormat )Format;
    Coating.samples = VK_SAMPLE_COUNT_1_BIT;

    Coating.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    Coating.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    Coating.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Coating.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    Coating.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    Coating.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference Aiming = { };
    Aiming.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription Painting = { };

    Painting.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Painting.colorAttachmentCount = 1;

    Painting.pColorAttachments = &Aiming;

    VkRenderPassCreateInfo Staging = { };

    Staging.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    Staging.attachmentCount = 1;

    Staging.pAttachments = &Coating;
    Staging.subpassCount = 1;

    Staging.pSubpasses = &Painting;

    if ( vkCreateRenderPass( Device, &Staging, nullptr, &Pass ) != VK_SUCCESS ) {
        Destroy( );
        return false;
    }

    VkCommandPoolCreateInfo Pooling = { };

    Pooling.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    Pooling.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    Pooling.queueFamilyIndex = Family;

    if ( vkCreateCommandPool( Device, &Pooling, nullptr, &Pool ) != VK_SUCCESS ) {
        Destroy( );
        return false;
    }

    VkCommandBufferAllocateInfo Ordering = { };

    Ordering.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    Ordering.commandPool = Pool;

    Ordering.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    Ordering.commandBufferCount = 2;

    if ( vkAllocateCommandBuffers( Device, &Ordering, Orders ) != VK_SUCCESS ) {
        Destroy( );
        return false;
    }

    for ( int Position = 0; Position < 2; Position++ ) {
        VkSemaphoreCreateInfo Signal = { };
        Signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        vkCreateSemaphore( Device, &Signal, nullptr, &Ready[ Position ] );

        VkFenceCreateInfo Guarding = { };

        Guarding.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        Guarding.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        vkCreateFence( Device, &Guarding, nullptr, &Guards[ Position ] );
    }

    SurfaceWidth = Width;
    SurfaceHeight = Height;

    CurrentSync = true;

    if ( !CreateChain( ) ) {
        Destroy( );
        return false;
    }

    if ( !Vulkan->Create( Physical, Device, Queue, Family, Pass, 2 ) ) {
        Destroy( );
        return false;
    }

    return true;
}

bool CVulkanHost::CreateChain( ) {
    ReleaseChain( );

    VkSurfaceCapabilitiesKHR Limits = { };
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( Physical, Surface, &Limits );

    VkExtent2D Extent = Limits.currentExtent;

    if ( Extent.width == 0 || Extent.height == 0 )
        return false;

    unsigned int Depth = Limits.minImageCount + 1;

    if ( Limits.maxImageCount > 0 && Depth > Limits.maxImageCount )
        Depth = Limits.maxImageCount;

    VkPresentModeKHR Pacing = VK_PRESENT_MODE_FIFO_KHR;

    if ( !CurrentSync ) {
        unsigned int Count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR( Physical, Surface, &Count, nullptr );

        std::vector< VkPresentModeKHR > Modes( Count );
        vkGetPhysicalDeviceSurfacePresentModesKHR( Physical, Surface, &Count, Modes.data( ) );

        for ( VkPresentModeKHR Mode : Modes ) {
            if ( Mode == VK_PRESENT_MODE_IMMEDIATE_KHR || Mode == VK_PRESENT_MODE_MAILBOX_KHR )
                Pacing = Mode;
        }
    }

    VkSwapchainCreateInfoKHR Chain = { };

    Chain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    Chain.surface = Surface;

    Chain.minImageCount = Depth;
    Chain.imageFormat = ( VkFormat )Format;

    Chain.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    Chain.imageExtent = Extent;

    Chain.imageArrayLayers = 1;
    Chain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    Chain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    Chain.preTransform = Limits.currentTransform;

    Chain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    Chain.presentMode = Pacing;

    Chain.clipped = VK_TRUE;

    if ( vkCreateSwapchainKHR( Device, &Chain, nullptr, &Swapchain ) != VK_SUCCESS )
        return false;

    SurfaceWidth = ( int )Extent.width;
    SurfaceHeight = ( int )Extent.height;

    unsigned int Count = 0;
    vkGetSwapchainImagesKHR( Device, Swapchain, &Count, nullptr );

    Images.resize( Count );
    vkGetSwapchainImagesKHR( Device, Swapchain, &Count, Images.data( ) );

    Sights.resize( Count );
    Buffers.resize( Count );

    Finished.resize( Count );

    for ( unsigned int Position = 0; Position < Count; Position++ ) {
        VkImageViewCreateInfo Sight = { };

        Sight.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        Sight.image = Images[ Position ];

        Sight.viewType = VK_IMAGE_VIEW_TYPE_2D;
        Sight.format = ( VkFormat )Format;

        Sight.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Sight.subresourceRange.levelCount = 1;

        Sight.subresourceRange.layerCount = 1;

        if ( vkCreateImageView( Device, &Sight, nullptr, &Sights[ Position ] ) != VK_SUCCESS )
            return false;

        VkFramebufferCreateInfo Framing = { };

        Framing.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        Framing.renderPass = Pass;

        Framing.attachmentCount = 1;
        Framing.pAttachments = &Sights[ Position ];

        Framing.width = Extent.width;
        Framing.height = Extent.height;

        Framing.layers = 1;

        if ( vkCreateFramebuffer( Device, &Framing, nullptr, &Buffers[ Position ] ) != VK_SUCCESS )
            return false;

        VkSemaphoreCreateInfo Signal = { };
        Signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if ( vkCreateSemaphore( Device, &Signal, nullptr, &Finished[ Position ] ) != VK_SUCCESS )
            return false;
    }

    Rebuild = false;
    return true;
}

void CVulkanHost::ReleaseChain( ) {
    if ( !Device )
        return;

    vkDeviceWaitIdle( Device );

    for ( VkSemaphore Signal : Finished )
        vkDestroySemaphore( Device, Signal, nullptr );

    Finished.clear( );

    for ( VkFramebuffer Framing : Buffers )
        vkDestroyFramebuffer( Device, Framing, nullptr );

    Buffers.clear( );

    for ( VkImageView Sight : Sights )
        vkDestroyImageView( Device, Sight, nullptr );

    Sights.clear( );
    Images.clear( );

    if ( Swapchain ) {
        vkDestroySwapchainKHR( Device, Swapchain, nullptr );
        Swapchain = VK_NULL_HANDLE;
    }
}

void CVulkanHost::Destroy( ) {
    if ( Device )
        vkDeviceWaitIdle( Device );

    Vulkan->Destroy( );
    ReleaseChain( );

    if ( Device ) {
        for ( int Position = 0; Position < 2; Position++ ) {
            if ( Ready[ Position ] ) {
                vkDestroySemaphore( Device, Ready[ Position ], nullptr );
                Ready[ Position ] = VK_NULL_HANDLE;
            }

            if ( Guards[ Position ] ) {
                vkDestroyFence( Device, Guards[ Position ], nullptr );
                Guards[ Position ] = VK_NULL_HANDLE;
            }

            Orders[ Position ] = VK_NULL_HANDLE;
        }

        if ( Pool ) {
            vkDestroyCommandPool( Device, Pool, nullptr );
            Pool = VK_NULL_HANDLE;
        }

        if ( Pass ) {
            vkDestroyRenderPass( Device, Pass, nullptr );
            Pass = VK_NULL_HANDLE;
        }

        vkDestroyDevice( Device, nullptr );
        Device = VK_NULL_HANDLE;
    }

    if ( Surface ) {
        vkDestroySurfaceKHR( Instance, Surface, nullptr );
        Surface = VK_NULL_HANDLE;
    }

    if ( Instance ) {
        vkDestroyInstance( Instance, nullptr );
        Instance = VK_NULL_HANDLE;
    }

    Physical = VK_NULL_HANDLE;
    Queue = VK_NULL_HANDLE;

    Family = 0;
    Format = 0;

    Slot = 0;
    Frame = 0;

    SurfaceWidth = 0;
    SurfaceHeight = 0;

    Rebuild = false;
    Skipping = false;

    CurrentSync = true;
}

void CVulkanHost::Resize( int Width, int Height ) {
    ( void )Width;
    ( void )Height;

    Rebuild = true;
}

void CVulkanHost::Begin( CColor Backdrop ) {
    Skipping = true;

    if ( !Device || !Swapchain )
        return;

    if ( Rebuild && !CreateChain( ) )
        return;

    vkWaitForFences( Device, 1, &Guards[ Frame ], VK_TRUE, 5000000000ull );

    VkResult Outcome = vkAcquireNextImageKHR( Device, Swapchain, 5000000000ull, Ready[ Frame ], VK_NULL_HANDLE, &Slot );

    if ( Outcome == VK_ERROR_OUT_OF_DATE_KHR ) {
        if ( !CreateChain( ) )
            return;

        Outcome = vkAcquireNextImageKHR( Device, Swapchain, 5000000000ull, Ready[ Frame ], VK_NULL_HANDLE, &Slot );
    }

    if ( Outcome != VK_SUCCESS && Outcome != VK_SUBOPTIMAL_KHR )
        return;

    vkResetFences( Device, 1, &Guards[ Frame ] );
    VkCommandBufferBeginInfo Opening = { };

    Opening.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    Opening.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkResetCommandBuffer( Orders[ Frame ], 0 );
    vkBeginCommandBuffer( Orders[ Frame ], &Opening );

    Vulkan->Prepare( Orders[ Frame ] );
    VkClearValue Wash = { };

    Wash.color.float32[ 0 ] = Backdrop.Red / 255.0f;
    Wash.color.float32[ 1 ] = Backdrop.Green / 255.0f;

    Wash.color.float32[ 2 ] = Backdrop.Blue / 255.0f;
    Wash.color.float32[ 3 ] = Backdrop.Alpha / 255.0f;

    VkRenderPassBeginInfo Painting = { };

    Painting.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    Painting.renderPass = Pass;

    Painting.framebuffer = Buffers[ Slot ];
    Painting.renderArea.extent.width = ( unsigned int )SurfaceWidth;

    Painting.renderArea.extent.height = ( unsigned int )SurfaceHeight;
    Painting.clearValueCount = 1;

    Painting.pClearValues = &Wash;

    vkCmdBeginRenderPass( Orders[ Frame ], &Painting, VK_SUBPASS_CONTENTS_INLINE );
    Skipping = false;
}

void* CVulkanHost::Stream( ) {
    return Orders[ Frame ];
}

CGraphics* CVulkanHost::Graphics( ) {
    return Vulkan.get( );
}

void CVulkanHost::End( bool VerticalSync ) {
    if ( VerticalSync != CurrentSync ) {
        CurrentSync = VerticalSync;
        Rebuild = true;
    }

    if ( Skipping )
        return;

    vkCmdEndRenderPass( Orders[ Frame ] );
    vkEndCommandBuffer( Orders[ Frame ] );

    VkPipelineStageFlags Waiting = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo Sending = { };

    Sending.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    Sending.waitSemaphoreCount = 1;

    Sending.pWaitSemaphores = &Ready[ Frame ];
    Sending.pWaitDstStageMask = &Waiting;

    Sending.commandBufferCount = 1;
    Sending.pCommandBuffers = &Orders[ Frame ];

    Sending.signalSemaphoreCount = 1;
    Sending.pSignalSemaphores = &Finished[ Slot ];

    vkQueueSubmit( Queue, 1, &Sending, Guards[ Frame ] );

    VkPresentInfoKHR Showing = { };

    Showing.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    Showing.waitSemaphoreCount = 1;

    Showing.pWaitSemaphores = &Finished[ Slot ];
    Showing.swapchainCount = 1;

    Showing.pSwapchains = &Swapchain;
    Showing.pImageIndices = &Slot;

    VkResult Outcome = vkQueuePresentKHR( Queue, &Showing );

    if ( Outcome == VK_ERROR_OUT_OF_DATE_KHR || Outcome == VK_SUBOPTIMAL_KHR )
        Rebuild = true;

    Frame = ( Frame + 1 ) % 2;
}

#endif