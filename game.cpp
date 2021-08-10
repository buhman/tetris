#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <thread>
#include <stdexcept>
#include <vector>

#include "platform.hpp"

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/ext/matrix_projection.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"

#include "game.hpp"
#include "input.hpp"
#include "tetris.hpp"

static GLFWwindow * window;
static VkInstance instance;
static VkPhysicalDevice physicalDevice;
static VkDevice logicalDevice;
static VkQueue deviceQueue;
static VkSurfaceKHR surface;
static VkSwapchainKHR swapChain;
static VkFormat swapChainImageFormat;
static VkExtent2D swapChainExtent;
static std::vector<VkImage> swapChainImages;
static std::vector<VkImageView> swapChainImageViews;
static VkRenderPass renderPass;
static VkDescriptorSetLayout descriptorSetLayout;
static VkPipelineLayout pipelineLayout;
static std::vector<VkPipeline> graphicsPipeline(2);
static std::vector<VkFramebuffer> swapChainFramebuffers;
static uint32_t queueFamilyIndex;
static VkBuffer vertexBuffer;
static VkDeviceMemory vertexBufferMemory;
static VkBuffer indexBuffer;
static VkDeviceMemory indexBufferMemory;

static std::vector<VkBuffer> uniformDynamicBuffers;
static std::vector<VkDeviceMemory> uniformDynamicBuffersMemory;

static std::vector<VkBuffer> uniformBuffers;
static std::vector<VkDeviceMemory> uniformBuffersMemory;
static size_t uniformBufferSize = sizeof (glm::mat4);

static VkDescriptorPool descriptorPool;
static std::vector<VkDescriptorSet> descriptorSets;
static VkCommandPool commandPool;
static std::vector<VkCommandBuffer> commandBuffers;
static VkImage textureImage;
static VkDeviceMemory textureImageMemory;
static VkImageView textureImageView;
static VkSampler textureSampler;
static VkImage depthImage;
static VkDeviceMemory depthImageMemory;
static VkImageView depthImageView;

static VkSurfaceCapabilitiesKHR surfaceCapabilities;
static std::vector<VkSurfaceFormatKHR> surfaceFormats;
static std::vector<VkPresentModeKHR> presentModes;

const std::vector<const char*> validationLayers = {
  "VK_LAYER_KHRONOS_validation"
};

const int MAX_FRAMES_IN_FLIGHT = 2;
static std::vector<VkSemaphore> imageAcquired;
static std::vector<VkSemaphore> renderFinished;
static std::vector<VkFence> frameInFlight;
static std::vector<VkFence> imageInFlight;
static size_t currentFrame = 0;

void recreateSwapChain();
void cleanupSwapChain();

struct vertex {
  alignas(16) glm::vec3 pos;
  alignas(16) glm::vec3 color;
  alignas(8) glm::vec2 texCoord;
};

struct _ubo {
  alignas(16) glm::mat4 model;
  alignas(16) glm::vec3 color;
};

static size_t uniformDynamicAlignment;
static int maxTetrisQueueSize = 5;
static int uniformModelInstances = (10 * 20) + (maxTetrisQueueSize * 4) + 4;
static size_t uniformDynamicBufferSize;
static size_t uboInstanceSize = (sizeof (_ubo));
static void * uboModels;

// clockwise
const std::vector<vertex> vertices = {
  {{ -1.0f, -1.0f, 0.2f}, {1.0f, 1.0f, 1.0f}, {-1.0f, -1.0f}},
  {{  1.0f, -1.0f, 0.2f}, {1.0f, 1.0f, 1.0f}, { 1.0f, -1.0f}},
  {{  1.0f,  1.0f, 0.2f}, {1.0f, 1.0f, 1.0f}, { 1.0f,  1.0f}},
  {{ -1.0f,  1.0f, 0.2f}, {1.0f, 1.0f, 1.0f}, {-1.0f,  1.0f}},

  {{ -1.0f, -1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}, {-1.0f, -1.0f}},
  {{  1.0f, -1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}, { 1.0f, -1.0f}},
  {{  1.0f,  1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}, { 1.0f,  1.0f}},
  {{ -1.0f,  1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}, {-1.0f,  1.0f}},

  {{ -0.75f, -0.75f, 0.0f}, {0.8f, 0.8f, 0.8f}, {-1.0f, -1.0f}},
  {{  0.75f, -0.75f, 0.0f}, {0.8f, 0.8f, 0.8f}, { 1.0f, -1.0f}},
  {{  0.75f,  0.75f, 0.0f}, {0.8f, 0.8f, 0.8f}, { 1.0f,  1.0f}},
  {{ -0.75f,  0.75f, 0.0f}, {0.8f, 0.8f, 0.8f}, {-1.0f,  1.0f}},
};

const std::vector<uint16_t> indices = {
  0, 1, 2,
  2, 3, 0,

  /*
     0------1
     |\    /|
     | 4  5 |
     |      |
     | 7  6 |
     |/    \|
     3------2
  */

  0, 1, 4,
  1, 5, 4,

  1, 2, 6,
  6, 5, 1,

  6, 2, 3,
  3, 7, 6,

  3, 0, 7,
  7, 0, 4,
};

struct mvp_t {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

VkVertexInputBindingDescription getBindingDescription() {
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
  std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(vertex, pos);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(vertex, color);

  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(vertex, texCoord);

  return attributeDescriptions;
}

void initWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window = glfwCreateWindow(800, 800, "tak tetris", nullptr, nullptr);
}

bool assertValidationLayers() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char* layerName : validationLayers) {
    for (const auto& layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        goto next_layer;
      }
    }
    return false;

  next_layer:
    continue;
  }
  return true;
}

void createInstance() {
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
  std::cerr << "available instance extensions:\n";
  for (const auto& extension : extensions)
    std::cerr << '\t' << extension.extensionName << '\n';

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

  std::array<VkValidationFeatureEnableEXT, 1> enables = {VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};
  VkValidationFeaturesEXT features = {};
  features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
  features.enabledValidationFeatureCount = enables.size();
  features.pEnabledValidationFeatures = enables.data();

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  std::array<const char*, 2> instanceExtensions{};
  instanceExtensions[0] = "VK_KHR_surface";
  #if defined _WIN32
  instanceExtensions[1] = "VK_KHR_win32_surface";
  #elif defined __linux__
  instanceExtensions[1] = "VK_KHR_xlib_surface";
  #endif

  std::cerr << "requested instance extensions:\n";
  for (const auto& extension : instanceExtensions)
    std::cerr << '\t' << extension << '\n';

  createInfo.enabledExtensionCount = instanceExtensions.size();
  createInfo.ppEnabledExtensionNames = instanceExtensions.data();

  if (assertValidationLayers()) {
    std::cerr << "requested validation layers\n";
    createInfo.pNext = &features;
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  }

  VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance");
  }
}

const std::vector<const char*> deviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  //VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
  //VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME
};

bool checkDeviceFeatures(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);

  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty() && deviceFeatures.samplerAnisotropy;
}

std::optional<uint32_t> findQueueFamilies(VkPhysicalDevice device) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  uint32_t i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
      if (presentSupport)
        return i;
    }
    i++;
  }

  return std::optional<uint32_t>{};
}

void findSurfaceFormats(VkPhysicalDevice device,
                        std::vector<VkSurfaceFormatKHR>& formats) {
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  if (formatCount != 0) {
    formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats.data());
  }
}

void findPresentModes(VkPhysicalDevice device,
                      std::vector<VkPresentModeKHR>& presentModes) {
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

  if (presentModeCount != 0) {
    presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModes.data());
  }
}

void createSurface() {

#if defined _WIN32
  VkWin32SurfaceCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  createInfo.hwnd = glfwGetWin32Window(window);
  createInfo.hinstance = GetModuleHandle(nullptr);

  if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
#elif defined __linux__
  VkXlibSurfaceCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
  createInfo.dpy = glfwGetX11Display();
  createInfo.window = glfwGetX11Window(window);


  if (vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
#endif
}

void pickPhysicalDevice() {
  physicalDevice = VK_NULL_HANDLE;

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0)
    throw std::runtime_error("vkEnumeratePhysicalDevices");

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  std::optional<uint32_t> index;
  for (const auto& device : devices) {
    if (checkDeviceFeatures(device)) {
      index = findQueueFamilies(device);
      if (!index.has_value())
        continue;

      findSurfaceFormats(device, surfaceFormats);
      if (surfaceFormats.empty())
        continue;

      findPresentModes(device, presentModes);
      if (presentModes.empty())
        continue;

      physicalDevice = device;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE)
    throw std::runtime_error("physicalDevice == VK_NULL_HANDLE");

  queueFamilyIndex = index.value();

  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
  queueCreateInfo.queueCount = 1;
  float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;
  deviceFeatures.fillModeNonSolid = VK_TRUE;
  deviceFeatures.wideLines = VK_TRUE;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();


  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
    throw std::runtime_error("failed to create logical device");

  vkGetDeviceQueue(logicalDevice, queueFamilyIndex, 0, &deviceQueue);
}

VkSurfaceFormatKHR pickSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& surfaceFormats) {
  for (const auto& surfaceFormat : surfaceFormats) {
    if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB
        && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return surfaceFormat;
  }
  throw std::runtime_error("no matching surface format");
}

VkPresentModeKHR pickPresentMode(const std::vector<VkPresentModeKHR>& presentModes) {
  for (const auto& presentMode : presentModes) {
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return presentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D pickExtent() {
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
  if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
    return surfaceCapabilities.currentExtent;
  } else {
    throw std::runtime_error("not implemented");
  }
}

void createSwapChain() {
  VkSurfaceFormatKHR surfaceFormat = pickSurfaceFormat(surfaceFormats);
  VkPresentModeKHR presentMode = pickPresentMode(presentModes);
  VkExtent2D extent = pickExtent();

  uint32_t imageCount = surfaceCapabilities.minImageCount + 1;

  if (surfaceCapabilities.maxImageCount > 0
      && imageCount > surfaceCapabilities.maxImageCount) {
    imageCount = surfaceCapabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.queueFamilyIndexCount = 0;
  createInfo.pQueueFamilyIndices = nullptr;

  createInfo.preTransform = surfaceCapabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
    throw std::runtime_error("failed to create swap chain");

  vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());
  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;
}

void createImageViews() {
  swapChainImageViews.resize(swapChainImages.size());

  for (size_t i = 0; i < swapChainImages.size(); i++) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapChainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create image view");

  }
}

std::vector<char> readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error("failed to open file");
  size_t size = (size_t)file.tellg();
  std::vector<char> buffer(size);

  file.seekg(0);
  file.read(buffer.data(), size);
  file.close();

  return buffer;
}

VkShaderModule createShaderModule(const std::vector<char>& code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    throw std::runtime_error("failed to create shader module");

  return shaderModule;
}


void createRenderPass() {
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = VK_FORMAT_D32_SFLOAT;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    throw std::runtime_error("failed to create render pass");
}

void createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding modelLayoutBinding{};
  modelLayoutBinding.binding = 0;
  modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  modelLayoutBinding.descriptorCount = 1;
  modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  modelLayoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  samplerLayoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding viewLayoutBinding{};
  viewLayoutBinding.binding = 2;
  viewLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  viewLayoutBinding.descriptorCount = 1;
  viewLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  viewLayoutBinding.pImmutableSamplers = nullptr;

  std::array<VkDescriptorSetLayoutBinding, 3> bindings = {modelLayoutBinding, samplerLayoutBinding, viewLayoutBinding};
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    throw std::runtime_error("failed to create descriptor set layout!");
}

void createGraphicsPipelines() {
  auto vertShaderCode = readFile("shader.vert.spv");
  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  auto fragShaderCode = readFile("shader.frag.spv");
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  auto bindingDescription = getBindingDescription();
  auto attributeDescriptions = getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  std::vector<VkPipelineInputAssemblyStateCreateInfo> inputAssemblies(2);
  inputAssemblies[0].sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblies[0].topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblies[0].primitiveRestartEnable = VK_FALSE;

  inputAssemblies[1].sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblies[1].topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  inputAssemblies[1].primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float) swapChainExtent.width;
  viewport.height = (float) swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  std::vector<VkPipelineRasterizationStateCreateInfo> rasterizers(2);
  VkPolygonMode polygonModes[2] = {VK_POLYGON_MODE_FILL, VK_POLYGON_MODE_LINE};
  for (size_t i = 0; i < rasterizers.size(); i++) {
    rasterizers[i].sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizers[i].depthClampEnable = VK_FALSE;
    rasterizers[i].rasterizerDiscardEnable = VK_FALSE;
    rasterizers[i].polygonMode = polygonModes[i];
    rasterizers[i].lineWidth = 2.0f;
    //rasterizers[i].cullMode = VK_CULL_MODE_NONE;
    rasterizers[i].cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizers[i].frontFace = VK_FRONT_FACE_CLOCKWISE;
    //rasterizers[i].frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizers[i].depthBiasEnable = VK_FALSE;
    rasterizers[i].depthBiasConstantFactor = 0.0f;
    rasterizers[i].depthBiasClamp = 0.0f;
    rasterizers[i].depthBiasSlopeFactor = 0.0f;
  }

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("failed to create pipeline layout");

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f;
  depthStencil.maxDepthBounds = 1.0f;
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.front = {};
  depthStencil.back = {};

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizers[0];
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  for (size_t i = 0; i < graphicsPipeline.size(); i++) {
    pipelineInfo.pInputAssemblyState = &inputAssemblies[i];
    if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create graphics pipeline");
  }

  vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
  vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
}

void createFramebuffers() {
  swapChainFramebuffers.resize(swapChainImageViews.size());

  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    std::array<VkImageView, 2> attachments = {
      swapChainImageViews[i],
      depthImageView
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create framebuffer");

  }
}

void createCommandPool() {
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndex;
  poolInfo.flags = 0;

  if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    throw std::runtime_error("failed to create command pool");

}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  throw std::runtime_error("failed to find memory type");
}

void createBuffer(VkDeviceSize size,
                  VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer& buffer,
                  VkDeviceMemory& bufferMemory) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    throw std::runtime_error("failed to create buffer");

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate buffer memory");

  vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);
}

void beginOneTimeCommands(VkCommandBuffer &commandBuffer) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
}

void endOneTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(deviceQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(deviceQueue);

  vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
  VkCommandBuffer commandBuffer;
  beginOneTimeCommands(commandBuffer);

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

  endOneTimeCommands(commandBuffer);
}

void createBufferWithMemory(VkBufferUsageFlags usageFlags,
                            VkBuffer& buffer,
                            VkDeviceMemory& bufferMemory,
                            VkDeviceSize size,
                            const void *src) {

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(size,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer,
               stagingBufferMemory);

  void* dst;
  vkMapMemory(logicalDevice, stagingBufferMemory, 0, size, 0, &dst);
  memcpy(dst, src, (size_t)size);
  vkUnmapMemory(logicalDevice, stagingBufferMemory);

  createBuffer(size,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               buffer,
               bufferMemory);

  copyBuffer(stagingBuffer, buffer, size);

  vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
  vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
}

void createVertexBuffer() {
  VkDeviceSize size = sizeof(vertices[0]) * vertices.size();
  auto usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

  createBufferWithMemory(usageFlags, vertexBuffer, vertexBufferMemory, size, vertices.data());
}

void createIndexBuffer() {
  VkDeviceSize size = sizeof(indices[0]) * indices.size();
  auto usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

  createBufferWithMemory(usageFlags, indexBuffer, indexBufferMemory, size, indices.data());
}

void createUniformBuffers() {

  uniformDynamicBuffers.resize(swapChainImages.size());
  uniformDynamicBuffersMemory.resize(swapChainImages.size());

  for (size_t i = 0; i < swapChainImages.size(); i++) {

    createBuffer(uniformDynamicBufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformDynamicBuffers[i],
                 uniformDynamicBuffersMemory[i]);
  }

  uniformBuffers.resize(swapChainImages.size());
  uniformBuffersMemory.resize(swapChainImages.size());

  for (size_t i = 0; i < swapChainImages.size(); i++) {

    createBuffer(uniformBufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformBuffers[i],
                 uniformBuffersMemory[i]);
  }
}

void prepareUniformBuffers() {
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);

  size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
  uniformDynamicAlignment = uboInstanceSize;
  if (minUboAlignment > 0)
    uniformDynamicAlignment = (uniformDynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
  uniformDynamicAlignment = pow(2, ceil(log(uniformDynamicAlignment)/log(2)));

  uniformDynamicBufferSize = uniformModelInstances * uniformDynamicAlignment;
  int ret = posix_memalign(&uboModels, uniformDynamicAlignment, uniformDynamicBufferSize);
  assert(ret == 0);

  createUniformBuffers();
}

void createDescriptorPool() {
  std::array<VkDescriptorPoolSize, 3> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
  poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

  if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    throw std::runtime_error("failed to create descriptor pool");
}

void createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(swapChainImages.size());
  if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate descriptor sets");

  for (size_t i = 0; i < descriptorSets.size(); i++) {
    VkDescriptorBufferInfo bufferDynamicInfo{};
    bufferDynamicInfo.buffer = uniformDynamicBuffers[i];
    bufferDynamicInfo.offset = 0;
    bufferDynamicInfo.range = uboInstanceSize;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageView;
    imageInfo.sampler = textureSampler;

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = uniformBufferSize;

    std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferDynamicInfo;
    descriptorWrites[0].pImageInfo = nullptr;
    descriptorWrites[0].pTexelBufferView = nullptr;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = nullptr;
    descriptorWrites[1].pImageInfo = &imageInfo;
    descriptorWrites[1].pTexelBufferView = nullptr;

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = descriptorSets[i];
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &bufferInfo;
    descriptorWrites[2].pImageInfo = nullptr;
    descriptorWrites[2].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
  }
}

void createCommandBuffers() {
  commandBuffers.resize(swapChainFramebuffers.size());

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

  if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }

  for (size_t i = 0; i < commandBuffers.size(); i++) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
      throw std::runtime_error("failed to begin recording command buffer");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[i];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.1, 0.1, 0.1, 1.0}};
    clearValues[1].depthStencil = {1.0, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    VkBuffer vertexBuffers[] = {vertexBuffer};

    vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    //

    vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline[0]);
    for (uint32_t j = 0; j < uniformModelInstances; j++) {
      uint32_t dynamicOffset = j * static_cast<uint32_t>(uniformDynamicAlignment);

      vkCmdBindDescriptorSets(commandBuffers[i],
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipelineLayout, 0, 1,
                              &descriptorSets[i], 1, &dynamicOffset);

      vkCmdDrawIndexed(commandBuffers[i], 6, 1, 0, 0, 0);
    }

    //

    //vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline[1]);
    for (uint32_t j = 0; j < uniformModelInstances; j++) {
      uint32_t dynamicOffset = j * static_cast<uint32_t>(uniformDynamicAlignment);

      vkCmdBindDescriptorSets(commandBuffers[i],
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipelineLayout, 0, 1,
                              &descriptorSets[i], 1, &dynamicOffset);

      vkCmdDrawIndexed(commandBuffers[i], 24, 1, 6, 4, 0);
    }

    //

    vkCmdEndRenderPass(commandBuffers[i]);

    if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to record command buffer");
  }
}

void createImage(uint32_t width, uint32_t height,
                 VkFormat format,
                 VkImageTiling tiling,
                 VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties,
                 VkImage& image,
                 VkDeviceMemory& imageMemory) {

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = 0;

  if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS)
    throw std::runtime_error("failed to create image!");

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate image memory!");

  vkBindImageMemory(logicalDevice, image, imageMemory, 0);
}

void transitionImageLayout(VkImage image,
                           VkFormat format,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout) {
  VkCommandBuffer commandBuffer;
  beginOneTimeCommands(commandBuffer);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition");
  }

  vkCmdPipelineBarrier(commandBuffer,
                       sourceStage,
                       destinationStage,
                       0,
                       0, nullptr,
                       0, nullptr,
                       1,
                       &barrier);

  endOneTimeCommands(commandBuffer);
}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
  VkCommandBuffer commandBuffer;
  beginOneTimeCommands(commandBuffer);

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {
    width,
    height,
    1
  };

  vkCmdCopyBufferToImage(commandBuffer,
                         buffer,
                         image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region);

  endOneTimeCommands(commandBuffer);
}

void createDepth() {
  createImage(swapChainExtent.width,
              swapChainExtent.height,
              VK_FORMAT_D32_SFLOAT,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              depthImage,
              depthImageMemory);

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = depthImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_D32_SFLOAT;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &depthImageView) != VK_SUCCESS)
    throw std::runtime_error("failed to create depth image view");
}

void createTextureImage() {
  int width, height, channels;
  stbi_uc* pixels = stbi_load("texture.jpg", &width, &height, &channels, STBI_rgb_alpha);
  if (!pixels)
    throw std::runtime_error("failed to load texture image");

  VkDeviceSize imageSize = width * height * 4;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(imageSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer,
               stagingBufferMemory);

  void *data;
  vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(logicalDevice, stagingBufferMemory);
  stbi_image_free(pixels);

  createImage(width, height,
              VK_FORMAT_R8G8B8A8_SRGB,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              textureImage,
              textureImageMemory);

  transitionImageLayout(textureImage,
                        VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  copyBufferToImage(stagingBuffer,
                    textureImage,
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height));

  transitionImageLayout(textureImage,
                        VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
  vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
}

void createTextureImageView() {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = textureImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS)
    throw std::runtime_error("failed to create texture image view");
}

void createTextureImageSampler() {
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
    throw std::runtime_error("failed to create texture sampler");
}

void createSync() {
  imageAcquired.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinished.resize(MAX_FRAMES_IN_FLIGHT);
  frameInFlight.resize(MAX_FRAMES_IN_FLIGHT);
  imageInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAcquired[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create semaphore");

    if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinished[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create semaphore");

    if (vkCreateFence(logicalDevice, &fenceInfo, nullptr, &frameInFlight[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create fence");
  }
}

void initVulkan() {
  createInstance();
  createSurface();
  pickPhysicalDevice();
  createCommandPool();
  createTextureImage();
  createTextureImageView();
  createTextureImageSampler();
  createVertexBuffer();
  createIndexBuffer();

  recreateSwapChain();

  createSync();
}

void updateUniformBuffer(uint32_t currentImage) {
  for (int u = 0; u < 10; u++) {
    for (int v = 0; v < 20; v++) {
      int i = u + (10 * v);
      assert(i < uniformModelInstances);

      tetris::cell cell = tetris::board[u][v];
      _ubo* ubo = (_ubo*)(((uint64_t)uboModels + (i * uniformDynamicAlignment)));
      ubo->model = glm::translate(glm::mat4(1.0f),
                                  glm::vec3(-0.45f + (float)u * 0.1f, -0.95f + (float)v * 0.1f, 0.2f));

      if (cell.empty)
        ubo->color = {0.0f, 0.0f, 0.0f};
      else
        ubo->color = cellColors[cell.color];
    }
  }

  tetris::coord *offset;
  for (int qi = 0; qi < maxTetrisQueueSize; qi++) {
    tetris::type queueType = tetris::queue[4 - qi];
    offset = tetris::offsets[queueType][0];
    for (int ti = 0; ti < 4; ti++) {
      int qti = 200 + qi + (5 * ti);
      assert(qti >= 200 && qti < (200 + (4 * 5)));
      _ubo* ubo = (_ubo*)(((uint64_t)uboModels + (qti * uniformDynamicAlignment)));
      //ubo->model = glm::mat4(1.0f);
      int u = 13 + offset[ti].u;
      int v = -4 + (qi * 3) + offset[ti].v;
      ubo->model = glm::translate(glm::mat4(1.0f),
                                  glm::vec3(-0.75f + (float)u * 0.1f, -0.95f + (float)v * 0.1f, 0.0f));
      ubo->model = glm::scale(glm::mat4(1.0f), glm::vec3(0.7f, 0.7f, 1.0f)) * ubo->model;

      ubo->color = cellColors[queueType];
    }
  }

  for (int i = 0; i < 4; i++) {
    int w = 200 + 20 + i;

    _ubo* ubo = (_ubo*)(((uint64_t)uboModels + (w * uniformDynamicAlignment)));

    if (tetris::swap != tetris::TET_LAST) {
      offset = tetris::offsets[tetris::swap][0];
      int u = -2 + offset[i].u;
      int v = 1 + offset[i].v;
      ubo->model = glm::translate(glm::mat4(1.0f),
                                  glm::vec3(-0.95f + (float)u * 0.1f, -0.95f + (float)v * 0.1f, 0.0f));
      ubo->color = cellColors[tetris::swap];
    } else {
      ubo->model = glm::mat4(0.0f);
      ubo->color = glm::vec3(0.0f, 0.0f, 0.0f);
    }
  }

  offset = tetris::offsets[tetris::piece.type][tetris::piece.facing];
  for (int i = 0; i < 4; i++) {
    int q = tetris::piece.pos.u + offset[i].u;
    int r = tetris::piece.pos.v + offset[i].v;
    int w0 = q + (10 * r);

    int r1 = tetris::drop_row + offset[i].v;
    int w1 = q + (10 * r1);
    if (w1 >= 0) {
      _ubo* ubo1 = (_ubo*)(((uint64_t)uboModels + (w1 * uniformDynamicAlignment)));
      ubo1->color = glm::vec3(0.2f, 0.2f, 0.2f) * cellColors[tetris::piece.type];
    }

    if (w0 >= 0) {
      _ubo* ubo0 = (_ubo*)(((uint64_t)uboModels + (w0 * uniformDynamicAlignment)));
      ubo0->color = cellColors[tetris::piece.type];
    }
  }

  void *data;
  vkMapMemory(logicalDevice, uniformDynamicBuffersMemory[currentImage], 0, uniformDynamicBufferSize, 0, &data);
  memcpy(data, uboModels, uniformDynamicBufferSize);
  vkUnmapMemory(logicalDevice, uniformDynamicBuffersMemory[currentImage]);

  //
  glm::mat4 view = glm::mat4(1.0f);
  float ratio = ((float)swapChainExtent.width / (float)swapChainExtent.height);
  view[0][0] = 1.0f / ratio;

  vkMapMemory(logicalDevice, uniformBuffersMemory[currentImage], 0, uniformBufferSize, 0, &data);
  memcpy(data, &view, uniformBufferSize);
  vkUnmapMemory(logicalDevice, uniformBuffersMemory[currentImage]);
}

void drawFrame() {
  vkWaitForFences(logicalDevice, 1, &frameInFlight[currentFrame], VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;

  VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAcquired[currentFrame], VK_NULL_HANDLE, &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    cleanupSwapChain();
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to acquire next image");
  }

  if (imageInFlight[imageIndex] != VK_NULL_HANDLE)
    vkWaitForFences(logicalDevice, 1, &imageInFlight[imageIndex], VK_TRUE, UINT64_MAX);
  imageInFlight[imageIndex] = frameInFlight[currentFrame];

  updateUniformBuffer(imageIndex);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAcquired[currentFrame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

  VkSemaphore signalSemaphores[] = {renderFinished[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  vkResetFences(logicalDevice, 1, &frameInFlight[currentFrame]);
  if (vkQueueSubmit(deviceQueue, 1, &submitInfo, frameInFlight[currentFrame]) != VK_SUCCESS)
    throw std::runtime_error("failed to submit draw command buffer");

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr;

  vkQueuePresentKHR(deviceQueue, &presentInfo);
  vkQueueWaitIdle(deviceQueue);

  currentFrame = (currentFrame + 1) & MAX_FRAMES_IN_FLIGHT;
}

typedef std::chrono::duration<float, std::chrono::seconds::period> duration;
typedef std::chrono::high_resolution_clock clock_;

void loop() {
  auto fpsTime = clock_::now();
  auto tickTime = clock_::now();
  int frames = 0;

  while (!glfwWindowShouldClose(window)) {
    auto start = clock_::now();

    glfwPollEvents();
    input::poll_gamepads();
    drawFrame();

    float time;
    auto currentTime = clock_::now();
    time = duration(currentTime - tickTime).count();
    if (time > 0.5f) {
      tickTime = currentTime;
      tetris::tick();
    }

    //
    frames += 1;
    currentTime = clock_::now();
    time = duration(currentTime - fpsTime).count();
    if (time > 1.0f) {
      std::cerr << "frames: " << frames << '\n';
      fpsTime = currentTime;
      frames = 0;
    }

    auto end = clock_::now();
    auto elapsed = duration(end - start);
    if (elapsed.count() < 0.016666666666666666f) {
      //std::cerr << "sleep for " << 0.2f - elapsed.count() << '\n';
      std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::duration<float>(0.016666666666666666f - elapsed.count())
      ));
    }
  }
  std::cerr << "should close\n";

  vkDeviceWaitIdle(logicalDevice);
}

void recreateSwapChain() {
  vkDeviceWaitIdle(logicalDevice);

  createSwapChain();
  createImageViews();
  createRenderPass();
  createDescriptorSetLayout();
  createGraphicsPipelines();
  createDepth();
  createFramebuffers();
  prepareUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
  createCommandBuffers();
}

void cleanupSwapChain() {
  vkDestroyImageView(logicalDevice, depthImageView, nullptr);
  vkDestroyImage(logicalDevice, depthImage, nullptr);
  vkFreeMemory(logicalDevice, depthImageMemory, nullptr);

  for (auto framebuffer : swapChainFramebuffers) {
    vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
  }

  vkFreeCommandBuffers(logicalDevice, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

  vkDestroyPipeline(logicalDevice, graphicsPipeline[0], nullptr);
  vkDestroyPipeline(logicalDevice, graphicsPipeline[1], nullptr);
  vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
  vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

  for (auto imageView : swapChainImageViews) {
    vkDestroyImageView(logicalDevice, imageView, nullptr);
  }

  vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);

  for (size_t i = 0; i < uniformDynamicBuffers.size(); i++) {
    vkDestroyBuffer(logicalDevice, uniformDynamicBuffers[i], nullptr);
    vkFreeMemory(logicalDevice, uniformDynamicBuffersMemory[i], nullptr);
  }

  for (size_t i = 0; i < uniformBuffers.size(); i++) {
    vkDestroyBuffer(logicalDevice, uniformBuffers[i], nullptr);
    vkFreeMemory(logicalDevice, uniformBuffersMemory[i], nullptr);
  }

  vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
}

void cleanup() {
  cleanupSwapChain();

  vkDestroySampler(logicalDevice, textureSampler, nullptr);
  vkDestroyImageView(logicalDevice, textureImageView, nullptr);
  vkDestroyImage(logicalDevice, textureImage, nullptr);
  vkFreeMemory(logicalDevice, textureImageMemory, nullptr);

  vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);
  vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
  vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);

  vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
  vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(logicalDevice, renderFinished[i], nullptr);
    vkDestroySemaphore(logicalDevice, imageAcquired[i], nullptr);
    vkDestroyFence(logicalDevice, frameInFlight[i], nullptr);
  }

  vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyDevice(logicalDevice, nullptr);
  vkDestroyInstance(instance, nullptr);

  glfwDestroyWindow(window);

  glfwTerminate();
}

int main() {
  tetris::initBoard();

  initWindow();
  input::init(window);
  initVulkan();
  loop();
  cleanup();

  return 0;
}
