#if defined _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_DLL
#elif __linux__
#define VK_USE_PLATFORM_XLIB_KHR
#define GLFW_EXPOSE_NATIVE_X11
#endif

#if defined _WIN32
#define posix_memalign(p, a, s) (((*(p)) = _aligned_malloc((s), (a))), *(p) ?0 :errno)
#endif
