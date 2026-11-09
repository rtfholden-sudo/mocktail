// Includes changes by vii from CoderDayton/nightcap.
// Android Vulkan loader ABI -> host Vulkan loader + SDL3 WSI.

#include <dlfcn.h>
#include <time.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <vector>

#include "mocktail/graphics/android_vulkan_wsi_adapter.h"
#include "mocktail/graphics/present_mode_policy.h"
#include "mocktail/graphics/render_scale_policy.h"
#include "mocktail/graphics/vulkan_etc2_emulation.h"
#include "mocktail/graphics/vulkan_text_overlay_compositor.h"
#include "mocktail/platform/platform_runtime.h"

extern "C" {
VKAPI_ATTR VkResult VKAPI_CALL
vkCreateInstance(const VkInstanceCreateInfo* create_info,
                 const VkAllocationCallbacks* allocator, VkInstance* instance);
VKAPI_ATTR void VKAPI_CALL
vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* allocator);
VKAPI_ATTR VkResult VKAPI_CALL vkCreateAndroidSurfaceKHR(
    VkInstance instance, const void* create_info,
    const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface);
VKAPI_ATTR void VKAPI_CALL
vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface,
                    const VkAllocationCallbacks* allocator);
VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
    const char* layer_name, std::uint32_t* property_count,
    VkExtensionProperties* properties);
VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
    std::uint32_t* property_count, VkLayerProperties* properties);
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddr(VkInstance instance, const char* name);
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device,
                                                             const char* name);
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
    VkPhysicalDevice physical_device, const VkDeviceCreateInfo* create_info,
    const VkAllocationCallbacks* allocator, VkDevice* device);
VKAPI_ATTR void VKAPI_CALL
vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* allocator);
VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice device,
                                            std::uint32_t queue_family_index,
                                            std::uint32_t queue_index,
                                            VkQueue* queue);
VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue2(
    VkDevice device, const VkDeviceQueueInfo2* queue_info, VkQueue* queue);
VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(
    VkDevice device, const VkSwapchainCreateInfoKHR* create_info,
    const VkAllocationCallbacks* allocator, VkSwapchainKHR* swapchain);
VKAPI_ATTR void VKAPI_CALL
vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain,
                      const VkAllocationCallbacks* allocator);
VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(
    VkDevice device, VkSwapchainKHR swapchain, std::uint64_t timeout,
    VkSemaphore semaphore, VkFence fence, std::uint32_t* image_index);
VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImage2KHR(
    VkDevice device, const VkAcquireNextImageInfoKHR* acquire_info,
    std::uint32_t* image_index);
VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(
    VkDevice device, std::uint32_t fence_count, const VkFence* fences,
    VkBool32 wait_all, std::uint64_t timeout);
VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(
    VkDevice device, std::uint32_t fence_count, const VkFence* fences);
VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(
    VkDevice device, VkCommandPool command_pool,
    VkCommandPoolResetFlags flags);
VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(
    VkDevice device, VkQueryPool query_pool, std::uint32_t first_query,
    std::uint32_t query_count, std::size_t data_size, void* data,
    VkDeviceSize stride, VkQueryResultFlags flags);
VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(
    VkDevice device, const VkCommandBufferAllocateInfo* allocate_info,
    VkCommandBuffer* command_buffers);
VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(
    VkDevice device, VkCommandPool command_pool,
    std::uint32_t command_buffer_count,
    const VkCommandBuffer* command_buffers);
VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(
    VkDevice device, VkCommandPool command_pool,
    const VkAllocationCallbacks* allocator);
VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(
    VkCommandBuffer command_buffer,
    const VkCommandBufferBeginInfo* begin_info);
VKAPI_ATTR VkResult VKAPI_CALL
vkEndCommandBuffer(VkCommandBuffer command_buffer);
VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(
    VkCommandBuffer command_buffer, VkCommandBufferResetFlags flags);
VKAPI_ATTR VkResult VKAPI_CALL vkWaitSemaphores(
    VkDevice device, const VkSemaphoreWaitInfo* wait_info,
    std::uint64_t timeout);
VKAPI_ATTR VkResult VKAPI_CALL vkWaitSemaphoresKHR(
    VkDevice device, const VkSemaphoreWaitInfo* wait_info,
    std::uint64_t timeout);
VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(
    VkQueue queue, std::uint32_t submit_count, const VkSubmitInfo* submits,
    VkFence fence);
VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit2(
    VkQueue queue, std::uint32_t submit_count, const VkSubmitInfo2* submits,
    VkFence fence);
VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit2KHR(
    VkQueue queue, std::uint32_t submit_count, const VkSubmitInfo2* submits,
    VkFence fence);
VKAPI_ATTR VkResult VKAPI_CALL vkQueueBindSparse(
    VkQueue queue, std::uint32_t bind_info_count,
    const VkBindSparseInfo* bind_info, VkFence fence);
VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(VkQueue queue);
VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(VkDevice device);
VKAPI_ATTR VkResult VKAPI_CALL
vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* present_info);
VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(
    VkCommandBuffer command_buffer, std::uint32_t first_viewport,
    std::uint32_t viewport_count, const VkViewport* viewports);
VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(
    VkCommandBuffer command_buffer, std::uint32_t first_scissor,
    std::uint32_t scissor_count, const VkRect2D* scissors);
VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWithCount(
    VkCommandBuffer command_buffer, std::uint32_t viewport_count,
    const VkViewport* viewports);
VKAPI_ATTR void VKAPI_CALL vkCmdSetScissorWithCount(
    VkCommandBuffer command_buffer, std::uint32_t scissor_count,
    const VkRect2D* scissors);
VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWithCountEXT(
    VkCommandBuffer command_buffer, std::uint32_t viewport_count,
    const VkViewport* viewports);
VKAPI_ATTR void VKAPI_CALL vkCmdSetScissorWithCountEXT(
    VkCommandBuffer command_buffer, std::uint32_t scissor_count,
    const VkRect2D* scissors);
VKAPI_ATTR void VKAPI_CALL vkCmdBeginRendering(
    VkCommandBuffer command_buffer, const VkRenderingInfo* rendering_info);
VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderingKHR(
    VkCommandBuffer command_buffer, const VkRenderingInfoKHR* rendering_info);
VKAPI_ATTR void VKAPI_CALL vkCmdEndRendering(VkCommandBuffer command_buffer);
VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderingKHR(VkCommandBuffer command_buffer);
VKAPI_ATTR void VKAPI_CALL vkDestroyImageView(
    VkDevice device, VkImageView image_view,
    const VkAllocationCallbacks* allocator);
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    VkPhysicalDevice physical_device, VkSurfaceKHR surface,
    VkSurfaceCapabilitiesKHR* capabilities);
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(
    VkPhysicalDevice physical_device, VkSurfaceKHR surface,
    std::uint32_t* present_mode_count, VkPresentModeKHR* present_modes);
}

namespace {

using BackendWindowFn = void* (*)();
using UsesDirectVulkanFn = bool (*)();
using NotePresentFn = void (*)();
using NoteHostPresentBeginFn = void (*)();
using NoteHostPresentEndFn = void (*)(std::int32_t);
using NoteVulkanCallBeginFn = std::uint64_t (*)(const char*);
using NoteVulkanCallEndFn = void (*)(std::uint64_t, std::int32_t);
using NoteSurfaceOutOfDateFn = void (*)();
using WindowDimensionFn = int (*)();

struct AdapterState {
  struct DeviceDispatch {
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    PFN_vkQueuePresentKHR queue_present = nullptr;
    PFN_vkQueueSubmit queue_submit = nullptr;
    PFN_vkQueueSubmit2 queue_submit2 = nullptr;
    PFN_vkQueueSubmit2KHR queue_submit2_khr = nullptr;
    PFN_vkQueueBindSparse queue_bind_sparse = nullptr;
    PFN_vkQueueWaitIdle queue_wait_idle = nullptr;
    PFN_vkAcquireNextImageKHR acquire_next_image = nullptr;
    PFN_vkAcquireNextImage2KHR acquire_next_image2 = nullptr;
    PFN_vkWaitForFences wait_for_fences = nullptr;
    PFN_vkWaitSemaphores wait_semaphores = nullptr;
    PFN_vkWaitSemaphoresKHR wait_semaphores_khr = nullptr;
    PFN_vkDeviceWaitIdle device_wait_idle = nullptr;
    PFN_vkResetFences reset_fences = nullptr;
    PFN_vkResetCommandPool reset_command_pool = nullptr;
    PFN_vkGetQueryPoolResults get_query_pool_results = nullptr;
    PFN_vkAllocateCommandBuffers allocate_command_buffers = nullptr;
    PFN_vkFreeCommandBuffers free_command_buffers = nullptr;
    PFN_vkDestroyCommandPool destroy_command_pool = nullptr;
    PFN_vkBeginCommandBuffer begin_command_buffer = nullptr;
    PFN_vkEndCommandBuffer end_command_buffer = nullptr;
    PFN_vkResetCommandBuffer reset_command_buffer = nullptr;
    PFN_vkCmdSetViewport cmd_set_viewport = nullptr;
    PFN_vkCmdSetScissor cmd_set_scissor = nullptr;
    PFN_vkCmdSetViewportWithCount cmd_set_viewport_with_count = nullptr;
    PFN_vkCmdSetScissorWithCount cmd_set_scissor_with_count = nullptr;
    PFN_vkCmdBeginRendering begin_rendering = nullptr;
    PFN_vkCmdEndRendering cmd_end_rendering = nullptr;
    PFN_vkDestroyImageView destroy_image_view = nullptr;
  };

  struct QueueBinding {
    VkQueue queue = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
  };

  struct CommandBufferBinding {
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
  };

  std::mutex mutex;
  mocktail::graphics::SdlVulkanWsi host_wsi;
  mocktail::graphics::AndroidVulkanWsiAdapter android_wsi;
  PFN_vkGetInstanceProcAddr host_get_instance_proc_addr = nullptr;
  std::atomic<PFN_vkGetDeviceProcAddr> host_get_device_proc_addr{nullptr};
  std::vector<DeviceDispatch> device_dispatches;
  std::vector<QueueBinding> queue_bindings;
  std::vector<CommandBufferBinding> command_buffer_bindings;
  PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR host_surface_capabilities =
      nullptr;
  PFN_vkGetPhysicalDeviceSurfacePresentModesKHR host_surface_present_modes =
      nullptr;
  VkInstance latest_instance = VK_NULL_HANDLE;
  size_t active_instance_count = 0;
  mocktail::graphics::VulkanTextOverlayCompositor text_overlay;
  mocktail::graphics::VulkanEtc2Emulation etc2;
  std::atomic<NotePresentFn> note_present{nullptr};
  std::atomic<NoteHostPresentBeginFn> note_host_present_begin{nullptr};
  std::atomic<NoteHostPresentEndFn> note_host_present_end{nullptr};
  std::atomic<NoteVulkanCallBeginFn> note_vulkan_call_begin{nullptr};
  std::atomic<NoteVulkanCallEndFn> note_vulkan_call_end{nullptr};
  std::atomic<NoteSurfaceOutOfDateFn> note_surface_out_of_date{nullptr};
  bool extent_translation_logged = false;
  bool scale_extent_logged = false;
  VkExtent2D advertised_framebuffer{0, 0};
  VkExtent2D swapchain_extent{0, 0};
  std::unordered_set<std::uint64_t> swapchain_images;
  std::unordered_map<std::uint64_t, std::uint64_t> view_to_image;
  std::unordered_set<std::uint64_t> present_pass_commands;
  bool present_policy_logged = false;
  bool initialized = false;
};

static std::atomic<bool> g_vulkan_call_observation_active{false};

AdapterState& State() {
  static AdapterState state;
  return state;
}

template <typename Function>
Function ResolveProcessFunction(const char* name) {
  return reinterpret_cast<Function>(dlsym(RTLD_DEFAULT, name));
}

bool EnsureInitialized() {
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  if (state.initialized) {
    return true;
  }

  const BackendWindowFn backend_window =
      ResolveProcessFunction<BackendWindowFn>("mocktail_window_backend_window");
  const UsesDirectVulkanFn uses_direct_vulkan =
      ResolveProcessFunction<UsesDirectVulkanFn>(
          "mocktail_window_uses_direct_vulkan");
  state.note_present.store(ResolveProcessFunction<NotePresentFn>(
                                "mocktail_window_note_vulkan_present"),
                           std::memory_order_release);
  state.note_host_present_begin.store(
      ResolveProcessFunction<NoteHostPresentBeginFn>(
          "mocktail_window_note_vulkan_host_present_begin"),
      std::memory_order_release);
  state.note_host_present_end.store(
      ResolveProcessFunction<NoteHostPresentEndFn>(
          "mocktail_window_note_vulkan_host_present_end"),
      std::memory_order_release);
  const auto call_begin = ResolveProcessFunction<NoteVulkanCallBeginFn>(
      "mocktail_window_note_vulkan_call_begin");
  const auto call_end = ResolveProcessFunction<NoteVulkanCallEndFn>(
      "mocktail_window_note_vulkan_call_end");
  state.note_vulkan_call_begin.store(call_begin, std::memory_order_release);
  state.note_vulkan_call_end.store(call_end, std::memory_order_release);
  g_vulkan_call_observation_active.store(
      call_begin != nullptr && call_end != nullptr, std::memory_order_release);
  state.note_surface_out_of_date.store(
      ResolveProcessFunction<NoteSurfaceOutOfDateFn>(
          "mocktail_window_note_vulkan_surface_out_of_date"),
      std::memory_order_release);
  if (backend_window == nullptr || uses_direct_vulkan == nullptr ||
      !uses_direct_vulkan() || backend_window() == nullptr) {
    std::fprintf(stderr,
                 "  [vulkan] direct SDL Vulkan window is unavailable\n");
    return false;
  }

  mocktail::platform::NativeWindowDescriptor descriptor;
  descriptor.surface_api = mocktail::platform::WindowSurfaceApi::kDirectVulkan;
  descriptor.backend_window = backend_window();
  mocktail::Status status = state.host_wsi.Initialize(descriptor);
  if (!status.ok()) {
    std::fprintf(stderr, "  [vulkan] SDL WSI initialization failed: %s\n",
                 status.message().c_str());
    return false;
  }
  status = state.android_wsi.Initialize(&state.host_wsi);
  if (!status.ok()) {
    std::fprintf(stderr, "  [vulkan] Android WSI initialization failed: %s\n",
                 status.message().c_str());
    state.host_wsi.Shutdown();
    return false;
  }
  state.host_get_instance_proc_addr =
      reinterpret_cast<PFN_vkGetInstanceProcAddr>(
          state.host_wsi.GetInstanceProcAddress());
  if (state.host_get_instance_proc_addr == nullptr) {
    state.android_wsi.Shutdown();
    state.host_wsi.Shutdown();
    return false;
  }
  state.initialized = true;
  std::fprintf(stderr, "  [vulkan] Android WSI -> SDL3 host adapter ready\n");
  return true;
}

void NoteSuboptimalTranslation() {
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true, std::memory_order_relaxed)) {
    std::fprintf(stderr,
                 "  [vulkan] accepted VK_SUBOPTIMAL_KHR; the acquired image "
                 "remains usable\n");
  }
}

VkResult NormalizeSwapchainResult(VkResult result) {
  if (result == VK_SUBOPTIMAL_KHR) {
    NoteSuboptimalTranslation();
  }
  return mocktail::graphics::NormalizeAndroidSwapchainResult(result);
}

PFN_vkVoidFunction HostInstanceProc(VkInstance instance, const char* name) {
  if (!EnsureInitialized() || name == nullptr) {
    return nullptr;
  }
  return State().host_get_instance_proc_addr(instance, name);
}

PFN_vkVoidFunction HostDeviceProc(VkDevice device, const char* name) {
  if (!EnsureInitialized() || name == nullptr) {
    return nullptr;
  }
  const PFN_vkGetDeviceProcAddr host_get_device_proc_addr =
      State().host_get_device_proc_addr.load(std::memory_order_acquire);
  return host_get_device_proc_addr != nullptr
             ? host_get_device_proc_addr(device, name)
             : nullptr;
}

static std::atomic<uint64_t> g_dispatch_generation{1};

struct ThreadQueueDispatchCache {
  uint64_t generation = 0;
  VkQueue queue = VK_NULL_HANDLE;
  AdapterState::DeviceDispatch dispatch{};
};
static thread_local ThreadQueueDispatchCache t_queue_cache;

struct ThreadDeviceDispatchCache {
  uint64_t generation = 0;
  VkDevice device = VK_NULL_HANDLE;
  AdapterState::DeviceDispatch dispatch{};
};
static thread_local ThreadDeviceDispatchCache t_device_cache;

struct ThreadCommandBufferDispatchCache {
  uint64_t generation = 0;
  VkCommandBuffer command_buffer = VK_NULL_HANDLE;
  AdapterState::DeviceDispatch dispatch{};
};
static thread_local ThreadCommandBufferDispatchCache t_command_buffer_cache;

void InvalidateFastDispatchCaches() {
  g_dispatch_generation.fetch_add(1, std::memory_order_release);
}

static const AdapterState::DeviceDispatch kEmptyDeviceDispatch{};

const AdapterState::DeviceDispatch& HostDispatchForQueue(VkQueue queue) {
  const uint64_t gen = g_dispatch_generation.load(std::memory_order_acquire);
  if (__builtin_expect(queue != VK_NULL_HANDLE &&
                           t_queue_cache.queue == queue &&
                           t_queue_cache.generation == gen,
                       1)) {
    return t_queue_cache.dispatch;
  }
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  const auto binding =
      std::find_if(state.queue_bindings.begin(), state.queue_bindings.end(),
                   [queue](const AdapterState::QueueBinding& candidate) {
                     return candidate.queue == queue;
                   });
  if (binding == state.queue_bindings.end()) {
    return kEmptyDeviceDispatch;
  }
  const auto dispatch = std::find_if(
      state.device_dispatches.begin(), state.device_dispatches.end(),
      [binding](const AdapterState::DeviceDispatch& candidate) {
        return candidate.device == binding->device;
      });
  if (dispatch != state.device_dispatches.end()) {
    t_queue_cache.generation = gen;
    t_queue_cache.queue = queue;
    t_queue_cache.dispatch = *dispatch;
    return t_queue_cache.dispatch;
  }
  return kEmptyDeviceDispatch;
}

const AdapterState::DeviceDispatch& HostDispatchForDevice(VkDevice device) {
  const uint64_t gen = g_dispatch_generation.load(std::memory_order_acquire);
  if (__builtin_expect(device != VK_NULL_HANDLE &&
                           t_device_cache.device == device &&
                           t_device_cache.generation == gen,
                       1)) {
    return t_device_cache.dispatch;
  }
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  const auto dispatch = std::find_if(
      state.device_dispatches.begin(), state.device_dispatches.end(),
      [device](const AdapterState::DeviceDispatch& candidate) {
        return candidate.device == device;
      });
  if (dispatch != state.device_dispatches.end()) {
    t_device_cache.generation = gen;
    t_device_cache.device = device;
    t_device_cache.dispatch = *dispatch;
    return t_device_cache.dispatch;
  }
  return kEmptyDeviceDispatch;
}

const AdapterState::DeviceDispatch& HostDispatchForCommandBuffer(
    VkCommandBuffer command_buffer) {
  const uint64_t gen = g_dispatch_generation.load(std::memory_order_acquire);
  if (__builtin_expect(command_buffer != VK_NULL_HANDLE &&
                           t_command_buffer_cache.command_buffer ==
                               command_buffer &&
                           t_command_buffer_cache.generation == gen,
                       1)) {
    return t_command_buffer_cache.dispatch;
  }
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  const auto binding = std::find_if(
      state.command_buffer_bindings.begin(),
      state.command_buffer_bindings.end(),
      [command_buffer](const AdapterState::CommandBufferBinding& candidate) {
        return candidate.command_buffer == command_buffer;
      });
  if (binding == state.command_buffer_bindings.end()) {
    return kEmptyDeviceDispatch;
  }
  const auto dispatch = std::find_if(
      state.device_dispatches.begin(), state.device_dispatches.end(),
      [binding](const AdapterState::DeviceDispatch& candidate) {
        return candidate.device == binding->device;
      });
  if (dispatch != state.device_dispatches.end()) {
    t_command_buffer_cache.generation = gen;
    t_command_buffer_cache.command_buffer = command_buffer;
    t_command_buffer_cache.dispatch = *dispatch;
    return t_command_buffer_cache.dispatch;
  }
  return kEmptyDeviceDispatch;
}

void RegisterHostDeviceDispatch(VkDevice device,
                                VkPhysicalDevice physical_device,
                                PFN_vkGetDeviceProcAddr get_device_proc_addr) {
  if (device == VK_NULL_HANDLE || get_device_proc_addr == nullptr) {
    return;
  }
  AdapterState::DeviceDispatch dispatch;
  dispatch.device = device;
  dispatch.physical_device = physical_device;
  dispatch.queue_present = reinterpret_cast<PFN_vkQueuePresentKHR>(
      get_device_proc_addr(device, "vkQueuePresentKHR"));
  dispatch.queue_submit = reinterpret_cast<PFN_vkQueueSubmit>(
      get_device_proc_addr(device, "vkQueueSubmit"));
  dispatch.queue_submit2 = reinterpret_cast<PFN_vkQueueSubmit2>(
      get_device_proc_addr(device, "vkQueueSubmit2"));
  dispatch.queue_submit2_khr = reinterpret_cast<PFN_vkQueueSubmit2KHR>(
      get_device_proc_addr(device, "vkQueueSubmit2KHR"));
  dispatch.queue_bind_sparse = reinterpret_cast<PFN_vkQueueBindSparse>(
      get_device_proc_addr(device, "vkQueueBindSparse"));
  dispatch.queue_wait_idle = reinterpret_cast<PFN_vkQueueWaitIdle>(
      get_device_proc_addr(device, "vkQueueWaitIdle"));
  dispatch.acquire_next_image = reinterpret_cast<PFN_vkAcquireNextImageKHR>(
      get_device_proc_addr(device, "vkAcquireNextImageKHR"));
  dispatch.acquire_next_image2 = reinterpret_cast<PFN_vkAcquireNextImage2KHR>(
      get_device_proc_addr(device, "vkAcquireNextImage2KHR"));
  dispatch.wait_for_fences = reinterpret_cast<PFN_vkWaitForFences>(
      get_device_proc_addr(device, "vkWaitForFences"));
  dispatch.wait_semaphores = reinterpret_cast<PFN_vkWaitSemaphores>(
      get_device_proc_addr(device, "vkWaitSemaphores"));
  dispatch.wait_semaphores_khr = reinterpret_cast<PFN_vkWaitSemaphoresKHR>(
      get_device_proc_addr(device, "vkWaitSemaphoresKHR"));
  dispatch.device_wait_idle = reinterpret_cast<PFN_vkDeviceWaitIdle>(
      get_device_proc_addr(device, "vkDeviceWaitIdle"));
  dispatch.reset_fences = reinterpret_cast<PFN_vkResetFences>(
      get_device_proc_addr(device, "vkResetFences"));
  dispatch.reset_command_pool = reinterpret_cast<PFN_vkResetCommandPool>(
      get_device_proc_addr(device, "vkResetCommandPool"));
  dispatch.get_query_pool_results =
      reinterpret_cast<PFN_vkGetQueryPoolResults>(
          get_device_proc_addr(device, "vkGetQueryPoolResults"));
  dispatch.allocate_command_buffers =
      reinterpret_cast<PFN_vkAllocateCommandBuffers>(
          get_device_proc_addr(device, "vkAllocateCommandBuffers"));
  dispatch.free_command_buffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(
      get_device_proc_addr(device, "vkFreeCommandBuffers"));
  dispatch.destroy_command_pool = reinterpret_cast<PFN_vkDestroyCommandPool>(
      get_device_proc_addr(device, "vkDestroyCommandPool"));
  dispatch.begin_command_buffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(
      get_device_proc_addr(device, "vkBeginCommandBuffer"));
  dispatch.end_command_buffer = reinterpret_cast<PFN_vkEndCommandBuffer>(
      get_device_proc_addr(device, "vkEndCommandBuffer"));
  dispatch.reset_command_buffer = reinterpret_cast<PFN_vkResetCommandBuffer>(
      get_device_proc_addr(device, "vkResetCommandBuffer"));
  dispatch.cmd_set_viewport = reinterpret_cast<PFN_vkCmdSetViewport>(
      get_device_proc_addr(device, "vkCmdSetViewport"));
  dispatch.cmd_set_scissor = reinterpret_cast<PFN_vkCmdSetScissor>(
      get_device_proc_addr(device, "vkCmdSetScissor"));
  dispatch.cmd_set_viewport_with_count =
      reinterpret_cast<PFN_vkCmdSetViewportWithCount>(
          get_device_proc_addr(device, "vkCmdSetViewportWithCount") !=
                  nullptr
              ? get_device_proc_addr(device, "vkCmdSetViewportWithCount")
              : get_device_proc_addr(device,
                                     "vkCmdSetViewportWithCountEXT"));
  dispatch.cmd_set_scissor_with_count =
      reinterpret_cast<PFN_vkCmdSetScissorWithCount>(
          get_device_proc_addr(device, "vkCmdSetScissorWithCount") !=
                  nullptr
              ? get_device_proc_addr(device, "vkCmdSetScissorWithCount")
              : get_device_proc_addr(device,
                                     "vkCmdSetScissorWithCountEXT"));
  dispatch.begin_rendering = reinterpret_cast<PFN_vkCmdBeginRendering>(
      get_device_proc_addr(device, "vkCmdBeginRendering") != nullptr
          ? get_device_proc_addr(device, "vkCmdBeginRendering")
          : get_device_proc_addr(device, "vkCmdBeginRenderingKHR"));
  dispatch.cmd_end_rendering = reinterpret_cast<PFN_vkCmdEndRendering>(
      get_device_proc_addr(device, "vkCmdEndRendering") != nullptr
          ? get_device_proc_addr(device, "vkCmdEndRendering")
          : get_device_proc_addr(device, "vkCmdEndRenderingKHR"));
  dispatch.destroy_image_view = reinterpret_cast<PFN_vkDestroyImageView>(
      get_device_proc_addr(device, "vkDestroyImageView"));

  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  const auto existing = std::find_if(
      state.device_dispatches.begin(), state.device_dispatches.end(),
      [device](const AdapterState::DeviceDispatch& candidate) {
        return candidate.device == device;
      });
  if (existing != state.device_dispatches.end()) {
    *existing = dispatch;
  } else {
    state.device_dispatches.push_back(dispatch);
  }
  InvalidateFastDispatchCaches();
}

void RegisterHostQueueBinding(VkDevice device, VkQueue queue) {
  if (device == VK_NULL_HANDLE || queue == VK_NULL_HANDLE) {
    return;
  }
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  const auto existing =
      std::find_if(state.queue_bindings.begin(), state.queue_bindings.end(),
                   [queue](const AdapterState::QueueBinding& candidate) {
                     return candidate.queue == queue;
                   });
  if (existing != state.queue_bindings.end()) {
    existing->device = device;
  } else {
    state.queue_bindings.push_back({queue, device});
  }
  InvalidateFastDispatchCaches();
}

void RegisterHostCommandBuffers(VkDevice device, VkCommandPool command_pool,
                                std::uint32_t command_buffer_count,
                                const VkCommandBuffer* command_buffers) {
  if (device == VK_NULL_HANDLE || command_buffers == nullptr) {
    return;
  }
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  for (std::uint32_t index = 0; index < command_buffer_count; ++index) {
    const VkCommandBuffer command_buffer = command_buffers[index];
    if (command_buffer == VK_NULL_HANDLE) {
      continue;
    }
    const auto existing = std::find_if(
        state.command_buffer_bindings.begin(),
        state.command_buffer_bindings.end(),
        [command_buffer](const AdapterState::CommandBufferBinding& candidate) {
          return candidate.command_buffer == command_buffer;
        });
    if (existing != state.command_buffer_bindings.end()) {
      existing->device = device;
      existing->command_pool = command_pool;
    } else {
      state.command_buffer_bindings.push_back(
          {command_buffer, device, command_pool});
    }
  }
  InvalidateFastDispatchCaches();
}

void RemoveHostCommandBuffers(VkDevice device, VkCommandPool command_pool,
                              std::uint32_t command_buffer_count,
                              const VkCommandBuffer* command_buffers) {
  if (command_buffers == nullptr || command_buffer_count == 0) {
    return;
  }
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  state.command_buffer_bindings.erase(
      std::remove_if(
          state.command_buffer_bindings.begin(),
          state.command_buffer_bindings.end(),
          [device, command_pool, command_buffer_count, command_buffers](
              const AdapterState::CommandBufferBinding& binding) {
            return binding.device == device &&
                   binding.command_pool == command_pool &&
                   std::find(command_buffers,
                             command_buffers + command_buffer_count,
                             binding.command_buffer) !=
                       command_buffers + command_buffer_count;
          }),
      state.command_buffer_bindings.end());
  InvalidateFastDispatchCaches();
}

void RemoveHostCommandPoolBindings(VkDevice device,
                                   VkCommandPool command_pool) {
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  state.command_buffer_bindings.erase(
      std::remove_if(
          state.command_buffer_bindings.begin(),
          state.command_buffer_bindings.end(),
          [device, command_pool](
              const AdapterState::CommandBufferBinding& binding) {
            return binding.device == device &&
                   binding.command_pool == command_pool;
          }),
      state.command_buffer_bindings.end());
  InvalidateFastDispatchCaches();
}

void RemoveHostDeviceDispatch(VkDevice device) {
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  state.queue_bindings.erase(
      std::remove_if(state.queue_bindings.begin(), state.queue_bindings.end(),
                     [device](const AdapterState::QueueBinding& binding) {
                       return binding.device == device;
                     }),
      state.queue_bindings.end());
  state.command_buffer_bindings.erase(
      std::remove_if(
          state.command_buffer_bindings.begin(),
          state.command_buffer_bindings.end(),
          [device](const AdapterState::CommandBufferBinding& binding) {
            return binding.device == device;
          }),
      state.command_buffer_bindings.end());
  state.device_dispatches.erase(
      std::remove_if(
          state.device_dispatches.begin(), state.device_dispatches.end(),
          [device](const AdapterState::DeviceDispatch& dispatch) {
            return dispatch.device == device;
          }),
      state.device_dispatches.end());
  InvalidateFastDispatchCaches();
}

bool IsHostWsiExtension(const char* name) {
  if (name == nullptr) {
    return false;
  }
  const auto& extensions = State().host_wsi.required_instance_extensions();
  return std::find(extensions.begin(), extensions.end(), name) !=
         extensions.end();
}

std::vector<VkExtensionProperties> AndroidVisibleExtensions(
    const char* layer_name, VkResult* result) {
  std::vector<VkExtensionProperties> output;
  const auto host_enumerate =
      reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
          HostInstanceProc(VK_NULL_HANDLE,
                           "vkEnumerateInstanceExtensionProperties"));
  if (host_enumerate == nullptr) {
    *result = VK_ERROR_INITIALIZATION_FAILED;
    return output;
  }
  std::uint32_t count = 0;
  *result = host_enumerate(layer_name, &count, nullptr);
  if (*result != VK_SUCCESS || count == 0) {
    return output;
  }
  output.resize(count);
  *result = host_enumerate(layer_name, &count, output.data());
  if (*result != VK_SUCCESS && *result != VK_INCOMPLETE) {
    output.clear();
    return output;
  }
  output.resize(count);
  if (layer_name != nullptr) {
    return output;
  }

  output.erase(
      std::remove_if(output.begin(), output.end(),
                     [](const auto& property) {
                       return IsHostWsiExtension(property.extensionName) &&
                              std::strcmp(property.extensionName,
                                          VK_KHR_SURFACE_EXTENSION_NAME) != 0;
                     }),
      output.end());
  const bool has_android =
      std::any_of(output.begin(), output.end(), [](const auto& property) {
        return std::strcmp(property.extensionName,
                           mocktail::graphics::kAndroidSurfaceExtension) == 0;
      });
  if (!has_android) {
    VkExtensionProperties android{};
    std::strncpy(android.extensionName,
                 mocktail::graphics::kAndroidSurfaceExtension,
                 sizeof(android.extensionName) - 1);
    android.specVersion = 6;
    output.push_back(android);
  }
  *result = VK_SUCCESS;
  return output;
}

PFN_vkVoidFunction AdapterProc(const char* name) {
  if (name == nullptr) {
    return nullptr;
  }
#define MOCKTAIL_VK_PROC(function)                         \
  if (std::strcmp(name, #function) == 0) {                 \
    return reinterpret_cast<PFN_vkVoidFunction>(function); \
  }
  MOCKTAIL_VK_PROC(vkCreateInstance)
  MOCKTAIL_VK_PROC(vkDestroyInstance)
  MOCKTAIL_VK_PROC(vkCreateAndroidSurfaceKHR)
  MOCKTAIL_VK_PROC(vkDestroySurfaceKHR)
  MOCKTAIL_VK_PROC(vkEnumerateInstanceExtensionProperties)
  MOCKTAIL_VK_PROC(vkEnumerateInstanceLayerProperties)
  MOCKTAIL_VK_PROC(vkGetInstanceProcAddr)
  MOCKTAIL_VK_PROC(vkGetDeviceProcAddr)
  MOCKTAIL_VK_PROC(vkCreateDevice)
  MOCKTAIL_VK_PROC(vkDestroyDevice)
  MOCKTAIL_VK_PROC(vkGetDeviceQueue)
  MOCKTAIL_VK_PROC(vkGetDeviceQueue2)
  MOCKTAIL_VK_PROC(vkCreateSwapchainKHR)
  MOCKTAIL_VK_PROC(vkDestroySwapchainKHR)
  MOCKTAIL_VK_PROC(vkAcquireNextImageKHR)
  MOCKTAIL_VK_PROC(vkAcquireNextImage2KHR)
  MOCKTAIL_VK_PROC(vkWaitForFences)
  MOCKTAIL_VK_PROC(vkResetFences)
  MOCKTAIL_VK_PROC(vkResetCommandPool)
  MOCKTAIL_VK_PROC(vkGetQueryPoolResults)
  MOCKTAIL_VK_PROC(vkAllocateCommandBuffers)
  MOCKTAIL_VK_PROC(vkFreeCommandBuffers)
  MOCKTAIL_VK_PROC(vkDestroyCommandPool)
  MOCKTAIL_VK_PROC(vkBeginCommandBuffer)
  MOCKTAIL_VK_PROC(vkEndCommandBuffer)
  MOCKTAIL_VK_PROC(vkResetCommandBuffer)
  MOCKTAIL_VK_PROC(vkWaitSemaphores)
  MOCKTAIL_VK_PROC(vkWaitSemaphoresKHR)
  MOCKTAIL_VK_PROC(vkQueueSubmit)
  MOCKTAIL_VK_PROC(vkQueueSubmit2)
  MOCKTAIL_VK_PROC(vkQueueSubmit2KHR)
  MOCKTAIL_VK_PROC(vkQueueBindSparse)
  MOCKTAIL_VK_PROC(vkQueueWaitIdle)
  MOCKTAIL_VK_PROC(vkDeviceWaitIdle)
  MOCKTAIL_VK_PROC(vkQueuePresentKHR)
  MOCKTAIL_VK_PROC(vkCmdSetViewport)
  MOCKTAIL_VK_PROC(vkCmdSetScissor)
  MOCKTAIL_VK_PROC(vkCmdSetViewportWithCount)
  MOCKTAIL_VK_PROC(vkCmdSetScissorWithCount)
  MOCKTAIL_VK_PROC(vkCmdSetViewportWithCountEXT)
  MOCKTAIL_VK_PROC(vkCmdSetScissorWithCountEXT)
  MOCKTAIL_VK_PROC(vkCmdBeginRendering)
  MOCKTAIL_VK_PROC(vkCmdBeginRenderingKHR)
  MOCKTAIL_VK_PROC(vkCmdEndRendering)
  MOCKTAIL_VK_PROC(vkCmdEndRenderingKHR)
  MOCKTAIL_VK_PROC(vkDestroyImageView)
  MOCKTAIL_VK_PROC(vkGetPhysicalDeviceFeatures)
  MOCKTAIL_VK_PROC(vkGetPhysicalDeviceFeatures2)
  MOCKTAIL_VK_PROC(vkGetPhysicalDeviceFormatProperties)
  MOCKTAIL_VK_PROC(vkGetPhysicalDeviceFormatProperties2)
  MOCKTAIL_VK_PROC(vkGetPhysicalDeviceImageFormatProperties)
  MOCKTAIL_VK_PROC(vkGetPhysicalDeviceImageFormatProperties2)
  MOCKTAIL_VK_PROC(vkCreateImage)
  MOCKTAIL_VK_PROC(vkDestroyImage)
  MOCKTAIL_VK_PROC(vkCreateImageView)
  MOCKTAIL_VK_PROC(vkBindBufferMemory)
  MOCKTAIL_VK_PROC(vkBindBufferMemory2)
  MOCKTAIL_VK_PROC(vkMapMemory)
  MOCKTAIL_VK_PROC(vkMapMemory2)
  MOCKTAIL_VK_PROC(vkUnmapMemory)
  MOCKTAIL_VK_PROC(vkUnmapMemory2)
  MOCKTAIL_VK_PROC(vkFreeMemory)
  MOCKTAIL_VK_PROC(vkDestroyBuffer)
  MOCKTAIL_VK_PROC(vkCmdCopyBufferToImage)
  MOCKTAIL_VK_PROC(vkCmdCopyImage)
  MOCKTAIL_VK_PROC(vkCmdCopyImage2)
  MOCKTAIL_VK_PROC(vkCmdCopyBufferToImage2)
  MOCKTAIL_VK_PROC(vkCmdExecuteCommands)
#undef MOCKTAIL_VK_PROC
#define MOCKTAIL_VK_ALIAS(alias, function)                 \
  if (std::strcmp(name, #alias) == 0) {                    \
    return reinterpret_cast<PFN_vkVoidFunction>(function); \
  }
  MOCKTAIL_VK_ALIAS(vkGetPhysicalDeviceFeatures2KHR,
                    vkGetPhysicalDeviceFeatures2)
  MOCKTAIL_VK_ALIAS(vkGetPhysicalDeviceFormatProperties2KHR,
                    vkGetPhysicalDeviceFormatProperties2)
  MOCKTAIL_VK_ALIAS(vkGetPhysicalDeviceImageFormatProperties2KHR,
                    vkGetPhysicalDeviceImageFormatProperties2)
  MOCKTAIL_VK_ALIAS(vkBindBufferMemory2KHR, vkBindBufferMemory2)
  MOCKTAIL_VK_ALIAS(vkCmdCopyImage2KHR, vkCmdCopyImage2)
  MOCKTAIL_VK_ALIAS(vkMapMemory2KHR, vkMapMemory2)
  MOCKTAIL_VK_ALIAS(vkUnmapMemory2KHR, vkUnmapMemory2)
  MOCKTAIL_VK_ALIAS(vkCmdCopyBufferToImage2KHR, vkCmdCopyBufferToImage2)
#undef MOCKTAIL_VK_ALIAS
  return nullptr;
}

bool NameInList(const char* name, const char* const* names,
                std::size_t count) {
  return name != nullptr &&
         std::any_of(names, names + count, [name](const char* candidate) {
           return std::strcmp(name, candidate) == 0;
         });
}

bool IsEtc2DeviceProc(const char* name) {
  static constexpr const char* kNames[] = {
      "vkCreateImage",          "vkDestroyImage",
      "vkCreateImageView",      "vkBindBufferMemory",
      "vkBindBufferMemory2",    "vkBindBufferMemory2KHR",
      "vkMapMemory",            "vkMapMemory2",
      "vkMapMemory2KHR",        "vkUnmapMemory",
      "vkUnmapMemory2",         "vkUnmapMemory2KHR",
      "vkFreeMemory",           "vkDestroyBuffer",
      "vkCmdCopyBufferToImage", "vkCmdCopyBufferToImage2",
      "vkCmdCopyImage",         "vkCmdCopyImage2",
      "vkCmdCopyImage2KHR",
      "vkCmdCopyBufferToImage2KHR", "vkCmdExecuteCommands",
  };
  return NameInList(name, kNames, sizeof(kNames) / sizeof(kNames[0]));
}

bool IsEtc2PhysicalDeviceProc(const char* name) {
  static constexpr const char* kNames[] = {
      "vkGetPhysicalDeviceFeatures",
      "vkGetPhysicalDeviceFeatures2",
      "vkGetPhysicalDeviceFeatures2KHR",
      "vkGetPhysicalDeviceFormatProperties",
      "vkGetPhysicalDeviceFormatProperties2",
      "vkGetPhysicalDeviceFormatProperties2KHR",
      "vkGetPhysicalDeviceImageFormatProperties",
      "vkGetPhysicalDeviceImageFormatProperties2",
      "vkGetPhysicalDeviceImageFormatProperties2KHR",
  };
  return NameInList(name, kNames, sizeof(kNames) / sizeof(kNames[0]));
}

bool IsDeviceAdapterProc(const char* name) {
  return name != nullptr && (IsEtc2DeviceProc(name) || std::strcmp(name, "vkDestroyDevice") == 0 ||
                             std::strcmp(name, "vkGetDeviceQueue") == 0 ||
                             std::strcmp(name, "vkGetDeviceQueue2") == 0 ||
                             std::strcmp(name, "vkCreateSwapchainKHR") == 0 ||
                             std::strcmp(name, "vkDestroySwapchainKHR") == 0 ||
                             std::strcmp(name, "vkAcquireNextImageKHR") == 0 ||
                             std::strcmp(name, "vkAcquireNextImage2KHR") == 0 ||
                             std::strcmp(name, "vkWaitForFences") == 0 ||
                             std::strcmp(name, "vkResetFences") == 0 ||
                             std::strcmp(name, "vkResetCommandPool") == 0 ||
                             std::strcmp(name, "vkGetQueryPoolResults") == 0 ||
                             std::strcmp(name, "vkAllocateCommandBuffers") ==
                                 0 ||
                             std::strcmp(name, "vkFreeCommandBuffers") == 0 ||
                             std::strcmp(name, "vkDestroyCommandPool") == 0 ||
                             std::strcmp(name, "vkBeginCommandBuffer") == 0 ||
                             std::strcmp(name, "vkEndCommandBuffer") == 0 ||
                             std::strcmp(name, "vkResetCommandBuffer") == 0 ||
                             std::strcmp(name, "vkWaitSemaphores") == 0 ||
                             std::strcmp(name, "vkWaitSemaphoresKHR") == 0 ||
                             std::strcmp(name, "vkQueueSubmit") == 0 ||
                             std::strcmp(name, "vkQueueSubmit2") == 0 ||
                             std::strcmp(name, "vkQueueSubmit2KHR") == 0 ||
                             std::strcmp(name, "vkQueueBindSparse") == 0 ||
                             std::strcmp(name, "vkQueueWaitIdle") == 0 ||
                             std::strcmp(name, "vkDeviceWaitIdle") == 0 ||
                             std::strcmp(name, "vkQueuePresentKHR") == 0 ||
                             std::strcmp(name, "vkCmdSetViewport") == 0 ||
                             std::strcmp(name, "vkCmdSetScissor") == 0 ||
                             std::strcmp(name, "vkCmdSetViewportWithCount") ==
                                 0 ||
                             std::strcmp(name, "vkCmdSetScissorWithCount") ==
                                 0 ||
                             std::strcmp(name, "vkCmdSetViewportWithCountEXT") ==
                                 0 ||
                             std::strcmp(name, "vkCmdSetScissorWithCountEXT") ==
                                 0 ||
                             std::strcmp(name, "vkCmdBeginRendering") == 0 ||
                             std::strcmp(name, "vkCmdBeginRenderingKHR") == 0 ||
                             std::strcmp(name, "vkCmdEndRendering") == 0 ||
                             std::strcmp(name, "vkCmdEndRenderingKHR") == 0 ||
                             std::strcmp(name, "vkDestroyImageView") == 0);
}

bool IsGlobalAdapterProc(const char* name) {
  return name != nullptr &&
         (std::strcmp(name, "vkCreateInstance") == 0 ||
          std::strcmp(name, "vkEnumerateInstanceExtensionProperties") == 0 ||
          std::strcmp(name, "vkEnumerateInstanceLayerProperties") == 0 ||
          std::strcmp(name, "vkGetInstanceProcAddr") == 0);
}

const VkBaseInStructure* FindFeature(const void* chain, VkStructureType type) {
  const auto* current = static_cast<const VkBaseInStructure*>(chain);
  while (current != nullptr) {
    if (current->sType == type) {
      return current;
    }
    current = current->pNext;
  }
  return nullptr;
}

struct EnabledDeviceFeatures {
  VkPhysicalDeviceFeatures2 root{};
  VkPhysicalDeviceVulkan11Features vulkan11{};
  VkPhysicalDeviceVulkan12Features vulkan12{};
  bool placebo_required = false;

  EnabledDeviceFeatures() {
    root.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    vulkan11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    root.pNext = &vulkan11;
    vulkan11.pNext = &vulkan12;
  }
};

EnabledDeviceFeatures InspectEnabledFeatures(
    const VkDeviceCreateInfo& create_info) {
  EnabledDeviceFeatures enabled;
  if (create_info.pEnabledFeatures != nullptr) {
    enabled.root.features = *create_info.pEnabledFeatures;
  }
  if (const auto* features2 =
          reinterpret_cast<const VkPhysicalDeviceFeatures2*>(FindFeature(
              create_info.pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2));
      features2 != nullptr) {
    enabled.root.features = features2->features;
  }

  bool timeline = false;
  bool host_query_reset = false;
  if (const auto* vulkan12 =
          reinterpret_cast<const VkPhysicalDeviceVulkan12Features*>(FindFeature(
              create_info.pNext,
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES));
      vulkan12 != nullptr) {
    timeline = vulkan12->timelineSemaphore == VK_TRUE;
    host_query_reset = vulkan12->hostQueryReset == VK_TRUE;
  }
  if (const auto* timeline_features = reinterpret_cast<
          const VkPhysicalDeviceTimelineSemaphoreFeatures*>(FindFeature(
          create_info.pNext,
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES));
      timeline_features != nullptr) {
    timeline = timeline_features->timelineSemaphore == VK_TRUE;
  }
  if (const auto* host_query_features =
          reinterpret_cast<const VkPhysicalDeviceHostQueryResetFeatures*>(
              FindFeature(
                  create_info.pNext,
                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES));
      host_query_features != nullptr) {
    host_query_reset = host_query_features->hostQueryReset == VK_TRUE;
  }
  enabled.vulkan12.timelineSemaphore = timeline;
  enabled.vulkan12.hostQueryReset = host_query_reset;
  enabled.placebo_required = timeline && host_query_reset;
  return enabled;
}

bool HasRequiredFeatureStructs(const VkDeviceCreateInfo& create_info) {
  return FindFeature(create_info.pNext,
                     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES) !=
             nullptr ||
         FindFeature(
             create_info.pNext,
             VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES) !=
             nullptr ||
         FindFeature(
             create_info.pNext,
             VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES) !=
             nullptr;
}

bool ShouldLogNegativeVulkanResult(VkResult result, std::uint64_t* occurrence) {
  static std::atomic<std::uint64_t> device_lost_count{0};
  static std::atomic<std::uint64_t> other_error_count{0};
  std::atomic<std::uint64_t>& counter =
      result == VK_ERROR_DEVICE_LOST ? device_lost_count : other_error_count;
  const std::uint64_t count =
      counter.fetch_add(1, std::memory_order_relaxed) + 1;
  if (occurrence != nullptr) {
    *occurrence = count;
  }
  return count <= 4 || (count & (count - 1)) == 0;
}

class VulkanCallObservation final {
 public:
  explicit VulkanCallObservation(const char* call_name)
      : call_name_(call_name) {
    if (__builtin_expect(
            g_vulkan_call_observation_active.load(std::memory_order_relaxed),
            0)) {
      AdapterState& state = State();
      begin_ = state.note_vulkan_call_begin.load(std::memory_order_acquire);
      end_ = state.note_vulkan_call_end.load(std::memory_order_acquire);
      if (begin_ != nullptr && end_ != nullptr) {
        sequence_ = begin_(call_name);
      }
    }
  }

  ~VulkanCallObservation() {
    if (__builtin_expect(sequence_ != 0 && end_ != nullptr, 0)) {
      end_(sequence_, static_cast<std::int32_t>(result_));
    }
  }

  VulkanCallObservation(const VulkanCallObservation&) = delete;
  VulkanCallObservation& operator=(const VulkanCallObservation&) = delete;

  void SetResult(VkResult result) {
    result_ = result;
    if (__builtin_expect(
            result < VK_SUCCESS && result != VK_ERROR_OUT_OF_DATE_KHR, 0)) {
      std::uint64_t occurrence = 0;
      if (ShouldLogNegativeVulkanResult(result, &occurrence)) {
        std::fprintf(stderr,
                     "  [vulkan] unexpected negative result: call=%s "
                     "result=%d device_lost=%u occurrence=%llu\n",
                     call_name_ != nullptr ? call_name_ : "unknown",
                     static_cast<int>(result),
                     result == VK_ERROR_DEVICE_LOST ? 1U : 0U,
                     static_cast<unsigned long long>(occurrence));
      }
    }
  }

 private:
  NoteVulkanCallBeginFn begin_ = nullptr;
  NoteVulkanCallEndFn end_ = nullptr;
  const char* call_name_ = nullptr;
  std::uint64_t sequence_ = 0;
  VkResult result_ = VK_ERROR_UNKNOWN;
};

VkResult WaitForSemaphoresObserved(const char* call_name,
                                   PFN_vkWaitSemaphores host_wait,
                                   VkDevice device,
                                   const VkSemaphoreWaitInfo* wait_info,
                                   std::uint64_t timeout,
                                   VulkanCallObservation* observation) {
  if (host_wait == nullptr) {
    if (observation != nullptr) {
      observation->SetResult(VK_ERROR_INITIALIZATION_FAILED);
    }
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  static_cast<void>(call_name);
  const VkResult result = host_wait(device, wait_info, timeout);
  if (observation != nullptr) {
    observation->SetResult(result);
  }
  return result;
}

bool FpsTraceEnabled() {
  static const bool enabled = [] {
    const char* value = std::getenv("MOCKTAIL_TRACE_FPS");
    return value != nullptr && value[0] != '\0' && std::strcmp(value, "0") != 0;
  }();
  return enabled;
}

std::uint64_t MonotonicNanos() {
  timespec ts{};
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    return 0;
  }
  return static_cast<std::uint64_t>(ts.tv_sec) * 1000000000ULL +
         static_cast<std::uint64_t>(ts.tv_nsec);
}

struct FpsWaitTrace {
  const char* name = nullptr;
  std::atomic<std::uint64_t> samples{0};
  std::atomic<std::uint64_t> total_ns{0};
  std::atomic<std::uint64_t> max_ns{0};
  std::atomic<std::uint64_t> window_start_ns{0};
  std::atomic<std::uint64_t> max_gap_ns{0};
  std::atomic<std::uint64_t> last_start_ns{0};

  void Record(std::uint64_t start_ns) {
    if (name == nullptr || start_ns == 0) {
      return;
    }
    const std::uint64_t previous_start =
        last_start_ns.exchange(start_ns, std::memory_order_relaxed);
    if (previous_start != 0 && start_ns > previous_start) {
      const std::uint64_t gap_ns = start_ns - previous_start;
      std::uint64_t max_gap = max_gap_ns.load(std::memory_order_relaxed);
      while (gap_ns > max_gap &&
             !max_gap_ns.compare_exchange_weak(max_gap, gap_ns,
                                               std::memory_order_relaxed)) {
      }
    }
    const std::uint64_t wait_ns = MonotonicNanos() - start_ns;
    total_ns.fetch_add(wait_ns, std::memory_order_relaxed);
    std::uint64_t max_wait = max_ns.load(std::memory_order_relaxed);
    while (wait_ns > max_wait &&
           !max_ns.compare_exchange_weak(max_wait, wait_ns,
                                         std::memory_order_relaxed)) {
    }
    const std::uint64_t n = samples.fetch_add(1, std::memory_order_relaxed) + 1;
    std::uint64_t window = window_start_ns.load(std::memory_order_relaxed);
    if (window == 0) {
      window_start_ns.compare_exchange_strong(window, start_ns,
                                              std::memory_order_relaxed);
      window = window_start_ns.load(std::memory_order_relaxed);
    }
    if (start_ns - window < 1000000000ULL || n == 0) {
      return;
    }
    const std::uint64_t total = total_ns.exchange(0, std::memory_order_relaxed);
    const std::uint64_t peak = max_ns.exchange(0, std::memory_order_relaxed);
    const std::uint64_t count = samples.exchange(0, std::memory_order_relaxed);
    const std::uint64_t gap_peak =
        max_gap_ns.exchange(0, std::memory_order_relaxed);
    window_start_ns.store(start_ns, std::memory_order_relaxed);
    if (count == 0) {
      return;
    }
    std::fprintf(stderr,
                 "  [fps] %s n=%llu avg=%llu us max=%llu us gap_max=%llu us\n",
                 name, static_cast<unsigned long long>(count),
                 static_cast<unsigned long long>(total / count / 1000ULL),
                 static_cast<unsigned long long>(peak / 1000ULL),
                 static_cast<unsigned long long>(gap_peak / 1000ULL));
  }
};

FpsWaitTrace& PresentWaitTrace() {
  static FpsWaitTrace trace{"vkQueuePresentKHR"};
  return trace;
}

FpsWaitTrace& AcquireWaitTrace() {
  static FpsWaitTrace trace{"vkAcquireNextImageKHR"};
  return trace;
}

FpsWaitTrace& FenceWaitTrace() {
  static FpsWaitTrace trace{"vkWaitForFences"};
  return trace;
}

FpsWaitTrace& QueueIdleWaitTrace() {
  static FpsWaitTrace trace{"vkQueueWaitIdle"};
  return trace;
}

VkResult VKAPI_CALL
ObservedHostQueuePresent(VkQueue queue, const VkPresentInfoKHR* present_info) {
  AdapterState& state = State();
  const PFN_vkQueuePresentKHR host_present =
      HostDispatchForQueue(queue).queue_present;
  if (host_present == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const NoteHostPresentBeginFn note_begin =
      state.note_host_present_begin.load(std::memory_order_acquire);
  if (note_begin != nullptr) {
    note_begin();
  }
  const bool fps_trace = FpsTraceEnabled();
  const std::uint64_t present_start_ns = fps_trace ? MonotonicNanos() : 0;
  const VkResult result = host_present(queue, present_info);
  if (fps_trace) {
    PresentWaitTrace().Record(present_start_ns);
  }
  const NoteHostPresentEndFn note_end =
      state.note_host_present_end.load(std::memory_order_acquire);
  if (note_end != nullptr) {
    note_end(static_cast<std::int32_t>(result));
  }
  return result;
}

template <typename Function>
Function HostPhysicalDeviceProc(const char* name, const char* alias) {
  VkInstance instance = VK_NULL_HANDLE;
  {
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    instance = state.latest_instance;
  }
  PFN_vkVoidFunction proc = HostInstanceProc(instance, name);
  if (proc == nullptr && alias != nullptr) {
    proc = HostInstanceProc(instance, alias);
  }
  return reinterpret_cast<Function>(proc);
}

bool Etc2EmulatedPhysicalDevice(VkPhysicalDevice physical_device) {
  return State().etc2.PhysicalDeviceNeedsEmulation(
      physical_device, HostPhysicalDeviceProc<PFN_vkGetPhysicalDeviceFeatures>(
                           "vkGetPhysicalDeviceFeatures", nullptr));
}

void ReleaseEtc2PoolCommandBuffers(VkDevice device,
                                   VkCommandPool command_pool) {
  std::vector<VkCommandBuffer> command_buffers;
  {
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    for (const auto& binding : state.command_buffer_bindings) {
      if (binding.device == device && binding.command_pool == command_pool) {
        command_buffers.push_back(binding.command_buffer);
      }
    }
  }
  for (VkCommandBuffer command_buffer : command_buffers) {
    State().etc2.ReleaseCommandBuffer(command_buffer);
  }
}

void PrepareEtc2Submit2(std::uint32_t submit_count,
                        const VkSubmitInfo2* submits) {
  if (submits == nullptr || !State().etc2.HasPendingUploads()) {
    return;
  }
  std::vector<VkCommandBuffer> command_buffers;
  for (std::uint32_t index = 0; index < submit_count; ++index) {
    for (std::uint32_t info = 0; info < submits[index].commandBufferInfoCount;
         ++info) {
      command_buffers.push_back(
          submits[index].pCommandBufferInfos[info].commandBuffer);
    }
  }
  State().etc2.PrepareSubmit(command_buffers.data(),
                             static_cast<std::uint32_t>(command_buffers.size()));
}

}  // namespace

extern "C" {

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateInstance(const VkInstanceCreateInfo* create_info,
                 const VkAllocationCallbacks* allocator, VkInstance* instance) {
  if (!EnsureInitialized() || create_info == nullptr || instance == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  std::vector<std::string> requested;
  requested.reserve(create_info->enabledExtensionCount);
  for (std::uint32_t index = 0; index < create_info->enabledExtensionCount;
       ++index) {
    const char* extension = create_info->ppEnabledExtensionNames[index];
    if (extension == nullptr) {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    requested.emplace_back(extension);
  }
  std::vector<std::string> translated;
  const mocktail::Status status =
      State().android_wsi.TranslateInstanceExtensions(requested, &translated);
  if (!status.ok()) {
    std::fprintf(stderr, "  [vulkan] instance extension rewrite failed: %s\n",
                 status.message().c_str());
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
  std::vector<const char*> translated_names;
  translated_names.reserve(translated.size());
  for (const std::string& extension : translated) {
    translated_names.push_back(extension.c_str());
  }
  VkInstanceCreateInfo host_info = *create_info;
  host_info.enabledExtensionCount =
      static_cast<std::uint32_t>(translated_names.size());
  host_info.ppEnabledExtensionNames = translated_names.data();
  VkApplicationInfo host_application_info{};
  if (create_info->pApplicationInfo != nullptr) {
    host_application_info = *create_info->pApplicationInfo;
  } else {
    host_application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  }
  const std::uint32_t requested_api = host_application_info.apiVersion;
  const auto host_enumerate_version =
      reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
          HostInstanceProc(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
  std::uint32_t host_api = VK_API_VERSION_1_0;
  if (host_enumerate_version != nullptr) {
    (void)host_enumerate_version(&host_api);
  }
  if (host_api >= VK_API_VERSION_1_2 && requested_api < VK_API_VERSION_1_2) {
    host_application_info.apiVersion = VK_API_VERSION_1_2;
    host_info.pApplicationInfo = &host_application_info;
  }
  const auto host_create = reinterpret_cast<PFN_vkCreateInstance>(
      HostInstanceProc(VK_NULL_HANDLE, "vkCreateInstance"));
  if (host_create == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const VkResult result = host_create(&host_info, allocator, instance);
  if (result == VK_SUCCESS) {
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    ++state.active_instance_count;
    state.latest_instance = *instance;
    state.host_get_device_proc_addr.store(
        reinterpret_cast<PFN_vkGetDeviceProcAddr>(
            state.host_get_instance_proc_addr(*instance, "vkGetDeviceProcAddr")),
        std::memory_order_release);
    std::fprintf(stderr, "  [vulkan] host VkInstance created\n");
    if (host_application_info.apiVersion != requested_api) {
      std::fprintf(stderr,
                   "  [vulkan] instance API raised to 1.2 for imported "
                   "libplacebo device interop\n");
    }
  }
  return result;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* allocator) {
  const auto host_destroy = reinterpret_cast<PFN_vkDestroyInstance>(
      HostInstanceProc(instance, "vkDestroyInstance"));
  if (host_destroy != nullptr) {
    host_destroy(instance, allocator);
  }
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  if (state.active_instance_count != 0) {
    --state.active_instance_count;
  }
  if (state.active_instance_count == 0 && state.initialized) {
    state.android_wsi.Shutdown();
    state.host_wsi.Shutdown();
    state.host_get_instance_proc_addr = nullptr;
    state.host_get_device_proc_addr.store(nullptr, std::memory_order_release);
    state.device_dispatches.clear();
    state.queue_bindings.clear();
    state.command_buffer_bindings.clear();
    InvalidateFastDispatchCaches();
    state.host_surface_capabilities = nullptr;
    state.host_surface_present_modes = nullptr;
    state.latest_instance = VK_NULL_HANDLE;
    state.note_present.store(nullptr, std::memory_order_release);
    state.note_host_present_begin.store(nullptr, std::memory_order_release);
    state.note_host_present_end.store(nullptr, std::memory_order_release);
    state.note_vulkan_call_begin.store(nullptr, std::memory_order_release);
    state.note_vulkan_call_end.store(nullptr, std::memory_order_release);
    g_vulkan_call_observation_active.store(false, std::memory_order_release);
    state.note_surface_out_of_date.store(nullptr, std::memory_order_release);
    state.extent_translation_logged = false;
    state.present_policy_logged = false;
    state.initialized = false;
    std::fprintf(stderr, "  [vulkan] SDL WSI adapter shut down\n");
  }
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAndroidSurfaceKHR(
    VkInstance instance, const void* create_info,
    const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface) {
  if (!EnsureInitialized()) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const auto result = State().android_wsi.CreateAndroidSurface(
      instance, create_info, allocator, surface);
  if (result == VK_SUCCESS) {
    std::fprintf(stderr, "  [vulkan] Android surface mapped to SDL WSI\n");
  }
  return static_cast<VkResult>(result);
}

VKAPI_ATTR void VKAPI_CALL
vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface,
                    const VkAllocationCallbacks* allocator) {
  if (EnsureInitialized()) {
    State().android_wsi.DestroySurface(instance, surface, allocator);
  }
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
    const char* layer_name, std::uint32_t* property_count,
    VkExtensionProperties* properties) {
  if (property_count == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  VkResult result = VK_SUCCESS;
  const std::vector<VkExtensionProperties> visible =
      AndroidVisibleExtensions(layer_name, &result);
  if (result != VK_SUCCESS) {
    return result;
  }
  if (properties == nullptr) {
    *property_count = static_cast<std::uint32_t>(visible.size());
    return VK_SUCCESS;
  }
  const std::uint32_t capacity = *property_count;
  const std::uint32_t copied =
      std::min(capacity, static_cast<std::uint32_t>(visible.size()));
  std::copy_n(visible.begin(), copied, properties);
  *property_count = copied;
  return copied < visible.size() ? VK_INCOMPLETE : VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
    std::uint32_t* property_count, VkLayerProperties* properties) {
  const auto host_enumerate =
      reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(HostInstanceProc(
          VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties"));
  return host_enumerate != nullptr ? host_enumerate(property_count, properties)
                                   : VK_ERROR_INITIALIZATION_FAILED;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddr(VkInstance instance, const char* name) {
  if (instance == VK_NULL_HANDLE) {
    const PFN_vkVoidFunction adapter = AdapterProc(name);
    return adapter != nullptr && IsGlobalAdapterProc(name)
               ? adapter
               : HostInstanceProc(VK_NULL_HANDLE, name);
  }
  if (name != nullptr &&
      std::strcmp(name, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR") == 0) {
    const PFN_vkVoidFunction host = HostInstanceProc(instance, name);
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.host_surface_capabilities =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(host);
    return host != nullptr ? reinterpret_cast<PFN_vkVoidFunction>(
                                 vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
                           : nullptr;
  }
  if (name != nullptr &&
      std::strcmp(name, "vkGetPhysicalDeviceSurfacePresentModesKHR") == 0) {
    const PFN_vkVoidFunction host = HostInstanceProc(instance, name);
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.host_surface_present_modes =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(host);
    return host != nullptr ? reinterpret_cast<PFN_vkVoidFunction>(
                                 vkGetPhysicalDeviceSurfacePresentModesKHR)
                           : nullptr;
  }
  if (const PFN_vkVoidFunction adapter = AdapterProc(name);
      adapter != nullptr) {
    if ((IsDeviceAdapterProc(name) || IsEtc2PhysicalDeviceProc(name)) &&
        HostInstanceProc(instance, name) == nullptr) {
      return nullptr;
    }
    return adapter;
  }
  return HostInstanceProc(instance, name);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device,
                                                             const char* name) {
  if (name == nullptr || !EnsureInitialized()) {
    return nullptr;
  }
  const PFN_vkGetDeviceProcAddr host_get_device_proc_addr =
      State().host_get_device_proc_addr.load(std::memory_order_acquire);
  const PFN_vkVoidFunction host = host_get_device_proc_addr != nullptr
                                      ? host_get_device_proc_addr(device, name)
                                      : nullptr;
  if (!IsDeviceAdapterProc(name)) {
    return host;
  }
  return host != nullptr ? AdapterProc(name) : nullptr;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
    VkPhysicalDevice physical_device, const VkDeviceCreateInfo* create_info,
    const VkAllocationCallbacks* allocator, VkDevice* device) {
  if (create_info == nullptr || device == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  VkInstance instance = VK_NULL_HANDLE;
  PFN_vkGetInstanceProcAddr host_get_instance_proc_addr = nullptr;
  PFN_vkGetDeviceProcAddr host_get_device_proc_addr = nullptr;
  {
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    instance = state.latest_instance;
    host_get_instance_proc_addr = state.host_get_instance_proc_addr;
    host_get_device_proc_addr =
        state.host_get_device_proc_addr.load(std::memory_order_acquire);
  }
  const auto host_create = reinterpret_cast<PFN_vkCreateDevice>(
      HostInstanceProc(instance, "vkCreateDevice"));
  if (host_create == nullptr || host_get_instance_proc_addr == nullptr ||
      host_get_device_proc_addr == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  VkPhysicalDeviceProperties properties{};
  const auto host_get_properties =
      reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
          HostInstanceProc(instance, "vkGetPhysicalDeviceProperties"));
  if (host_get_properties != nullptr) {
    host_get_properties(physical_device, &properties);
  }

  VkDeviceCreateInfo host_info = *create_info;
  EnabledDeviceFeatures enabled = InspectEnabledFeatures(*create_info);
  const bool etc2_emulated = State().etc2.PhysicalDeviceNeedsEmulation(
      physical_device, reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(
                           HostInstanceProc(instance,
                                            "vkGetPhysicalDeviceFeatures")));
  VkPhysicalDeviceFeatures host_enabled_features{};
  VkPhysicalDeviceFeatures2* requested_features2 = nullptr;
  VkBool32 requested_features2_etc2 = VK_FALSE;
  if (etc2_emulated) {
    enabled.root.features.textureCompressionETC2 = VK_FALSE;
    if (create_info->pEnabledFeatures != nullptr) {
      host_enabled_features = *create_info->pEnabledFeatures;
      host_enabled_features.textureCompressionETC2 = VK_FALSE;
      host_info.pEnabledFeatures = &host_enabled_features;
    }
    // Restore the caller-owned feature bit after the host call.
    requested_features2 = const_cast<VkPhysicalDeviceFeatures2*>(
        reinterpret_cast<const VkPhysicalDeviceFeatures2*>(FindFeature(
            create_info->pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2)));
    if (requested_features2 != nullptr) {
      requested_features2_etc2 =
          requested_features2->features.textureCompressionETC2;
      requested_features2->features.textureCompressionETC2 = VK_FALSE;
    }
  }
  enabled.root.pNext = &enabled.vulkan11;
  enabled.vulkan11.pNext = &enabled.vulkan12;
  enabled.vulkan12.pNext = nullptr;
  VkPhysicalDeviceVulkan12Features injected{};
  injected.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  bool injected_required_features = false;
  if (properties.apiVersion >= VK_API_VERSION_1_2 &&
      !enabled.placebo_required && !HasRequiredFeatureStructs(*create_info)) {
    VkPhysicalDeviceFeatures2 supported{};
    supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    VkPhysicalDeviceVulkan12Features supported_vulkan12{};
    supported_vulkan12.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    supported.pNext = &supported_vulkan12;
    const auto host_get_features2 =
        reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
            HostInstanceProc(instance, "vkGetPhysicalDeviceFeatures2"));
    if (host_get_features2 != nullptr) {
      host_get_features2(physical_device, &supported);
      if (supported_vulkan12.timelineSemaphore == VK_TRUE &&
          supported_vulkan12.hostQueryReset == VK_TRUE) {
        injected.timelineSemaphore = VK_TRUE;
        injected.hostQueryReset = VK_TRUE;
        injected.pNext = const_cast<void*>(host_info.pNext);
        host_info.pNext = &injected;
        enabled.vulkan12.timelineSemaphore = VK_TRUE;
        enabled.vulkan12.hostQueryReset = VK_TRUE;
        enabled.placebo_required = true;
        injected_required_features = true;
      }
    }
  }

  const VkResult result =
      host_create(physical_device, &host_info, allocator, device);
  if (requested_features2 != nullptr) {
    requested_features2->features.textureCompressionETC2 =
        requested_features2_etc2;
  }
  if (result != VK_SUCCESS) {
    return result;
  }
  RegisterHostDeviceDispatch(*device, physical_device,
                             host_get_device_proc_addr);
  VkPhysicalDeviceMemoryProperties memory_properties{};
  if (const auto host_memory_properties =
          reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(
              HostInstanceProc(instance,
                               "vkGetPhysicalDeviceMemoryProperties"));
      host_memory_properties != nullptr) {
    host_memory_properties(physical_device, &memory_properties);
  }
  State().etc2.RegisterDevice(*device, physical_device, etc2_emulated,
                              memory_properties, host_get_device_proc_addr);

  std::uint32_t queue_family_count = 0;
  const auto host_get_queue_families =
      reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
          HostInstanceProc(instance,
                           "vkGetPhysicalDeviceQueueFamilyProperties"));
  if (host_get_queue_families != nullptr) {
    host_get_queue_families(physical_device, &queue_family_count, nullptr);
  }
  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  if (host_get_queue_families != nullptr && queue_family_count != 0) {
    host_get_queue_families(physical_device, &queue_family_count,
                            queue_families.data());
    queue_families.resize(queue_family_count);
  }
  std::uint32_t graphics_family = UINT32_MAX;
  std::uint32_t graphics_count = 0;
  for (std::uint32_t index = 0; index < create_info->queueCreateInfoCount;
       ++index) {
    const VkDeviceQueueCreateInfo& queue_info =
        create_info->pQueueCreateInfos[index];
    if (queue_info.queueFamilyIndex >= queue_families.size() ||
        queue_info.queueCount == 0 ||
        (queue_families[queue_info.queueFamilyIndex].queueFlags &
         VK_QUEUE_GRAPHICS_BIT) == 0) {
      continue;
    }
    graphics_family = queue_info.queueFamilyIndex;
    graphics_count = 1;
    break;
  }

  const bool registered =
      enabled.placebo_required && graphics_family != UINT32_MAX &&
      State().text_overlay.RegisterDevice(
          instance, physical_device, *device, VK_API_VERSION_1_2,
          create_info->ppEnabledExtensionNames,
          create_info->enabledExtensionCount, graphics_family, graphics_count,
          &enabled.root, host_get_instance_proc_addr,
          host_get_device_proc_addr);
  if (!registered) {
    std::fprintf(stderr,
                 "  [vulkan] same-surface text compositor unavailable for "
                 "this VkDevice\n");
  } else if (injected_required_features) {
    std::fprintf(stderr,
                 "  [vulkan] enabled Vulkan 1.2 timeline/host-query features "
                 "for libplacebo interop\n");
  }
  return result;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* allocator) {
  const auto host_destroy = reinterpret_cast<PFN_vkDestroyDevice>(
      HostDeviceProc(device, "vkDestroyDevice"));
  State().text_overlay.DestroyDevice(device);
  State().etc2.DestroyDevice(device);
  if (host_destroy != nullptr) {
    host_destroy(device, allocator);
  }
  RemoveHostDeviceDispatch(device);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice device,
                                            std::uint32_t queue_family_index,
                                            std::uint32_t queue_index,
                                            VkQueue* queue) {
  const auto host_get_queue = reinterpret_cast<PFN_vkGetDeviceQueue>(
      HostDeviceProc(device, "vkGetDeviceQueue"));
  if (host_get_queue == nullptr || queue == nullptr) {
    return;
  }
  host_get_queue(device, queue_family_index, queue_index, queue);
  if (*queue != VK_NULL_HANDLE) {
    RegisterHostQueueBinding(device, *queue);
    (void)State().text_overlay.RegisterQueue(device, *queue, queue_family_index,
                                             queue_index);
  }
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue2(
    VkDevice device, const VkDeviceQueueInfo2* queue_info, VkQueue* queue) {
  const auto host_get_queue = reinterpret_cast<PFN_vkGetDeviceQueue2>(
      HostDeviceProc(device, "vkGetDeviceQueue2"));
  if (host_get_queue == nullptr || queue_info == nullptr || queue == nullptr) {
    return;
  }
  host_get_queue(device, queue_info, queue);
  if (*queue != VK_NULL_HANDLE) {
    RegisterHostQueueBinding(device, *queue);
    (void)State().text_overlay.RegisterQueue(
        device, *queue, queue_info->queueFamilyIndex, queue_info->queueIndex);
  }
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(
    VkDevice device, const VkSwapchainCreateInfoKHR* create_info,
    const VkAllocationCallbacks* allocator, VkSwapchainKHR* swapchain) {
  const auto host_create = reinterpret_cast<PFN_vkCreateSwapchainKHR>(
      HostDeviceProc(device, "vkCreateSwapchainKHR"));
  if (host_create == nullptr || create_info == nullptr ||
      swapchain == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  VkSwapchainCreateInfoKHR host_info = *create_info;
  const mocktail::graphics::PresentModePolicy present_policy =
      mocktail::graphics::CachedPresentModePolicy();
  const auto explicit_mode = mocktail::graphics::ExplicitPresentMode(present_policy);
  auto image_count_policy = present_policy;
  if (present_policy != mocktail::graphics::PresentModePolicy::kHostDefault) {
    bool mode_selected = false;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR host_caps = nullptr;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR host_modes_query = nullptr;
    {
      AdapterState& state = State();
      std::lock_guard<std::mutex> lock(state.mutex);
      host_caps = state.host_surface_capabilities;
      host_modes_query = state.host_surface_present_modes;
    }
    const VkPhysicalDevice physical_device =
        HostDispatchForDevice(device).physical_device;
    if (host_modes_query != nullptr && physical_device != VK_NULL_HANDLE) {
      std::uint32_t host_count = 0;
      if (host_modes_query(physical_device, create_info->surface, &host_count,
                           nullptr) == VK_SUCCESS &&
          host_count != 0) {
        std::vector<VkPresentModeKHR> host_modes(host_count);
        const VkResult modes_result = host_modes_query(
            physical_device, create_info->surface, &host_count,
            host_modes.data());
        if (modes_result == VK_SUCCESS || modes_result == VK_INCOMPLETE) {
          host_modes.resize(host_count);
          const std::vector<VkPresentModeKHR> selected =
              mocktail::graphics::FilterPresentModes(present_policy,
                                                     host_modes);
          if (!selected.empty()) {
            static std::atomic<bool> mode_logged{false};
            if (!mode_logged.exchange(true, std::memory_order_relaxed)) {
              std::fprintf(
                  stderr,
                  "  [vulkan] present_mode=%s requested=%s applied=%s%s\n",
                  mocktail::graphics::PresentModePolicyName(present_policy),
                  mocktail::graphics::PresentModeKhrName(
                      create_info->presentMode),
                  mocktail::graphics::PresentModeKhrName(selected.front()),
                  explicit_mode && selected.front() != *explicit_mode
                      ? " (requested mode unsupported; using FIFO)" : "");
            }
            host_info.presentMode = selected.front();
            mode_selected = true;
            if (explicit_mode && selected.front() != *explicit_mode) {
              image_count_policy = mocktail::graphics::PresentModePolicy::kFifo;
            }
          }
        }
      }
    }
    if (explicit_mode && !mode_selected) {
      std::fprintf(stderr, "  [vulkan] cannot resolve present_mode=%s for surface\n",
                   mocktail::graphics::PresentModePolicyName(present_policy));
      return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkSurfaceCapabilitiesKHR capabilities{};
    if (host_caps != nullptr && physical_device != VK_NULL_HANDLE &&
        host_caps(physical_device, create_info->surface, &capabilities) ==
            VK_SUCCESS) {
      const std::uint32_t preferred =
          mocktail::graphics::PreferSwapchainMinImageCount(
              image_count_policy, create_info->minImageCount,
              capabilities.minImageCount, capabilities.maxImageCount);
      if (preferred != host_info.minImageCount) {
        host_info.minImageCount = preferred;
        static std::atomic<bool> logged{false};
        if (!logged.exchange(true, std::memory_order_relaxed)) {
          std::fprintf(stderr,
                       "  [vulkan] present policy=%s swapchain "
                       "minImageCount=%u (requested=%u max=%u)\n",
                       mocktail::graphics::PresentModePolicyName(
                           present_policy),
                       preferred, create_info->minImageCount,
                       capabilities.maxImageCount);
        }
      }
    }
  }
  {
    // Host-side framebuffer scale: the surface currentExtent is advertised at
    // render_scale of the real window (the game sizes its offscreen targets to
    // that smaller framebuffer), but the swapchain itself is kept at the real
    // window extent so presentation fills the window even on WSI
    // implementations that do not upscale a smaller swapchain - that is what
    // avoids black bars / top-left pinning. The composite pass that renders
    // into the swapchain is stretched onto it by the vkCmdBeginRendering /
    // vkCmdSetViewport / vkCmdSetScissor hooks (MapRectToSwapchain /
    // MapViewportToSwapchain). This needs no engine flag, so it also works on
    // Roblox builds that ignore locally-set flags (the 2025-09-29 allowlist)
    // - unlike the FIntAutomaticDRS flag.
    const double render_scale = mocktail::graphics::CachedRenderScale();
    if (render_scale > 0.0 && render_scale < 1.0) {
      PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR caps_query = nullptr;
      {
        AdapterState& state = State();
        std::lock_guard<std::mutex> lock(state.mutex);
        caps_query = state.host_surface_capabilities;
      }
      const VkPhysicalDevice physical_device =
          HostDispatchForDevice(device).physical_device;
      VkExtent2D min_extent{1, 1};
      VkExtent2D max_extent{UINT32_MAX, UINT32_MAX};
      VkExtent2D current_extent{0, 0};
      if (caps_query != nullptr && physical_device != VK_NULL_HANDLE) {
        VkSurfaceCapabilitiesKHR capabilities{};
        if (caps_query(physical_device, create_info->surface, &capabilities) ==
            VK_SUCCESS) {
          min_extent = capabilities.minImageExtent;
          max_extent = capabilities.maxImageExtent;
          current_extent = capabilities.currentExtent;
        }
      }
      // The live SDL window exports are authoritative for the real window
      // extent; the host caps currentExtent is only a fallback (it can lag a
      // resize or report UINT32_MAX on Android-style WSI).
      VkExtent2D real_window{0, 0};
      {
        const WindowDimensionFn window_width =
            ResolveProcessFunction<WindowDimensionFn>("mocktail_window_width");
        const WindowDimensionFn window_height =
            ResolveProcessFunction<WindowDimensionFn>("mocktail_window_height");
        if (window_width != nullptr && window_height != nullptr &&
            window_width() > 0 && window_height() > 0) {
          real_window = {static_cast<std::uint32_t>(window_width()),
                         static_cast<std::uint32_t>(window_height())};
        }
      }
      if (real_window.width == 0 || real_window.height == 0 ||
          current_extent.width == UINT32_MAX ||
          current_extent.height == UINT32_MAX) {
        // keep the host-reported extent only when it is a real extent
      }
      if (real_window.width == 0 || real_window.height == 0) {
        if (current_extent.width != 0 && current_extent.width != UINT32_MAX &&
            current_extent.height != 0 &&
            current_extent.height != UINT32_MAX) {
          real_window = current_extent;
        }
      }
      if (real_window.width != 0 && real_window.height != 0) {
        const VkExtent2D advertised = mocktail::graphics::ScaleSwapchainExtent(
            real_window, render_scale, min_extent, max_extent);
        const VkExtent2D target{
            std::clamp(real_window.width, min_extent.width, max_extent.width),
            std::clamp(real_window.height, min_extent.height,
                       max_extent.height)};
        static std::atomic<bool> scale_logged{false};
        if (!scale_logged.exchange(true, std::memory_order_relaxed)) {
          std::fprintf(stderr,
                       "  [vulkan] render_scale=%.2f game surface advertised "
                       "%ux%u, swapchain kept at window %ux%u (requested "
                       "%ux%u; composite pass is stretched onto it)\n",
                       render_scale, advertised.width, advertised.height,
                       target.width, target.height, host_info.imageExtent.width,
                       host_info.imageExtent.height);
        }
        host_info.imageExtent = target;
        AdapterState& state = State();
        std::lock_guard<std::mutex> lock(state.mutex);
        state.swapchain_extent = target;
        state.advertised_framebuffer = advertised;
      }
    }
  }
  const VkResult result =
      host_create(device, &host_info, allocator, swapchain);
  if (result != VK_SUCCESS) {
    return result;
  }
  const auto host_get_images = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(
      HostDeviceProc(device, "vkGetSwapchainImagesKHR"));
  std::uint32_t image_count = 0;
  if (host_get_images == nullptr ||
      host_get_images(device, *swapchain, &image_count, nullptr) !=
          VK_SUCCESS ||
      image_count == 0) {
    return result;
  }
  std::vector<VkImage> images(image_count);
  const VkResult images_result =
      host_get_images(device, *swapchain, &image_count, images.data());
  if (images_result == VK_SUCCESS || images_result == VK_INCOMPLETE) {
    images.resize(image_count);
    (void)State().text_overlay.RegisterSwapchain(
        device, *swapchain, host_info, images.data(), image_count);
    {
      AdapterState& state = State();
      std::lock_guard<std::mutex> lock(state.mutex);
      for (const VkImage image : images) {
        state.swapchain_images.insert(reinterpret_cast<std::uint64_t>(image));
      }
    }
  }
  return result;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain,
                      const VkAllocationCallbacks* allocator) {
  const auto host_destroy = reinterpret_cast<PFN_vkDestroySwapchainKHR>(
      HostDeviceProc(device, "vkDestroySwapchainKHR"));
  {
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    for (auto it = state.view_to_image.begin(); it != state.view_to_image.end();) {
      if (state.swapchain_images.count(it->second) != 0) {
        it = state.view_to_image.erase(it);
      } else {
        ++it;
      }
    }
    state.swapchain_images.clear();
  }
  State().text_overlay.DestroySwapchain(device, swapchain);
  if (host_destroy != nullptr) {
    host_destroy(device, swapchain, allocator);
  }
}

VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(
    VkDevice device, VkSwapchainKHR swapchain, std::uint64_t timeout,
    VkSemaphore semaphore, VkFence fence, std::uint32_t* image_index) {
  VulkanCallObservation observation("vkAcquireNextImageKHR");
  const PFN_vkAcquireNextImageKHR host_acquire =
      HostDispatchForDevice(device).acquire_next_image;
  if (host_acquire == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const bool fps_trace = FpsTraceEnabled();
  const std::uint64_t acquire_start_ns = fps_trace ? MonotonicNanos() : 0;
  const VkResult host_result = host_acquire(
      device, swapchain, timeout, semaphore, fence, image_index);
  if (fps_trace) {
    AcquireWaitTrace().Record(acquire_start_ns);
  }
  const VkResult result = NormalizeSwapchainResult(host_result);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    const NoteSurfaceOutOfDateFn note_surface_out_of_date =
        State().note_surface_out_of_date.load(std::memory_order_acquire);
    if (note_surface_out_of_date != nullptr) {
      note_surface_out_of_date();
    }
  }
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImage2KHR(
    VkDevice device, const VkAcquireNextImageInfoKHR* acquire_info,
    std::uint32_t* image_index) {
  VulkanCallObservation observation("vkAcquireNextImage2KHR");
  const PFN_vkAcquireNextImage2KHR host_acquire =
      HostDispatchForDevice(device).acquire_next_image2;
  if (host_acquire == nullptr || acquire_info == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const VkResult host_result =
      host_acquire(device, acquire_info, image_index);
  const VkResult result = NormalizeSwapchainResult(host_result);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    const NoteSurfaceOutOfDateFn note_surface_out_of_date =
        State().note_surface_out_of_date.load(std::memory_order_acquire);
    if (note_surface_out_of_date != nullptr) {
      note_surface_out_of_date();
    }
  }
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(
    VkDevice device, std::uint32_t fence_count, const VkFence* fences,
    VkBool32 wait_all, std::uint64_t timeout) {
  VulkanCallObservation observation("vkWaitForFences");
  const PFN_vkWaitForFences host_wait =
      HostDispatchForDevice(device).wait_for_fences;
  if (host_wait == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const bool fps_trace = FpsTraceEnabled();
  const std::uint64_t fence_start_ns = fps_trace ? MonotonicNanos() : 0;
  const VkResult result =
      host_wait(device, fence_count, fences, wait_all, timeout);
  if (fps_trace) {
    FenceWaitTrace().Record(fence_start_ns);
  }
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(
    VkDevice device, std::uint32_t fence_count, const VkFence* fences) {
  VulkanCallObservation observation("vkResetFences");
  const PFN_vkResetFences host_reset =
      HostDispatchForDevice(device).reset_fences;
  if (host_reset == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const VkResult result = host_reset(device, fence_count, fences);
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(
    VkDevice device, VkCommandPool command_pool,
    VkCommandPoolResetFlags flags) {
  VulkanCallObservation observation("vkResetCommandPool");
  const PFN_vkResetCommandPool host_reset =
      HostDispatchForDevice(device).reset_command_pool;
  if (host_reset == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  ReleaseEtc2PoolCommandBuffers(device, command_pool);
  const VkResult result = host_reset(device, command_pool, flags);
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(
    VkDevice device, VkQueryPool query_pool, std::uint32_t first_query,
    std::uint32_t query_count, std::size_t data_size, void* data,
    VkDeviceSize stride, VkQueryResultFlags flags) {
  VulkanCallObservation observation("vkGetQueryPoolResults");
  const PFN_vkGetQueryPoolResults host_get_results =
      HostDispatchForDevice(device).get_query_pool_results;
  if (host_get_results == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const VkResult result = host_get_results(device, query_pool, first_query,
                                           query_count, data_size, data,
                                           stride, flags);
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(
    VkDevice device, const VkCommandBufferAllocateInfo* allocate_info,
    VkCommandBuffer* command_buffers) {
  VulkanCallObservation observation("vkAllocateCommandBuffers");
  const PFN_vkAllocateCommandBuffers host_allocate =
      HostDispatchForDevice(device).allocate_command_buffers;
  if (host_allocate == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const VkResult result =
      host_allocate(device, allocate_info, command_buffers);
  if (result == VK_SUCCESS && allocate_info != nullptr &&
      command_buffers != nullptr) {
    RegisterHostCommandBuffers(device, allocate_info->commandPool,
                               allocate_info->commandBufferCount,
                               command_buffers);
  }
  observation.SetResult(result);
  return result;
}

namespace {
void SetPresentPass(VkCommandBuffer command_buffer, bool present);
}  // namespace

VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(
    VkDevice device, VkCommandPool command_pool,
    std::uint32_t command_buffer_count,
    const VkCommandBuffer* command_buffers) {
  VulkanCallObservation observation("vkFreeCommandBuffers");
  const PFN_vkFreeCommandBuffers host_free =
      HostDispatchForDevice(device).free_command_buffers;
  if (host_free == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return;
  }
  if (command_buffers != nullptr) {
    for (std::uint32_t index = 0; index < command_buffer_count; ++index) {
      State().etc2.ReleaseCommandBuffer(command_buffers[index]);
      SetPresentPass(command_buffers[index], false);
    }
  }
  host_free(device, command_pool, command_buffer_count, command_buffers);
  RemoveHostCommandBuffers(device, command_pool, command_buffer_count,
                           command_buffers);
  observation.SetResult(VK_SUCCESS);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(
    VkDevice device, VkCommandPool command_pool,
    const VkAllocationCallbacks* allocator) {
  VulkanCallObservation observation("vkDestroyCommandPool");
  const PFN_vkDestroyCommandPool host_destroy =
      HostDispatchForDevice(device).destroy_command_pool;
  if (host_destroy == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return;
  }
  ReleaseEtc2PoolCommandBuffers(device, command_pool);
  {
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.present_pass_commands.clear();
  }
  host_destroy(device, command_pool, allocator);
  RemoveHostCommandPoolBindings(device, command_pool);
  observation.SetResult(VK_SUCCESS);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(
    VkCommandBuffer command_buffer,
    const VkCommandBufferBeginInfo* begin_info) {
  VulkanCallObservation observation("vkBeginCommandBuffer");
  const PFN_vkBeginCommandBuffer host_begin =
      HostDispatchForCommandBuffer(command_buffer).begin_command_buffer;
  if (host_begin == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  State().etc2.ReleaseCommandBuffer(command_buffer);
  SetPresentPass(command_buffer, false);
  const VkResult result = host_begin(command_buffer, begin_info);
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkEndCommandBuffer(VkCommandBuffer command_buffer) {
  VulkanCallObservation observation("vkEndCommandBuffer");
  const PFN_vkEndCommandBuffer host_end =
      HostDispatchForCommandBuffer(command_buffer).end_command_buffer;
  if (host_end == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const VkResult result = host_end(command_buffer);
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(
    VkCommandBuffer command_buffer, VkCommandBufferResetFlags flags) {
  VulkanCallObservation observation("vkResetCommandBuffer");
  const PFN_vkResetCommandBuffer host_reset =
      HostDispatchForCommandBuffer(command_buffer).reset_command_buffer;
  if (host_reset == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  State().etc2.ReleaseCommandBuffer(command_buffer);
  {
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.present_pass_commands.erase(
        reinterpret_cast<std::uint64_t>(command_buffer));
  }
  const VkResult result = host_reset(command_buffer, flags);
  observation.SetResult(result);
  return result;
}

namespace {

// Full-window pixel extent via the host window exports, or {0, 0}.
VkExtent2D HostWindowExtent() {
  using DimensionFn = int (*)();
  const DimensionFn width =
      ResolveProcessFunction<DimensionFn>("mocktail_window_width");
  const DimensionFn height =
      ResolveProcessFunction<DimensionFn>("mocktail_window_height");
  if (width == nullptr || height == nullptr || width() <= 0 ||
      height() <= 0) {
    return {0, 0};
  }
  return {static_cast<std::uint32_t>(width()),
          static_cast<std::uint32_t>(height())};
}

// True when `image` belongs to a swapchain created while render scaling is on.
bool IsSwapchainImage(std::uint64_t image) {
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  return state.swapchain_images.count(image) != 0;
}

// True when `view` was created over a swapchain image (a present attachment).
bool IsSwapchainView(std::uint64_t view) {
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  const auto it = state.view_to_image.find(view);
  return it != state.view_to_image.end() &&
         state.swapchain_images.count(it->second) != 0;
}

void NoteImageViewForImage(std::uint64_t image, std::uint64_t view) {
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  if (state.swapchain_images.count(image) != 0) {
    state.view_to_image[view] = image;
  }
}

void ForgetImageView(std::uint64_t view) {
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  state.view_to_image.erase(view);
}

bool CommandBufferRendersToSwapchain(VkCommandBuffer command_buffer) {
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  return state.present_pass_commands.count(
             reinterpret_cast<std::uint64_t>(command_buffer)) != 0;
}

void SetPresentPass(VkCommandBuffer command_buffer, bool present) {
  AdapterState& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  const std::uint64_t key = reinterpret_cast<std::uint64_t>(command_buffer);
  if (present) {
    state.present_pass_commands.insert(key);
  } else {
    state.present_pass_commands.erase(key);
  }
}

// The framebuffer the game is told the surface has (the clamped currentExtent).
VkExtent2D AdvertisedFramebuffer() {
  VkExtent2D framebuffer{0, 0};
  {
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    framebuffer = state.advertised_framebuffer;
  }
  if (framebuffer.width == 0 || framebuffer.height == 0) {
    const VkExtent2D window = HostWindowExtent();
    if (window.width == 0 || window.height == 0) {
      return {0, 0};
    }
    framebuffer = mocktail::graphics::ScaleSwapchainExtent(
        window, mocktail::graphics::CachedRenderScale(), {1, 1},
        {UINT32_MAX, UINT32_MAX});
  }
  return framebuffer;
}

// The real swapchain extent (the window) used to map present-pass rects.
VkExtent2D RealSwapchainExtent() {
  VkExtent2D extent{0, 0};
  {
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    extent = state.swapchain_extent;
  }
  if (extent.width == 0 || extent.height == 0) {
    extent = HostWindowExtent();
  }
  return extent;
}

}  // namespace

// Host-side render scale (window.render_scale < 1, direct-vulkan backend):
// the surface currentExtent is advertised at render_scale of the real window
// so the game sizes its offscreen targets to that smaller framebuffer, while
// the swapchain itself stays at the real window extent so presentation fills
// the window (no black bars, no top-left pinning, even on WSI implementations
// that do not upscale a smaller swapchain). Recorded rects are mapped
// accordingly: passes rendering into offscreen targets get full-window rects
// scaled onto the advertised framebuffer while framebuffer-sized rects are
// left alone, and the composite pass rendering into the swapchain (detected
// via its attachment image views in vkCmdBeginRendering) is stretched onto the
// real swapchain extent. The window itself is not resized, so input keeps
// mapping correctly in the game's full-window coordinate space. Scale 1.0 is
// a passthrough; no engine flags are involved.
VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(
    VkCommandBuffer command_buffer, std::uint32_t first_viewport,
    std::uint32_t viewport_count, const VkViewport* viewports) {
  const PFN_vkCmdSetViewport host_set_viewport =
      HostDispatchForCommandBuffer(command_buffer).cmd_set_viewport;
  if (host_set_viewport == nullptr) {
    return;
  }
  if (viewports == nullptr || viewport_count == 0 ||
      !mocktail::graphics::HostFramebufferScaleActive()) {
    host_set_viewport(command_buffer, first_viewport, viewport_count,
                      viewports);
    return;
  }
  const VkExtent2D window = HostWindowExtent();
  const VkExtent2D advertised = AdvertisedFramebuffer();
  if (window.width == 0 || window.height == 0 || advertised.width == 0 ||
      advertised.height == 0) {
    host_set_viewport(command_buffer, first_viewport, viewport_count,
                      viewports);
    return;
  }
  std::vector<VkViewport> scaled(viewports, viewports + viewport_count);
  if (CommandBufferRendersToSwapchain(command_buffer)) {
    // Present pass: stretch rects recorded in the advertised framebuffer's
    // coordinates onto the real (window-sized) swapchain.
    const VkExtent2D swapchain = RealSwapchainExtent();
    for (std::uint32_t i = 0; i < viewport_count; ++i) {
      mocktail::graphics::MapViewportToSwapchain(&scaled[i], advertised,
                                                 swapchain, window);
    }
  } else {
    // Offscreen pass: full-window rects are scaled onto the advertised
    // framebuffer; framebuffer-sized rects are left alone.
    const double scale_x =
        static_cast<double>(advertised.width) / window.width;
    const double scale_y =
        static_cast<double>(advertised.height) / window.height;
    mocktail::graphics::ScaleViewportsToFramebuffer(
        scaled.data(), viewport_count, scale_x, scale_y, advertised);
  }
  host_set_viewport(command_buffer, first_viewport, viewport_count,
                    scaled.data());
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(
    VkCommandBuffer command_buffer, std::uint32_t first_scissor,
    std::uint32_t scissor_count, const VkRect2D* scissors) {
  const PFN_vkCmdSetScissor host_set_scissor =
      HostDispatchForCommandBuffer(command_buffer).cmd_set_scissor;
  if (host_set_scissor == nullptr) {
    return;
  }
  if (scissors == nullptr || scissor_count == 0 ||
      !mocktail::graphics::HostFramebufferScaleActive()) {
    host_set_scissor(command_buffer, first_scissor, scissor_count, scissors);
    return;
  }
  const VkExtent2D window = HostWindowExtent();
  const VkExtent2D advertised = AdvertisedFramebuffer();
  if (window.width == 0 || window.height == 0 || advertised.width == 0 ||
      advertised.height == 0) {
    host_set_scissor(command_buffer, first_scissor, scissor_count, scissors);
    return;
  }
  std::vector<VkRect2D> scaled(scissors, scissors + scissor_count);
  if (CommandBufferRendersToSwapchain(command_buffer)) {
    const VkExtent2D swapchain = RealSwapchainExtent();
    for (std::uint32_t i = 0; i < scissor_count; ++i) {
      mocktail::graphics::MapRectToSwapchain(&scaled[i], advertised, swapchain,
                                            window);
    }
  } else {
    const double scale_x =
        static_cast<double>(advertised.width) / window.width;
    const double scale_y =
        static_cast<double>(advertised.height) / window.height;
    mocktail::graphics::ScaleScissorsToFramebuffer(
        scaled.data(), scissor_count, scale_x, scale_y, advertised);
  }
  host_set_scissor(command_buffer, first_scissor, scissor_count, scaled.data());
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWithCount(
    VkCommandBuffer command_buffer, std::uint32_t viewport_count,
    const VkViewport* viewports) {
  vkCmdSetViewport(command_buffer, 0, viewport_count, viewports);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetScissorWithCount(
    VkCommandBuffer command_buffer, std::uint32_t scissor_count,
    const VkRect2D* scissors) {
  vkCmdSetScissor(command_buffer, 0, scissor_count, scissors);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWithCountEXT(
    VkCommandBuffer command_buffer, std::uint32_t viewport_count,
    const VkViewport* viewports) {
  vkCmdSetViewportWithCount(command_buffer, viewport_count, viewports);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetScissorWithCountEXT(
    VkCommandBuffer command_buffer, std::uint32_t scissor_count,
    const VkRect2D* scissors) {
  vkCmdSetScissorWithCount(command_buffer, scissor_count, scissors);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginRendering(
    VkCommandBuffer command_buffer, const VkRenderingInfo* rendering_info) {
  const PFN_vkCmdBeginRendering host_begin =
      HostDispatchForCommandBuffer(command_buffer).begin_rendering;
  if (host_begin == nullptr) {
    return;
  }
  if (rendering_info == nullptr ||
      !mocktail::graphics::HostFramebufferScaleActive()) {
    SetPresentPass(command_buffer, false);
    host_begin(command_buffer, rendering_info);
    return;
  }
  bool present_pass = false;
  if (rendering_info->pColorAttachments != nullptr) {
    for (std::uint32_t i = 0; i < rendering_info->colorAttachmentCount; ++i) {
      const VkImageView view = rendering_info->pColorAttachments[i].imageView;
      if (view != VK_NULL_HANDLE &&
          IsSwapchainView(reinterpret_cast<std::uint64_t>(view))) {
        present_pass = true;
        break;
      }
    }
  }
  if (!present_pass && rendering_info->pDepthAttachment != nullptr &&
      rendering_info->pDepthAttachment->imageView != VK_NULL_HANDLE &&
      IsSwapchainView(reinterpret_cast<std::uint64_t>(
          rendering_info->pDepthAttachment->imageView))) {
    present_pass = true;
  }
  if (!present_pass && rendering_info->pStencilAttachment != nullptr &&
      rendering_info->pStencilAttachment->imageView != VK_NULL_HANDLE &&
      IsSwapchainView(reinterpret_cast<std::uint64_t>(
          rendering_info->pStencilAttachment->imageView))) {
    present_pass = true;
  }
  SetPresentPass(command_buffer, present_pass);
  const VkExtent2D window = HostWindowExtent();
  const VkExtent2D advertised = AdvertisedFramebuffer();
  if (!present_pass || window.width == 0 || window.height == 0 ||
      advertised.width == 0 || advertised.height == 0) {
    host_begin(command_buffer, rendering_info);
    return;
  }
  VkRenderingInfo scaled = *rendering_info;
  const VkExtent2D swapchain = RealSwapchainExtent();
  mocktail::graphics::MapRectToSwapchain(&scaled.renderArea, advertised,
                                        swapchain, window);
  host_begin(command_buffer, &scaled);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderingKHR(
    VkCommandBuffer command_buffer, const VkRenderingInfoKHR* rendering_info) {
  vkCmdBeginRendering(command_buffer, rendering_info);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndRendering(VkCommandBuffer command_buffer) {
  const PFN_vkCmdEndRendering host_end =
      HostDispatchForCommandBuffer(command_buffer).cmd_end_rendering;
  SetPresentPass(command_buffer, false);
  if (host_end != nullptr) {
    host_end(command_buffer);
  }
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderingKHR(VkCommandBuffer command_buffer) {
  vkCmdEndRendering(command_buffer);
}

VKAPI_ATTR VkResult VKAPI_CALL vkWaitSemaphores(
    VkDevice device, const VkSemaphoreWaitInfo* wait_info,
    std::uint64_t timeout) {
  VulkanCallObservation observation("vkWaitSemaphores");
  const PFN_vkWaitSemaphores host_wait =
      HostDispatchForDevice(device).wait_semaphores;
  return WaitForSemaphoresObserved("vkWaitSemaphores", host_wait, device,
                                   wait_info, timeout, &observation);
}

VKAPI_ATTR VkResult VKAPI_CALL vkWaitSemaphoresKHR(
    VkDevice device, const VkSemaphoreWaitInfo* wait_info,
    std::uint64_t timeout) {
  VulkanCallObservation observation("vkWaitSemaphoresKHR");
  const PFN_vkWaitSemaphoresKHR host_wait =
      HostDispatchForDevice(device).wait_semaphores_khr;
  return WaitForSemaphoresObserved(
      "vkWaitSemaphoresKHR", reinterpret_cast<PFN_vkWaitSemaphores>(host_wait),
      device, wait_info, timeout, &observation);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(
    VkQueue queue, std::uint32_t submit_count, const VkSubmitInfo* submits,
    VkFence fence) {
  VulkanCallObservation observation("vkQueueSubmit");
  const PFN_vkQueueSubmit host_submit =
      HostDispatchForQueue(queue).queue_submit;
  if (host_submit == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  if (submits != nullptr && State().etc2.HasPendingUploads()) {
    if (submit_count == 1) {
      State().etc2.PrepareSubmit(submits->pCommandBuffers,
                                 submits->commandBufferCount);
    } else {
      std::vector<VkCommandBuffer> command_buffers;
      for (std::uint32_t index = 0; index < submit_count; ++index) {
        if (submits[index].commandBufferCount != 0) {
          command_buffers.insert(command_buffers.end(),
                                   submits[index].pCommandBuffers,
                                   submits[index].pCommandBuffers +
                                       submits[index].commandBufferCount);
        }
      }
      State().etc2.PrepareSubmit(
          command_buffers.data(),
          static_cast<std::uint32_t>(command_buffers.size()));
    }
  }
  const VkResult result = State().text_overlay.QueueSubmit(
      queue, submit_count, submits, fence, host_submit);
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit2(
    VkQueue queue, std::uint32_t submit_count, const VkSubmitInfo2* submits,
    VkFence fence) {
  VulkanCallObservation observation("vkQueueSubmit2");
  const PFN_vkQueueSubmit2 host_submit =
      HostDispatchForQueue(queue).queue_submit2;
  if (host_submit == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  PrepareEtc2Submit2(submit_count, submits);
  const VkResult result = State().text_overlay.QueueSubmit2(
      queue, submit_count, submits, fence, host_submit);
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit2KHR(
    VkQueue queue, std::uint32_t submit_count, const VkSubmitInfo2* submits,
    VkFence fence) {
  VulkanCallObservation observation("vkQueueSubmit2KHR");
  const PFN_vkQueueSubmit2KHR host_submit =
      HostDispatchForQueue(queue).queue_submit2_khr;
  if (host_submit == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  PrepareEtc2Submit2(submit_count, submits);
  const VkResult result = State().text_overlay.QueueSubmit2(
      queue, submit_count, submits, fence,
      reinterpret_cast<PFN_vkQueueSubmit2>(host_submit));
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueBindSparse(
    VkQueue queue, std::uint32_t bind_info_count,
    const VkBindSparseInfo* bind_info, VkFence fence) {
  VulkanCallObservation observation("vkQueueBindSparse");
  const PFN_vkQueueBindSparse host_bind =
      HostDispatchForQueue(queue).queue_bind_sparse;
  if (host_bind == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const VkResult result = State().text_overlay.QueueBindSparse(
      queue, bind_info_count, bind_info, fence, host_bind);
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(VkQueue queue) {
  VulkanCallObservation observation("vkQueueWaitIdle");
  const PFN_vkQueueWaitIdle host_wait =
      HostDispatchForQueue(queue).queue_wait_idle;
  if (host_wait == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const bool fps_trace = FpsTraceEnabled();
  const std::uint64_t idle_start_ns = fps_trace ? MonotonicNanos() : 0;
  const VkResult result =
      State().text_overlay.QueueWaitIdle(queue, host_wait);
  if (fps_trace) {
    QueueIdleWaitTrace().Record(idle_start_ns);
  }
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(VkDevice device) {
  VulkanCallObservation observation("vkDeviceWaitIdle");
  const PFN_vkDeviceWaitIdle host_wait =
      HostDispatchForDevice(device).device_wait_idle;
  if (host_wait == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const VkResult result =
      State().text_overlay.DeviceWaitIdle(device, host_wait);
  observation.SetResult(result);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    VkPhysicalDevice physical_device, VkSurfaceKHR surface,
    VkSurfaceCapabilitiesKHR* capabilities) {
  PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR host_capabilities = nullptr;
  {
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    host_capabilities = state.host_surface_capabilities;
  }
  if (host_capabilities == nullptr || capabilities == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const VkResult result =
      host_capabilities(physical_device, surface, capabilities);
  if (result == VK_SUCCESS &&
      (capabilities->currentExtent.width == UINT32_MAX ||
       capabilities->currentExtent.height == UINT32_MAX)) {
    const WindowDimensionFn window_width =
        ResolveProcessFunction<WindowDimensionFn>("mocktail_window_width");
    const WindowDimensionFn window_height =
        ResolveProcessFunction<WindowDimensionFn>("mocktail_window_height");
    if (window_width != nullptr && window_height != nullptr &&
        window_width() > 0 && window_height() > 0) {
      capabilities->currentExtent.width = std::clamp(
          static_cast<std::uint32_t>(window_width()),
          capabilities->minImageExtent.width,
          capabilities->maxImageExtent.width);
      capabilities->currentExtent.height = std::clamp(
          static_cast<std::uint32_t>(window_height()),
          capabilities->minImageExtent.height,
          capabilities->maxImageExtent.height);

      AdapterState& state = State();
      std::lock_guard<std::mutex> lock(state.mutex);
      if (!state.extent_translation_logged) {
        state.extent_translation_logged = true;
        std::fprintf(
            stderr, "  [vulkan] Android currentExtent translated to %ux%u\n",
            capabilities->currentExtent.width, capabilities->currentExtent.height);
      }
    }
  }
  if (result == VK_SUCCESS &&
      capabilities->currentExtent.width != UINT32_MAX &&
      capabilities->currentExtent.height != UINT32_MAX) {
    // Host-side framebuffer scale: report the scaled surface extent so the
    // game sizes its viewport and swapchain to the clamped framebuffer
    // instead of the full window (which would crop when the smaller
    // swapchain is upscaled at present).
    const double render_scale = mocktail::graphics::CachedRenderScale();
    if (render_scale > 0.0 && render_scale < 1.0) {
      const VkExtent2D scaled = mocktail::graphics::ScaleSwapchainExtent(
          capabilities->currentExtent, render_scale,
          capabilities->minImageExtent, capabilities->maxImageExtent);
      if (scaled.width != capabilities->currentExtent.width ||
          scaled.height != capabilities->currentExtent.height) {
        capabilities->currentExtent = scaled;
        AdapterState& state = State();
        std::lock_guard<std::mutex> lock(state.mutex);
        state.advertised_framebuffer = scaled;
        if (!state.scale_extent_logged) {
          state.scale_extent_logged = true;
          std::fprintf(
              stderr,
              "  [vulkan] render_scale=%.2f surface currentExtent -> %ux%u "
              "(composite pass stretched onto the window-sized swapchain)\n",
              render_scale, scaled.width, scaled.height);
        }
      }
    }
  }
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(
    VkPhysicalDevice physical_device, VkSurfaceKHR surface,
    std::uint32_t* present_mode_count, VkPresentModeKHR* present_modes) {
  if (present_mode_count == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  PFN_vkGetPhysicalDeviceSurfacePresentModesKHR host_query = nullptr;
  {
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    host_query = state.host_surface_present_modes;
  }
  if (host_query == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  std::uint32_t host_count = 0;
  VkResult result = host_query(physical_device, surface, &host_count, nullptr);
  if (result != VK_SUCCESS) {
    return result;
  }
  std::vector<VkPresentModeKHR> host_modes(host_count);
  if (host_count != 0) {
    result =
        host_query(physical_device, surface, &host_count, host_modes.data());
    if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
      return result;
    }
    host_modes.resize(host_count);
  }
  const mocktail::graphics::PresentModePolicy policy = mocktail::graphics::CachedPresentModePolicy();
  const std::vector<VkPresentModeKHR> visible =
      mocktail::graphics::FilterPresentModes(policy, host_modes);
  {
    AdapterState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (!state.present_policy_logged) {
      state.present_policy_logged = true;
      std::fprintf(stderr, "  [vulkan] present policy=%s modes=%zu\n",
                   mocktail::graphics::PresentModePolicyName(policy),
                   visible.size());
    }
  }
  if (present_modes == nullptr) {
    *present_mode_count = static_cast<std::uint32_t>(visible.size());
    return VK_SUCCESS;
  }
  const std::uint32_t capacity = *present_mode_count;
  const std::uint32_t copied =
      std::min(capacity, static_cast<std::uint32_t>(visible.size()));
  std::copy_n(visible.begin(), copied, present_modes);
  *present_mode_count = copied;
  return copied < visible.size() ? VK_INCOMPLETE : VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* present_info) {
  VulkanCallObservation observation("vkQueuePresentKHR(adapter)");
  AdapterState& state = State();
  const PFN_vkQueuePresentKHR host_present =
      HostDispatchForQueue(queue).queue_present;
  const NotePresentFn note_present =
      state.note_present.load(std::memory_order_acquire);
  if (host_present == nullptr) {
    observation.SetResult(VK_ERROR_INITIALIZATION_FAILED);
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  const VkResult result = state.text_overlay.QueuePresent(
      queue, present_info, ObservedHostQueuePresent);
  const VkResult normalized_result = NormalizeSwapchainResult(result);
  if (normalized_result == VK_ERROR_OUT_OF_DATE_KHR) {
    const NoteSurfaceOutOfDateFn note_surface_out_of_date =
        state.note_surface_out_of_date.load(std::memory_order_acquire);
    if (note_surface_out_of_date != nullptr) {
      note_surface_out_of_date();
    }
  }
  if (normalized_result == VK_SUCCESS && note_present != nullptr) {
    note_present();
  }
  if (present_info != nullptr && present_info->pResults != nullptr) {
    for (std::uint32_t index = 0; index < present_info->swapchainCount;
         ++index) {
      present_info->pResults[index] =
          NormalizeSwapchainResult(present_info->pResults[index]);
    }
  }
  observation.SetResult(normalized_result);
  return normalized_result;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(
    VkPhysicalDevice physical_device, VkPhysicalDeviceFeatures* features) {
  const auto host = HostPhysicalDeviceProc<PFN_vkGetPhysicalDeviceFeatures>(
      "vkGetPhysicalDeviceFeatures", nullptr);
  if (host == nullptr || features == nullptr) {
    return;
  }
  host(physical_device, features);
  if (Etc2EmulatedPhysicalDevice(physical_device) &&
      mocktail::graphics::Etc2SupportAdvertised()) {
    features->textureCompressionETC2 = VK_TRUE;
  }
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2(
    VkPhysicalDevice physical_device, VkPhysicalDeviceFeatures2* features) {
  const auto host = HostPhysicalDeviceProc<PFN_vkGetPhysicalDeviceFeatures2>(
      "vkGetPhysicalDeviceFeatures2", "vkGetPhysicalDeviceFeatures2KHR");
  if (host == nullptr || features == nullptr) {
    return;
  }
  host(physical_device, features);
  if (Etc2EmulatedPhysicalDevice(physical_device) &&
      mocktail::graphics::Etc2SupportAdvertised()) {
    features->features.textureCompressionETC2 = VK_TRUE;
  }
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties(
    VkPhysicalDevice physical_device, VkFormat format,
    VkFormatProperties* properties) {
  const auto host =
      HostPhysicalDeviceProc<PFN_vkGetPhysicalDeviceFormatProperties>(
          "vkGetPhysicalDeviceFormatProperties", nullptr);
  if (host == nullptr || properties == nullptr) {
    return;
  }
  mocktail::graphics::Etc2EmulatedFormat mapped;
  if (!mocktail::graphics::LookupEmulatedEtc2Format(format, &mapped) ||
      !Etc2EmulatedPhysicalDevice(physical_device)) {
    host(physical_device, format, properties);
    return;
  }
  host(physical_device, mapped.host_format, properties);
  properties->linearTilingFeatures = 0;
  properties->optimalTilingFeatures =
      mocktail::graphics::EmulatedEtc2FormatFeatures(
          properties->optimalTilingFeatures);
  properties->bufferFeatures = 0;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties2(
    VkPhysicalDevice physical_device, VkFormat format,
    VkFormatProperties2* properties) {
  const auto host =
      HostPhysicalDeviceProc<PFN_vkGetPhysicalDeviceFormatProperties2>(
          "vkGetPhysicalDeviceFormatProperties2",
          "vkGetPhysicalDeviceFormatProperties2KHR");
  if (host == nullptr || properties == nullptr) {
    return;
  }
  mocktail::graphics::Etc2EmulatedFormat mapped;
  if (!mocktail::graphics::LookupEmulatedEtc2Format(format, &mapped) ||
      !Etc2EmulatedPhysicalDevice(physical_device)) {
    host(physical_device, format, properties);
    return;
  }
  host(physical_device, mapped.host_format, properties);
  VkFormatProperties& base = properties->formatProperties;
  base.linearTilingFeatures = 0;
  base.optimalTilingFeatures =
      mocktail::graphics::EmulatedEtc2FormatFeatures(base.optimalTilingFeatures);
  base.bufferFeatures = 0;
  if (auto* extended = const_cast<VkFormatProperties3*>(
          reinterpret_cast<const VkFormatProperties3*>(FindFeature(
              properties->pNext, VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3)));
      extended != nullptr) {
    extended->linearTilingFeatures = 0;
    extended->optimalTilingFeatures =
        mocktail::graphics::EmulatedEtc2FormatFeatures(
            static_cast<VkFormatFeatureFlags>(
                extended->optimalTilingFeatures));
    extended->bufferFeatures = 0;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties(
    VkPhysicalDevice physical_device, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkImageFormatProperties* properties) {
  const auto host =
      HostPhysicalDeviceProc<PFN_vkGetPhysicalDeviceImageFormatProperties>(
          "vkGetPhysicalDeviceImageFormatProperties", nullptr);
  if (host == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  mocktail::graphics::Etc2EmulatedFormat mapped;
  if (!mocktail::graphics::LookupEmulatedEtc2Format(format, &mapped) ||
      !Etc2EmulatedPhysicalDevice(physical_device)) {
    return host(physical_device, format, type, tiling, usage, flags,
                properties);
  }
  if (tiling != VK_IMAGE_TILING_OPTIMAL || type != VK_IMAGE_TYPE_2D) {
    return VK_ERROR_FORMAT_NOT_SUPPORTED;
  }
  return host(physical_device, mapped.host_format, type, tiling, usage,
              flags & ~static_cast<VkImageCreateFlags>(
                          VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT),
              properties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties2(
    VkPhysicalDevice physical_device,
    const VkPhysicalDeviceImageFormatInfo2* info,
    VkImageFormatProperties2* properties) {
  const auto host =
      HostPhysicalDeviceProc<PFN_vkGetPhysicalDeviceImageFormatProperties2>(
          "vkGetPhysicalDeviceImageFormatProperties2",
          "vkGetPhysicalDeviceImageFormatProperties2KHR");
  if (host == nullptr || info == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  mocktail::graphics::Etc2EmulatedFormat mapped;
  if (!mocktail::graphics::LookupEmulatedEtc2Format(info->format, &mapped) ||
      !Etc2EmulatedPhysicalDevice(physical_device)) {
    return host(physical_device, info, properties);
  }
  if (info->tiling != VK_IMAGE_TILING_OPTIMAL ||
      info->type != VK_IMAGE_TYPE_2D) {
    return VK_ERROR_FORMAT_NOT_SUPPORTED;
  }
  VkPhysicalDeviceImageFormatInfo2 host_info = *info;
  host_info.format = mapped.host_format;
  host_info.flags &= ~static_cast<VkImageCreateFlags>(
      VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT);
  return host(physical_device, &host_info, properties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(
    VkDevice device, const VkImageCreateInfo* create_info,
    const VkAllocationCallbacks* allocator, VkImage* image) {
  return State().etc2.CreateImage(device, create_info, allocator, image);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyImage(
    VkDevice device, VkImage image, const VkAllocationCallbacks* allocator) {
  State().etc2.DestroyImage(device, image, allocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(
    VkDevice device, const VkImageViewCreateInfo* create_info,
    const VkAllocationCallbacks* allocator, VkImageView* view) {
  const VkResult result =
      State().etc2.CreateImageView(device, create_info, allocator, view);
  if (result == VK_SUCCESS && create_info != nullptr && view != nullptr &&
      create_info->image != VK_NULL_HANDLE && *view != VK_NULL_HANDLE) {
    NoteImageViewForImage(reinterpret_cast<std::uint64_t>(create_info->image),
                          reinterpret_cast<std::uint64_t>(*view));
  }
  return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyImageView(
    VkDevice device, VkImageView image_view,
    const VkAllocationCallbacks* allocator) {
  ForgetImageView(reinterpret_cast<std::uint64_t>(image_view));
  const PFN_vkDestroyImageView host_destroy =
      HostDispatchForDevice(device).destroy_image_view;
  if (host_destroy != nullptr) {
    host_destroy(device, image_view, allocator);
  }
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(VkDevice device,
                                                  VkBuffer buffer,
                                                  VkDeviceMemory memory,
                                                  VkDeviceSize offset) {
  return State().etc2.BindBufferMemory(device, buffer, memory, offset);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory2(
    VkDevice device, std::uint32_t count,
    const VkBindBufferMemoryInfo* infos) {
  return State().etc2.BindBufferMemory2(device, count, infos);
}

VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(VkDevice device,
                                           VkDeviceMemory memory,
                                           VkDeviceSize offset,
                                           VkDeviceSize size,
                                           VkMemoryMapFlags flags,
                                           void** data) {
  return State().etc2.MapMemory(device, memory, offset, size, flags, data);
}

VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory2(VkDevice device,
                                            const VkMemoryMapInfo* info,
                                            void** data) {
  return State().etc2.MapMemory2(device, info, data);
}

VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(VkDevice device,
                                         VkDeviceMemory memory) {
  State().etc2.UnmapMemory(device, memory);
}

VKAPI_ATTR VkResult VKAPI_CALL vkUnmapMemory2(VkDevice device,
                                              const VkMemoryUnmapInfo* info) {
  return State().etc2.UnmapMemory2(device, info);
}

VKAPI_ATTR void VKAPI_CALL vkFreeMemory(
    VkDevice device, VkDeviceMemory memory,
    const VkAllocationCallbacks* allocator) {
  State().etc2.FreeMemory(device, memory, allocator);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(
    VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* allocator) {
  State().etc2.DestroyBuffer(device, buffer, allocator);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage(
    VkCommandBuffer command_buffer, VkBuffer source, VkImage destination,
    VkImageLayout layout, std::uint32_t region_count,
    const VkBufferImageCopy* regions) {
  State().etc2.CmdCopyBufferToImage(
      HostDispatchForCommandBuffer(command_buffer).device, command_buffer,
      source, destination, layout, region_count, regions);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage2(
    VkCommandBuffer command_buffer, const VkCopyBufferToImageInfo2* info) {
  State().etc2.CmdCopyBufferToImage2(
      HostDispatchForCommandBuffer(command_buffer).device, command_buffer,
      info);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage(
    VkCommandBuffer command_buffer, VkImage source,
    VkImageLayout source_layout, VkImage destination,
    VkImageLayout destination_layout, std::uint32_t region_count,
    const VkImageCopy* regions) {
  State().etc2.CmdCopyImage(
      HostDispatchForCommandBuffer(command_buffer).device, command_buffer,
      source, source_layout, destination, destination_layout, region_count,
      regions);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage2(VkCommandBuffer command_buffer,
                                           const VkCopyImageInfo2* info) {
  State().etc2.CmdCopyImage2(
      HostDispatchForCommandBuffer(command_buffer).device, command_buffer,
      info);
}

VKAPI_ATTR void VKAPI_CALL vkCmdExecuteCommands(
    VkCommandBuffer command_buffer, std::uint32_t count,
    const VkCommandBuffer* secondaries) {
  State().etc2.CmdExecuteCommands(
      HostDispatchForCommandBuffer(command_buffer).device, command_buffer,
      count, secondaries);
}

}  // extern "C"
