#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include "binding.hpp"

std::unique_ptr<int, void (*)(int *)> open_file(const Napi::Env &env, const std::string &path)
{
  int fd = 0;
  const auto shm_name = get_shm_name(path);
  if (!shm_name.empty())
  {
    fd = ::shm_open(shm_name.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  }
  else
  {
    fd = ::open(path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  }
  if (fd == -1)
  {
    Napi::TypeError::New(env, "open failed, errno=" + std::to_string(errno)).ThrowAsJavaScriptException();
    return std::unique_ptr<int, void (*)(int *)>(nullptr, nullptr);
  }
  return std::unique_ptr<int, void (*)(int *)>(new int(fd), [](int *fd)
                                               { ::close(*fd); delete fd; });
}

Napi::Buffer<uint8_t> node_mmap_impl(const Napi::Env &env, std::string path, int64_t length)
{
  const auto fd = open_file(env, path);
  if (!fd)
  {
    return Napi::Buffer<uint8_t>::New(env, 0);
  }

  struct stat sb;
  if (fstat(*fd, &sb) == -1)
  {
    Napi::TypeError::New(env, "fstat failed, errno=" + std::to_string(errno)).ThrowAsJavaScriptException();
    return Napi::Buffer<uint8_t>::New(env, 0);
  }
  if (length <= 0)
  {
    length = sb.st_size;
  }
  else if (length > sb.st_size)
  {
    if (ftruncate(*fd, length) == -1)
    {
      length = sb.st_size;
    }
  }
  if (length <= 0)
  {
    return Napi::Buffer<uint8_t>::New(env, 0);
  }
  void *ptr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
  if (ptr == MAP_FAILED)
  {
    Napi::Error::New(env, "mmap failed, errno=" + std::to_string(errno)).ThrowAsJavaScriptException();
    return Napi::Buffer<uint8_t>::New(env, 0);
  }
  return Napi::Buffer<uint8_t>::New(env, static_cast<uint8_t *>(ptr), length, [=](Napi::Env env, void *data)
                                    { munmap(data, length); });
}
