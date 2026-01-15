#include <napi.h>
#include <memory>

// Extract shm name from path
inline std::string get_shm_name(const std::string &path)
{
  if (path.rfind("/dev/shm/", 0) == 0 && path.find('/', 10) == std::string::npos)
  {
    return path.substr(9); // remove /dev/shm/ to get just the name
  }
  return "";
}

// Platform-specific implementations
Napi::Buffer<uint8_t> node_mmap_impl(const Napi::Env &env, std::string path, int64_t length);
